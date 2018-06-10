package hld;

@:enum
abstract CType(String) {

	public var int = 'int';
	public var bool = 'bool';
	public var string = 'string';
	public var pointer = 'pointer';
	public var long = 'long';
	public var size_t = 'size_t';

	var self(get, never) : CType;

	inline function new(v) { this = v; }
	inline function get_self() return new CType(this);

	public inline function ref() : CType {
		return Ref.refType(self);
	}

	public inline function alloc() : CValue {
		return Ref.alloc(self);
	}

}


extern class CValue extends js.node.Buffer {

	public function deref() : CValue;
	public function ref() : CValue;
	public function address() : Int;
	public function hexAddress() : String;
	public function isNull() : Bool;

}

abstract CStruct(Dynamic) {
	public function new() {
		this = untyped require('ref-struct')();
	}
	public inline function defineProperty( name : String, t : CType ) {
		this.defineProperty(name, t);
	}
	public inline function alloc( obj : {} ) : CValue {
		return js.Syntax.construct(this, [obj]);
	}
}

@:jsRequire('ref')
extern class Ref {

	public static var types : {
		var void : CType;
		var CString : CType;
	};

	public static function refType( t : CType ) : CType;
	public static function alloc( t : CType ) : CValue;
	public static function readPointer( buf : Buffer, offset : Int, ?length : Int ) : CValue;

}

@:jsRequire('ffi')
extern class NodeFFI {

	public static function Library( name : String, decls : {} ) : Dynamic;

	public static inline function makeDecl( ret : CType, args : Array<CType> ) : Dynamic {
		return cast ([ret, args] : Array<Dynamic>);
	}

}