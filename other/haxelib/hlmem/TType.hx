package hlmem;

import format.hl.Data;
using format.hl.Tools;

class TType {
	public var tid : Int;
	public var t : HLType;
	public var closure : HLType;
	public var bmp : #if js haxe.io.Bytes #else hl.Bytes #end;
	public var hasPtr : Bool;
	public var isDyn : Bool;

	public var memFields : Array<TType>;
	public var memFieldsNames : Array<String>;

	public var constructs : Array<Array<TType>>;
	public var nullWrap : TType;
	public var ptrTags : haxe.io.Bytes;
	public var parentClass : TType;

	public var falsePositive = 0;
	public var falsePositiveIndexes = [];

	public function new(tid, t, ?cl) {
		this.tid = tid;
		this.t = t;
		this.closure = cl;
		isDyn = t.isDynamic();
		switch( t ) {
		case HFun(_):
			hasPtr = cl != null && cl.isPtr();
		default:
			hasPtr = t.containsPointer();
		}
	}

	public function match( t : TType ) {
		if( t == this ) return true;
		if( parentClass != null ) return parentClass.match(t);
		return false;
	}

	function tagPtr( pos : Int ) {
		var p = pos >> 3;
		if( ptrTags == null || ptrTags.length <= p ) {
			var nc = haxe.io.Bytes.alloc(p + 1);
			if( ptrTags != null ) nc.blit(0, ptrTags, 0, ptrTags.length);
			ptrTags = nc;
		}
		ptrTags.set(p, ptrTags.get(p) | (1 << (pos & 7)));
	}

	function appendField( m : Memory, pos : Int, name : String, t : HLType ) {
		switch( t ) {
		case HPacked({ v : HStruct(p) }):
			var all = [p];
			while( p.tsuper != null ) {
				switch( p.tsuper ) {
				case HStruct(p2):
					p = p2;
					all.unshift(p2);
				default:
					throw "assert";
				}
			}
			for( p in all )
				for( f in p.fields )
					pos = appendField(m, pos, name+"."+f.name, f.t);
		case HPacked(_):
			throw "assert";
		default:
			var size = m.typeSize(t);
			pos = align(pos, size);
			memFields[pos >> m.ptrBits] = m.getType(t);
			memFieldsNames[pos >> m.ptrBits] = name;
			if( t.isPtr() ) tagPtr(pos >> m.ptrBits);
			pos += size;
		}
		return pos;
	}

	public function buildTypes( m : Memory, tvoid : TType ) {
		if( !hasPtr ) return;

		// layout of data inside memory
		inline function fill(fields:Array<TType>, pos:Int) {
			if( m.is64 ) {
				for( i in 0...pos >> m.ptrBits )
					if( fields[i] == null )
						fields[i] = tvoid;
			} else {
				// fill two slots for 64bit data
				var i = pos >> m.ptrBits;
				while( --i >= 0 )
					if( fields[i] == null )
						fields[i] = (fields[i-1] == null || fields[i-1].t != HF64) ? tvoid : fields[i-1];
			}
		}

		switch( t ) {
		case HObj(p), HStruct(p):
			var protos = [p];
			while( p.tsuper != null )
				switch( p.tsuper ) {
				case HObj(p2), HStruct(p2):
					protos.unshift(p2);
					p = p2;
				default:
				}

			memFields = [];
			memFieldsNames = [];

			var pos = t.match(HStruct(_)) ? 0 : 1 << m.ptrBits; // type
			for( p in protos )
				for( f in p.fields )
					pos = appendField(m, pos, f.name, f.t);
			fill(memFields, pos);
		case HEnum(e):
			constructs = [];
			for( c in e.constructs ) {
				var pos = (1<<m.ptrBits) + 4; // type + index
				var fields = [];
				for( t in c.params ) {
					var size = m.typeSize(t);
					pos = align(pos, size);
					fields[pos>>m.ptrBits] = m.getType(t);
					if( t.isPtr() ) tagPtr(pos >> m.ptrBits);
					pos += size;
				}
				fill(fields, pos);
				constructs.push(fields);
			}
		case HVirtual(fl):
			memFields = [
				tvoid, // type
				m.getType(HDyn), // obj
				m.getType(HDyn), // next
			];
			memFieldsNames = [];
			var pos = (fl.length + 3) << m.ptrBits;
			tagPtr(1);
			tagPtr(2);
			for( f in fl ) {
				var size = m.typeSize(f.t);
				pos = align(pos, size);
				memFields[pos >> m.ptrBits] = m.getType(f.t);
				memFieldsNames[pos >> m.ptrBits] = f.name;
				if( f.t.isPtr() ) tagPtr(pos >> m.ptrBits);
				pos += size;
			}

			fill(memFields, pos);
			// keep null our fields pointers since they might point to a DynObj data head
			for( i in 0...fl.length )
				memFields[i+3] = null;
		case HNull(t):
			if( m.is64 )
				memFields = [tvoid, m.getType(t)];
			else
				memFields = [tvoid, tvoid, m.getType(t), tvoid];
		case HFun(_):
			memFields = [tvoid, tvoid, tvoid, m.getType(closure)];
		default:
		}
	}

	inline function align(pos, size) {
		var d = pos & (size - 1);
		if( d != 0 ) pos += size - d;
		return pos;
	}

	public function toString() {
		switch( t ) {
		case HAbstract("roots"):
			return Analyzer.withColor("roots", Green);
		case HAbstract(p):
			return p;
		case HFun(_), HMethod(_):
			return 'Function(${t.toString()})';
		default:
			return t.toString();
		}
	}

}
