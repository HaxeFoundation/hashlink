class Node {
	public var id : Int;
	public var next : Node;
	public var other : Node;
	public function new( id ) {
		this.id = id;
	}
}

class Test {
	static var nodes : Array<Node>;
	static var head : Node;
	static var map : Map<Node, Int>;
	public static function check( step : Int ) {
		if( nodes == null ) {
			var n = 200000;
			nodes = [for( i in 0...n ) new Node(i)];
			for( i in 0...n ) {
				nodes[i].next = i + 1 < n ? nodes[i + 1] : null;
				nodes[i].other = nodes[(i * 7919) % n];
			}
			head = nodes[0];
			map = new Map();
			for( i in 0...50000 ) map.set(nodes[i * 4], i);
		}
		var count = 0, sum = 0.;
		var n = head;
		while( n != null ) {
			if( n.other.id == (n.id * 7919) % nodes.length ) count++;
			n = n.next;
		}
		var found = 0;
		for( i in 0...50000 ) if( map.get(nodes[i * 4]) == i ) found++;
		Sys.println("v0 step " + step + " linked=" + count + " map=" + found + " same=" + (head == nodes[0]));
	}
}
