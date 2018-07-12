package hld;

#if hl

@:forward(getI32, getUI8, setI32, setUI8, getUI16, getF32, getF64)
abstract Buffer(hl.Bytes) {

	public function new(size) {
		this = new hl.Bytes(size);
	}

	public function readStringUCS2( pos : Int, length : Int ) {
		return @:privateAccess String.fromUCS2(this.sub(pos,(length + 1) << 1));
	}

	public function getPointer( pos : Int, align : Align ) {
		if( align.is64 )
			return Pointer.make(this.getI32(pos), this.getI32(pos + 4));
		return Pointer.make(this.getI32(pos), 0);
	}

	public function setPointer( pos : Int, v : Pointer, align : Align ) {
		if( align.is64 ) {
			var a = v.i64;
			this.setI32(pos, a.low);
			this.setI32(pos + 4, a.high);
		} else
			this.setI32(pos, v.toInt());
	}

}

#else

abstract Buffer(js.node.Buffer) {

	public function new(size) {
		this = js.node.Buffer.alloc(size);
	}

	public function getPointer( pos : Int, align : Align ) {
		if( align.is64 )
			return Pointer.make(getI32(pos), getI32(pos + 4));
		return Pointer.make(getI32(pos), 0);
	}

	public function setPointer( pos : Int, v : Pointer, align : Align ) {
		if( align.is64 ) {
			var a = v.i64;
			setI32(pos, a.low);
			setI32(pos + 4, a.high);
		} else
			setI32(pos, v.toInt());
	}

	public inline function getI32(pos) {
		return this.readInt32LE(pos);
	}

	public inline function getUI8(pos) {
		return this.readUInt8(pos);
	}

	public inline function getUI16(pos) {
		return this.readUInt16LE(pos);
	}

	public inline function getF32(pos) {
		return this.readFloatLE(pos);
	}

	public inline function getF64(pos) {
		return this.readDoubleLE(pos);
	}

	public inline function setI32(pos,value) {
		this.writeInt32LE(value, pos);
	}

	public inline function setUI16(pos,value) {
		this.writeUInt16LE(value, pos);
	}

	public inline function setUI8(pos,value) {
		this.writeUInt8(value, pos);
	}

	public function readStringUCS2(pos, length) {
		var str = "";
		for( i in 0...length ) {
			var c = getUI16(pos);
			str += String.fromCharCode(c);
			pos += 2;
		}
		return str;
	}

}

#end