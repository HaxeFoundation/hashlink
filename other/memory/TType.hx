import format.hl.Data;
using format.hl.Tools;

class TType {
	public var tid : Int;
	public var t : HLType;
	public var closure : HLType;
	public var bmp : hl.Bytes;
	public var hasPtr : Bool;
	public var isDyn : Bool;
	public var fields : Array<TType>;
	public var constructs : Array<Array<TType>>;
	public var nullWrap : TType;

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

	public function buildTypes( m : Memory, tvoid : TType ) {
		if( !hasPtr ) return;

		// layout of data inside memory

		switch( t ) {
		case HObj(p):
			var protos = [p];
			while( p.tsuper != null )
				switch( p.tsuper ) {
				case HObj(p2):
					protos.unshift(p2);
					p = p2;
				default:
				}

			fields = [];
			var pos = 1 << m.ptrBits; // type
			for( p in protos )
				for( f in p.fields ) {
					var size = m.typeSize(f.t);
					pos = align(pos, size);
					fields[pos>>m.ptrBits] = m.getType(f.t);
					pos += size;
				}
			for( i in 0...pos >> m.ptrBits )
				if( fields[i] == null )
					fields[i] = tvoid;
		case HEnum(e):
			constructs = [];
			for( c in e.constructs ) {
				var pos = 4; // index
				var fields = [];
				for( t in c.params ) {
					var size = m.typeSize(t);
					pos = align(pos, size);
					fields[pos>>m.ptrBits] = m.getType(t);
					pos += size;
				}
				for( i in 0...pos >> m.ptrBits )
					if( fields[i] == null )
						fields[i] = tvoid;
				constructs.push(fields);
			}
		case HVirtual(fl):
			fields = [
				tvoid, // type
				m.getType(HDyn), // obj
				m.getType(HDyn), // next
			];
			var pos = (fl.length + 3) << m.ptrBits;
			for( f in fl ) {
				var size = m.typeSize(f.t);
				pos = align(pos, size);
				fields[pos >> m.ptrBits] = m.getType(f.t);
				pos += size;
			}
			for( i in 0...pos >> m.ptrBits )
				if( fields[i] == null )
					fields[i] = tvoid;
		case HNull(t):
			if( m.is64 )
				fields = [tvoid, m.getType(t)];
			else
				fields = [tvoid, tvoid, m.getType(t)];
		case HFun(_):
			fields = [tvoid, tvoid, tvoid, m.getType(closure)];
		default:
		}
	}

	inline function align(pos, size) {
		var d = pos & (size - 1);
		if( d != 0 ) pos += size - d;
		return pos;
	}

	public function toString() {
		return t.toString();
	}

}