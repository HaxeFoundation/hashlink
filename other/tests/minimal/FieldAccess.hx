// Test 4: Object field access
class Point {
    public var x:Int;
    public var y:Int;
    public function new(x:Int, y:Int) {
        this.x = x;
        this.y = y;
    }
}

class FieldAccess {
    static function main() {
        var p = new Point(10, 20);
        var sum = p.x + p.y;
    }
}
