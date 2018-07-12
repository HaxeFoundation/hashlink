package hld;

enum ValueRepr {
	VUndef;
	VNull;
	VInt( i : Int );
	VFloat( v : Float );
	VBool( b : Bool );
	VPointer( v : Pointer );
	VString( v : String, p : Pointer );
	VClosure( f : FunRepr, d : Value, p : Pointer );
	VFunction( f : FunRepr, p : Pointer );
	VArray( t : HLType, length : Int, read : Int -> Value, p : Pointer );
	VMap( tkey : HLType, nkeys : Int, readKey : Int -> Value, readValue : Int -> Value, p : Pointer );
	VType( t : HLType );
	VEnum( c : String, values : Array<Value>, p : Pointer );
	VBytes( length : Int, read : Int -> Int, p : Pointer );
}

enum FunRepr {
	FUnknown( p : Pointer );
	FIndex( i : Int );
}

typedef Value = { v : ValueRepr, t : HLType };
