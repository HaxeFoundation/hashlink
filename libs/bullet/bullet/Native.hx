package bullet;

typedef Native = haxe.macro.MacroType<[webidl.Module.build({ idlFile : "bullet.idl", chopPrefix : "bt", autoGC : true, nativeLib : "bullet" })]>;
