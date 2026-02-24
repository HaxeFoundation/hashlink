package hlprof;

class StackElement {
	static var UID = 0;
	public var id : Int;
	public var desc : String;
	public var file : String;
	public var line : Int;
	public function new(desc:String) {
		id = UID++;
		// Parse obj.field(file:line)
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
		if( file != null ) {
			file = file.split("\\").join("/");
			if( file.charCodeAt(1) == ':'.code )
				file = file.split("/haxe/std/").pop();
			file = file.split("/").join(".");
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
	public var repeat : Int = 1;
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
	public var marks : Array<{ thread : Int, time : Float, msgId : Int, data : String }> = [];
	public var startTime : Float;
	public function new() {
	}
}

class Thread {

	public var tid : Int;
	public var curFrame : Frame;
	public var frames : Array<Frame>;
	public var name : String;

	public function new(tid) {
		this.tid = tid;
		curFrame = new Frame();
		frames = [curFrame];
	}
}

class ProfileGen {

	static function readDump( file : String, mintime : Float, keepLines : Bool ) : Array<Thread> {
		#if js
		var f = new haxe.io.BytesInput(sys.io.File.getBytes(file));
		#else
		var f = sys.io.File.read(file);
		#end
		if( f.readString(4) != "PROF" ) throw "Invalid profiler file";
		var version = f.readInt32();
		var sampleCount = f.readInt32();
		var gcMajor = new StackElement("GC Major");
		var hthreads = new Map();
		var threads = [];
		var tcur : Thread = null;
		var fileMaps : Array<{ byDesc : Map<String,StackElement>, byLine : Map<Int,StackElement> }> = [];
		while( true ) {
			var time = try f.readDouble() catch( e : haxe.io.Eof ) break;
			if( time == -1 ) break;
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
				var count = msgId & 0x3FFFFFFF;
				var stack = [];
				for( i in 0...count ) {
					var file = f.readInt32();
					if( file == -1 )
						continue;
					var line = f.readInt32();
					var elt : StackElement = null;
					if( file < 0 ) {
						file &= 0x7FFFFFFF;
						elt = fileMaps[file].byLine.get(line);
						if( elt == null ) throw "assert";
					} else {
						var len = f.readInt32();
						var buf = new StringBuf();
						for( i in 0...len ) buf.addChar(f.readUInt16());
						var str = buf.toString();
						var maps = fileMaps[file];
						if( maps == null ) {
							maps = { byLine : new Map(), byDesc : new Map() };
							fileMaps[file] = maps;
						}
						elt = new StackElement(str);
						if( !keepLines ) {
							var ePrev = maps.byDesc.get(elt.desc);
							if( ePrev != null ) {
								if( elt.line < ePrev.line ) ePrev.line = elt.line;
								elt = ePrev;
							} else
								maps.byDesc.set(elt.desc, elt);
						}
						maps.byLine.set(line,elt);
					}
					stack[i] = elt;
				}
				if( msgId & 0x40000000 != 0 )
					stack.unshift(gcMajor);
				if( tcur.curFrame.samples.length == 100000 ) {
					tcur.curFrame = new Frame();
					tcur.curFrame.startTime = time;
					tcur.frames.push(tcur.curFrame);
				}
				tcur.curFrame.samples.push({ time : time, thread : tid, stack : stack });
			} else {
				var size = f.readInt32();
				var data = f.read(size);
				switch( msgId ) {
				case 0:
					// End of frame
					if( mintime > 0 && tcur.frames.length > 0 ) {
						var lastFrame = tcur.frames[tcur.frames.length-1];
						if( lastFrame.samples.length == 0 || (lastFrame.samples[lastFrame.samples.length-1].time - lastFrame.startTime) < mintime )
							tcur.frames.pop();
					}
					tcur.curFrame = new Frame();
					tcur.curFrame.startTime = time;
					tcur.frames.push(tcur.curFrame);
				default:
					// Custom event, parse data as String label
					var dataStr = data.getString(0, data.length, RawNative); // our string is encoded in hl native format utf16 (ucs2)
					tcur.curFrame.marks.push({ thread : tid, time : time, msgId : msgId, data : dataStr });
				}
			}
		}

		for( t in threads )
			t.name = "Thread "+t.tid;
		threads[0].name = "Main";

		var namesCount = try f.readInt32() catch( e : haxe.io.Eof ) 0;
		for( i in 0...namesCount ) {
			var tid = f.readInt32();
			var tname = f.readString(f.readInt32());
			var t = hthreads.get(tid);
			if( t != null )
				t.name = tname;
		}

		f.close();
		return threads;
	}

	static function genJson( threads : Array<Thread>, collapseRecursion : Bool ) {
		var rootElt = new StackElement("(root)");
		var json : Array<Dynamic> = [for( t in threads )
			{
				pid : 0,
				tid : t.tid,
				ts : 0,
				ph : "M",
				cat : "__metadata",
				name : "thread_name",
				args : { name : t.name }
			}
		];
		json.push({
			args: {
				data: {
					frameTreeNodeId: 0,
					frames: [
						{
							processId: 0,
							url: "http://_"
						}
					],
					persistentIds: true
				}
			},
			cat: "disabled-by-default-devtools.timeline",
			name: "TracingStartedInBrowser",
			pid: 0,
			tid: 0,
			ts: 0
		});

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

			for( findex in 0...thread.frames.length ) {
				var f = thread.frames[findex];
				if( f.samples.length == 0 ) continue;
				var ts = timeStamp(f.startTime);
				var tend = timeStamp(f.samples[f.samples.length-1].time);
				json.push({
					pid : 0,
					tid : tid,
					ts : ts,
					dur : tend - ts,
					ph : "X",
					cat : "disabled-by-default-devtools.timeline",
					name : "RunTask",
				});
				// Insert event marker as performance.measure
				if( f.marks.length == 0 ) continue;
				for( mindex in 0...f.marks.length ) {
					var mark = f.marks[mindex];
					var markStart = timeStamp(mark.time);
					var markEnd = ( mindex == f.marks.length-1 ) ? tend : timeStamp(f.marks[mindex+1].time);
					json.push({
						pid : 0,
						tid : tid,
						ts : markStart,
						ph : "b",
						cat : "blink.user_timing",
						id2 : { local : '$findex' },
						args : {},
						name : '${mark.msgId}:${mark.data}',
					});
					json.push({
						pid : 0,
						tid : tid,
						ts : markEnd,
						ph : "e",
						cat : "blink.user_timing",
						id2 : { local : '$findex' },
						args : {},
						name : '${mark.msgId}:${mark.data}',
					});
				}
			}
			for( f in thread.frames ) {
				if( f.samples.length == 0 ) continue;

				var timeDeltas = [];
				var allStacks = [];
				var lines = [];

				for( s in f.samples ) {
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
								nodes : makeStacks(allStacks, collapseRecursion),
								samples : [for( s in allStacks ) s.id],
								//lines : lines,
							},
							timeDeltas : timeDeltas,
						}
					}
				});
			}
		}
		return json;
	}

	static function makeStacks( st : Array<StackLink>, collapseRecursion : Bool ) {
		var write = [];
		for( sleaf in st ) {
			var s = sleaf;
			while( s != null ) {
				if( s.written ) break;
				s.written = true;
				if( collapseRecursion ) {
					collapseStackLink(s);
				}
				write.push(s);
				s = s.parent;
			}
		}
		write.sort(function(s1,s2) return s1.id - s2.id);
		return [for( s in write ) {
			callFrame : s.elt.file == null ? cast {
				functionName : s.elt.desc + (s.repeat > 1 ? '(${s.repeat})' : ""),
				scriptId : 0,
			} : {
				functionName : s.elt.desc + (s.repeat > 1 ? '(${s.repeat})' : ""),
				scriptId : 1,
				url : s.elt.file.split("\\").join("/"),
				lineNumber : s.elt.line - 1,
			},
			id : s.id,
			parent : s.parent == null ? null : s.parent.id,
		}];
	}

	static function collapseStackLink( s : StackLink ) {
		var p1 = s.parent;
		// Collapse AA
		while( p1 != null && s.elt.desc == p1.elt.desc ) {
			s.repeat += p1.repeat;
			s.parent = p1.parent;
			p1 = p1.parent;
		}
		if( p1 == null || p1.parent == null )
			return;
		var p2 = p1.parent;
		var p3 = p2.parent;
		// Collapse ABAB
		while( p3 != null && s.elt.desc == p2.elt.desc && p1.elt.desc == p3.elt.desc ) {
			s.repeat += p2.repeat;
			p1.repeat += p3.repeat;
			p1.parent = p3.parent;
			p2 = p3.parent;
			if( p2 == null )
				break;
			p3 = p2.parent;
		}
		if( p3 == null || p3.parent == null )
			return;
		var p4 = p3.parent;
		var p5 = p4.parent;
		// Collapse ABCABC
		while( p5 != null && s.elt.desc == p3.elt.desc && p1.elt.desc == p4.elt.desc && p2.elt.desc == p5.elt.desc ) {
			s.repeat += p3.repeat;
			p1.repeat += p4.repeat;
			p2.repeat += p5.repeat;
			p2.parent = p5.parent;
			p3 = p5.parent;
			if( p3 == null || p3.parent == null )
				break;
			p4 = p3.parent;
			p5 = p4.parent;
		}
	}

	public static function run( args : Array<String> ) {
		var outFile = null;
		var file = null;
		var debug = false;
		var mintime = 0.0;
		var keepLines = false;
		var collapseRecursion = false;

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
			case "--keep-lines":
				keepLines = true;
			case "--collapse-recursion":
				collapseRecursion = true;
			default:
				Sys.println("[WARNING] Unknown parameter " + arg + ", skipping the rest");
				break;
			}
		}

		if( file == null ) file = "hlprofile.dump";
		if( sys.FileSystem.isDirectory(file) ) file += "/hlprofile.dump";
		if( outFile == null ) outFile = file;

		var threads = readDump(file, mintime, keepLines);

		// var mainTid = threads[0].tid;
		var json = genJson(threads, collapseRecursion);

		sys.io.File.saveContent(outFile, debug ? haxe.Json.stringify(json,"\t") : haxe.Json.stringify(json));
	}

	static function main() {
		run(Sys.args());
	}
}
