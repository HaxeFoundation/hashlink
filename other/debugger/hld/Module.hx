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


class Module {

	public var code : format.hl.Data;
	var fileIndexes : Map<String, Int>;
	var functionsByFile : Map<Int, Array<{ f : HLFunction, ifun : Int, lmin : Int, lmax : Int }>>;
	var globalsOffsets : Array<Int>;
	var globalTable : GlobalAccess;
	var protoCache : Map<String,ModuleProto>;
	var functionRegsCache : Array<Array<{ t : HLType, offset : Int }>>;
	var size : Size;

	public function new() {
		protoCache = new Map();
		functionRegsCache = [];
	}

	public function load( data : haxe.io.Bytes ) {
		code = new format.hl.Reader(false).read(new haxe.io.BytesInput(data));

		// init files
		fileIndexes = new Map();
		for( i in 0...code.debugFiles.length ) {
			var f = code.debugFiles[i];
			fileIndexes.set(f, i);
			var low = f.split("\\").join("/").toLowerCase();
			fileIndexes.set(f, i);
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

	public function init( size : Size ) {
		this.size = size;

		// init globals
		var globalsPos = 0;
		globalsOffsets = [];
		for( g in code.globals ) {
			globalsPos += size.pad(globalsPos, g);
			globalsOffsets.push(globalsPos);
			globalsPos += size.type(g);
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
		var size = parent == null ? size.ptr : parent.size;
		var fields = parent == null ? new Map() : [for( k in parent.fields.keys() ) k => parent.fields.get(k)];

		for( f in o.fields ) {
			size += this.size.pad(size, f.t);
			fields.set(f.name, { name : f.name, t : f.t, offset : size });
			size += this.size.type(f.t);
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

	public function getBreaks( file : String, line : Int ) {
		var ifile = fileIndexes.get(file);
		if( ifile == null )
			ifile = fileIndexes.get(file.split("\\").join("//").toLowerCase());

		var functions = functionsByFile.get(ifile);
		if( ifile == null || functions == null )
			return null;

		var breaks = [];
		for( f in functions ) {
			if( f.lmin > line || f.lmax < line ) continue;
			var ifun = f.ifun;
			var f = f.f;
			var i = 0;
			var len = f.debug.length >> 1;
			while( i < len ) {
				var dfile = f.debug[i << 1];
				if( dfile != ifile ) {
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
					if( dfile == ifile && dline != line )
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

}