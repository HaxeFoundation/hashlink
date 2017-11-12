package hld;

private enum Control {
	CNo;
	CJCond( d : Int );
	CJAlways( d : Int );
	CTry( d : Int );
	CSwitch( arr : Array<Int> );
	CRet;
	CThrow;
	CLabel;
}

class CodeBlock {

	public var start : Int;
	public var end : Int;
	public var loop : Bool;
	public var prev : Array<CodeBlock>;
	public var next : Array<CodeBlock>;

	public function new(pos) {
		start = pos;
		prev = [];
		next = [];
	}

}

class CodeGraph {

	var fun : format.hl.Data.HLFunction;
	var blockPos : Map<Int,CodeBlock>;
	var allBlocks : Map<Int,Bool>;

	public function new(f) {
		this.fun = f;
		blockPos = new Map();
		allBlocks = new Map();
		// build graph
		for( i in 0...f.ops.length )
			switch( control(i) ) {
			case CJAlways(d), CJCond(d):
				allBlocks.set(i + 1 + d, true);
			default:
			}
		makeBlock(0);
	}

	function makeBlock( pos : Int ) {
		var b = blockPos.get(pos);
		if( b != null )
			return b;
		var b = new CodeBlock(pos);
		blockPos.set(pos, b);
		var i = pos;
		while( true ) {
			inline function goto(d) {
				var b2 = makeBlock(i + 1 + d);
				b2.prev.push(b);
				return b2;
			}
			if( i > pos && allBlocks.exists(i) ) {
				b.end = i - 1;
				b.next = [goto( -1)];
				break;
			}
			switch( control(i) ) {
			case CNo:
				i++;
				continue;
			case CRet, CThrow:
				b.end = i;
			case CJAlways(d):
				b.end = i;
				b.next = [goto(d)];
			case CSwitch(pl):
				b.end = i;
				b.next.push(goto(0));
				for( p in pl )
					b.next.push(goto(p));
			case CJCond(d), CTry(d):
				b.end = i;
				b.next = [goto(0), goto(d)];
			case CLabel:
				i++;
				b.loop = true;
				continue;
			}
			break;
		}
		return b;
	}

	function control( i ) {
		return switch( fun.ops[i] ) {
		case OJTrue (_,d), OJFalse (_,d), OJNull (_,d), OJNotNull (_,d),
			OJSLt (_, _, d), OJSGte (_, _, d), OJSGt (_, _, d), OJSLte (_, _, d),
			OJULt (_, _, d), OJUGte (_, _, d), OJEq (_, _, d), OJNotEq (_, _, d),
			OJNotLt (_,_,d), OJNotGte (_,_,d):
				CJCond(d);
		case OJAlways(d):
			CJAlways(d);
		case OLabel:
			CLabel;
		case ORet(_):
			CRet;
		case OThrow(_), ORethrow(_):
			CThrow;
		case OSwitch(_,cases,_):
			CSwitch(cases);
		case OTrap(_,d):
			CTry(d);
		default:
			CNo;
		}
	}

}
