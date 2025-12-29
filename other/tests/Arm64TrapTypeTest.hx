class Arm64TrapTypeTest {
    static function test() {
        trace("Testing OTrap type checking...");
        var caught = false;
        try {
            throw new MyError("test");
        } catch(e:MyError) {
            caught = true;
            trace("Caught specific error!");
        } catch(e:Dynamic) {
            trace("Caught dynamic error (wrong!)");
        }
        
        if (!caught) {
            trace("FAILED: Did not catch MyError in specific catch block");
            Sys.exit(1);
        } else {
            trace("PASSED: Caught MyError correctly");
        }
    }

    static function main() {
        test();
    }
}

class MyError {
    var msg:String;
    public function new(m:String) { msg = m; }
}
