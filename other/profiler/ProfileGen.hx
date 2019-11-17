
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

class ProfileGen {

	static function makeStacks( st : Array<StackLink> ) {
		var m = new Map();
		for( s in st ) {
			var s = s;
			while( s != null ) {
				if( m.exists(s.id) ) break;
				m.set(s.id, s);
				s = s.parent;
			}
		}
		var unique = Lambda.array(m);
		unique.sort(function(s1,s2) return s1.id - s2.id);
		return [for( s in unique ) {
			callFrame : s.elt.file == null ? cast {
				functionName : s.elt.desc,
				scriptId : 0,
			} : {
				functionName : s.elt.desc,
				scriptId : 1,
				url : "file://"+s.elt.file.split("\\").join("/"),
				lineNumber : s.elt.line,
			},
			id : s.id,
			parent : s.parent == null ? null : s.parent.id,
		}];
	}

	static function main() {
		var file = Sys.args()[0];
		if( file == null ) file = "hlprofile.dump";
		if( sys.FileSystem.isDirectory(file) ) file += "/hlprofile.dump";

		var f = sys.io.File.read(file);
		if( f.readString(4) != "PROF" ) throw "Invalid profiler file";
		var version = f.readInt32();
		var sampleCount = f.readInt32();
		var cache = new Map();
		var frames = [];
		var rootElt = new StackElement("(root)");
		while( true ) {
			var time = try f.readDouble() catch( e : haxe.io.Eof ) break;
			var tid = f.readInt32();
			var count = f.readInt32();
			var stack = [];
			for( i in 0...count ) {
				var file = f.readInt32();
				if( file == -1 )
					continue;
				var line = f.readInt32();
				var elt : StackElement;
				if( file < 0 ) {
					file &= 0x7FFFFFFF;
					elt = cache.get(file+"-"+line);
					if( elt == null ) throw "assert";
				} else {
					var len = f.readInt32();
					var buf = new StringBuf();
					for( i in 0...len ) buf.addChar(f.readUInt16());
					var str = buf.toString();
					elt = new StackElement(str);
					cache.set(file+"-"+line, elt);
				}
				stack[i] = elt;
			}
			frames.push({ time : time, thread : tid, stack : stack });
		}
		var lastT = frames[0].time - 1 / sampleCount;
		var defStart = frames[0].stack[frames[0].stack.length - 1];
		var rootStack = new StackLink(rootElt);
		var timeDeltas = [];
		var allStacks = [];
		for( f in frames ) {
			var st = rootStack;
			var start = 0;
			if( f.stack[f.stack.length-1].desc == defStart.desc ) start++;
			for( i in start...f.stack.length ) {
				var s = f.stack[f.stack.length - 1 - i];
				if( s == null ) continue;
				st = st.getChildren(s);
			}
			allStacks.push(st);
			timeDeltas.push(Std.int((f.time - lastT)*1000000));
			lastT = f.time;
		}

		var t0 = frames[0].time;
		function timeStamp(t:Float) {
			return Std.int((t - t0) * 1000000);
		}

		var tid = frames[0].thread;
		var json : Dynamic = [
			{
				pid : 0,
				tid : tid,
				ts : 0,
				ph : "P",
				cat : "disabled-by-default-v8.cpu_profiler",
			    name : "Profile",
				id : "0x1",
				args: { data : { startTime : 0 } },
			},
			{
				pid : 0,
				tid : tid,
				ts : 0,
				ph : "B",
				cat : "devtools.timeline",
				name : "FunctionCall",
			},
			{
				pid : 0,
				tid : tid,
				ts : timeStamp(frames[frames.length-1].time),
				ph : "E",
				cat : "devtools.timeline",
				name : "FunctionCall"
			},
			{
				pid : 0,
				tid : tid,
				ts : 0,
				ph : "P",
				cat : "disabled-by-default-v8.cpu_profiler",
				name : "ProfileChunk",
				id : "0x1",
				args : {
					data : {
						cpuProfile : {
							nodes : makeStacks(allStacks),
							samples : [for( s in allStacks ) s.id],
						},
						timeDeltas : timeDeltas,
					}
				}
			}
		];
		sys.io.File.saveContent("Profile.json", haxe.Json.stringify(json,"\t"));
	}

}