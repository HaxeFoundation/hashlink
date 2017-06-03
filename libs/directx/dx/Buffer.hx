package dx;

@:enum abstract BufferUsage(Int) {
	var Default = 0;
	var Immutable = 1;
	var Dynamic = 2;
	var Staging = 3;
}

@:enum abstract BufferBind(Int) {
	var VertexBuffer = 1;
	var IndexBuffer = 2;
	var ConstantBuffer = 4;
	var ShaderResource = 8;
	var StreamOuput = 16;
	var RenderTarget = 32;
	var DepthStencil = 64;
	var UnorderedAccess = 128;
	var Decoder = 512;
	var VideoDecoder = 1024;
	@:op(a | b) static function or(a:BufferBind, b:BufferBind) : BufferBind;
}

@:enum abstract BufferAccess(Int) {
	var None = 0;
	var CpuWrite = 0x10000;
	var CpuRead = 0x20000;
	@:op(a | b) static function or(a:BufferAccess, b:BufferAccess) : BufferAccess;
}

@:enum abstract BufferMisc(Int) {
	var None = 0;
	var GenerateMips = 1;
	var Shared = 2;
	var TextureCube = 4;
	var DrawIndirectArgs = 0x10;
	var BufferAllowRawView = 0x20;
	var BufferStructured = 0x40;
	var ResourceClamp = 0x80;
	var SharedKeyedMutex = 0x100;
	var GdiCompatible = 0x200;
	var SharedNTHandle = 0x800;
	var RestrictedContent = 0x1000;
	var RestrictSharedResource = 0x2000;
	var RestrictSharedResourceDriver = 0x4000;
	var Guarded = 0x8000;
	var TilePool = 0x20000;
	var Tiled = 0x40000;
	var HWProtected = 0x80000;
	@:op(a | b) static function or(a:BufferMisc, b:BufferMisc) : BufferMisc;
}

abstract Buffer(hl.Abstract<"dx_buffer">) {

	@:hlNative("directx","dx_create_buffer")
	public static function alloc( size : Int, usage : BufferUsage, bind : BufferBind, access : BufferAccess, misc : BufferMisc, stride : Int, data : hl.Bytes ) : Buffer {
		return null;
	}

	@:hlNative("directx", "dx_release_buffer")
	public function release() {
	}

}