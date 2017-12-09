package hld;
import format.hl.Data;

private typedef GlobalAccess = {
	var sub : Map<String,GlobalAccess>;
	var gid : Null<Int>;
}

typedef ModuleProto = {
	var name : String;
	var size :  Int;
	var fieldNames : Array<String>;
	var parent : ModuleProto;
	var fields : Map<String,{
		var name : String;
		var t : HLType;
		var offset : Int;
	}>;
}

typedef ModuleEProto = Array<{ name : String, size : Int, params : Array<{ offset : Int, t : HLType }> }>;

class Module {

	public var code : format.hl.Data;
	var fileIndexes : Map<String, Int>;
	var functionsByFile : Map<Int, Array<{ f : HLFunction, ifun : Int, lmin : Int, lmax : Int }>>;
	var globalsOffsets : Array<Int>;
	var globalTable : GlobalAccess;
	var protoCache : Map<String,ModuleProto>;
	var eprotoCache : Map<String,ModuleEProto>;
	var functionRegsCache : Array<Array<{ t : HLType, offset : Int }>>;
	var align : Align;
	var reversedHashes : Map<Int,String>;
	var graphCache : Map<Int, CodeGraph>;

	public function new() {
		protoCache = new Map();
		eprotoCache = new Map();
		graphCache = new Map();
		functionRegsCache = [];
	}

	public function load( data : haxe.io.Bytes ) {
		code = new format.hl.Reader().read(new haxe.io.BytesInput(data));

		// init files
		fileIndexes = new Map();
		for( i in 0...code.debugFiles.length ) {
			var f = code.debugFiles[i];
			fileIndexes.set(f, i);
			var low = f.split("\\").join("/").toLowerCase();
			fileIndexes.set(low, i);
			var fileOnly = low.split("/").pop();
			if( !fileIndexes.exists(fileOnly) ) {
				fileIndexes.set(fileOnly, i);
				if( StringTools.endsWith(fileOnly,".hx") )
					fileIndexes.set(fileOnly.substr(0, -3), i);
			}
		}

		functionsByFile = new Map();
		for( ifun in 0...code.functions.length ) {
			var f = code.functions[ifun];
			var files = new Map();
			for( i in 0...f.debug.length >> 1 ) {
				var ifile = f.debug[i << 1];
				var dline = f.debug[(i << 1) + 1];
				var inf = files.get(ifile);
				if( inf == null ) {
					inf = { f : f, ifun : ifun, lmin : 1000000, lmax : -1 };
					files.set(ifile, inf);
					var fl = functionsByFile.get(ifile);
					if( fl == null ) {
						fl = [];
						functionsByFile.set(ifile, fl);
					}
					fl.push(inf);
				}
				if( dline < inf.lmin ) inf.lmin = dline;
				if( dline > inf.lmax ) inf.lmax = dline;
			}
		}
	}

	public function init( align : Align ) {
		this.align = align;

		// init globals
		var globalsPos = 0;
		globalsOffsets = [];
		for( g in code.globals ) {
			globalsPos += align.padSize(globalsPos, g);
			globalsOffsets.push(globalsPos);
			globalsPos += align.typeSize(g);
		}

		globalTable = {
			sub : new Map(),
			gid : null,
		};
		function addGlobal( path : Array<String>, gid : Int ) {
			var t = globalTable;
			for( p in path ) {
				if( t.sub == null )
					t.sub = new Map();
				var next = t.sub.get(p);
				if( next == null ) {
					next = { sub : null, gid : null };
					t.sub.set(p, next);
				}
				t = next;
			}
			t.gid = gid;
		}
		for( t in code.types )
			switch( t ) {
			case HObj(o) if( o.globalValue != null ):
				addGlobal(o.name.split("."), o.globalValue);
			case HEnum(e) if( e.globalValue != null ):
				addGlobal(e.name.split("."), e.globalValue);
			default:
			}
	}

	public function getObjectProto( o : ObjPrototype ) : ModuleProto {

		var p = protoCache.get(o.name);
		if( p != null )
			return p;

		var parent = o.tsuper == null ? null : switch( o.tsuper ) { case HObj(o): getObjectProto(o); default: throw "assert"; };
		var size = parent == null ? align.ptr : parent.size;
		var fields = parent == null ? new Map() : [for( k in parent.fields.keys() ) k => parent.fields.get(k)];

		for( f in o.fields ) {
			size += align.padStruct(size, f.t);
			fields.set(f.name, { name : f.name, t : f.t, offset : size });
			size += align.typeSize(f.t);
		}

		p = {
			name : o.name,
			size : size,
			parent : parent,
			fields : fields,
			fieldNames : [for( o in o.fields ) o.name],
		};
		protoCache.set(p.name, p);

		return p;
	}

	public function getEnumProto( e : EnumPrototype ) : ModuleEProto {
		var p = eprotoCache.get(e.name);
		if( p != null )
			return p;
		p = [];
		for( c in e.constructs ) {
			var size = align.ptr;
			size += align.padStruct(size, HI32);
			size += 4; // index
			var params = [];
			for( t in c.params ) {
				size += align.padStruct(size, t);
				params.push({ offset : size, t : t });
				size += align.typeSize(t);
			}
			p.push({ name : c.name, size : size, params : params });
		}
		eprotoCache.set(e.name, p);
		return p;
	}

	public function resolveGlobal( path : Array<String> ) {
		var g = globalTable;
		while( path.length > 0 ) {
			if( g.sub == null ) break;
			var p = path[0];
			var n = g.sub.get(p);
			if( n == null ) break;
			path.shift();
			g = n;
		}
		return g == globalTable ? null : { type : code.globals[g.gid], offset : globalsOffsets[g.gid] };
	}

	public function getFileFunctions( file : String ) {
		var ifile = fileIndexes.get(file);
		if( ifile == null )
			ifile = fileIndexes.get(file.split("\\").join("/").toLowerCase());
		if( ifile == null )
			return null;
		var functions = functionsByFile.get(ifile);
		if( functions == null )
			return null;
		return { functions : functions, fidx : ifile };
	}

	public function getBreaks( file : String, line : Int ) {
		var ffuns = getFileFunctions(file);
		if( ffuns == null )
			return null;

		var breaks = [];
		for( f in ffuns.functions ) {
			if( f.lmin > line || f.lmax < line ) continue;
			var ifun = f.ifun;
			var f = f.f;
			var i = 0;
			var len = f.debug.length >> 1;
			while( i < len ) {
				var dfile = f.debug[i << 1];
				if( dfile != ffuns.fidx ) {
					i++;
					continue;
				}
				var dline = f.debug[(i << 1) + 1];
				if( dline != line ) {
					i++;
					continue;
				}
				breaks.push({ ifun : ifun, pos : i });
				// skip
				i++;
				while( i < len ) {
					var dfile = f.debug[i << 1];
					var dline = f.debug[(i << 1) + 1];
					if( dfile == ffuns.fidx && dline != line )
						break;
					i++;
				}
			}
		}
		return breaks;
	}

	public function resolveSymbol( fidx : Int, fpos : Int ) {
		var f = code.functions[fidx];
		var fid = f.debug[fpos << 1];
		var fline = f.debug[(fpos << 1) + 1];
		return { file : code.debugFiles[fid], line : fline };
	}

	public function getFunctionRegs( fidx : Int ) {
		var regs = functionRegsCache[fidx];
		if( regs != null )
			return regs;
		var f = code.functions[fidx];
		var nargs = switch( f.t ) { case HFun(f): f.args.length; default: throw "assert"; };
		regs = [];

		var argsSize = 0;
		var size = 0;
		for( i in 0...nargs ) {

			if( align.is64 )
				throw "TODO : handle x64 calling conventions";

			var t = f.regs[i];
			regs[i] = { t : t, offset : argsSize + align.ptr * 2 };
			argsSize += align.stackSize(t);
		}
		for( i in nargs...f.regs.length ) {
			var t = f.regs[i];
			size += align.typeSize(t);
			size += align.padSize(size, t);
			regs[i] = { t : t, offset : -size };
		}
		functionRegsCache[fidx] = regs;
		return regs;
	}

	public function reverseHash( h : Int ) {
		if( reversedHashes == null ) {
			reversedHashes = new Map();
			for( s in code.strings )
				reversedHashes.set(s.hash(), s);
		}
		return reversedHashes.get(h);
	}

	public function getGraph( fidx : Int ) {
		var g = graphCache.get(fidx);
		if( g != null )
			return g;
		g = new CodeGraph(code, code.functions[fidx]);
		graphCache.set(fidx, g);
		return g;
	}

}
