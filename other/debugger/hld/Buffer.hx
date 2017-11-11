package hld;

@:forward(getI32, getUI8, setI32, setUI8, getUI16, getF32, getF64)
abstract Buffer(hl.Bytes) {

	public function new(size) {
		this = new hl.Bytes(size);
	}

	public function readStringUCS2( pos : Int, length : Int ) {
		return @:privateAccess String.fromUCS2(this.sub(pos,(length + 1) << 1));
	}

}