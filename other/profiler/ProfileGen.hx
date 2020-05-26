
class StackElement {
	static var UID = 1;
	public var id : Int;
	public var desc : String;
	public var file : String;
	public var line : Int;
	public function new(desc:String) {
		id = UID++;
		if( desc.charCodeAt(desc.length-1) == ')'.code ) {
			var p = desc.lastIndexOf('(');
			var sep = desc.lastIndexOf(':');
			if( p > 0 && sep > p ) {
				file = desc.substr(p+1,sep-p-1);
				var sline = desc.substr(sep+1,desc.length - sep - 2);
				line = Std.parseInt(sline);
				desc = desc.substr(0,p);
				desc = desc.split("$").join("");
				if( StringTools.endsWith(desc,".__constructor__") )
					desc = desc.substr(0,-15)+"new";
			}
		}
		this.desc = desc;
	}
}

class StackLink {
	static var UID = 1;
	public var id : Int;
	public var elt : StackElement;
	public var parent : StackLink;
	public var children : Map<String,StackLink> = new Map();
	public var written : Bool;
	public function new(elt) {
		id = UID++;
		this.elt = elt;
	}
	public function getChildren(elt:StackElement) {
		var c = children.get(elt.desc);
		if( c == null ) {
			c = new StackLink(elt);
			c.parent = this;
			children.set(elt.desc,c);
		}
		return c;
	}
}

class Frame {
	public var samples : Array<{ thread : Int, time : Float, stack : Array<StackElement> }> = [];
	public var startTime : Float;
	public function new() {
	}
}

class Thread {

	public var tid : Int;
	public var curFrame : Frame;
	public var frames : Array<Frame>;

	public function new(tid) {
		this.tid = tid;
		curFrame = new Frame();
		frames = [curFrame];
	}
}

class ProfileGen {

	static function makeStacks( st : Array<StackLink> ) {
		var write = [];
		for( s in st ) {
			var s = s;
			while( s != null ) {
				if( s.written ) break;
				s.written = true;
				write.push(s);
				s = s.parent;
			}
		}
		write.sort(function(s1,s2) return s1.id - s2.id);
		return [for( s in write ) {
			callFrame : s.elt.file == null ? cast {
				functionName : s.elt.desc,
				scriptId : 0,
			} : {
				functionName : s.elt.desc,
				scriptId : 1,
				url : s.elt.file.split("\\").join("/"),
				lineNumber : s.elt.line - 1,
			},
			id : s.id,
			parent : s.parent == null ? null : s.parent.id,
		}];
	}

	static function main() {
		var args = Sys.args();
		var outFile = null;
		var file = null;
		var debug = false;
		var mintime = 0.0;

		while( args.length > 0 ) {
			var arg = args[0];
			args.shift();
			if( arg.charCodeAt(0) != "-".code ) {
				file = arg;
				continue;
			}
			switch( arg ) {
			case "-d"|"--debug":
				debug = true;
			case "-o"|"--out":
				outFile = args.shift();
			case "--min-time-ms":
				mintime = Std.parseFloat(args.shift()) / 1000.0;
			default:
				throw "Unknown parameter "+arg;
			}
		}

		if( file == null ) file = "hlprofile.dump";
		if( sys.FileSystem.isDirectory(file) ) file += "/hlprofile.dump";
		if( outFile == null ) outFile = file;

		var f = sys.io.File.read(file);
		if( f.readString(4) != "PROF" ) throw "Invalid profiler file";
		var version = f.readInt32();
		var sampleCount = f.readInt32();
		var rootElt = new StackElement("(root)");
		var hthreads = new Map();
		var threads = [];
		var tcur : Thread = null;
		var fileMaps : Array<Map<Int,StackElement>> = [];
		while( true ) {
			var time = try f.readDouble() catch( e : haxe.io.Eof ) break;
			var tid = f.readInt32();
			if( tcur == null || tid != tcur.tid ) {
				tcur = hthreads.get(tid);
				if( tcur == null ) {
					tcur = new Thread(tid);
					tcur.curFrame.startTime = time;
					hthreads.set(tid,tcur);
					threads.push(tcur);
				}
			}
			var msgId = f.readInt32();
			if( msgId < 0 ) {
				var count = msgId & 0x7FFFFFFF;
				var stack = [];
				for( i in 0...count ) {
					var file = f.readInt32();
					if( file == -1 )
						continue;
					var line = f.readInt32();
					var elt : StackElement;
					if( file < 0 ) {
						file &= 0x7FFFFFFF;
						elt = fileMaps[file].get(line);
						if( elt == null ) throw "assert";
					} else {
						var len = f.readInt32();
						var buf = new StringBuf();
						for( i in 0...len ) buf.addChar(f.readUInt16());
						var str = buf.toString();
						elt = new StackElement(str);
						var m = fileMaps[file];
						if( m == null ) {
							m = new Map();
							fileMaps[file] = m;
						}
						m.set(line,elt);
					}
					stack[i] = elt;
				}
				tcur.curFrame.samples.push({ time : time, thread : tid, stack : stack });
			} else {
				var size = f.readInt32();
				var data = f.read(size);
				switch( msgId ) {
				case 0:
					if(mintime > 0 && tcur.frames.length > 0) {
						var lastFrame = tcur.frames[tcur.frames.length-1];
						if(lastFrame.samples.length == 0 || (lastFrame.samples[lastFrame.samples.length-1].time - lastFrame.startTime) < mintime)
							tcur.frames.pop();
					}
					tcur.curFrame = new Frame();
					tcur.curFrame.startTime = time;
					tcur.frames.push(tcur.curFrame);
				default:
					Sys.println("Unknown profile message #"+msgId);
				}
			}
		}

		var mainTid = threads[0].tid;
		var json : Array<Dynamic> = [
			{
    			pid : 0,
    			tid : mainTid,
 	 			ts : 0,
				ph : "M",
				cat : "__metadata",
				name : "thread_name",
				args : { name : "CrBrowserMain" }
			}
		];

		var count = 1;
		var f0 = threads[0].frames[0];
		var t0 = f0.samples.length == 0 ? f0.startTime : f0.samples[0].time;

		for( thread in threads ) {
			var tid = thread.tid;

			function timeStamp(t:Float) {
				return Std.int((t - t0) * 1000000) + 1;
			}

			var lastT = 0.;
			var rootStack = new StackLink(rootElt);
			var profileId = count++;

			json.push({
				pid : 0,
				tid : tid,
				ts : 0,
				ph : "P",
				cat : "disabled-by-default-v8.cpu_profiler",
			    name : "Profile",
				id : "0x"+profileId,
				args: { data : { startTime : 0 } },
			});

			for( f in thread.frames ) {
				if( f.samples.length == 0 ) continue;
				json.push({
					pid : 0,
					tid : tid,
					ts : timeStamp(f.startTime),
					ph : "B",
					cat : "devtools.timeline",
					name : "FunctionCall",
				});
				json.push({
					pid : 0,
					tid : tid,
					ts : timeStamp(f.samples[f.samples.length-1].time),
					ph : "E",
					cat : "devtools.timeline",
					name : "FunctionCall"
				});
			}
			for( f in thread.frames ) {
				if( f.samples.length == 0 ) continue;

				var timeDeltas = [];
				var allStacks = [];
				var lines = [];

				for( s in f.samples) {
					var st = rootStack;
					var line = 0;
					for( i in 0...s.stack.length ) {
						var s = s.stack[s.stack.length - 1 - i];
						if( s == null || s.file == "?" ) continue;
						line = s.line;
						st = st.getChildren(s);
					}
					lines.push(line);
					allStacks.push(st);
					var t = Math.ffloor((s.time - t0) * 1000000);
					timeDeltas.push(t - lastT);
					lastT = t;
				}
				json.push({
					pid : 0,
					tid : tid,
					ts : 0,
					ph : "P",
					cat : "disabled-by-default-v8.cpu_profiler",
					name : "ProfileChunk",
					id : "0x"+profileId,
					args : {
						data : {
							cpuProfile : {
								nodes : makeStacks(allStacks),
								samples : [for( s in allStacks ) s.id],
								//lines : lines,
							},
							timeDeltas : timeDeltas,
						}
					}
				});
			}
		}
		sys.io.File.saveContent(outFile, debug ? haxe.Json.stringify(json,"\t") : haxe.Json.stringify(json));
	}

}