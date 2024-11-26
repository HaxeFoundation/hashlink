package dx;

import haxe.Int64;

typedef DriverInstance = hl.Abstract<"dx_driver">;

enum DriverInitFlag {
	DEBUG;
}

enum abstract Constant(Int) to Int {
	public var TEXTURE_DATA_PITCH_ALIGNMENT;
	public var TEXTURE_DATA_PLACEMENT_ALIGNMENT;
	public var DESCRIPTOR_RANGE_OFFSET_APPEND;
}

enum abstract CommandListType(Int) {
	public var DIRECT = 0;
	public var BUNDLE = 1;
	public var COMPUTE = 2;
	public var COPY = 3;
	public var VIDEO_DECODE = 4;
	public var VIDEO_PROCESS = 5;
	public var VIDEO_ENCODE = 6;
}

typedef DriverInitFlags = haxe.EnumFlags<DriverInitFlag>;

@:hlNative("dx12","resource_")
abstract Resource(hl.Abstract<"dx_resource">) {
	public function release() {}
	public inline function setName( name : String ) {
		set_name(@:privateAccess name.bytes);
	}
	function set_name( name : hl.Bytes ) {}
}

@:struct class Range {
	public var begin : Int64;
	public var end : Int64;
	public function new() {
	}
}

@:hlNative("dx12","resource_") @:forward(release, setName)
abstract GpuResource(Resource) {
	@:hlNative("dx12","resource_get_gpu_virtual_address")
	public function getGpuVirtualAddress() : Int64 { return 0; }
	@:hlNative("dx12","get_required_intermediate_size")
	public function getRequiredIntermediateSize( subRes : Int, resCount : Int ) : Int64 { return 0; }
	public function map( subResource : Int, range : Range ) : hl.Bytes { return null; }
	public function unmap( subResource : Int, writtenRange : Range ) {}
	@:to inline function to() : Resource { return cast this; }
}

abstract PipelineState(Resource) {
}

@:forward(release)
abstract CommandSignature(Resource) {
}

@:hlNative("dx12","command_allocator_")
abstract CommandAllocator(Resource) {
	public function new(type) {
		this = create(type);
	}
	public function reset() {}
	static function create( type : CommandListType ) : Resource { return null; }
}

enum abstract ClearFlags(Int) {
	var DEPTH = 1;
	var STENCIL = 2;
	var BOTH = 3;
}

enum abstract PrimitiveTopology(Int) {
	var UNDEFINED = 0;
	var POINTLIST = 1;
	var LINELIST = 2;
	var LINESTRIP = 3;
	var TRIANGLELIST = 4;
	var TRIANGLESTRIP = 5;
	var LINELIST_ADJ = 10;
	var LINESTRIP_ADJ = 11;
	var TRIANGLELIST_ADJ = 12;
	var TRIANGLESTRIP_ADJ = 13;
	public static function controlPointPatchList( count : Int ) : PrimitiveTopology {
		return cast (32 + count);
	}
}

@:struct class VertexBufferView {
	public var bufferLocation : Address;
	public var sizeInBytes : Int;
	public var strideInBytes : Int;
	public function new() {
	}
}

@:struct class IndexBufferView {
	public var bufferLocation : Address;
	public var sizeInBytes : Int;
	public var format : DxgiFormat;
	public function new() {
	}
}

@:struct class Viewport {
	public var topLeftX : Single;
	public var topLeftY : Single;
	public var width : Single;
	public var height : Single;
	public var minDepth : Single;
	public var maxDepth : Single;
	public function new() {
	}
}

@:struct class Rect {
	public var left : Int;
	public var top : Int;
	public var right : Int;
	public var bottom : Int;
	public function new() {
	}
}

@:struct class Box {
	public var left : Int;
	public var top : Int;
	public var front : Int;
	public var right : Int;
	public var bottom : Int;
	public var back : Int;
	public function new() {
	}
}

enum abstract TextureCopyType(Int) {
	var SUBRESOURCE_INDEX = 0;
	var PLACED_FOOTPRINT = 1;
}

@:struct class SubresourceFootprint {
	public var format : DxgiFormat;
	public var width : Int;
	public var height : Int;
	public var depth : Int;
	public var rowPitch : Int;
	public function new() {
	}
}

@:struct class PlacedSubresourceFootprint {
	public var offset : Int64;
	@:packed public var footprint(default,null) : SubresourceFootprint;
	public function new() {
	}
}

@:struct class TextureCopyLocation {
	public var res : GpuResource;
	public var type : TextureCopyType;
	var __unionPadding : Int;

	public var subResourceIndex(get,set) : Int;
	inline function get_subResourceIndex() : Int return placedFootprint.offset.low & 0xFF;
	inline function set_subResourceIndex(v: Int) { placedFootprint.offset = v; return v; }

	@:packed public var placedFootprint(default,null) : PlacedSubresourceFootprint;
	public function new() {
	}
}

@:hlNative("dx12","command_list_")
abstract CommandList(Resource) {
	public function new(type,alloc,state) {
		this = create(type,alloc,state);
	}

	public function close() {}
	public function execute() {}
	public function clearRenderTargetView( rtv : Address, color : ClearColor ) {}
	public function clearDepthStencilView( rtv : Address, flags : ClearFlags, depth : Single, stencil : Int ) {}
	public function reset( alloc : CommandAllocator, state : PipelineState ) {}
	public function resourceBarrier( b : ResourceBarrier ) {}
	public function resourceBarriers( b : hl.CArray<ResourceBarrier>, barrierCount : Int ) {}
	public function setPipelineState( state : PipelineState ) {}
	public function setDescriptorHeaps( heaps : hl.NativeArray<DescriptorHeap> ) {}
	public function copyBufferRegion( dst : GpuResource, dstOffset : Int64, src : GpuResource, srcOffset : Int64, size : Int64 ) {}
	public function copyTextureRegion( dst : TextureCopyLocation, dstX : Int, dstY : Int, dstZ : Int, src : TextureCopyLocation, srcBox : Box ) {}

	public function setGraphicsRootSignature( sign : RootSignature ) {}
	public function setGraphicsRoot32BitConstants( index : Int, numValues : Int, data : hl.Bytes, dstOffset : Int ) {}
	public function setGraphicsRootConstantBufferView( index : Int, address : Address ) {}
	public function setGraphicsRootDescriptorTable( index : Int, address : Address ) {}
	public function setGraphicsRootShaderResourceView( index : Int, address : Address ) {}
	public function setGraphicsRootUnorderedAccessView( index : Int, address : Address ) {}

	public function iaSetPrimitiveTopology( top : PrimitiveTopology ) {}
	public function iaSetVertexBuffers( startSlot : Int, numViews : Int, views : VertexBufferView /* hl.CArray */ ) {}
	public function iaSetIndexBuffer( view : IndexBufferView ) {}

	public function drawInstanced( vertexCountPerInstance : Int, instanceCount : Int, startVertexLocation : Int, startInstanceLocation : Int ) {}
	public function drawIndexedInstanced( indexCountPerInstance : Int, instanceCount : Int, startIndexLocation : Int, baseVertexLocation : Int, startInstanceLocation : Int ) {}
	public function executeIndirect( sign : CommandSignature, maxCommandCount : Int, args : Resource, argsOffset : Int64, count : Resource, countOffset : Int64 ) {}

	public function omSetRenderTargets( count : Int, handles : hl.BytesAccess<Address>, flag : Bool32, depthStencils : hl.BytesAccess<Address> ) {}
	public function omSetStencilRef( value : Int ) {}

	public function rsSetViewports( count : Int, viewports : Viewport ) {}
	public function rsSetScissorRects( count : Int, rects : Rect ) {}

	public function beginQuery( heap : QueryHeap, type : QueryType, index : Int ) {}
	public function endQuery( heap : QueryHeap, type : QueryType, index : Int ) {}
	public function resolveQueryData( heap : QueryHeap, type : QueryType, index : Int, count : Int, dest : Resource, offset : Int64 ) {}

	public function setPredication( res : Resource, offset : Int64, op : PredicationOp ) {}

	public function setComputeRootSignature( sign : RootSignature ) {}
	public function setComputeRoot32BitConstants( index : Int, numValues : Int, data : hl.Bytes, dstOffset : Int ) {}
	public function setComputeRootConstantBufferView( index : Int, address : Address ) {}
	public function setComputeRootDescriptorTable( index : Int, address : Address ) {}
	public function setComputeRootShaderResourceView( index : Int, address : Address ) {}
	public function setComputeRootUnorderedAccessView( index : Int, address : Address ) {}
	public function dispatch( x : Int, y : Int, z : Int ) {}

	static function create( type : CommandListType, alloc : CommandAllocator, state : PipelineState ) : Resource { return null; }
}

enum abstract FenceFlags(Int) {
	var NONE = 0;
	var SHARED = 1;
	var SHARED_CROSS_ADAPTER = 2;
	var NON_MONITORED = 4;
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
	var NONE = 0;
	var SHADER_VISIBLE = 1;
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

	public var value(get,never) : Int64;

	public inline function new( v : Int64 ) {
		this = v;
	}

	inline function get_value() return this;

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
		/*
			Compiling source can trigger a validation error if DXCOMPILER.DLL is missing
		*/
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
abstract DescriptorHeap(Resource) to Resource {
	public function new(desc) {
		this = create(desc);
	}

	@:hlNative("dx12","descriptor_heap_get_handle")
	public function getHandle( gpu : Bool ) : Address { return cast null; }
	static function create( desc : DescriptorHeapDesc ) : Resource { return null; }
}

enum abstract ResourceBarrierType(Int) {
	var TRANSITION = 0;
	var ALIASING = 1;
	var UAV = 2;
}

enum abstract ResourceBarrierFlags(Int) {
	var NONE = 0;
	var BEGIN_ONLY = 1;
	var END_ONLY = 2;
}

enum abstract ResourceState(Int) {
	public var COMMON = 0;
	public var VERTEX_AND_CONSTANT_BUFFER = 0x1;
	public var INDEX_BUFFER = 0x2;
	public var RENDER_TARGET = 0x4;
	public var UNORDERED_ACCESS = 0x8;
	public var DEPTH_WRITE = 0x10;
	public var DEPTH_READ = 0x20;
	public var NON_PIXEL_SHADER_RESOURCE = 0x40;
	public var PIXEL_SHADER_RESOURCE = 0x80;
	public var STREAM_OUT = 0x100;
	public var INDIRECT_ARGUMENT = 0x200;
	public var COPY_DEST = 0x400;
	public var COPY_SOURCE = 0x800;
	public var RESOLVE_DESC = 0x1000;
	public var RESOLVE_SOURCE = 0x2000;
	public var RAYTRACING_ACCELERATION_STRUCTURE = 0x400000;
	public var SHADING_RATE_SOURCE = 0x1000000;
	public var GENERIC_READ = 0x1 | 0x2 | 0x40 | 0x80 | 0x200 | 0x800;
	public var ALL_SHADER_RESOURCE = 0x40 | 0x80;
	public var PRESENT = #if ( console && !xbogdk ) 0x100000 #else 0 #end;
	public var PREDICATION = 0x200;
	public var VIDE_DECODE_READ = 0x10000;
	public var VIDE_DECODE_WRITE = 0x20000;
	public var VIDE_PROCESS_READ = 0x40000;
	public var VIDE_PROCESS_WRITE = 0x80000;
	public var VIDE_ENCODE_READ = 0x200000;
	public var VIDE_ENCODE_WRITE = 0x800000;
	@:op(a|b) function or(r:ResourceState):ResourceState;
	@:op(a&b) function and(r:ResourceState):ResourceState;
}

@:struct class Color {
	public var r : Single;
	public var g : Single;
	public var b : Single;
	public var a : Single;
	public function new() {
	}
}

typedef ClearColor = Color;

@:struct class ResourceBarrier {
	var type : ResourceBarrierType;
	public var flags : ResourceBarrierFlags;
	public var resource : Resource;
	public var subResource : Int;
	public var stateBefore : ResourceState;
	public var stateAfter : ResourceState;
	public function new() { type = TRANSITION; }
}

enum abstract DsvDimension(Int) {
	var UNKNOWN	= 0;
	var TEXTURE1D = 1;
	var TEXTURE1DARRAY = 2;
	var TEXTURE2D = 3;
	var TEXTURE2DARRAY = 4;
	var TEXTURE2DMS = 5;
	var TEXTURE2DMSARRAY = 6;
}

enum abstract RtvDimension(Int) {
	var UNKNOWN	= 0;
	var BUFFER = 1;
	var TEXTURE1D = 2;
	var TEXTURE1DARRAY = 3;
	var TEXTURE2D = 4;
	var TEXTURE2DARRAY = 5;
	var TEXTURE2DMS = 6;
	var TEXTURE2DMSARRAY = 7;
	var TEXTURE3D = 8;
}

@:struct class RenderTargetViewDesc {
	public var format : DxgiFormat;
	public var viewDimension : RtvDimension;

	var int0 : Int;
	var int1 : Int;
	var int2 : Int;
	var int3 : Int;

	public var mipSlice(get,set) : Int;

	public var firstElement(get,set) : Int;
	public var numElements(get,set) : Int;

	public var firstArraySlice(get,set) : Int;
	public var arraySize(get,set) : Int;
	public var planeSlice(get,set) : Int;

	inline function get_firstElement() return int0;
	inline function set_firstElement(v) return int0 = v;
	inline function get_numElements() return int1;
	inline function set_numElements(v) return int1 = v;
	inline function get_mipSlice() return int0;
	inline function set_mipSlice(v) return int0 = v;

	inline function get_firstArraySlice() return switch( viewDimension ) { case TEXTURE2DMSARRAY: int0; default: int1; };
	inline function set_firstArraySlice(v) return switch( viewDimension ) { case TEXTURE2DMSARRAY: int0 = v; default: int1 = v; };

	inline function get_arraySize() return switch( viewDimension ) { case TEXTURE2DMSARRAY: int1; default: int2; };
	inline function set_arraySize(v) return switch( viewDimension ) { case TEXTURE2DMSARRAY: int1 = v; default: int2 = v; };

	inline function get_planeSlice() return switch( viewDimension ) { case TEXTURE2D: int1; default: int3; };
	inline function set_planeSlice(v) return switch( viewDimension ) { case TEXTURE2D: int1 = v; default: int3 = v; };

	function new() {
	}
}


enum DsvFlags {
	READ_ONLY_DEPTH;
	READ_ONLY_STENCIL;
}

@:struct class DepthStencilViewDesc {
	public var format : DxgiFormat;
	public var viewDimension : DsvDimension;
	public var flags : haxe.EnumFlags<DsvFlags>;
	public var mipSlice : Int;
	public var firstArraySlice : Int;
	public var arraySize : Int;
	public function new() {
	}
}

@:struct class ConstantBufferViewDesc {
	public var bufferLocation : Address;
	public var sizeInBytes : Int;
	public function new() {
	}
}

enum RootSignatureFlag {
	ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	DENY_VERTEX_SHADER_ROOT_ACCESS;
	DENY_HULL_SHADER_ROOT_ACCESS;
	DENY_DOMAIN_SHADER_ROOT_ACCESS;
	DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	DENY_PIXEL_SHADER_ROOT_ACCESS;
	ALLOW_STREAM_OUTPUT;
	LOCAL_ROOT_SIGNATURE;
	DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
	DENY_MESH_SHADER_ROOT_ACCESS;
	CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
	SAMPLER_HEAP_DIRECTLY_INDEXED;
}

enum abstract RootParameterType(Int) {
	var DESCRIPTOR_TABLE = 0;
	var CONSTANTS = 1;
	var CBV = 2;
	var SRV = 3;
	var UAV = 4;
}

enum abstract DescriptorRangeType(Int) {
	var SRV = 0;
	var UAV = 1;
	var CBV = 2;
	var SAMPLER = 3;
}

@:struct class RootParameter {
	public var parameterType : RootParameterType;
	var __paddingBeforeUnion : Int;
}

@:struct class DescriptorRange {
	public var rangeType : DescriptorRangeType;
	public var numDescriptors : Int;
	public var baseShaderRegister : Int;
	public var registerSpace : Int;
	public var offsetInDescriptorsFromTableStart : Int;
	public function new() {
	}
}

@:struct class RootParameterDescriptorTable extends RootParameter {
	public var numDescriptorRanges : Int;
	public var __padding : Int;
	public var descriptorRanges : hl.CArray<DescriptorRange>;
	public var shaderVisibility : ShaderVisibility;
	public function new() {
		parameterType = DESCRIPTOR_TABLE;
	}
}

@:struct class RootParameterConstants extends RootParameter {
	public var shaderRegister : Int;
	public var registerSpace : Int;
	public var num32BitValues : Int;
	var __unused : Int;
	public var shaderVisibility : ShaderVisibility;
	public function new() {
		parameterType = CONSTANTS;
	}
}

@:struct class RootParameterDescriptor extends RootParameter {
	public var shaderRegister : Int;
	public var registerSpace : Int;
	var __unused : Int;
	var __unused2 : Int;
	public var shaderVisibility : ShaderVisibility;
	public function new(t) {
		parameterType = t;
	}
}

enum abstract AddressMode(Int) {
	var WRAP = 1;
	var MIRROR = 2;
	var CLAMP = 3;
	var BORDER = 4;
	var ONCE = 5;
}

enum abstract Filter(Int) {
	var MIN_MAG_MIP_POINT = 0;
	var MIN_MAG_POINT_MIP_LINEAR = 0x1;
	var MIN_POINT_MAG_LINEAR_MIP_POINT = 0x4;
	var MIN_POINT_MAG_MIP_LINEAR = 0x5;
	var MIN_LINEAR_MAG_MIP_POINT = 0x10;
	var MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x11;
	var MIN_MAG_LINEAR_MIP_POINT = 0x14;
	var MIN_MAG_MIP_LINEAR = 0x15;
	var ANISOTROPIC = 0x55;
	var COMPARISON_MIN_MAG_MIP_POINT = 0x80;
	var COMPARISON_MIN_MAG_POINT_MIP_LINEAR = 0x81;
	var COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x84;
	var COMPARISON_MIN_POINT_MAG_MIP_LINEAR = 0x85;
	var COMPARISON_MIN_LINEAR_MAG_MIP_POINT = 0x90;
	var COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91;
	var COMPARISON_MIN_MAG_LINEAR_MIP_POINT = 0x94;
	var COMPARISON_MIN_MAG_MIP_LINEAR = 0x95;
	var COMPARISON_ANISOTROPIC = 0xd5;
	var MINIMUM_MIN_MAG_MIP_POINT = 0x100;
	var MINIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x101;
	var MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x104;
	var MINIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x105;
	var MINIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x110;
	var MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x111;
	var MINIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x114;
	var MINIMUM_MIN_MAG_MIP_LINEAR = 0x115;
	var MINIMUM_ANISOTROPIC = 0x155;
	var MAXIMUM_MIN_MAG_MIP_POINT = 0x180;
	var MAXIMUM_MIN_MAG_POINT_MIP_LINEAR = 0x181;
	var MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT = 0x184;
	var MAXIMUM_MIN_POINT_MAG_MIP_LINEAR = 0x185;
	var MAXIMUM_MIN_LINEAR_MAG_MIP_POINT = 0x190;
	var MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x191;
	var MAXIMUM_MIN_MAG_LINEAR_MIP_POINT = 0x194;
	var MAXIMUM_MIN_MAG_MIP_LINEAR = 0x195;
	var MAXIMUM_ANISOTROPIC = 0x1d5;
}

enum abstract ComparisonFunc(Int) {
	var NEVER = 1;
	var LESS = 2;
	var EQUAL = 3;
	var LESS_EQUAL = 4;
	var GREATER = 5;
	var NOT_EQUAL = 6;
	var GREATER_EQUAL = 7;
	var ALWAYS = 8;
}

enum abstract StaticBorderColor(Int) {
	var TRANSPARENT_BLACK = 0;
	var OPAQUE_BLACK = 1;
	var OPAQUE_WHITE = 2;
}

enum abstract ShaderVisibility(Int) {
	var ALL = 0;
	var VERTEX = 1;
	var HULL = 2;
	var DOMAIN = 3;
	var GEOMETRY = 4;
	var PIXEL = 5;
	var AMPLIFICATION = 6;
	var MESH = 7;
}

@:struct class StaticSamplerDesc {
	public var filter : Filter;
	public var addressU : AddressMode;
	public var addressV : AddressMode;
	public var addressW : AddressMode;
	public var mipLODBias : Single;
	public var maxAnisotropy : Int;
	public var comparisonFunc : ComparisonFunc;
	public var borderColor : StaticBorderColor;
	public var minLOD : Single;
	public var maxLOD : Single;
	public var shaderRegister : Int;
	public var registerSpace : Int;
	public var shaderVisibility : ShaderVisibility;
	public function new() {
	}
}

@:struct class RootSignatureDesc {
	public var numParameters : Int;
	public var parameters : hl.CArray<RootParameter>;
	public var numStaticSamplers : Int;
	public var staticSamplers : hl.CArray<StaticSamplerDesc>;
	public var flags : haxe.EnumFlags<RootSignatureFlag>;
	public function new() {
	}
}

@:hlNative("dx12","rootsignature_")
abstract RootSignature(Resource) {
	public function new( bytes : hl.Bytes, len : Int ) {
		this = create(bytes,len);
	}
	static function create(bytes:hl.Bytes,len:Int) : Resource { return null; }
}

abstract GraphicsPipelineState(Resource) {
	@:to inline function toPS():PipelineState {
		return cast this;
	}
}

abstract ComputePipelineState(Resource) {
	@:to inline function toPS():PipelineState {
		return cast this;
	}
}

abstract Bool32(Int) {
	@:to inline function toBool() return this != 1;
	@:from static inline function fromBool(b:Bool) : Bool32 { return cast b?1:0; };
}

enum abstract DxgiFormat(Int) {
	var UNKNOWN = 0;
	var R32G32B32A32_TYPELESS = 1;
	var R32G32B32A32_FLOAT = 2;
	var R32G32B32A32_UINT = 3;
	var R32G32B32A32_SINT = 4;
	var R32G32B32_TYPELESS = 5;
	var R32G32B32_FLOAT = 6;
	var R32G32B32_UINT = 7;
	var R32G32B32_SINT = 8;
	var R16G16B16A16_TYPELESS = 9;
	var R16G16B16A16_FLOAT = 10;
	var R16G16B16A16_UNORM = 11;
	var R16G16B16A16_UINT = 12;
	var R16G16B16A16_SNORM = 13;
	var R16G16B16A16_SINT = 14;
	var R32G32_TYPELESS = 15;
	var R32G32_FLOAT = 16;
	var R32G32_UINT = 17;
	var R32G32_SINT = 18;
	var R32G8X24_TYPELESS = 19;
	var D32_FLOAT_S8X24_UINT = 20;
	var R32_FLOAT_X8X24_TYPELESS = 21;
	var X32_TYPELESS_G8X24_UINT = 22;
	var R10G10B10A2_TYPELESS = 23;
	var R10G10B10A2_UNORM = 24;
	var R10G10B10A2_UINT = 25;
	var R11G11B10_FLOAT = 26;
	var R8G8B8A8_TYPELESS = 27;
	var R8G8B8A8_UNORM = 28;
	var R8G8B8A8_UNORM_SRGB = 29;
	var R8G8B8A8_UINT = 30;
	var R8G8B8A8_SNORM = 31;
	var R8G8B8A8_SINT = 32;
	var R16G16_TYPELESS = 33;
	var R16G16_FLOAT = 34;
	var R16G16_UNORM = 35;
	var R16G16_UINT = 36;
	var R16G16_SNORM = 37;
	var R16G16_SINT = 38;
	var R32_TYPELESS = 39;
	var D32_FLOAT = 40;
	var R32_FLOAT = 41;
	var R32_UINT = 42;
	var R32_SINT = 43;
	var R24G8_TYPELESS = 44;
	var D24_UNORM_S8_UINT = 45;
	var R24_UNORM_X8_TYPELESS = 46;
	var X24_TYPELESS_G8_UINT = 47;
	var R8G8_TYPELESS = 48;
	var R8G8_UNORM = 49;
	var R8G8_UINT = 50;
	var R8G8_SNORM = 51;
	var R8G8_SINT = 52;
	var R16_TYPELESS = 53;
	var R16_FLOAT = 54;
	var D16_UNORM = 55;
	var R16_UNORM = 56;
	var R16_UINT = 57;
	var R16_SNORM = 58;
	var R16_SINT = 59;
	var R8_TYPELESS = 60;
	var R8_UNORM = 61;
	var R8_UINT = 62;
	var R8_SNORM = 63;
	var R8_SINT = 64;
	var A8_UNORM = 65;
	var R1_UNORM = 66;
	var R9G9B9E5_SHAREDEXP = 67;
	var R8G8_B8G8_UNORM = 68;
	var G8R8_G8B8_UNORM = 69;
	var BC1_TYPELESS = 70;
	var BC1_UNORM = 71;
	var BC1_UNORM_SRGB = 72;
	var BC2_TYPELESS = 73;
	var BC2_UNORM = 74;
	var BC2_UNORM_SRGB = 75;
	var BC3_TYPELESS = 76;
	var BC3_UNORM = 77;
	var BC3_UNORM_SRGB = 78;
	var BC4_TYPELESS = 79;
	var BC4_UNORM = 80;
	var BC4_SNORM = 81;
	var BC5_TYPELESS = 82;
	var BC5_UNORM = 83;
	var BC5_SNORM = 84;
	var B5G6R5_UNORM = 85;
	var B5G5R5A1_UNORM = 86;
	var B8G8R8A8_UNORM = 87;
	var B8G8R8X8_UNORM = 88;
	var R10G10B10_XR_BIAS_A2_UNORM = 89;
	var B8G8R8A8_TYPELESS = 90;
	var B8G8R8A8_UNORM_SRGB = 91;
	var B8G8R8X8_TYPELESS = 92;
	var B8G8R8X8_UNORM_SRGB = 93;
	var BC6H_TYPELESS = 94;
	var BC6H_UF16 = 95;
	var BC6H_SF16 = 96;
	var BC7_TYPELESS = 97;
	var BC7_UNORM = 98;
	var BC7_UNORM_SRGB = 99;
	var AYUV = 100;
	var Y410 = 101;
	var Y416 = 102;
	var NV12 = 103;
	var P010 = 104;
	var P016 = 105;
	var _420_OPAQUE = 106;
	var YUY2 = 107;
	var Y210 = 108;
	var Y216 = 109;
	var NV11 = 110;
	var AI44 = 111;
	var IA44 = 112;
	var P8 = 113;
	var A8P8 = 114;
	var B4G4R4A4_UNORM = 115;
	var P208 = 130;
	var V208 = 131;
	var V408 = 132;
	public inline function toInt() return this;
}

@:struct class ShaderBytecode {
	public var shaderBytecode : hl.Bytes;
	public var bytecodeLength : hl.I64;
}

@:struct class SoDeclarationEntry {
	public var stream : Int;
	public var semanticName : hl.Bytes;
	public var semanticIndex : Int;
	public var startComponent : hl.UI8;
	public var componentCount : hl.UI8;
	public var outputSlot : hl.UI8;
	public function new() {
	}
}

@:struct class StreamOutputDesc {
	public var soDeclaration : hl.CArray<SoDeclarationEntry>;
	public var numEntries : Int;
	public var bufferStrides : hl.Bytes;
	public var numStrides : Int;
	public var rasterizedStream : Int;
}

abstract GraphicsRTVFormats(GraphicsPipelineStateDesc) {
	public inline function new(g) {
		this = g;
	}
	@:arrayAccess inline function get( index : Int ) {
		return @:privateAccess switch( index ) {
		case 0: this.rtvFormat0;
		case 1: this.rtvFormat1;
		case 2: this.rtvFormat2;
		case 3: this.rtvFormat3;
		case 4: this.rtvFormat4;
		case 5: this.rtvFormat5;
		case 6: this.rtvFormat6;
		case 7: this.rtvFormat7;
		default: throw "assert";
		}
	}
	@:arrayAccess inline function set( index : Int, v : DxgiFormat ) {
		@:privateAccess switch( index ) {
		case 0: this.rtvFormat0 = v;
		case 1: this.rtvFormat1 = v;
		case 2: this.rtvFormat2 = v;
		case 3: this.rtvFormat3 = v;
		case 4: this.rtvFormat4 = v;
		case 5: this.rtvFormat5 = v;
		case 6: this.rtvFormat6 = v;
		case 7: this.rtvFormat7 = v;
		default: throw "assert";
		}
	}
}

abstract BlendDescRenderTargets(BlendDesc) {
	public inline function new(d) {
		this = d;
	}
	@:arrayAccess inline function get( index : Int ) {
		return @:privateAccess switch( index ) {
		case 0: this.renderTarget0;
		case 1: this.renderTarget1;
		case 2: this.renderTarget2;
		case 3: this.renderTarget3;
		case 4: this.renderTarget4;
		case 5: this.renderTarget5;
		case 6: this.renderTarget6;
		case 7: this.renderTarget7;
		default: throw "assert";
		}
	}
}

enum abstract Blend(Int) {
	var ZERO = 1;
	var ONE = 2;
	var SRC_COLOR = 3;
	var INV_SRC_COLOR = 4;
	var SRC_ALPHA = 5;
	var INV_SRC_ALPHA = 6;
	var DEST_ALPHA = 7;
	var INV_DEST_ALPHA = 8;
	var DEST_COLOR = 9;
	var INV_DEST_COLOR = 10;
	var SRC_ALPHA_SAT = 11;
	var BLEND_FACTOR = 14;
	var INV_BLEND_FACTOR = 15;
	var SRC1_COLOR = 16;
	var INV_SRC1_COLOR = 17;
	var SRC1_ALPHA = 18;
	var INV_SRC1_ALPHA = 19;
}

enum abstract BlendOp(Int) {
	var ADD = 1;
	var SUBTRACT = 2;
	var REV_SUBTRACT = 3;
	var MIN = 4;
	var MAX = 5;
}

enum abstract LogicOp(Int) {
	var CLEAR;
	var SET;
	var COPY;
	var COPY_INVERTED;
	var NOOP;
	var INVERT;
	var AND;
	var NAND;
	var OR;
	var NOR;
	var XOR;
	var EQUIV;
	var AND_REVERSE;
	var AND_INVERTED;
	var OR_REVERSE;
	var OR_INVERTED;
}

@:struct class RenderTargetBlendDesc {
	public var blendEnable : Bool32;
	public var logicOpEnable : Bool32;
	public var srcBlend : Blend;
	public var dstBlend : Blend;
	public var blendOp : BlendOp;
	public var srcBlendAlpha : Blend;
	public var dstBlendAlpha : Blend;
	public var blendOpAlpha : BlendOp;
	public var logicOp : LogicOp;
	public var renderTargetWriteMask : hl.UI8;
	public function new() {
	}
}

@:struct class BlendDesc {
	public var alphaToCoverage : Bool32;
	public var independentBlendEnable : Bool32;
	public var renderTargets(get,never) : BlendDescRenderTargets;
	inline function get_renderTargets() return new BlendDescRenderTargets(this);
	@:packed var renderTarget0 : RenderTargetBlendDesc;
	@:packed var renderTarget1 : RenderTargetBlendDesc;
	@:packed var renderTarget2 : RenderTargetBlendDesc;
	@:packed var renderTarget3 : RenderTargetBlendDesc;
	@:packed var renderTarget4 : RenderTargetBlendDesc;
	@:packed var renderTarget5 : RenderTargetBlendDesc;
	@:packed var renderTarget6 : RenderTargetBlendDesc;
	@:packed var renderTarget7 : RenderTargetBlendDesc;
	public function new() {
	}
}

enum abstract FillMode(Int) {
	var WIREFRAME = 2;
	var SOLID = 3;
}

enum abstract CullMode(Int) {
	var NONE = 1;
	var FRONT = 2;
	var BACK = 3;
}

enum abstract ConservativeRasterMode(Int) {
	var OFF = 0;
	var ON = 1;
}

@:struct class RasterizerDesc {
	public var fillMode : FillMode;
	public var cullMode : CullMode;
	public var frontCounterClockwise : Bool32;
	public var depthBias : Int;
	public var depthBiasClamp : Single;
	public var slopeScaledDepthBias : Single;
	public var depthClipEnable : Bool32;
	public var multisampleEnable : Bool32;
	public var antialiasedLineEnable : Bool32;
	public var forcedSampleCount : Int;
	public var conservativeRaster : ConservativeRasterMode;
	public function new() {
	}
}

enum abstract DepthWriteMask(Int) {
	var ZERO = 0;
	var ALL = 1;
}

enum abstract StencilOp(Int) {
	var KEEP = 1;
	var ZERO = 2;
	var REPLACE = 3;
	var INCR_SAT = 4;
	var DECR_SAT = 5;
	var INVERT = 6;
	var INCR = 7;
	var DECR = 8;
}

@:struct class DepthStencilOpDesc {
	public var stencilFailOp : StencilOp;
	public var stencilDepthFailOp : StencilOp;
	public var stencilPassOp : StencilOp;
	public var stencilFunc : ComparisonFunc;
	public function new() {
	}
}

@:struct class DepthStencilDesc {
	public var depthEnable : Bool32;
	public var depthWriteMask : DepthWriteMask;
	public var depthFunc : ComparisonFunc;
	public var stencilEnable : Bool32;
	public var stencilReadMask : hl.UI8;
	public var stencilWriteMask : hl.UI8;
	@:packed public var frontFace(default,null) : DepthStencilOpDesc;
	@:packed public var backFace(default,null) : DepthStencilOpDesc;
	public function new() {
	}
}

enum abstract InputClassification(Int) {
	var PER_VERTEX_DATA = 0;
	var PER_INSTANCE_DATA = 1;
}

@:struct class InputElementDesc {
	public var semanticName : hl.Bytes;
	public var semanticIndex : Int;
	public var format : DxgiFormat;
	public var inputSlot : Int;
	public var alignedByteOffset : Int;
	public var inputSlotClass : InputClassification;
	public var instanceDataStepRate : Int;
	public function new() {
	}
}

@:struct class InputLayoutDesc {
	public var inputElementDescs : hl.CArray<InputElementDesc>;
	public var numElements : Int;
	public var __padding : Int; // largest element
	public function new() {
	}
}

enum abstract IndexBufferStripCutValue(Int) {
	var DISABLED = 0;
	var _0xFFFF = 1;
	var _0xFFFFFFFF = 2;
}

enum abstract PrimitiveTopologyType(Int) {
	var UNDEFINED = 0;
	var POINT = 1;
	var LINE = 2;
	var TRIANGLE = 3;
	var PATCH = 4;
}

@:struct class DxgiSampleDesc {
	public var count : Int;
	public var quality : Int;
	public function new() {
	}
}

@:struct class CachedPipelineState {
	public var cachedBlob : hl.Bytes;
	public var cachedBlobSizeInBytes : hl.I64;
	public function new() {
	}
}

enum abstract PipelineStateFlags(Int) {
	var NONE = 0;
	var TOOL_DEBUG = 1;
}

@:struct class GraphicsPipelineStateDesc {
	public var rootSignature : RootSignature;
	@:packed public var vs(default,null) : ShaderBytecode;
	@:packed public var ps(default,null) : ShaderBytecode;
	@:packed public var ds(default,null) : ShaderBytecode;
	@:packed public var hs(default,null) : ShaderBytecode;
	@:packed public var gs(default,null) : ShaderBytecode;
	@:packed public var streamOutput(default,null) : StreamOutputDesc;
	@:packed public var blendState(default,null) : BlendDesc;
	public var sampleMask : Int;
	@:packed public var rasterizerState(default,null) : RasterizerDesc;
	@:packed public var depthStencilDesc(default,null) : DepthStencilDesc;
	@:packed public var inputLayout(default,null) : InputLayoutDesc;
	public var ibStripCutValue : IndexBufferStripCutValue;
	public var primitiveTopologyType : PrimitiveTopologyType;
	public var numRenderTargets : Int;
	public var rtvFormats(get,never) : GraphicsRTVFormats;
	inline function get_rtvFormats() return new GraphicsRTVFormats(this);
	var rtvFormat0 : DxgiFormat;
	var rtvFormat1 : DxgiFormat;
	var rtvFormat2 : DxgiFormat;
	var rtvFormat3 : DxgiFormat;
	var rtvFormat4 : DxgiFormat;
	var rtvFormat5 : DxgiFormat;
	var rtvFormat6 : DxgiFormat;
	var rtvFormat7 : DxgiFormat;
	public var dsvFormat : DxgiFormat;
	@:packed public var sampleDesc(default,null) : DxgiSampleDesc;
	public var nodeMask : Int;
	@:packed public var cachedPSO(default,null) : CachedPipelineState;
	public var flags : PipelineStateFlags;
	public function new() {
	}
}

@:struct class ComputePipelineStateDesc {
	public var rootSignature : RootSignature;
	@:packed public var cs(default,null) : ShaderBytecode;
	public var nodeMask : Int;
	@:packed public var cachedPSO(default,null) : CachedPipelineState;
	public var flags : PipelineStateFlags;
	public function new() {
	}
}

enum abstract HeapType(Int) {
	var DEFAULT = 1;
	var UPLOAD = 2;
	var READBACK = 3;
	var CUSTOM = 4;
}

enum abstract CpuPageProperty(Int) {
	var UNKNOWN = 0;
	var NOT_AVAILABLE = 1;
	var WRITE_COMBINE = 2;
	var WRITE_BACK = 3;
}

enum abstract MemoryPool(Int) {
	var UNKNOWN = 0;
	var L0 = 1;
	var L1 = 2;
}

@:struct class HeapProperties {
	public var type : HeapType;
	public var cpuPageProperty : CpuPageProperty;
	public var memoryPoolReference : MemoryPool;
	public var creationNodeMask : Int;
	public var visibleNodeMask : Int;
	public function new() {
	}
}

enum HeapFlag {
	SHARED;
	DENY_BUFFERS;
	ALLOW_DISPLAY;
	__UNUSED;
	SHARED_CROSS_ADAPTER;
	DENY_RT_DS_TEXTURES;
	DENY_NON_RT_DS_TEXTURES;
	HARDWARE_PROTECTED;
	ALLOW_WRITE_WATCH;
	ALLOW_SHADER_ATOMICS;
	CREATE_NOT_RESIDENT;
	CREATE_NOT_ZEROED;
}

enum abstract ResourceDimension(Int) {
	var UNKNOWN = 0;
	var BUFFER = 1;
	var TEXTURE1D = 2;
	var TEXTURE2D = 3;
	var TEXTURE3D = 4;
}

enum abstract TextureLayout(Int) {
	var UNKNOWN = 0;
	var ROW_MAJOR = 1;
	var _64KB_UNDEFINED_SWIZZLE = 2;
	var _64KB_STANDARD_SWIZZLE = 3;
}

enum ResourceFlag {
	ALLOW_RENDER_TARGET;
	ALLOW_DEPTH_STENCIL;
	ALLOW_UNORDERED_ACCESS;
	DENY_SHADER_RESOURCE;
	ALLOW_CROSS_ADAPTER;
	ALLOW_SIMULTANEOUS_ACCESS;
	VIDEO_DECODE_REFERENCE_ONLY;
	VIDEO_ENCODE_REFERENCE_ONLY;
}

@:struct class ResourceDesc {
	public var dimension : ResourceDimension;
	public var alignment : Int64;
	public var width : Int64;
	public var height : Int;
	public var depthOrArraySize : hl.UI16;
	public var mipLevels : hl.UI16;
	public var format : DxgiFormat;
	@:packed public var sampleDesc(default,null) : DxgiSampleDesc;
	public var layout : TextureLayout;
	public var flags : haxe.EnumFlags<ResourceFlag>;
	public function new() {
	}
}

@:struct class ClearValue {
	public var format : DxgiFormat;
	@:packed public var color(default,never) : Color;
	public var depth(get,set) : Float;
	public var stencil(get,set) : Int;
	inline function get_depth() return color.r;
	inline function set_depth(v) return color.r = v;
	function get_stencil() return haxe.io.FPHelper.floatToI32(color.g);
	function set_stencil(v) {
		color.g = haxe.io.FPHelper.i32ToFloat(v);
		return v;
	}
}

enum abstract SrvDimension(Int) {
	var UNKNOWN = 0;
	var BUFFER = 1;
	var TEXTURE1D = 2;
	var TEXTURE1DARRAY = 3;
	var TEXTURE2D = 4;
	var TEXTURE2DARRAY = 5;
	var TEXTURE2DMS = 6;
	var TEXTURE2DMSARRAY = 7;
	var TEXTURE3D = 8;
	var TEXTURECUBE = 9;
	var TEXTURECUBEARRAY = 10;
	var RAYTRACING_ACCELERATION_STRUCTURE = 11;
}

enum abstract ShaderComponentValue(Int) {
	var R = 0;
	var G = 1;
	var B = 2;
	var A = 3;
	var ZERO = 4;
	var ONE = 5;
}

abstract ShaderComponentMapping(Int) {
	public var red(get,set) : ShaderComponentValue;
	public var green(get,set) : ShaderComponentValue;
	public var blue(get,set) : ShaderComponentValue;
	public var alpha(get,set) : ShaderComponentValue;
	public function new() {
		this = 1 << 12;
	}
	inline function get_red() : ShaderComponentValue { return cast (this & 7); }
	inline function get_green() : ShaderComponentValue { return cast ((this >> 3) & 7); }
	inline function get_blue() : ShaderComponentValue { return cast ((this >> 6) & 7); }
	inline function get_alpha() : ShaderComponentValue { return cast ((this >> 9) & 7); }
	inline function set_red(v : ShaderComponentValue) { this = (this & ~(3<<0)) | ((cast v : Int) << 0); return v; }
	inline function set_green(v : ShaderComponentValue) { this = (this & ~(3<<3)) | ((cast v : Int) << 3); return v; }
	inline function set_blue(v : ShaderComponentValue) { this = (this & ~(3<<6)) | ((cast v : Int) << 6); return v; }
	inline function set_alpha(v : ShaderComponentValue) { this = (this & ~(3<<9)) | ((cast v : Int) << 9); return v; }

	public static inline var DEFAULT : ShaderComponentMapping = cast 0x1688;
}

@:struct class ShaderResourceViewDesc {
	public var format : DxgiFormat;
	public var dimension : SrvDimension;
	public var shader4ComponentMapping : ShaderComponentMapping;
	var __unionPadding : Int;
}

enum abstract BufferSRVFlags(Int) {
	var NONE = 0;
	var RAW = 1;
}

@:struct class BufferSRV extends ShaderResourceViewDesc {
	public var firstElement : Int64;
	public var numElements : Int;
	public var structureByteStride : Int;
	public var flags : BufferSRVFlags;
	var unused : Int;
	public function new() {
		dimension = BUFFER;
	}
}

@:struct class Tex1DSRV extends ShaderResourceViewDesc {
	public var mostDetailedMip : Int;
	public var mipLevels : Int;
	public var resourceMinLODClamp : Single;
	var unused1 : Int;
	var unused2 : Int;
	var unused3 : Int;
	public function new() {
		dimension = TEXTURE1D;
	}
}

@:struct class Tex1DArraySRV extends ShaderResourceViewDesc {
	public var mostDetailedMip : Int;
	public var mipLevels : Int;
	public var firstArraySlice : Int;
	public var arraySize : Int;
	public var resourceMinLODClamp : Single;
	var unused1 : Int;
	public function new() {
		dimension = TEXTURE1DARRAY;
	}
}

@:struct class Tex2DSRV extends ShaderResourceViewDesc {
	public var mostDetailedMip : Int;
	public var mipLevels : Int;
	public var planeSlice : Int;
	public var resourceMinLODClamp : Single;
	var unused1 : Int;
	var unused2 : Int;
	public function new() {
		dimension = TEXTURE2D;
	}
}

@:struct class Tex2DArraySRV extends ShaderResourceViewDesc {
	public var mostDetailedMip : Int;
	public var mipLevels : Int;
	public var firstArraySlice : Int;
	public var arraySize : Int;
	public var planeSlice : Int;
	public var resourceMinLODClamp : Single;
	public function new() {
		dimension = TEXTURE2DARRAY;
	}
}

@:struct class Tex3DSRV extends ShaderResourceViewDesc {
	public var mostDetailedMip : Int;
	public var mipLevels : Int;
	public var resourceMinLODClamp : Single;
	var unused1 : Int;
	var unused2 : Int;
	var unused3 : Int;
	public function new() {
		dimension = TEXTURE3D;
	}
}

@:struct class TexCubeSRV extends ShaderResourceViewDesc {
	public var mostDetailedMip : Int;
	public var mipLevels : Int;
	public var resourceMinLODClamp : Single;
	var unused1 : Int;
	var unused2 : Int;
	var unused3 : Int;
	public function new() {
		dimension = TEXTURECUBE;
	}
}

@:struct class TexCubeArraySRV extends ShaderResourceViewDesc {
	public var mostDetailedMip : Int;
	public var mipLevels : Int;
	public var first2DArrayFace : Int;
	public var numCubes : Int;
	public var resourceMinLODClamp : Single;
	var unused1 : Int;
	public function new() {
		dimension = TEXTURECUBEARRAY;
	}
}

@:struct class SamplerDesc {
	public var filter : Filter;
	public var addressU : AddressMode;
	public var addressV : AddressMode;
	public var addressW : AddressMode;
	public var mipLODBias : Single;
	public var maxAnisotropy : Int;
	public var comparisonFunc : ComparisonFunc;
	@:packed public var borderColor(default,never) : Color;
	public var minLod : Single;
	public var maxLod : Single;
	public function new() {
	}
}

@:struct class SubResourceData {
	public var data : hl.Bytes;
	public var rowPitch : Int64;
	public var slicePitch : Int64;
	public function new() {
	}
}

enum abstract IndirectArgumentType(Int) {
	var DRAW = 0;
	var DRAW_INDEXED = 1;
	var DISPATCH = 2;
	var VERTEX_BUFFER_VIEW = 3;
	var INDEX_BUFFER_VIEW = 4;
	var CONSTANT = 5;
	var CONSTANT_BUFFER_VIEW = 6;
	var SHADER_RESOURCE_VIEW = 7;
	var UNORDERED_ACCESS_VIEW = 8;
	var DISPATCH_RAYS = 9;
	var DISPATCH_MESH = 10;
}

@:struct class IndirectArgumentDesc {
	public var type : IndirectArgumentType;
	public var rootParameterIndex : Int;
	public var destOffsetIn32BitValues : Int;
	public var num32BitValuesToSet : Int;
	public var slot(get,set) : Int;
	inline function get_slot() return rootParameterIndex;
	inline function set_slot(v) return rootParameterIndex = v;
	public function new() {
	}
}

@:struct class CommandSignatureDesc {
	public var byteStride :Int;
	public var numArgumentDescs : Int;
	public var argumentDescs : hl.CArray<IndirectArgumentDesc>;
	public var nodeMask : Int;
	public function new() {
	}
}

enum abstract UAVDimension(Int) {
	public var UNKNOWN = 0;
	public var BUFFER = 1;
	public var TEXTURE1D = 2;
	public var TEXTURE1DARRAY = 3;
	public var TEXTURE2D = 4;
	public var TEXTURE2DARRAY = 5;
	public var TEXTURE3D = 8;
}

@:struct class UnorderedAccessViewDesc {
	public var format : DxgiFormat;
	public var viewDimension : UAVDimension;
}

enum UAVBufferFlags {
	RAW;
}

@:struct class UAVBufferViewDesc extends UnorderedAccessViewDesc {
	public var firstElement : hl.I64;
	public var numElements : Int;
	public var structureSizeInBytes : Int;
	public var counterOffsetInBytes : hl.I64;
	public var flags : haxe.EnumFlags<UAVBufferFlags>;
	public function new() {
		viewDimension = BUFFER;
	}
}

@:struct class UAVTextureViewDesc extends UnorderedAccessViewDesc {
	var int0 : Int;
	var int1 : Int;
	var int2 : Int;
	var int3 : Int;
	var padding1 : hl.I64;
	var padding2 : Int;
	public function new(dim) {
		viewDimension = dim;
	}
	public var mipSlice(get,set) : Int;
	public var firstArraySlice(get,set) : Int;
	public var firstWSlice(get,set) : Int;
	public var arraySize(get,set) : Int;
	public var planeSlice(get,set) : Int;
	public var wSlice(get,set) : Int;
	inline function get_mipSlice() return int0;
	inline function set_mipSlice(v) return int0 = v;

	inline function get_planeSlice() return switch( viewDimension ) { case TEXTURE2DARRAY: int3; default: int1; }
	inline function set_planeSlice(v) return switch( viewDimension ) { case TEXTURE2DARRAY: int3 = v; default: int1 = v; }

	inline function get_firstArraySlice() return int1;
	inline function set_firstArraySlice(v) return int1 = v;
	inline function get_arraySize() return int2;
	inline function set_arraySize(v) return int2 = v;

	inline function get_firstWSlice() return int1;
	inline function set_firstWSlice(v) return int1 = v;
	inline function get_wSlice() return int2;
	inline function set_wSlice(v) return int2 = v;
}

enum abstract QueryType(Int) {
	var OCCLUSION = 0;
	var BINARY_OCCLUSION = 1;
	var TIMESTAMP = 2;
	var PIPELINE_STATISTICS = 3;
	var SO_STATISTICS_STREAM0 = 4;
	var SO_STATISTICS_STREAM1 = 5;
	var SO_STATISTICS_STREAM2 = 6;
	var SO_STATISTICS_STREAM3 = 7;
	var VIDEO_DECODE_STATISTICS = 8;
}

enum abstract QueryHeapType(Int) {
	var OCCLUSION = 0;
	var TIMESTAMP = 1;
	var PIPELINE_STATISTICS = 2;
	var SO_STATISTICS = 3;
	var VIDEO_DECODE_STATISTICS = 4;
	var COPY_QUEUE_TIMESTAMP = 5;
}

@:struct class QueryHeapDesc {
	public var type : QueryHeapType;
	public var count : Int;
	public var nodeMask : Int;
	public function new() {
	}
}

@:forward(release)
abstract QueryHeap(Resource) {
}

enum abstract PredicationOp(Int) {
	var EQUAL_ZERO = 0;
	var NOT_EQUAL_ZERO = 1;
}


@:hlNative("dx12")
class Dx12 {

	public static function create( win : Window, flags : DriverInitFlags, ?deviceName : String ) {
		return dxCreate(@:privateAccess win.win, flags, deviceName == null ? null : @:privateAccess deviceName.bytes);
	}

	public static function flushMessages() {
	}

	public static function getDescriptorHandleIncrementSize( type : DescriptorHeapType ) : Int {
		return 0;
	}

	public static function createGraphicsPipelineState( desc : GraphicsPipelineStateDesc ) : GraphicsPipelineState {
		return null;
	}

	public static function createComputePipelineState( desc : ComputePipelineStateDesc ) : ComputePipelineState {
		return null;
	}

	public static function serializeRootSignature( desc : RootSignatureDesc, version : Int, size : hl.Ref<Int> ) : hl.Bytes {
		return null;
	}

	public static function getBackBuffer( index : Int ) : GpuResource {
		return null;
	}

	public static function getCurrentBackBufferIndex() : Int {
		return 0;
	}

	public static function createRenderTargetView( buffer : Resource, desc : RenderTargetViewDesc, target : Address ) {
	}

	public static function createDepthStencilView( buffer : Resource, desc : DepthStencilViewDesc, target : Address ) {
	}

	public static function createConstantBufferView( desc : ConstantBufferViewDesc, target : Address ) {
	}

	public static function createUnorderedAccessView( res : Resource, counter : Resource, desc : UnorderedAccessViewDesc, target : Address ) {
	}

	public static function createShaderResourceView( resource : Resource, desc : ShaderResourceViewDesc, target : Address ) {
	}

	public static function createQueryHeap( desc : QueryHeapDesc ) : QueryHeap {
		return null;
	}

	public static function getCopyableFootprints( srcDesc : ResourceDesc, firstSubResource : Int, numSubResources : Int, baseOffset : Int64, layouts : PlacedSubresourceFootprint, numRows : hl.BytesAccess<Int>, rowSizeInBytes : hl.BytesAccess<Int64>, totalBytes : hl.BytesAccess<Int64> ) : Void {
	}

	public static function createSampler( desc : SamplerDesc, target : Address ) {
	}

	public static function createCommittedResource( heapProperties : HeapProperties, heapFlags : haxe.EnumFlags<HeapFlag>, desc : ResourceDesc, initialState : ResourceState, clearValue : ClearValue ) : GpuResource {
		return null;
	}

	public static function createCommandSignature( desc : CommandSignatureDesc, root : RootSignature ) : CommandSignature {
		return null;
	}

	public static function resize( width : Int, height : Int, bufferCount : Int, format : DxgiFormat ) {
	}

	public static function updateSubResource( commandList : CommandList, dst : GpuResource, src : GpuResource, srcOffset : Int64, first : Int, count : Int, data : SubResourceData ) : Bool {
		return false;
	}

	public static function signal( fence : Fence, value : Int64 ) {
	}

	public static function present( vsync : Bool ) {
	}

	public static function suspend() {
	}

	public static function resume() {
	}

	public static function getConstant( index : Int ) : Int {
		return 0;
	}

	public static function getDeviceName() {
		return @:privateAccess String.fromUCS2(dxGetDeviceName());
	}

	public static function listDevices() {
		var arr = dxListDevices();
		var out = [];
		for( i in 0...arr.length ) {
			if( arr[i] == null ) break;
			out.push(@:privateAccess String.fromUCS2(arr[i]));
		}
		return out;
	}

	@:hlNative("dx12","get_timestamp_frequency")
	public static function getTimestampFrequency() : Int64 {
		return 0;
	}

	@:hlNative("dx12", "list_devices")
	static function dxListDevices() : hl.NativeArray<hl.Bytes> {
		return null;
	}

	@:hlNative("dx12", "get_device_name")
	static function dxGetDeviceName() : hl.Bytes {
		return null;
	}

	@:hlNative("dx12", "create")
	static function dxCreate( win : hl.Abstract<"dx_window">, flags : DriverInitFlags, deviceName : hl.Bytes ) : DriverInstance {
		return null;
	}
}