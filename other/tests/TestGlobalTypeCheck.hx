class CustomError {
    public var msg:String;
    public function new(m:String) {
        msg = m;
    }
}

class TestGlobalTypeCheck {
    static function main() {
        trace("Starting global type check test...");
        
        // Test 1: Try-Catch with specific type
        // This exercises the OCatch path in the JIT where it loads the global type
        var caught = false;
        try {
            throw new CustomError("test error");
        } catch( e : CustomError ) {
            trace("Caught CustomError successfully: " + e.msg);
            caught = true;
        } catch( e : Dynamic ) {
            trace("Failed to match CustomError, caught as Dynamic: " + e);
        }

        if (!caught) {
            trace("Test 1 FAILED: Did not catch CustomError");
            Sys.exit(1);
        }

        // Test 2: Std.is
        // This exercises the OGetGlobal + OCall2 (likely Std.is implementation details) path
        var c = new CustomError("check");
        if( Std.isOfType(c, CustomError) ) {
            trace("Std.isOfType(c, CustomError) is true");
        } else {
            trace("Std.isOfType(c, CustomError) is false (FAILED)");
            Sys.exit(1);
        }
        
        trace("All tests passed");
    }
}
