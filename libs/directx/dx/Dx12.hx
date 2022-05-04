package dx;

import haxe.Int64;

typedef DriverInstance = hl.Abstract<"dx_driver">;

enum DriverInitFlag {
	Debug;
}

enum abstract CommandListType(Int) {
	public var Direct = 0;
	public var Bundle = 1;
	public var Compute = 2;
	public var Copy = 3;
	public var VideoDecode = 4;
	public var VideoProcess = 5;
	public var VideoEncode = 6;
}

typedef DriverInitFlags = haxe.EnumFlags<DriverInitFlag>;

@:hlNative("dx12","resource_")
abstract Resource(hl.Abstract<"dx_resource">) {
	public function release() {}
}

abstract PipelineState(Resource) {
}

@:hlNative("dx12","command_allocator_")
abstract CommandAllocator(Resource) {
	public function new(type) {
		this = create(type);
	}
	public function reset() {}
	static function create( type : CommandListType ) : Resource { return null; }
}

@:hlNative("dx12","command_list_")
abstract CommandList(Resource) {
	public function new(mask,type,alloc,state) {
		this = create(mask,type,alloc,state);
	}
	public function close() {}
	public function execute() {}
	public function clearRenderTargetView( rtv : Address, color : ClearColor ) {}
	public function reset( alloc : CommandAllocator, state : PipelineState ) {}
	public function resourceBarrier( b : ResourceBarrier ) {}
	static function create( mask : Int, type : CommandListType, alloc : CommandAllocator, state : PipelineState ) : Resource { return null; }
}

enum abstract FenceFlags(Int) {
	var None = 0;
	var Shared = 1;
	var SharedCrossAdapter = 2;
	var NonMonitored = 4;
}

@:hlNative("dx12","fence_")
abstract Fence(Resource) {
	public function new(value,flags) {
		this = create(value, flags);
	}
	@:hlNative("dx12","fence_get_completed_value")
	public function getValue() : Int64 { return 0; }
	public function setEvent( value : Int64, event : WaitEvent ) {}
	static function create( value : Int64, flags : FenceFlags ) : Resource { return null; }
}

@:hlNative("dx12","waitevent_")
abstract WaitEvent(hl.Abstract<"dx_event">) {

	public function new(state) {
		this = cast create(state);
	}

	public function wait( time : Int ) : Bool { return false; }
	static function create( state : Bool ) : WaitEvent { return null; }
}

enum abstract DescriptorHeapType(Int) {
	var CBV_SRV_UAV = 0;
	var SAMPLER = 1;
	var RTV = 2;
	var DSV = 3;
}

enum abstract DescriptorHeapFlags(Int) {
	var None = 0;
	var ShaderVisible = 1;
}

@:struct class DescriptorHeapDesc {
	public var type : DescriptorHeapType;
	public var numDescriptors : Int;
	public var flags : DescriptorHeapFlags;
	public var nodeMask : Int;
	public function new() {
	}
}

abstract Address(Int64) from Int64 {
	public inline function offset( delta : Int ) : Address {
		return cast this + delta;
	}
}

@:hlNative("dx12","compiler_")
abstract ShaderCompiler(hl.Abstract<"dx_compiler">) {
	public function new() {
		this = cast create();
	}
	public function compile( source : String, profile : String, args : Array<String> ) : haxe.io.Bytes {
		var outLen = 0;
		var nargs = new hl.NativeArray(args.length);
		for( i in 0...args.length )
			nargs[i] = @:privateAccess args[i].bytes;
		var bytes = do_compile(cast this, @:privateAccess source.bytes, @:privateAccess profile.bytes, nargs, outLen);
		return @:privateAccess new haxe.io.Bytes(bytes, outLen);
	}
	@:hlNative("dx12","compiler_compile")
	static function do_compile( comp : ShaderCompiler, source : hl.Bytes, profile : hl.Bytes, args : hl.NativeArray<hl.Bytes>, out : hl.Ref<Int> ) : hl.Bytes {
		return null;
	}
	static function create() : ShaderCompiler { return null; }
}

@:hlNative("dx12","descriptor_heap_")
abstract DescriptorHeap(Resource) {
	public function new(desc) {
		this = create(desc);
	}

	@:hlNative("dx12","descriptor_heap_get_handle")
	public function getHandle( gpu : Bool ) : Address { return cast null; }
	static function create( desc : DescriptorHeapDesc ) : Resource { return null; }
}

enum abstract ResourceBarrierType(Int) {
	var Transition = 0;
	var Aliasing = 1;
	var UAV = 2;
}

enum abstract ResourceBarrierFlags(Int) {
	var None = 0;
	var BeginOnly = 1;
	var EndOnly = 2;
}

enum abstract ResourceState(Int) {
	public var Common = 0;
	public var VertexAndConstantBuffer = 0x1;
	public var IndexBuffer = 0x2;
	public var RenderTarget = 0x4;
	public var UnorderedAccess = 0x8;
	public var DepthWrite = 0x10;
	public var DepthRead = 0x20;
	public var NonPixelShaderResource = 0x40;
	public var PixelShaderResource = 0x80;
	public var StreamOut = 0x100;
	public var IndirectArgument = 0x200;
	public var CopyDest = 0x400;
	public var CopySource = 0x800;
	public var ResolveDest = 0x1000;
	public var ResolveSource = 0x2000;
	public var RaytracingAccelerationStructure = 0x400000;
	public var ShadingRateSource = 0x1000000;
	public var GenericRead = 0x1 | 0x2 | 0x40  | 0x80  | 0x200  | 0x800;
	public var AllShaderResource = 0x40 | 0x80;
	public var Present = 0;
	public var Predication = 0x200;
	public var VideoDecodeRead = 0x10000;
	public var VideoDecodeWrite = 0x20000;
	public var VideoProcessRead = 0x40000;
	public var VideoProcessWrite = 0x80000;
	public var VideoEncodeRead = 0x200000;
	public var VideoEncodeWrite = 0x800000;
}

@:struct class ClearColor {
	public var r : Single;
	public var g : Single;
	public var b : Single;
	public var a : Single;
	public function new() {
	}
}

@:struct class ResourceBarrier {
	var type : ResourceBarrierType;
	public var flags : ResourceBarrierFlags;
	public var resource : Resource;
	public var subResource : Int;
	public var stateBefore : ResourceState;
	public var stateAfter : ResourceState;
	public function new() { type = Transition; }
}

@:struct class RenderTargetViewDesc {
}

@:hlNative("dx12")
class Dx12 {

	public static function create( win : Window, flags : DriverInitFlags ) {
		return dxCreate(@:privateAccess win.win, flags);
	}

	public static function flushMessages() {
	}

	public static function getDescriptorHandleIncrementSize( type : DescriptorHeapType ) : Int {
		return 0;
	}

	public static function getBackBuffer( index : Int ) : Resource {
		return null;
	}

	public static function getCurrentBackBufferIndex() : Int {
		return 0;
	}

	public static function createRenderTargetView( buffer : Resource, desc : RenderTargetViewDesc, target : Address ) {
	}

	public static function resize( width : Int, height : Int, bufferCount : Int ) : Bool {
		return false;
	}

	public static function signal( fence : Fence, value : Int64 ) {
	}

	public static function present( vsync : Bool ) {
	}

	public static function getDeviceName() {
		return @:privateAccess String.fromUCS2(dxGetDeviceName());
	}

	@:hlNative("dx12", "get_device_name")
	static function dxGetDeviceName() : hl.Bytes {
		return null;
	}

	@:hlNative("dx12", "create")
	static function dxCreate( win : hl.Abstract<"dx_window">, flags : DriverInitFlags ) : DriverInstance {
		return null;
	}

}