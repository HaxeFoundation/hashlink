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
	VArray( t : HLType, length : Int, read : Int -> Value );
	VMap( tkey : HLType, nkeys : Int, readKey : Int -> Value, readValue : Int -> Value );
	VType( t : HLType );
	VEnum( c : String, values : Array<Value> );
}

enum FunRepr {
	FUnknown( p : Pointer );
	FIndex( i : Int );
}

typedef Value = { v : ValueRepr, t : HLType };
