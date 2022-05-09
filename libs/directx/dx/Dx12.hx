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
	public function new(type,alloc,state) {
		this = create(type,alloc,state);
	}
	public function close() {}
	public function execute() {}
	public function clearRenderTargetView( rtv : Address, color : ClearColor ) {}
	public function reset( alloc : CommandAllocator, state : PipelineState ) {}
	public function resourceBarrier( b : ResourceBarrier ) {}
	static function create( type : CommandListType, alloc : CommandAllocator, state : PipelineState ) : Resource { return null; }
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

enum RootSignatureFlag {
	NONE;
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
	var DescriptorTable = 0;
	var Constants = 1;
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
	var parameterType : RootParameterType;
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
	public var descriptorRanges : DescriptorRange;
	public var shaderVisibility : ShaderVisibility;
	public function new() {
		parameterType = DescriptorTable;
	}
}

@:struct class RootParameterConstants extends RootParameter {
	public var shaderRegister : Int;
	public var registerSpace : Int;
	public var num32BitValues : Int;
	var __unused : Int;
	public var shaderVisibility : ShaderVisibility;
	public function new() {
		parameterType = Constants;
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
	var Wrap = 1;
	var Mirror = 2;
	var Clamp = 3;
	var Border = 4;
	var Once = 5;
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
	var Never = 1;
	var Less = 2;
	var Equal = 3;
	var LessEqual = 4;
	var Greater = 5;
	var NotEqual = 6;
	var GreaterEqual = 7;
	var Always = 8;
}

enum abstract StaticBorderColor(Int) {
	var TransparentBlack = 0;
	var OpaqueBlack = 1;
	var OpaqueWhite = 2;
}

enum abstract ShaderVisibility(Int) {
	var All = 0;
	var Vertex = 1;
	var Hull = 2;
	var Domain = 3;
	var Geometry = 4;
	var Pixel = 5;
	var Amplification = 6;
	var Mesh = 7;
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
	public var parameters : RootParameter;
	public var numStaticSamplers : Int;
	public var staticSamplers : StaticSamplerDesc;
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
}

@:struct class ShaderBytecode {
	public var shaderBytecode : hl.Bytes;
	public var bytecodeLength : Int;
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
	public var soDeclaration : SoDeclarationEntry;
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
	public var renderTergetWriteMask : hl.UI8;
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
	var WireFrame = 2;
	var Solid = 3;
}

enum abstract CullMode(Int) {
	var None = 1;
	var Front = 2;
	var Back = 3;
}

enum abstract ConservativeRasterMode(Int) {
	var Off = 0;
	var On = 1;
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
	var Zero = 0;
	var All = 1;
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
	public var inputElementDescs : InputElementDesc;
	public var numElements : Int;
	public function new() {
	}
}

enum abstract IndexBufferStripCutValue(Int) {
	var Disabled = 0;
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
	var __padding : Int; // ?
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

	public static function createGraphicsPipelineState( desc : GraphicsPipelineStateDesc ) : GraphicsPipelineState {
		return null;
	}

	public static function serializeRootSignature( desc : RootSignatureDesc, version : Int, size : hl.Ref<Int> ) : hl.Bytes {
		return null;
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