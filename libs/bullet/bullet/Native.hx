package bullet;

typedef Native = haxe.macro.MacroType<[webidl.Module.build({ file : "bullet.idl", chopPrefix : "bt", autoGC : true, nativeLib : "bullet" })]>;
