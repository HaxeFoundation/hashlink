package hld;

@:forward(getI32,getUI8,setI32,setUI8)
abstract Buffer(hl.Bytes) {

	public function new(size) {
		this = new hl.Bytes(size);
	}

}