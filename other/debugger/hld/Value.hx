package hld;

enum ValueRepr {
	VUndef;
	VNull;
	VInt( i : Int );
	VFloat( v : Float );
	VBool( b : Bool );
	VPointer( v : Pointer );
	VString( v : String );
	VClosure( p : FunRepr, d : Value );
	VFunction( p : FunRepr );
}

enum FunRepr {
	FUnknown( p : Pointer );
	FIndex( i : Int );
}

typedef Value = { v : ValueRepr, t : HLType };
