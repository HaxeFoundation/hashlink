class Arm64JitTest {

    static function testFloatRegPressure() {
        trace("Testing Float (Double) Register Pressure...");
        var v0 = 1.1;
        var v1 = 2.2;
        var v2 = 3.3;
        var v3 = 4.4;
        var v4 = 5.5;
        var v5 = 6.6;
        var v6 = 7.7;
        var v7 = 8.8; 
        var v8 = 9.9; 
        
        var val = Math.random() > 0.5 ? 123.456 : 123.456;
        
        // Use Float (Double) array
        var arr = new hl.NativeArray<Float>(10);
        arr[0] = 1.0; 
        
        arr[1] = val; 

        var sum = v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8;
        var expected = 1.1 + 2.2 + 3.3 + 4.4 + 5.5 + 6.6 + 7.7 + 8.8 + 9.9;
        
        if (Math.abs(sum - expected) > 0.0001) {
            throw "Float register corruption detected! Sum: " + sum + ", Expected: " + expected;
        }
        
        if (arr[1] != 123.456) {
             throw "Double Array write failed! Got " + arr[1];
        }
        trace("Float (Double) Register Pressure Test Passed");
    }

    static function testSingleRegPressure() {
        trace("Testing Single (F32) Register Pressure...");
        // Use Singles
        var v0 : Single = 1.5;
        var v1 : Single = 2.5;
        var v2 : Single = 3.5;
        var v3 : Single = 4.5;
        var v4 : Single = 5.5;
        var v5 : Single = 6.5;
        var v6 : Single = 7.5;
        var v7 : Single = 8.5;
        var v8 : Single = 9.5;
        
        var val : Single = 123.5; // Exact in float32
        
        var arr = new hl.NativeArray<Single>(10);
        arr[1] = val;
        
        var sum = v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7 + v8;
        var expected = 1.5 + 2.5 + 3.5 + 4.5 + 5.5 + 6.5 + 7.5 + 8.5 + 9.5;
        
        if (Math.abs(sum - expected) > 0.001) {
             throw "Single register corruption detected!";
        }
        
        if (arr[1] != 123.5) {
            throw "Single Array write failed! Got " + arr[1];
        }
        trace("Single (F32) Register Pressure Test Passed");
    }

    static function testIntRegPressure() {
        trace("Testing Int Register Pressure...");
        var i0 = 10;
        var i1 = 11;
        var i2 = 12;
        var i3 = 13;
        var i4 = 14;
        var i5 = 15;
        var i6 = 16;
        var i7 = 17;
        var i8 = 18;
        var i9 = 19; 
        
        // Use a value calculated at runtime to avoid immediate encoding optimizations if possible
        var val = Std.int(Math.random() * 0) + 999;
        
        var arr = new hl.NativeArray<Int>(10);
        arr[0] = val;
        
        var sum = i0 + i1 + i2 + i3 + i4 + i5 + i6 + i7 + i8 + i9;
        var expected = 10+11+12+13+14+15+16+17+18+19;
        
        if (sum != expected) {
             throw "Int register corruption detected! Sum: " + sum + ", Expected: " + expected;
        }
        
        if (arr[0] != 999) {
            throw "Int Array write failed";
        }
        trace("Int Register Pressure Test Passed");
    }
    
    static function testMemOps() {
        trace("Testing Memory Ops (structs)...");
        // Test struct field access which uses op_get_mem / op_set_mem
        var c = new TestClass();
        c.a = 1;
        c.b = 2.5;
        c.c = 3;
        
        var val = c.a + Std.int(c.b) + c.c;
        if (val != 6) throw "Memory Op test failed";
        trace("Memory Ops Test Passed");
    }

    static function main() {
        try {
            testFloatRegPressure();
            testSingleRegPressure();
            testIntRegPressure();
            testMemOps();
            trace("All tests passed!");
        } catch(e:Dynamic) {
            trace("TEST FAILED: " + e);
            Sys.exit(1);
        }
    }
}

class TestClass {
    public var a : Int;
    public var b : Float;
    public var c : Int;
    public function new() {}
}
