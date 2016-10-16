class TreeNode {

	var left : TreeNode;
	var right : TreeNode;
	var item : Int;

	public function new(left,right,item){
	   this.left = left;
	   this.right = right;
	   this.item = item;
	}

	public function itemCheck() {
		if( left == null) return item;
		return item + left.itemCheck() - right.itemCheck();
	}

}

@:result(26208)
class BinaryTrees {

	static function bottomUpTree(item,depth){
		if (depth>0)
			return new TreeNode(
				bottomUpTree(2*item-1, depth-1)
				,bottomUpTree(2*item, depth-1)
				,item
			);
		return new TreeNode(null,null,item);
	}

	static function main() {
		var minDepth = 4;
		var n = 14;
		var maxDepth = Std.int(Math.max(minDepth + 2, n));
		var stretchDepth = maxDepth + 1;
		var check = bottomUpTree(0, stretchDepth).itemCheck();

		var result = check;

		var longLivedTree = bottomUpTree(0,maxDepth);
		var depth = minDepth;
		while( depth <= maxDepth ) {
			var iterations = 1 << (maxDepth - depth + minDepth);
			check = 0;
			for( i in 0...iterations ) {
				check += bottomUpTree(i,depth).itemCheck();
				check += bottomUpTree(-i,depth).itemCheck();
			}
			result ^= check;
			//console.log(iterations*2 + "\t trees of depth " + depth + "\t check: " + check);
			depth += 2;
		}

		result ^= longLivedTree.itemCheck();
		//console.log("long lived tree of depth " + maxDepth + "\t check: " + longLivedTree.itemCheck());
		Benchs.result(result);
	}
}
