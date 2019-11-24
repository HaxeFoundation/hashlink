package dx;

typedef DriverInstance = hl.Abstract<"dx_driver">;

typedef Pointer = hl.Abstract<"dx_pointer">;

abstract Shader(Pointer) {
	public inline function release() {
		Driver.releasePointer(this);
	}
}

abstract Layout(Pointer) {
	public inline function release() {
		Driver.releasePointer(this);
	}
}

abstract RasterState(Pointer) {
	public inline function release() {
		Driver.releasePointer(this);
	}
}

abstract RenderTargetView(Pointer) {
	public inline function release() {
		Driver.releasePointer(this);
	}
}

abstract DepthStencilView(Pointer) {
	public inline function release() {
		Driver.releasePointer(this);
	}
}

abstract DepthStencilState(Pointer) {
	public inline function release() {
		Driver.releasePointer(this);
	}
}

abstract BlendState(Pointer) {
	public inline function release() {
		Driver.releasePointer(this);
	}
}

abstract SamplerState(Pointer) {
	public inline function release() {
		Driver.releasePointer(this);
	}
}

abstract ShaderResourceView(Pointer) {
	public inline function release() {
		Driver.releasePointer(this);
	}
}

abstract DxBool(Int) {
	@:to public inline function toBool() : Bool return cast this;
	@:from static function fromBool( b : Bool ) : DxBool return cast b;
}

@:enum abstract DriverInitFlags(Int) {
	var None = 0;
	var SingleThread = 1;
	var DebugLayer = 2;
	var SwitchToRef = 4;
	var PreventInternalThreadingOptimizations = 8;
	var BgraSupport = 32;
	var Debuggable = 64;
	var PreventAlteringLayerSettingsFromRegistry = 128;
	var DisableGpuTimeout = 256;
	var VideoSupport = 2048;
	@:op(a | b) static function or(a:DriverInitFlags, b:DriverInitFlags) : DriverInitFlags;
}

@:enum abstract ResourceUsage(Int) {
	var Default = 0;
	var Immutable = 1;
	var Dynamic = 2;
	var Staging = 3;
}

@:enum abstract ResourceBind(Int) {
	var None = 0;
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
	@:op(a | b) static function or(a:ResourceBind, b:ResourceBind) : ResourceBind;
}

@:enum abstract ResourceAccess(Int) {
	var None = 0;
	var CpuWrite = 0x10000;
	var CpuRead = 0x20000;
	@:op(a | b) static function or(a:ResourceAccess, b:ResourceAccess) : ResourceAccess;
}

@:enum abstract ResourceMisc(Int) {
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
	@:op(a | b) static function or(a:ResourceMisc, b:ResourceMisc) : ResourceMisc;
}

@:enum abstract ShaderFlags(Int) {
	var None = 0;
	var Debug = 0x1;
	var SkipValidation = 0x2;
	var SkipOptimization = 0x4;
	var PackMatrixRowMajor = 0x8;
	var PackMatrixColumnMajor = 0x10;
	var PartialPrecision = 0x20;
	var ForceVSSoftwareNoOpt = 0x40;
	var ForcePSSoftwareNoOpt = 0x80;
	var NoPreshader = 0x100;
	var AvoidFlowControl = 0x200;
	var PreferFlowControl = 0x400;
	var EnableStrictness = 0x800;
	var EnableBackwardsCompatibility = 0x1000;
	var IEEEStrictness = 0x2000;
	var OptimizationLevel0 = 0x4000;
	var OptimizationLevel1 = 0; // default
	var OptimizationLevel2 = 0x4000 | 0x8000;
	var OptimizationLevel3 = 0x8000;
	var WarningsAreErrors = 0x40000;
	var ResourcesMayAlias = 0x80000;
	var EnableUnboundedDescriptorTables = 0x100000;
	var AllResourcesBound = 0x200000;
	@:op(a | b) static function or(a:ShaderFlags, b:ShaderFlags) : ShaderFlags;
}

@:enum abstract PrimitiveTopology(Int) {
	var Undefined = 0;
	var PointList = 1;
	var LineList = 2;
	var LineStrip = 3;
	var TriangleList = 4;
	var TriangleStrip = 5;
	var LineListAdj = 10;
	var TriangleListAdj = 12;
	var TriangleStripAdj = 13;
	static inline function controlPointPatchList(count:Int) : PrimitiveTopology return cast (count + 32);
}

@:enum abstract DisassembleFlags(Int) {
	var None = 0;
	var EnableColorCode = 1;
	var EnableDefaultValuePrints = 2;
	var EnableInstructionNumbering = 4;
	var EnableInsructionCycle = 8;
	var DisableDebugInfo = 0x10;
	var EnableInstructionOffset = 0x20;
	var InstructionOnly = 0x40;
	var PrintHexLiterals = 0x80;
	@:op(a | b) static function or(a:DisassembleFlags, b:DisassembleFlags) : DisassembleFlags;
}

@:enum abstract LayoutClassification(Int) {
	var PerVertexData = 0;
	var PerInstanceData = 1;
}

@:keep
class LayoutElement {
	public var semanticName : hl.Bytes;
	public var semanticIndex : Int;
	public var format : Format;
	public var inputSlot : Int;
	public var alignedByteOffset : Int;
	public var inputSlotClass : LayoutClassification;
	public var instanceDataStepRate : Int;
	public function new() {
	}
}

@:enum abstract ResourceDimension(Int) {
	var Unknown = 0;
	var Buffer = 1;
	var Texture1D = 2;
	var Texture1DArray = 3;
	var Texture2D = 4;
	var Texture2DArray = 5;
	var Texture2DMS = 6;
	var Texture2DMSArray = 7;
	var Texture3D = 8;
	var TextureCube = 9;
	var TextureCubeArray = 10;
	var TextureBufferEx = 11;
}

@:keep
class RenderTargetDesc {
	public var format : Format;
	public var dimension : ResourceDimension;
	public var mipMap : Int;
	public var firstSlice : Int;
	public var sliceCount : Int;

	// for buffer
	public var firstElement(get, set) : Int;
	public var elementCount(get, set) : Int;

	public function new(format, dimension = Unknown) {
		this.format = format;
		this.dimension = dimension;
	}

	inline function get_firstElement() return mipMap;
	inline function set_firstElement(m) return mipMap = m;
	inline function get_elementCount() return firstSlice;
	inline function set_elementCount(m) return firstSlice = m;
}

@:enum abstract FillMode(Int) {
	public var WireFrame = 2;
	public var Solid = 3;
}

@:enum abstract CullMode(Int) {
	public var None = 1;
	public var Front = 2;
	public var Back = 3;
}

@:keep
class RasterizerDesc {
	public var fillMode : FillMode;
	public var cullMode : CullMode;
	public var frontCounterClockwise : DxBool;
	public var depthBias : Int;
	public var depthBiasClamp : hl.F32;
	public var slopeScaledDepthBias : hl.F32;
	public var depthClipEnable : DxBool;
	public var scissorEnable : DxBool;
	public var multisampleEnable : DxBool;
	public var antialiasedLineEnable : DxBool;
	public function new() {
	}
}

@:keep
class Texture2dDesc {
	public var width : Int;
	public var height : Int;
	public var mipLevels : Int;
	public var arraySize : Int;
	public var format : Format;
	public var sampleCount : Int;
	public var sampleQuality : Int;
	public var usage : ResourceUsage;
	public var bind : ResourceBind;
	public var access : ResourceAccess;
	public var misc : ResourceMisc;
	#if hlxbo
	var esramOffset : Int;
	var esramUsage : Int;
	#end
	public function new() {
		mipLevels = arraySize = sampleCount = 1;
	}
}

@:enum abstract ComparisonFunc(Int) {
	var Never = 1;
	var Less = 2;
	var Equal = 3;
	var LessEqual = 4;
	var Greater = 5;
	var NotEqual = 6;
	var GreaterEqual = 7;
	var Always = 8;
}

@:enum abstract StencilOp(Int) {
	var Keep = 1;
	var Zero = 2;
	var Replace = 3;
	var IncrSat = 4;
	var DecrSat = 5;
	var Invert = 6;
	var Incr = 7;
	var Decr = 8;
}

@:keep
class DepthStencilDesc {
	public var depthEnable : DxBool;
	public var depthWrite : DxBool;
	public var depthFunc : ComparisonFunc;

	public var stencilEnable : DxBool;
	public var stencilReadMask : hl.UI8;
	public var stencilWriteMask : hl.UI8;

	public var frontFaceFail : StencilOp;
	public var frontFaceDepthFail : StencilOp;
	public var frontFacePass : StencilOp;
	public var frontFaceFunc : ComparisonFunc;

	public var backFaceFail : StencilOp;
	public var backFaceDepthFail : StencilOp;
	public var backFacePass : StencilOp;
	public var backFaceFunc : ComparisonFunc;

	#if hlxbo
	public var backfaceEnable : DxBool;
    public var depthBoundsEnable : DxBool;
    public var colorWritesOnDepthFailEnable : DxBool;
    public var colorWritesOnDepthPassDisable : DxBool;

    public var stencilReadMaskBack : hl.UI8;
    public var stencilWriteMaskBack : hl.UI8;

    public var stencilTestRefValueFront : hl.UI8;
    public var stencilTestRefValueBack : hl.UI8;

    public var stencilOpRefValueFront : hl.UI8;
    public var stencilOpRefValueBack : hl.UI8;
	#end

	public function new() {
	}
}

@:enum abstract Blend(Int) {
	var Zero = 1; // (;_;)
	var One = 2;
	var SrcColor = 3;
	var InvSrcColor = 4;
	var SrcAlpha = 5;
	var InvSrcAlpha = 6;
	var DestAlpha = 7;
	var InvDestAlpha = 8;
	var DestColor = 9;
	var InvDestColor = 10;
	var SrcAlphaSat = 11;
	var BlendFactor = 14;
	var InvBlendFactor = 15;
	var Src1Color = 16;
	var InvSrc1Color = 17;
	var Src1Alpha = 18;
	var InvSrc1Alpha = 19;
}

@:enum abstract BlendOp(Int) {
	var Add = 1;
	var Subtract = 2;
	var RevSubstract = 3;
	var Min = 4;
	var Max = 5;
}

@:keep
class RenderTargetBlendDesc {

	public var blendEnable : DxBool;
	public var srcBlend : Blend;
	public var destBlend : Blend;
	public var blendOp : BlendOp;
	public var srcBlendAlpha : Blend;
	public var destBlendAlpha : Blend;
	public var blendOpAlpha : BlendOp;
	public var renderTargetWriteMask : hl.UI8;

	public function new() {
	}
}

@:enum abstract Filter(Int) {
	var MinMagMipPoint = 0;
	var MinMagPointMipLinear = 0x1;
	var MinPointMagLinearMipPoint = 0x4;
	var MinPointMagMipLinear = 0x5;
	var MinLinearMagMipPoint = 0x10;
	var MinLinearMagPointMipLinear = 0x11;
	var MinMagLinearMipPoint = 0x14;
	var MinMagMipLinear = 0x15;
	var Anisotropic = 0x55;
	var ComparisonMinMagMipPoint = 0x80;
	var ComparisonMinMagPointMipLinear = 0x81;
	var ComparisonMinPointMagLinearMipPoint = 0x84;
	var ComparisonMinPointMagMipLinear = 0x85;
	var ComparisonMinLinearMagMipPoint = 0x90;
	var ComparisonMinLinearMagPointMipLinear = 0x91;
	var ComparisonMinMagLinearMipPoint = 0x94;
	var ComparisonMinMagMipLinear = 0x95;
	var ComparisonAnisotropic = 0xd5;
	var MininumMinMagMipPoint = 0x100;
	var MininumMinMagPointMipLinear = 0x101;
	var MininumMinPointMagLinearMipPoint = 0x104;
	var MininumMinPointMagMipLinear = 0x105;
	var MininumMinLinearMagMipPoint = 0x110;
	var MininumMinLinearMagPointMipLinear = 0x111;
	var MininumMinMagLinearMipPoint = 0x114;
	var MininumMinMagMipLinear = 0x115;
	var MininumAnisotropic = 0x155;
	var MaximumMinMagMipPoint = 0x180;
	var MaximumMinMagPointMipLinear = 0x181;
	var MaximumMinPointMagLinearMipPoint = 0x184;
	var MaximumMinPointMagMipLinear = 0x185;
	var MaximumMinLinearMagMipPoint = 0x190;
	var MaximumMinLinearMagPointMipLinear = 0x191;
	var MaximumMinMagLinearMipPoint = 0x194;
	var MaximumMinMagMipLinear = 0x195;
	var MaximumAnisotropic = 0x1d5;
}

@:enum abstract AddressMode(Int) {
	var Wrap = 1;
	var Mirror = 2;
	var Clamp = 3;
	var Border = 4;
	var MirrorOnce = 5;
}

@:keep
class SamplerDesc {
	public var filter : Filter;
	public var addressU : AddressMode;
	public var addressV : AddressMode;
	public var addressW : AddressMode;
	public var mipLodBias : hl.F32;
	public var maxAnisotropy : Int;
	public var comparisonFunc : ComparisonFunc;
	public var borderColorR : hl.F32;
	public var borderColorG : hl.F32;
	public var borderColorB : hl.F32;
	public var borderColorA : hl.F32;
	public var minLod : hl.F32;
	public var maxLod : hl.F32;
	public function new() {
	}
}

@:keep
class ShaderResourceViewDesc {
	public var format : Format;
	public var dimension : ResourceDimension;
	public var start : Int;
	public var count : Int;
	public var firstArraySlice : Int;
	public var arraySize : Int;
	public function new() {
	}
}

@:enum abstract PresentFlags(Int) {
	var None = 0;
	var Test = 1;
	var DoNotSequence = 2;
	var Restart = 4;
	var DoNotWait = 8;
	var RestrictToOutput = 0x10;
	var StereoPreferRight = 0x20;
	var StereoTemporaryMono = 0x40;
	var UseDuration = 0x100;
	var AllowTearing = 0x200;
	@:op(a | b) static function or(a:PresentFlags, b:PresentFlags) : PresentFlags;
}

@:hlNative("directx")
class Driver {

	public static var fullScreen(get, set) : Bool;

	/**
		Setup an error handler instead of getting String exceptions:
		The first parameter is the DirectX error code
		The second parameter is the removed reason code if the first is DXGI_ERROR_DEVICE_REMOVED
		The third parameter is the line in directx.cpp sources where was triggered the error.
		Allocation methods will return null if an error handler is setup and does not raise exception.
	**/
	public static function setErrorHandler( f : Int -> Int -> Int -> Void ) {
	}

	public static function create( win : Window, format : Format, flags : DriverInitFlags = None, restrictLevel = 0 ) {
		return dxCreate(@:privateAccess win.win, format, flags, restrictLevel);
	}

	public static function disposeDriver( driver : DriverInstance ) {
	}

	public static function resize( width : Int, height : Int, format : Format ) : Bool {
		return false;
	}

	public static function getBackBuffer() : Resource {
		return null;
	}

	public static function createRenderTargetView( r : Resource, ?desc : RenderTargetDesc ) : RenderTargetView {
		return dxCreateRenderTargetView(r,desc);
	}

	public static function omSetRenderTargets( count : Int, arr : hl.Ref<RenderTargetView>, ?depth : DepthStencilView ) {
	}

	public static function createRasterizerState( desc : RasterizerDesc ) : RasterState {
		return dxCreateRasterizerState(desc);
	}

	public static function rsSetState( r : RasterState ) {
	}

	public static function rsSetViewports( count : Int, bytes : hl.BytesAccess<hl.F32> ) {
	}

	public static function rsSetScissorRects( count : Int, rects : hl.BytesAccess<Int> ) {
	}

	public static function clearColor( rt : RenderTargetView, r : Float, g : Float, b : Float, a : Float ) {
	}

	public static function present( intervals : Int, flags : PresentFlags ) {
	}

	public static function getDeviceName() {
		return @:privateAccess String.fromUCS2(dxGetDeviceName());
	}

	public static function getSupportedVersion() : Float {
		return 0.;
	}

	public static function compileShader( data : String, source : String, entryPoint : String, target : String, flags : ShaderFlags ) : haxe.io.Bytes @:privateAccess {
		var isError = false, size = 0;
		var out = dxCompileShader(data.toUtf8(), data.length, source.toUtf8(), entryPoint.toUtf8(), target.toUtf8(), flags, isError, size);
		if( isError )
			throw String.fromUTF8(out);
		#if (haxe_ver < 4)
		throw "Haxe 4.x required";
		#else
		return out.toBytes(size);
		#end
	}

	public static function disassembleShader( data : haxe.io.Bytes, flags : DisassembleFlags, ?comments : String ) : String {
		var size = 0;
		var out = dxDisassembleShader(data, data.length, flags, comments == null ? null : @:privateAccess comments.toUtf8(), size);
		if( out == null )
			throw "Could not disassemble shader";
		return @:privateAccess String.fromUTF8(out);
	}

	public static function releasePointer( p : Pointer ) {
	}

	public static function createVertexShader( bytes : haxe.io.Bytes ) {
		return dxCreateVertexShader(bytes, bytes.length);
	}

	public static function createPixelShader( bytes : haxe.io.Bytes ) {
		return dxCreatePixelShader(bytes, bytes.length);
	}

	public static function drawIndexed( indexCount : Int, startIndex : Int, baseVertex : Int ) : Void {
	}

	public static function drawIndexedInstanced( indexCountPerInstance : Int, instanceCount : Int, startIndexLocation : Int, baseVertexLocation : Int, startInstanceLocation : Int ) {
	}

	public static function drawIndexedInstancedIndirect( buffer : Resource, offset : Int ) : Void {
	}

	public static function vsSetShader( shader : Shader ) : Void {
	}

	public static function vsSetConstantBuffers( start : Int, count : Int, buffers : hl.Ref<Resource> ) : Void {
	}

	public static function psSetShader( shader : Shader ) : Void {
	}

	public static function psSetConstantBuffers( start : Int, count : Int, buffers : hl.Ref<Resource> ) : Void {
	}

	public static function iaSetPrimitiveTopology( topology : PrimitiveTopology ) : Void {
	}

	public static function iaSetIndexBuffer( buffer : Resource, is32Bits : Bool, offset : Int ) : Void {
	}

	public static function iaSetVertexBuffers( start : Int, count : Int, buffers : hl.Ref<Resource>, strides : hl.BytesAccess<Int>, offsets : hl.BytesAccess<Int> ) : Void {
	}

	public static function iaSetInputLayout( layout : Layout ) : Void {
	}

	public static function createInputLayout( elements : hl.NativeArray<LayoutElement>, shaderBytes : hl.Bytes, shaderSize : Int ) : Layout {
		return null;
	}

	public static function createBuffer( size : Int, usage : ResourceUsage, bind : ResourceBind, access : ResourceAccess, misc : ResourceMisc, stride : Int, data : hl.Bytes ) : Resource {
		return null;
	}

	public static function createTexture2d( desc : Texture2dDesc, ?data : hl.Bytes ) : Resource {
		return dxCreateTexture2d(desc,data);
	}

	public static function createDepthStencilView( texture : Resource, format : Format ) : DepthStencilView {
		return null;
	}

	public static function omSetDepthStencilState( state : DepthStencilState, ref : Int ) : Void {
	}

	public static function clearDepthStencilView( view : DepthStencilView, depth : Null<Float>, stencil : Null<Int> ) {
	}

	public static function createDepthStencilState( desc : DepthStencilDesc ) : DepthStencilState {
		return dxCreateDepthStencilState(desc);
	}

	public static function createBlendState( alphaToCoverage : Bool, independentBlend : Bool, blendDesc : hl.NativeArray<RenderTargetBlendDesc>, count : Int ) : BlendState {
		return null;
	}

	public static function omSetBlendState( state : BlendState, factors : hl.BytesAccess<hl.F32>, sampleMask : Int ) {
	}

	public static function createSamplerState( state : SamplerDesc ) : SamplerState {
		return dxCreateSamplerState(state);
	}

	public static function createShaderResourceView( res : Resource, desc : ShaderResourceViewDesc ) : ShaderResourceView {
		return dxCreateShaderResourceView(res, desc);
	}

	public static function psSetSamplers( start : Int, count : Int, arr : hl.Ref<SamplerState> ) {
	}

	public static function vsSetSamplers( start : Int, count : Int, arr : hl.Ref<SamplerState> ) {
	}

	public static function psSetShaderResources( start : Int, count : Int, arr : hl.Ref<ShaderResourceView> ) {
	}

	public static function vsSetShaderResources( start : Int, count : Int, arr : hl.Ref<ShaderResourceView> ) {
	}

	public static function generateMips( res : ShaderResourceView ) {
	}

	public static function debugPrint( v : Dynamic ) {
		dxDebugPrint(@:privateAccess Std.string(v).bytes);
	}

	static function get_fullScreen() return getFullscreenState();
	static function set_fullScreen(b) {
		if( !setFullscreenState(b) )
			return false;
		return b;
	}

	static function getFullscreenState() {
		return false;
	}

	static function setFullscreenState( b : Bool ) {
		return false;
	}

	@:hlNative("directx", "create_depth_stencil_state")
	static function dxCreateDepthStencilState( desc : Dynamic ) : DepthStencilState {
		return null;
	}

	@:hlNative("directx", "create_rasterizer_state")
	static function dxCreateRasterizerState( desc : Dynamic ) : RasterState {
		return null;
	}

	@:hlNative("directx", "create_render_target_view")
	static function dxCreateRenderTargetView( r : Resource, desc : Dynamic ) : RenderTargetView {
		return null;
	}

	@:hlNative("directx","create")
	static function dxCreate( win : hl.Abstract<"dx_window">, format : Format, flags : DriverInitFlags, restrictLevel : Int ) : DriverInstance { return null; }

	@:hlNative("directx","get_device_name")
	static function dxGetDeviceName() : hl.Bytes { return null; }

	@:hlNative("directx","compile_shader")
	static function dxCompileShader( data : hl.Bytes, size : Int, source : hl.Bytes, entry : hl.Bytes, target : hl.Bytes, flags : ShaderFlags, error : hl.Ref<Bool>, outSize : hl.Ref<Int> ) : hl.Bytes {
		return null;
	}

	@:hlNative("directx", "disassemble_shader")
	static function dxDisassembleShader( data : hl.Bytes, size : Int, flags : DisassembleFlags, comments : hl.Bytes, outSize : hl.Ref<Int> ) : hl.Bytes {
		return null;
	}

	@:hlNative("directx","create_vertex_shader")
	static function dxCreateVertexShader( data : hl.Bytes, size : Int ) : Shader {
		return null;
	}

	@:hlNative("directx","create_pixel_shader")
	static function dxCreatePixelShader( data : hl.Bytes, size : Int ) : Shader {
		return null;
	}

	@:hlNative("directx","create_texture_2d")
	static function dxCreateTexture2d( desc : Dynamic, data : hl.Bytes ) : Resource {
		return null;
	}

	@:hlNative("directx","create_sampler_state")
	static function dxCreateSamplerState( desc : Dynamic ) : SamplerState {
		return null;
	}

	@:hlNative("directx","create_shader_resource_view")
	static function dxCreateShaderResourceView( res : Resource, desc : Dynamic ) : ShaderResourceView {
		return null;
	}

	@:hlNative("directx","debug_print")
	static function dxDebugPrint( str : hl.Bytes ) {
	}

	public static function detectKeyboardLayout() @:privateAccess {
		return String.fromUTF8( dxDetectKeyboardLayout() );
	}

	@:hlNative("directx", "detect_keyboard_layout")
	static function dxDetectKeyboardLayout() : hl.Bytes {
		return null;
	}

}