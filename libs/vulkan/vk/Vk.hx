package vk;

typedef Image = hl.Abstract<"vk_image">;
typedef ImageView = hl.Abstract<"vk_image_view">;
typedef Sampler = hl.Abstract<"vk_sampler">;
typedef Buffer = hl.Abstract<"vk_buffer">;
typedef Fence = hl.Abstract<"vk_fence">;
typedef Semaphore = hl.Abstract<"vk_semaphore">;
typedef CommandPool = hl.Abstract<"vk_command_pool">;
typedef CommandBuffer = hl.Abstract<"vk_command_buffer">;
typedef ShaderModule = hl.Abstract<"vk_shader_module">;
typedef DescriptorSetLayout = hl.Abstract<"vk_descriptor_set_layout">;
typedef PipelineLayout = hl.Abstract<"vk_pipeline_layout">;
typedef Pipeline = hl.Abstract<"vk_pipeline">;
typedef PipelineCache = hl.Abstract<"vk_pipeline_cache">;
typedef DescriptorPool = hl.Abstract<"vk_descriptor_pool">;
typedef DescriptorSet = hl.Abstract<"vk_descriptor_set">;
typedef QueryPool = hl.Abstract<"vk_query_pool">;
typedef Instance = hl.Abstract<"vk_instance">;
typedef Surface = hl.Abstract<"vk_surface">;

typedef PhysicalDevice = hl.Abstract<"vk_physical_device">;
typedef Device = hl.Abstract<"vk_device">;
typedef Queue = hl.Abstract<"vk_queue">;
typedef Swapchain = hl.Abstract<"vk_swapchain">;
typedef NativeImage = hl.Abstract<"vk_native_image">;
typedef NativeBuffer = hl.Abstract<"vk_native_buffer">;
typedef Proc = hl.Abstract<"vk_proc">;

enum abstract Result(Int) to Int {
	var SUCCESS = 0;
	var NOT_READY = 1;
	var TIMEOUT = 2;
	var SUBOPTIMAL = 1000001003;
	var ERROR_OUT_OF_HOST_MEMORY = -1;
	var ERROR_OUT_OF_DEVICE_MEMORY = -2;
	var ERROR_INITIALIZATION_FAILED = -3;
	var ERROR_DEVICE_LOST = -4;
	var ERROR_EXTENSION_NOT_PRESENT = -7;
	var ERROR_FEATURE_NOT_PRESENT = -8;
	var ERROR_INCOMPATIBLE_DRIVER = -9;
	var ERROR_FRAGMENTED_POOL = -12;
	var ERROR_OUT_OF_POOL_MEMORY = -1000069000;
	var ERROR_SURFACE_LOST = -1000000000;
	var ERROR_OUT_OF_DATE = -1000001004;
}

enum abstract ColorSpace(Int) to Int {
	var SRGB_NONLINEAR = 0;
}

enum abstract PresentMode(Int) to Int {
	var IMMEDIATE = 0;
	var MAILBOX = 1;
	var FIFO = 2;
	var FIFO_RELAXED = 3;
}

enum abstract CompositeAlpha(Int) to Int {
	var OPAQUE = 1;
	var PRE_MULTIPLIED = 2;
	var POST_MULTIPLIED = 4;
	var INHERIT = 8;
}

enum abstract SurfaceTransform(Int) to Int {
	var IDENTITY = 1;
}

enum abstract Format(Int) to Int {
	var UNDEFINED = 0;
	var R8_UNORM = 9;
	var R8_SNORM = 10;
	var R8G8_UNORM = 16;
	var R8G8_SNORM = 17;
	var R8G8B8A8_UNORM = 37;
	var R8G8B8A8_SNORM = 38;
	var R8G8B8A8_UINT = 41;
	var R8G8B8A8_SRGB = 43;
	var B8G8R8A8_UNORM = 44;
	var A2B10G10R10_UNORM_PACK32 = 64;
	var R16_UNORM = 70;
	var R16_SFLOAT = 76;
	var R16G16_UNORM = 77;
	var R16G16_SFLOAT = 83;
	var R16G16B16A16_UNORM = 91;
	var R16G16B16A16_SFLOAT = 97;
	var R32_UINT = 98;
	var R32_SFLOAT = 100;
	var R32G32_SFLOAT = 103;
	var R32G32B32_SFLOAT = 106;
	var R32G32B32A32_SFLOAT = 109;
	var B10G11R11_UFLOAT_PACK32 = 122;
	var D16_UNORM = 124;
	var D32_SFLOAT = 126;
	var D24_UNORM_S8_UINT = 129;
	var D32_SFLOAT_S8_UINT = 130;
	var BC1_RGBA_UNORM_BLOCK = 133;
	var BC2_UNORM_BLOCK = 135;
	var BC3_UNORM_BLOCK = 137;
	var BC4_UNORM_BLOCK = 139;
	var BC5_UNORM_BLOCK = 141;
	var BC6H_UFLOAT_BLOCK = 143;
	var BC7_UNORM_BLOCK = 145;
}

enum abstract FormatFeature(Int) to Int {
	var SAMPLED_IMAGE = 0x1;
	var STORAGE_IMAGE = 0x2;
	var COLOR_ATTACHMENT = 0x80;
	var COLOR_ATTACHMENT_BLEND = 0x100;
	var DEPTH_STENCIL_ATTACHMENT = 0x200;
	var BLIT_SRC = 0x400;
	var BLIT_DST = 0x800;
	var SAMPLED_IMAGE_FILTER_LINEAR = 0x1000;
}

enum abstract ImageLayout(Int) to Int {
	var UNDEFINED = 0;
	var GENERAL = 1;
	var COLOR_ATTACHMENT_OPTIMAL = 2;
	var DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3;
	var DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4;
	var SHADER_READ_ONLY_OPTIMAL = 5;
	var TRANSFER_SRC_OPTIMAL = 6;
	var TRANSFER_DST_OPTIMAL = 7;
	var PRESENT_SRC = 1000001002;
}

enum abstract ImageType(Int) to Int {
	var TYPE_1D = 0;
	var TYPE_2D = 1;
	var TYPE_3D = 2;
}

enum abstract ImageViewType(Int) to Int {
	var TYPE_1D = 0;
	var TYPE_2D = 1;
	var TYPE_3D = 2;
	var CUBE = 3;
	var TYPE_1D_ARRAY = 4;
	var TYPE_2D_ARRAY = 5;
	var CUBE_ARRAY = 6;
}

enum abstract ImageCreateFlags(Int) to Int {
	var CUBE_COMPATIBLE = 0x10;
	var ARRAY_2D_COMPATIBLE = 0x20;
}

enum abstract ImageUsage(Int) to Int {
	var TRANSFER_SRC = 0x1;
	var TRANSFER_DST = 0x2;
	var SAMPLED = 0x4;
	var STORAGE = 0x8;
	var COLOR_ATTACHMENT = 0x10;
	var DEPTH_STENCIL_ATTACHMENT = 0x20;
}

enum abstract ImageAspect(Int) to Int {
	var COLOR = 1;
	var DEPTH = 2;
	var STENCIL = 4;
	var DEPTH_STENCIL = 6;
}

enum abstract BufferUsage(Int) to Int {
	var SHADER_DEVICE_ADDRESS = 0x20000;
	var TRANSFER_SRC = 0x1;
	var TRANSFER_DST = 0x2;
	var UNIFORM = 0x10;
	var STORAGE = 0x20;
	var INDEX = 0x40;
	var VERTEX = 0x80;
	var INDIRECT = 0x100;
}

enum abstract PipelineStage(Int) to Int {
	var NONE = 0;
	var TOP_OF_PIPE = 0x1;
	var DRAW_INDIRECT = 0x2;
	var VERTEX_INPUT = 0x4;
	var VERTEX_SHADER = 0x8;
	var FRAGMENT_SHADER = 0x80;
	var EARLY_FRAGMENT_TESTS = 0x100;
	var LATE_FRAGMENT_TESTS = 0x200;
	var COLOR_ATTACHMENT_OUTPUT = 0x400;
	var COMPUTE_SHADER = 0x800;
	var TRANSFER = 0x1000;
	var BOTTOM_OF_PIPE = 0x2000;
	var HOST = 0x4000;
	var ALL_GRAPHICS = 0x8000;
	var ALL_COMMANDS = 0x10000;
}

enum abstract Access(Int) to Int {
	var NONE = 0;
	var INDIRECT_COMMAND_READ = 0x1;
	var INDEX_READ = 0x2;
	var VERTEX_ATTRIBUTE_READ = 0x4;
	var UNIFORM_READ = 0x8;
	var SHADER_READ = 0x20;
	var SHADER_WRITE = 0x40;
	var COLOR_ATTACHMENT_READ = 0x80;
	var COLOR_ATTACHMENT_WRITE = 0x100;
	var DEPTH_STENCIL_ATTACHMENT_READ = 0x200;
	var DEPTH_STENCIL_ATTACHMENT_WRITE = 0x400;
	var TRANSFER_READ = 0x800;
	var TRANSFER_WRITE = 0x1000;
	var HOST_READ = 0x2000;
	var HOST_WRITE = 0x4000;
	var MEMORY_READ = 0x8000;
	var MEMORY_WRITE = 0x10000;
}

enum abstract DescriptorType(Int) to Int {
	var SAMPLER = 0;
	var SAMPLED_IMAGE = 2;
	var STORAGE_IMAGE = 3;
	var UNIFORM_BUFFER = 6;
	var STORAGE_BUFFER = 7;
	var UNIFORM_BUFFER_DYNAMIC = 8;
}

enum abstract DescriptorBindingFlags(Int) to Int {
	var UPDATE_AFTER_BIND = 0x1;
	var UPDATE_UNUSED_WHILE_PENDING = 0x2;
	var PARTIALLY_BOUND = 0x4;
	var VARIABLE_DESCRIPTOR_COUNT = 0x8;
}

enum abstract DescriptorSetLayoutCreateFlags(Int) to Int {
	var UPDATE_AFTER_BIND_POOL = 0x2;
}

enum abstract DescriptorPoolCreateFlags(Int) to Int {
	var FREE_DESCRIPTOR_SET = 0x1;
	var UPDATE_AFTER_BIND = 0x2;
}

enum abstract ShaderStage(Int) to Int {
	var VERTEX = 0x1;
	var FRAGMENT = 0x10;
	var COMPUTE = 0x20;
	var ALL_GRAPHICS = 0x1F;
	var ALL = 0x7FFFFFFF;
}

enum abstract Filter(Int) to Int {
	var NEAREST = 0;
	var LINEAR = 1;
}

enum abstract SamplerMipmapMode(Int) to Int {
	var NEAREST = 0;
	var LINEAR = 1;
}

enum abstract SamplerAddressMode(Int) to Int {
	var REPEAT = 0;
	var MIRRORED_REPEAT = 1;
	var CLAMP_TO_EDGE = 2;
	var CLAMP_TO_BORDER = 3;
}

@:struct class SurfaceCaps {
	public var minImageCount : Int;
	public var maxImageCount : Int;
	public var currentWidth : Int;
	public var currentHeight : Int;
	public var minWidth : Int;
	public var minHeight : Int;
	public var maxWidth : Int;
	public var maxHeight : Int;
	public var maxImageArrayLayers : Int;
	public var supportedTransforms : Int;
	public var currentTransform : Int;
	public var supportedCompositeAlpha : Int;
	public var supportedUsage : Int;
	public function new() {
	}
}

@:hlNative("vulkan","compiler_")
abstract ShaderCompiler(hl.Abstract<"vk_compiler">) {
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
	@:hlNative("vulkan","compiler_compile")
	static function do_compile( comp : ShaderCompiler, source : hl.Bytes, profile : hl.Bytes, args : hl.NativeArray<hl.Bytes>, out : hl.Ref<Int> ) : hl.Bytes {
		return null;
	}
	static function create() : ShaderCompiler { return null; }
}

enum abstract StructureType(Int) to Int {
	var COMMAND_BUFFER_BEGIN_INFO = 42;
	var PRESENT_INFO = 1000001001;
	var RENDERING_INFO = 1000044000;
	var RENDERING_ATTACHMENT_INFO = 1000044001;
	var MEMORY_BARRIER_2 = 1000314000;
	var BUFFER_MEMORY_BARRIER_2 = 1000314001;
	var IMAGE_MEMORY_BARRIER_2 = 1000314002;
	var DEPENDENCY_INFO = 1000314003;
	var SUBMIT_INFO_2 = 1000314004;
	var SEMAPHORE_SUBMIT_INFO = 1000314005;
	var COMMAND_BUFFER_SUBMIT_INFO = 1000314006;
	var COPY_BUFFER_INFO_2 = 1000337000;
	var COPY_IMAGE_INFO_2 = 1000337001;
	var COPY_BUFFER_TO_IMAGE_INFO_2 = 1000337002;
	var COPY_IMAGE_TO_BUFFER_INFO_2 = 1000337003;
	var BLIT_IMAGE_INFO_2 = 1000337004;
	var BUFFER_COPY_2 = 1000337006;
	var IMAGE_COPY_2 = 1000337007;
	var IMAGE_BLIT_2 = 1000337008;
	var BUFFER_IMAGE_COPY_2 = 1000337009;
	var BUFFER_CREATE_INFO = 12;
	var IMAGE_CREATE_INFO = 14;
	var IMAGE_VIEW_CREATE_INFO = 15;
	var SAMPLER_CREATE_INFO = 31;
	var PIPELINE_LAYOUT_CREATE_INFO = 30;
	var DESCRIPTOR_SET_LAYOUT_CREATE_INFO = 32;
	var DESCRIPTOR_POOL_CREATE_INFO = 33;
	var DESCRIPTOR_SET_ALLOCATE_INFO = 34;
	var WRITE_DESCRIPTOR_SET = 35;
	var SHADER_MODULE_CREATE_INFO = 16;
	var PIPELINE_CACHE_CREATE_INFO = 17;
	var PIPELINE_SHADER_STAGE_CREATE_INFO = 18;
	var PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO = 19;
	var PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO = 20;
	var PIPELINE_VIEWPORT_STATE_CREATE_INFO = 22;
	var PIPELINE_RASTERIZATION_STATE_CREATE_INFO = 23;
	var PIPELINE_MULTISAMPLE_STATE_CREATE_INFO = 24;
	var PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO = 25;
	var PIPELINE_COLOR_BLEND_STATE_CREATE_INFO = 26;
	var PIPELINE_DYNAMIC_STATE_CREATE_INFO = 27;
	var GRAPHICS_PIPELINE_CREATE_INFO = 28;
	var COMPUTE_PIPELINE_CREATE_INFO = 29;
	var PIPELINE_RENDERING_CREATE_INFO = 1000044002;
	var APPLICATION_INFO = 0;
	var INSTANCE_CREATE_INFO = 1;
	var DEVICE_QUEUE_CREATE_INFO = 2;
	var DEVICE_CREATE_INFO = 3;
	var PHYSICAL_DEVICE_VULKAN_1_1_FEATURES = 49;
	var PHYSICAL_DEVICE_VULKAN_1_2_FEATURES = 51;
	var PHYSICAL_DEVICE_VULKAN_1_3_FEATURES = 53;
	var PHYSICAL_DEVICE_FEATURES_2 = 1000059000;
	var PHYSICAL_DEVICE_PROPERTIES_2 = 1000059001;
	var RESOLVE_IMAGE_INFO_2 = 1000337005;
	var IMAGE_RESOLVE_2 = 1000337010;
	var COMMAND_BUFFER_INHERITANCE_INFO = 41;
	var COMMAND_BUFFER_INHERITANCE_RENDERING_INFO = 1000044004;
	var BUFFER_DEVICE_ADDRESS_INFO = 1000244001;
	var SWAPCHAIN_CREATE_INFO = 1000001000;
	var DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO = 1000161000;
}

enum abstract AttachmentLoadOp(Int) to Int {
	var LOAD = 0;
	var CLEAR = 1;
	var DONT_CARE = 2;
}

enum abstract AttachmentStoreOp(Int) to Int {
	var STORE = 0;
	var DONT_CARE = 1;
}

enum abstract PipelineBindPoint(Int) to Int {
	var GRAPHICS = 0;
	var COMPUTE = 1;
}

enum abstract StencilFace(Int) to Int {
	var FRONT = 1;
	var BACK = 2;
	var FRONT_AND_BACK = 3;
}

enum abstract CommandBufferUsage(Int) to Int {
	var ONE_TIME_SUBMIT = 1;
}

enum abstract QueryResultFlags(Int) to Int {
	var RESULT_64 = 1;
	var WAIT = 2;
	var WITH_AVAILABILITY = 4;
	var PARTIAL = 8;
}

class VkConst {
	public static inline var IGNORED = -1;
	public static var WHOLE_SIZE(get, never) : hl.I64;
	static inline function get_WHOLE_SIZE() : hl.I64 return -1;

	public static inline function pointer<T>( s : T ) : hl.Bytes {
		return untyped $unsafecast(s);
	}
}

@:struct class ImageSubresourceRange {
	public var aspectMask : Int;
	public var baseMipLevel : Int;
	public var levelCount : Int;
	public var baseArrayLayer : Int;
	public var layerCount : Int;
	public function new() {
	}
}

@:struct class ImageSubresourceLayers {
	public var aspectMask : Int;
	public var mipLevel : Int;
	public var baseArrayLayer : Int;
	public var layerCount : Int;
	public function new() {
	}
}

@:struct class Offset3D {
	public var x : Int;
	public var y : Int;
	public var z : Int;
	public function new() {
	}
	public inline function set( x : Int, y : Int, z : Int ) {
		this.x = x;
		this.y = y;
		this.z = z;
	}
}

@:struct class Rect2D {
	public var x : Int;
	public var y : Int;
	public var width : Int;
	public var height : Int;
	public function new() {
	}
	public inline function set( x : Int, y : Int, width : Int, height : Int ) {
		this.x = x;
		this.y = y;
		this.width = width;
		this.height = height;
	}
}

@:struct class Viewport {
	public var x : Single;
	public var y : Single;
	public var width : Single;
	public var height : Single;
	public var minDepth : Single;
	public var maxDepth : Single;
	public function new() {
	}
}

@:struct class ClearValue {
	var v0 : Int;
	var v1 : Int;
	var v2 : Int;
	var v3 : Int;
	public function new() {
	}
	public inline function setColor( r : Float, g : Float, b : Float, a : Float ) {
		v0 = haxe.io.FPHelper.floatToI32(r);
		v1 = haxe.io.FPHelper.floatToI32(g);
		v2 = haxe.io.FPHelper.floatToI32(b);
		v3 = haxe.io.FPHelper.floatToI32(a);
	}
	public inline function setDepthStencil( depth : Float, stencil : Int ) {
		v0 = haxe.io.FPHelper.floatToI32(depth);
		v1 = stencil;
	}
}

@:struct class ClearAttachment {
	public var aspectMask : Int;
	public var colorAttachment : Int;
	@:packed public var clearValue(default, null) : ClearValue;
	public function new() {
	}
}

@:struct class ClearRect {
	@:packed public var rect(default, null) : Rect2D;
	public var baseArrayLayer : Int;
	public var layerCount : Int;
	public function new() {
	}
}

@:struct class MemoryBarrier2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcStageMask : hl.I64;
	public var srcAccessMask : hl.I64;
	public var dstStageMask : hl.I64;
	public var dstAccessMask : hl.I64;
	public function new() {
		sType = MEMORY_BARRIER_2;
	}
	public static function alloc( count : Int ) : hl.CArray<MemoryBarrier2> {
		var arr = hl.CArray.alloc(MemoryBarrier2, count);
		for( i in 0...count ) arr[i].sType = MEMORY_BARRIER_2;
		return arr;
	}
}

@:struct class BufferMemoryBarrier2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcStageMask : hl.I64;
	public var srcAccessMask : hl.I64;
	public var dstStageMask : hl.I64;
	public var dstAccessMask : hl.I64;
	public var srcQueueFamilyIndex : Int;
	public var dstQueueFamilyIndex : Int;
	public var buffer : NativeBuffer;
	public var offset : hl.I64;
	public var size : hl.I64;
	public function new() {
		sType = BUFFER_MEMORY_BARRIER_2;
	}
	public static function alloc( count : Int ) : hl.CArray<BufferMemoryBarrier2> {
		var arr = hl.CArray.alloc(BufferMemoryBarrier2, count);
		for( i in 0...count ) arr[i].sType = BUFFER_MEMORY_BARRIER_2;
		return arr;
	}
}

@:struct class ImageMemoryBarrier2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcStageMask : hl.I64;
	public var srcAccessMask : hl.I64;
	public var dstStageMask : hl.I64;
	public var dstAccessMask : hl.I64;
	public var oldLayout : ImageLayout;
	public var newLayout : ImageLayout;
	public var srcQueueFamilyIndex : Int;
	public var dstQueueFamilyIndex : Int;
	public var image : NativeImage;
	@:packed public var subresourceRange(default, null) : ImageSubresourceRange;
	public function new() {
		sType = IMAGE_MEMORY_BARRIER_2;
	}
	public static function alloc( count : Int ) : hl.CArray<ImageMemoryBarrier2> {
		var arr = hl.CArray.alloc(ImageMemoryBarrier2, count);
		for( i in 0...count ) arr[i].sType = IMAGE_MEMORY_BARRIER_2;
		return arr;
	}
}

@:struct class DependencyInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var dependencyFlags : Int;
	public var memoryBarrierCount : Int;
	public var pMemoryBarriers : hl.CArray<MemoryBarrier2>;
	public var bufferMemoryBarrierCount : Int;
	public var pBufferMemoryBarriers : hl.CArray<BufferMemoryBarrier2>;
	public var imageMemoryBarrierCount : Int;
	public var pImageMemoryBarriers : hl.CArray<ImageMemoryBarrier2>;
	public function new() {
		sType = DEPENDENCY_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<DependencyInfo> {
		var arr = hl.CArray.alloc(DependencyInfo, count);
		for( i in 0...count ) arr[i].sType = DEPENDENCY_INFO;
		return arr;
	}
}

@:struct class RenderingAttachmentInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var imageView : ImageView;
	public var imageLayout : ImageLayout;
	public var resolveMode : Int;
	public var resolveImageView : ImageView;
	public var resolveImageLayout : ImageLayout;
	public var loadOp : AttachmentLoadOp;
	public var storeOp : AttachmentStoreOp;
	@:packed public var clearValue(default, null) : ClearValue;
	public function new() {
		sType = RENDERING_ATTACHMENT_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<RenderingAttachmentInfo> {
		var arr = hl.CArray.alloc(RenderingAttachmentInfo, count);
		for( i in 0...count ) arr[i].sType = RENDERING_ATTACHMENT_INFO;
		return arr;
	}
}

@:struct class RenderingInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	@:packed public var renderArea(default, null) : Rect2D;
	public var layerCount : Int;
	public var viewMask : Int;
	public var colorAttachmentCount : Int;
	public var pColorAttachments : hl.CArray<RenderingAttachmentInfo>;
	public var pDepthAttachment : RenderingAttachmentInfo;
	public var pStencilAttachment : RenderingAttachmentInfo;
	public function new() {
		sType = RENDERING_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<RenderingInfo> {
		var arr = hl.CArray.alloc(RenderingInfo, count);
		for( i in 0...count ) arr[i].sType = RENDERING_INFO;
		return arr;
	}
}

@:struct class BufferCopy2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcOffset : hl.I64;
	public var dstOffset : hl.I64;
	public var size : hl.I64;
	public function new() {
		sType = BUFFER_COPY_2;
	}
	public static function alloc( count : Int ) : hl.CArray<BufferCopy2> {
		var arr = hl.CArray.alloc(BufferCopy2, count);
		for( i in 0...count ) arr[i].sType = BUFFER_COPY_2;
		return arr;
	}
}

@:struct class CopyBufferInfo2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcBuffer : NativeBuffer;
	public var dstBuffer : NativeBuffer;
	public var regionCount : Int;
	public var pRegions : hl.CArray<BufferCopy2>;
	public function new() {
		sType = COPY_BUFFER_INFO_2;
	}
	public static function alloc( count : Int ) : hl.CArray<CopyBufferInfo2> {
		var arr = hl.CArray.alloc(CopyBufferInfo2, count);
		for( i in 0...count ) arr[i].sType = COPY_BUFFER_INFO_2;
		return arr;
	}
}

@:struct class BufferImageCopy2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var bufferOffset : hl.I64;
	public var bufferRowLength : Int;
	public var bufferImageHeight : Int;
	@:packed public var imageSubresource(default, null) : ImageSubresourceLayers;
	@:packed public var imageOffset(default, null) : Offset3D;
	@:packed public var imageExtent(default, null) : Offset3D;
	public function new() {
		sType = BUFFER_IMAGE_COPY_2;
	}
	public static function alloc( count : Int ) : hl.CArray<BufferImageCopy2> {
		var arr = hl.CArray.alloc(BufferImageCopy2, count);
		for( i in 0...count ) arr[i].sType = BUFFER_IMAGE_COPY_2;
		return arr;
	}
}

@:struct class CopyBufferToImageInfo2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcBuffer : NativeBuffer;
	public var dstImage : NativeImage;
	public var dstImageLayout : ImageLayout;
	public var regionCount : Int;
	public var pRegions : hl.CArray<BufferImageCopy2>;
	public function new() {
		sType = COPY_BUFFER_TO_IMAGE_INFO_2;
	}
	public static function alloc( count : Int ) : hl.CArray<CopyBufferToImageInfo2> {
		var arr = hl.CArray.alloc(CopyBufferToImageInfo2, count);
		for( i in 0...count ) arr[i].sType = COPY_BUFFER_TO_IMAGE_INFO_2;
		return arr;
	}
}

@:struct class CopyImageToBufferInfo2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcImage : NativeImage;
	public var srcImageLayout : ImageLayout;
	public var dstBuffer : NativeBuffer;
	public var regionCount : Int;
	public var pRegions : hl.CArray<BufferImageCopy2>;
	public function new() {
		sType = COPY_IMAGE_TO_BUFFER_INFO_2;
	}
	public static function alloc( count : Int ) : hl.CArray<CopyImageToBufferInfo2> {
		var arr = hl.CArray.alloc(CopyImageToBufferInfo2, count);
		for( i in 0...count ) arr[i].sType = COPY_IMAGE_TO_BUFFER_INFO_2;
		return arr;
	}
}

@:struct class ImageCopy2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	@:packed public var srcSubresource(default, null) : ImageSubresourceLayers;
	@:packed public var srcOffset(default, null) : Offset3D;
	@:packed public var dstSubresource(default, null) : ImageSubresourceLayers;
	@:packed public var dstOffset(default, null) : Offset3D;
	@:packed public var extent(default, null) : Offset3D;
	public function new() {
		sType = IMAGE_COPY_2;
	}
	public static function alloc( count : Int ) : hl.CArray<ImageCopy2> {
		var arr = hl.CArray.alloc(ImageCopy2, count);
		for( i in 0...count ) arr[i].sType = IMAGE_COPY_2;
		return arr;
	}
}

@:struct class CopyImageInfo2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcImage : NativeImage;
	public var srcImageLayout : ImageLayout;
	public var dstImage : NativeImage;
	public var dstImageLayout : ImageLayout;
	public var regionCount : Int;
	public var pRegions : hl.CArray<ImageCopy2>;
	public function new() {
		sType = COPY_IMAGE_INFO_2;
	}
	public static function alloc( count : Int ) : hl.CArray<CopyImageInfo2> {
		var arr = hl.CArray.alloc(CopyImageInfo2, count);
		for( i in 0...count ) arr[i].sType = COPY_IMAGE_INFO_2;
		return arr;
	}
}

@:struct class ImageBlit2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	@:packed public var srcSubresource(default, null) : ImageSubresourceLayers;
	@:packed public var srcOffset0(default, null) : Offset3D;
	@:packed public var srcOffset1(default, null) : Offset3D;
	@:packed public var dstSubresource(default, null) : ImageSubresourceLayers;
	@:packed public var dstOffset0(default, null) : Offset3D;
	@:packed public var dstOffset1(default, null) : Offset3D;
	public function new() {
		sType = IMAGE_BLIT_2;
	}
	public static function alloc( count : Int ) : hl.CArray<ImageBlit2> {
		var arr = hl.CArray.alloc(ImageBlit2, count);
		for( i in 0...count ) arr[i].sType = IMAGE_BLIT_2;
		return arr;
	}
}

@:struct class BlitImageInfo2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcImage : NativeImage;
	public var srcImageLayout : ImageLayout;
	public var dstImage : NativeImage;
	public var dstImageLayout : ImageLayout;
	public var regionCount : Int;
	public var pRegions : hl.CArray<ImageBlit2>;
	public var filter : Filter;
	public function new() {
		sType = BLIT_IMAGE_INFO_2;
	}
	public static function alloc( count : Int ) : hl.CArray<BlitImageInfo2> {
		var arr = hl.CArray.alloc(BlitImageInfo2, count);
		for( i in 0...count ) arr[i].sType = BLIT_IMAGE_INFO_2;
		return arr;
	}
}

@:struct class CommandBufferBeginInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var pInheritanceInfo : CommandBufferInheritanceInfo;
	public function new() {
		sType = COMMAND_BUFFER_BEGIN_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<CommandBufferBeginInfo> {
		var arr = hl.CArray.alloc(CommandBufferBeginInfo, count);
		for( i in 0...count ) arr[i].sType = COMMAND_BUFFER_BEGIN_INFO;
		return arr;
	}
}

@:struct class CommandBufferSubmitInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var commandBuffer : CommandBuffer;
	public var deviceMask : Int;
	public function new() {
		sType = COMMAND_BUFFER_SUBMIT_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<CommandBufferSubmitInfo> {
		var arr = hl.CArray.alloc(CommandBufferSubmitInfo, count);
		for( i in 0...count ) arr[i].sType = COMMAND_BUFFER_SUBMIT_INFO;
		return arr;
	}
}

@:struct class SemaphoreSubmitInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var semaphore : Semaphore;
	public var value : hl.I64;
	public var stageMask : hl.I64;
	public var deviceIndex : Int;
	public function new() {
		sType = SEMAPHORE_SUBMIT_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<SemaphoreSubmitInfo> {
		var arr = hl.CArray.alloc(SemaphoreSubmitInfo, count);
		for( i in 0...count ) arr[i].sType = SEMAPHORE_SUBMIT_INFO;
		return arr;
	}
}

@:struct class SubmitInfo2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var waitSemaphoreInfoCount : Int;
	public var pWaitSemaphoreInfos : hl.CArray<SemaphoreSubmitInfo>;
	public var commandBufferInfoCount : Int;
	public var pCommandBufferInfos : hl.CArray<CommandBufferSubmitInfo>;
	public var signalSemaphoreInfoCount : Int;
	public var pSignalSemaphoreInfos : hl.CArray<SemaphoreSubmitInfo>;
	public function new() {
		sType = SUBMIT_INFO_2;
	}
	public static function alloc( count : Int ) : hl.CArray<SubmitInfo2> {
		var arr = hl.CArray.alloc(SubmitInfo2, count);
		for( i in 0...count ) arr[i].sType = SUBMIT_INFO_2;
		return arr;
	}
}

@:struct class SemaphoreHandle {
	public var handle : Semaphore;
	public function new() {
	}
}

@:struct class SwapchainHandle {
	public var handle : Swapchain;
	public function new() {
	}
}

@:struct class PresentInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var waitSemaphoreCount : Int;
	public var pWaitSemaphores : hl.CArray<SemaphoreHandle>;
	public var swapchainCount : Int;
	public var pSwapchains : hl.CArray<SwapchainHandle>;
	public var pImageIndices : hl.Bytes;
	public var pResults : hl.Bytes;
	public function new() {
		sType = PRESENT_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PresentInfo> {
		var arr = hl.CArray.alloc(PresentInfo, count);
		for( i in 0...count ) arr[i].sType = PRESENT_INFO;
		return arr;
	}
}

enum abstract SharingMode(Int) to Int {
	var EXCLUSIVE = 0;
	var CONCURRENT = 1;
}

enum abstract ImageTiling(Int) to Int {
	var OPTIMAL = 0;
	var LINEAR = 1;
}

enum abstract IndexType(Int) to Int {
	var UINT16 = 0;
	var UINT32 = 1;
}

enum abstract FenceCreateFlags(Int) to Int {
	var SIGNALED = 1;
}

enum abstract CommandPoolCreateFlags(Int) to Int {
	var TRANSIENT = 1;
	var RESET_COMMAND_BUFFER = 2;
}

enum abstract CommandBufferLevel(Int) to Int {
	var PRIMARY = 0;
	var SECONDARY = 1;
}

enum abstract QueryType(Int) to Int {
	var OCCLUSION = 0;
	var PIPELINE_STATISTICS = 1;
	var TIMESTAMP = 2;
}

enum abstract MemoryProperty(Int) to Int {
	var DEVICE_LOCAL = 1;
	var HOST_VISIBLE = 2;
	var HOST_COHERENT = 4;
	var HOST_CACHED = 8;
}

enum abstract MemoryHeapFlags(Int) to Int {
	var DEVICE_LOCAL = 1;
}

enum abstract MemoryUsage(Int) to Int {
	var UNKNOWN = 0;
	var AUTO = 7;
	var AUTO_PREFER_DEVICE = 8;
	var AUTO_PREFER_HOST = 9;
}

enum abstract AllocationCreateFlags(Int) to Int {
	var DEDICATED_MEMORY = 0x1;
	var MAPPED = 0x4;
	var HOST_ACCESS_SEQUENTIAL_WRITE = 0x400;
	var HOST_ACCESS_RANDOM = 0x800;
}

@:struct class AllocationCreateInfo {
	public var flags : Int;
	public var usage : MemoryUsage;
	public var requiredFlags : Int;
	public var preferredFlags : Int;
	public var memoryTypeBits : Int;
	public var pool : hl.Bytes;
	public var pUserData : hl.Bytes;
	public var priority : Single;
	public var minAlignment : hl.I64;
	public function new() {
	}
}

@:struct class BufferCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var size : hl.I64;
	public var usage : Int;
	public var sharingMode : SharingMode;
	public var queueFamilyIndexCount : Int;
	public var pQueueFamilyIndices : hl.Bytes;
	public function new() {
		sType = BUFFER_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<BufferCreateInfo> {
		var arr = hl.CArray.alloc(BufferCreateInfo, count);
		for( i in 0...count ) arr[i].sType = BUFFER_CREATE_INFO;
		return arr;
	}
}

@:struct class ImageCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var imageType : ImageType;
	public var format : Format;
	@:packed public var extent(default, null) : Offset3D;
	public var mipLevels : Int;
	public var arrayLayers : Int;
	public var samples : Int;
	public var tiling : ImageTiling;
	public var usage : Int;
	public var sharingMode : SharingMode;
	public var queueFamilyIndexCount : Int;
	public var pQueueFamilyIndices : hl.Bytes;
	public var initialLayout : ImageLayout;
	public function new() {
		sType = IMAGE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<ImageCreateInfo> {
		var arr = hl.CArray.alloc(ImageCreateInfo, count);
		for( i in 0...count ) arr[i].sType = IMAGE_CREATE_INFO;
		return arr;
	}
}

@:struct class ComponentMapping {
	public var r : Int;
	public var g : Int;
	public var b : Int;
	public var a : Int;
	public function new() {
	}
}

@:struct class ImageViewCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var image : NativeImage;
	public var viewType : ImageViewType;
	public var format : Format;
	@:packed public var components(default, null) : ComponentMapping;
	@:packed public var subresourceRange(default, null) : ImageSubresourceRange;
	public function new() {
		sType = IMAGE_VIEW_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<ImageViewCreateInfo> {
		var arr = hl.CArray.alloc(ImageViewCreateInfo, count);
		for( i in 0...count ) arr[i].sType = IMAGE_VIEW_CREATE_INFO;
		return arr;
	}
}

@:struct class SamplerCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var magFilter : Filter;
	public var minFilter : Filter;
	public var mipmapMode : SamplerMipmapMode;
	public var addressModeU : SamplerAddressMode;
	public var addressModeV : SamplerAddressMode;
	public var addressModeW : SamplerAddressMode;
	public var mipLodBias : Single;
	public var anisotropyEnable : Int;
	public var maxAnisotropy : Single;
	public var compareEnable : Int;
	public var compareOp : Int;
	public var minLod : Single;
	public var maxLod : Single;
	public var borderColor : Int;
	public var unnormalizedCoordinates : Int;
	public function new() {
		sType = SAMPLER_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<SamplerCreateInfo> {
		var arr = hl.CArray.alloc(SamplerCreateInfo, count);
		for( i in 0...count ) arr[i].sType = SAMPLER_CREATE_INFO;
		return arr;
	}
}

@:struct class BufferHandle {
	public var handle : NativeBuffer;
	public function new() {
	}
}

@:struct class DeviceSize {
	public var value : hl.I64;
	public function new() {
	}
}

class MemoryReport {
	public static inline var PROPERTIES_SIZE = 520;
	public static inline var HEAP_COUNT_OFFSET = 260;
	public static inline var HEAPS_OFFSET = 264;
	public static inline var HEAP_STRIDE = 16;
	public static inline var MAX_HEAPS = 16;
	public static inline var BUDGET_STRIDE = 40;
	public static inline var BUDGET_USAGE_OFFSET = 24;
	public static inline var BUDGET_BUDGET_OFFSET = 32;
}

@:struct class DescriptorSetLayoutBinding {
	public var binding : Int;
	public var descriptorType : DescriptorType;
	public var descriptorCount : Int;
	public var stageFlags : Int;
	public var pImmutableSamplers : hl.Bytes;
	public function new() {
	}
}

@:struct class DescriptorSetLayoutCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var bindingCount : Int;
	public var pBindings : hl.CArray<DescriptorSetLayoutBinding>;
	public function new() {
		sType = DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<DescriptorSetLayoutCreateInfo> {
		var arr = hl.CArray.alloc(DescriptorSetLayoutCreateInfo, count);
		for( i in 0...count ) arr[i].sType = DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		return arr;
	}
}

@:struct class DescriptorSetLayoutBindingFlagsCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var bindingCount : Int;
	public var pBindingFlags : hl.Bytes;
	public function new() {
		sType = DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<DescriptorSetLayoutBindingFlagsCreateInfo> {
		var arr = hl.CArray.alloc(DescriptorSetLayoutBindingFlagsCreateInfo, count);
		for( i in 0...count ) arr[i].sType = DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		return arr;
	}
}

@:struct class DescriptorSetLayoutHandle {
	public var handle : DescriptorSetLayout;
	public function new() {
	}
}

@:struct class PushConstantRange {
	public var stageFlags : Int;
	public var offset : Int;
	public var size : Int;
	public function new() {
	}
}

@:struct class PipelineLayoutCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var setLayoutCount : Int;
	public var pSetLayouts : hl.CArray<DescriptorSetLayoutHandle>;
	public var pushConstantRangeCount : Int;
	public var pPushConstantRanges : hl.CArray<PushConstantRange>;
	public function new() {
		sType = PIPELINE_LAYOUT_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineLayoutCreateInfo> {
		var arr = hl.CArray.alloc(PipelineLayoutCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_LAYOUT_CREATE_INFO;
		return arr;
	}
}

@:struct class DescriptorPoolSize {
	public var type : DescriptorType;
	public var descriptorCount : Int;
	public function new() {
	}
}

@:struct class DescriptorPoolCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var maxSets : Int;
	public var poolSizeCount : Int;
	public var pPoolSizes : hl.CArray<DescriptorPoolSize>;
	public function new() {
		sType = DESCRIPTOR_POOL_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<DescriptorPoolCreateInfo> {
		var arr = hl.CArray.alloc(DescriptorPoolCreateInfo, count);
		for( i in 0...count ) arr[i].sType = DESCRIPTOR_POOL_CREATE_INFO;
		return arr;
	}
}

@:struct class DescriptorSetHandle {
	public var handle : DescriptorSet;
	public function new() {
	}
}

@:struct class DescriptorSetAllocateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var descriptorPool : DescriptorPool;
	public var descriptorSetCount : Int;
	public var pSetLayouts : hl.CArray<DescriptorSetLayoutHandle>;
	public function new() {
		sType = DESCRIPTOR_SET_ALLOCATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<DescriptorSetAllocateInfo> {
		var arr = hl.CArray.alloc(DescriptorSetAllocateInfo, count);
		for( i in 0...count ) arr[i].sType = DESCRIPTOR_SET_ALLOCATE_INFO;
		return arr;
	}
}

@:struct class DescriptorImageInfo {
	public var sampler : Sampler;
	public var imageView : ImageView;
	public var imageLayout : ImageLayout;
	public function new() {
	}
}

@:struct class DescriptorBufferInfo {
	public var buffer : NativeBuffer;
	public var offset : hl.I64;
	public var range : hl.I64;
	public function new() {
	}
}

@:struct class WriteDescriptorSet {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var dstSet : DescriptorSet;
	public var dstBinding : Int;
	public var dstArrayElement : Int;
	public var descriptorCount : Int;
	public var descriptorType : DescriptorType;
	public var pImageInfo : DescriptorImageInfo;
	public var pBufferInfo : DescriptorBufferInfo;
	public var pTexelBufferView : hl.Bytes;
	public function new() {
		sType = WRITE_DESCRIPTOR_SET;
	}
	public static function alloc( count : Int ) : hl.CArray<WriteDescriptorSet> {
		var arr = hl.CArray.alloc(WriteDescriptorSet, count);
		for( i in 0...count ) arr[i].sType = WRITE_DESCRIPTOR_SET;
		return arr;
	}
}

enum abstract DynamicState(Int) to Int {
	var VIEWPORT = 0;
	var SCISSOR = 1;
	var DEPTH_BIAS = 3;
	var BLEND_CONSTANTS = 4;
	var STENCIL_COMPARE_MASK = 6;
	var STENCIL_WRITE_MASK = 7;
	var STENCIL_REFERENCE = 8;
	var VERTEX_INPUT_BINDING_STRIDE = 1000267005;
}

@:struct class ShaderModuleCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var codeSize : hl.I64;
	public var pCode : hl.Bytes;
	public function new() {
		sType = SHADER_MODULE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<ShaderModuleCreateInfo> {
		var arr = hl.CArray.alloc(ShaderModuleCreateInfo, count);
		for( i in 0...count ) arr[i].sType = SHADER_MODULE_CREATE_INFO;
		return arr;
	}
}

@:struct class PipelineCacheCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var initialDataSize : hl.I64;
	public var pInitialData : hl.Bytes;
	public function new() {
		sType = PIPELINE_CACHE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineCacheCreateInfo> {
		var arr = hl.CArray.alloc(PipelineCacheCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_CACHE_CREATE_INFO;
		return arr;
	}
}

@:struct class PipelineHandle {
	public var handle : Pipeline;
	public function new() {
	}
}

@:struct class PipelineShaderStageCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var stage : Int;
	public var module : ShaderModule;
	public var pName : hl.Bytes;
	public var pSpecializationInfo : hl.Bytes;
	public function new() {
		sType = PIPELINE_SHADER_STAGE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineShaderStageCreateInfo> {
		var arr = hl.CArray.alloc(PipelineShaderStageCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_SHADER_STAGE_CREATE_INFO;
		return arr;
	}
	public inline function setType() {
		sType = PIPELINE_SHADER_STAGE_CREATE_INFO;
	}
}

@:struct class VertexInputBindingDescription {
	public var binding : Int;
	public var stride : Int;
	public var inputRate : Int;
	public function new() {
	}
}

@:struct class VertexInputAttributeDescription {
	public var location : Int;
	public var binding : Int;
	public var format : Format;
	public var offset : Int;
	public function new() {
	}
}

@:struct class PipelineVertexInputStateCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var vertexBindingDescriptionCount : Int;
	public var pVertexBindingDescriptions : hl.CArray<VertexInputBindingDescription>;
	public var vertexAttributeDescriptionCount : Int;
	public var pVertexAttributeDescriptions : hl.CArray<VertexInputAttributeDescription>;
	public function new() {
		sType = PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineVertexInputStateCreateInfo> {
		var arr = hl.CArray.alloc(PipelineVertexInputStateCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		return arr;
	}
}

@:struct class PipelineInputAssemblyStateCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var topology : Int;
	public var primitiveRestartEnable : Int;
	public function new() {
		sType = PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineInputAssemblyStateCreateInfo> {
		var arr = hl.CArray.alloc(PipelineInputAssemblyStateCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		return arr;
	}
}

@:struct class PipelineViewportStateCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var viewportCount : Int;
	public var pViewports : hl.CArray<Viewport>;
	public var scissorCount : Int;
	public var pScissors : hl.CArray<Rect2D>;
	public function new() {
		sType = PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineViewportStateCreateInfo> {
		var arr = hl.CArray.alloc(PipelineViewportStateCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		return arr;
	}
}

@:struct class PipelineRasterizationStateCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var depthClampEnable : Int;
	public var rasterizerDiscardEnable : Int;
	public var polygonMode : Int;
	public var cullMode : Int;
	public var frontFace : Int;
	public var depthBiasEnable : Int;
	public var depthBiasConstantFactor : Single;
	public var depthBiasClamp : Single;
	public var depthBiasSlopeFactor : Single;
	public var lineWidth : Single;
	public function new() {
		sType = PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineRasterizationStateCreateInfo> {
		var arr = hl.CArray.alloc(PipelineRasterizationStateCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		return arr;
	}
}

@:struct class PipelineMultisampleStateCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var rasterizationSamples : Int;
	public var sampleShadingEnable : Int;
	public var minSampleShading : Single;
	public var pSampleMask : hl.Bytes;
	public var alphaToCoverageEnable : Int;
	public var alphaToOneEnable : Int;
	public function new() {
		sType = PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineMultisampleStateCreateInfo> {
		var arr = hl.CArray.alloc(PipelineMultisampleStateCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		return arr;
	}
}

@:struct class StencilOpState {
	public var failOp : Int;
	public var passOp : Int;
	public var depthFailOp : Int;
	public var compareOp : Int;
	public var compareMask : Int;
	public var writeMask : Int;
	public var reference : Int;
	public function new() {
	}
}

@:struct class PipelineDepthStencilStateCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var depthTestEnable : Int;
	public var depthWriteEnable : Int;
	public var depthCompareOp : Int;
	public var depthBoundsTestEnable : Int;
	public var stencilTestEnable : Int;
	@:packed public var front(default, null) : StencilOpState;
	@:packed public var back(default, null) : StencilOpState;
	public var minDepthBounds : Single;
	public var maxDepthBounds : Single;
	public function new() {
		sType = PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineDepthStencilStateCreateInfo> {
		var arr = hl.CArray.alloc(PipelineDepthStencilStateCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		return arr;
	}
}

@:struct class PipelineColorBlendAttachmentState {
	public var blendEnable : Int;
	public var srcColorBlendFactor : Int;
	public var dstColorBlendFactor : Int;
	public var colorBlendOp : Int;
	public var srcAlphaBlendFactor : Int;
	public var dstAlphaBlendFactor : Int;
	public var alphaBlendOp : Int;
	public var colorWriteMask : Int;
	public function new() {
	}
}

@:struct class PipelineColorBlendStateCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var logicOpEnable : Int;
	public var logicOp : Int;
	public var attachmentCount : Int;
	public var pAttachments : hl.CArray<PipelineColorBlendAttachmentState>;
	public var blendConstant0 : Single;
	public var blendConstant1 : Single;
	public var blendConstant2 : Single;
	public var blendConstant3 : Single;
	public function new() {
		sType = PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineColorBlendStateCreateInfo> {
		var arr = hl.CArray.alloc(PipelineColorBlendStateCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		return arr;
	}
}

@:struct class PipelineDynamicStateCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var dynamicStateCount : Int;
	public var pDynamicStates : hl.Bytes;
	public function new() {
		sType = PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineDynamicStateCreateInfo> {
		var arr = hl.CArray.alloc(PipelineDynamicStateCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		return arr;
	}
}

@:struct class PipelineRenderingCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var viewMask : Int;
	public var colorAttachmentCount : Int;
	public var pColorAttachmentFormats : hl.Bytes;
	public var depthAttachmentFormat : Format;
	public var stencilAttachmentFormat : Format;
	public function new() {
		sType = PIPELINE_RENDERING_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<PipelineRenderingCreateInfo> {
		var arr = hl.CArray.alloc(PipelineRenderingCreateInfo, count);
		for( i in 0...count ) arr[i].sType = PIPELINE_RENDERING_CREATE_INFO;
		return arr;
	}
}

@:struct class GraphicsPipelineCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var stageCount : Int;
	public var pStages : hl.CArray<PipelineShaderStageCreateInfo>;
	public var pVertexInputState : PipelineVertexInputStateCreateInfo;
	public var pInputAssemblyState : PipelineInputAssemblyStateCreateInfo;
	public var pTessellationState : hl.Bytes;
	public var pViewportState : PipelineViewportStateCreateInfo;
	public var pRasterizationState : PipelineRasterizationStateCreateInfo;
	public var pMultisampleState : PipelineMultisampleStateCreateInfo;
	public var pDepthStencilState : PipelineDepthStencilStateCreateInfo;
	public var pColorBlendState : PipelineColorBlendStateCreateInfo;
	public var pDynamicState : PipelineDynamicStateCreateInfo;
	public var layout : PipelineLayout;
	public var renderPass : hl.Bytes;
	public var subpass : Int;
	public var basePipelineHandle : Pipeline;
	public var basePipelineIndex : Int;
	public function new() {
		sType = GRAPHICS_PIPELINE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<GraphicsPipelineCreateInfo> {
		var arr = hl.CArray.alloc(GraphicsPipelineCreateInfo, count);
		for( i in 0...count ) arr[i].sType = GRAPHICS_PIPELINE_CREATE_INFO;
		return arr;
	}
}

@:struct class ComputePipelineCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	@:packed public var stage(default, null) : PipelineShaderStageCreateInfo;
	public var layout : PipelineLayout;
	public var basePipelineHandle : Pipeline;
	public var basePipelineIndex : Int;
	public function new() {
		sType = COMPUTE_PIPELINE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<ComputePipelineCreateInfo> {
		var arr = hl.CArray.alloc(ComputePipelineCreateInfo, count);
		for( i in 0...count ) arr[i].sType = COMPUTE_PIPELINE_CREATE_INFO;
		return arr;
	}
}

enum abstract PhysicalDeviceType(Int) to Int {
	var OTHER = 0;
	var INTEGRATED_GPU = 1;
	var DISCRETE_GPU = 2;
	var VIRTUAL_GPU = 3;
	var CPU = 4;
}

enum abstract QueueFlags(Int) to Int {
	var GRAPHICS = 1;
	var COMPUTE = 2;
	var TRANSFER = 4;
}

enum abstract DebugMessageSeverity(Int) to Int {
	var VERBOSE = 0x1;
	var INFO = 0x10;
	var WARNING = 0x100;
	var ERROR = 0x1000;
}

enum abstract DebugMessageType(Int) to Int {
	var GENERAL = 0x1;
	var VALIDATION = 0x2;
	var PERFORMANCE = 0x4;
}

enum abstract AllocatorCreateFlags(Int) to Int {
	var EXT_MEMORY_BUDGET = 0x8;
}

@:struct class NameHandle {
	public var name : hl.Bytes;
	public function new() {
	}
}

@:struct class ApplicationInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var pApplicationName : hl.Bytes;
	public var applicationVersion : Int;
	public var pEngineName : hl.Bytes;
	public var engineVersion : Int;
	public var apiVersion : Int;
	public function new() {
		sType = APPLICATION_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<ApplicationInfo> {
		var arr = hl.CArray.alloc(ApplicationInfo, count);
		for( i in 0...count ) arr[i].sType = APPLICATION_INFO;
		return arr;
	}
}

@:struct class InstanceCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var pApplicationInfo : ApplicationInfo;
	public var enabledLayerCount : Int;
	public var ppEnabledLayerNames : hl.CArray<NameHandle>;
	public var enabledExtensionCount : Int;
	public var ppEnabledExtensionNames : hl.CArray<NameHandle>;
	public function new() {
		sType = INSTANCE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<InstanceCreateInfo> {
		var arr = hl.CArray.alloc(InstanceCreateInfo, count);
		for( i in 0...count ) arr[i].sType = INSTANCE_CREATE_INFO;
		return arr;
	}
}

@:struct class PhysicalDeviceHandle {
	public var handle : PhysicalDevice;
	public function new() {
	}
}

@:struct class QueueFamilyProperties {
	public var queueFlags : Int;
	public var queueCount : Int;
	public var timestampValidBits : Int;
	@:packed public var minImageTransferGranularity(default, null) : Offset3D;
	public function new() {
	}
}

@:struct class FormatProperties {
	public var linearTilingFeatures : Int;
	public var optimalTilingFeatures : Int;
	public var bufferFeatures : Int;
	public function new() {
	}
}

@:struct class DeviceQueueCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var queueFamilyIndex : Int;
	public var queueCount : Int;
	public var pQueuePriorities : hl.Bytes;
	public function new() {
		sType = DEVICE_QUEUE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<DeviceQueueCreateInfo> {
		var arr = hl.CArray.alloc(DeviceQueueCreateInfo, count);
		for( i in 0...count ) arr[i].sType = DEVICE_QUEUE_CREATE_INFO;
		return arr;
	}
}

@:struct class DeviceCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var queueCreateInfoCount : Int;
	public var pQueueCreateInfos : hl.CArray<DeviceQueueCreateInfo>;
	public var enabledLayerCount : Int;
	public var ppEnabledLayerNames : hl.CArray<NameHandle>;
	public var enabledExtensionCount : Int;
	public var ppEnabledExtensionNames : hl.CArray<NameHandle>;
	public var pEnabledFeatures : hl.Bytes;
	public function new() {
		sType = DEVICE_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<DeviceCreateInfo> {
		var arr = hl.CArray.alloc(DeviceCreateInfo, count);
		for( i in 0...count ) arr[i].sType = DEVICE_CREATE_INFO;
		return arr;
	}
}

@:struct class FixedBytes256 {
	var b0 : Int;
	var b1 : Int;
	var b2 : Int;
	var b3 : Int;
	var b4 : Int;
	var b5 : Int;
	var b6 : Int;
	var b7 : Int;
	var b8 : Int;
	var b9 : Int;
	var b10 : Int;
	var b11 : Int;
	var b12 : Int;
	var b13 : Int;
	var b14 : Int;
	var b15 : Int;
	var b16 : Int;
	var b17 : Int;
	var b18 : Int;
	var b19 : Int;
	var b20 : Int;
	var b21 : Int;
	var b22 : Int;
	var b23 : Int;
	var b24 : Int;
	var b25 : Int;
	var b26 : Int;
	var b27 : Int;
	var b28 : Int;
	var b29 : Int;
	var b30 : Int;
	var b31 : Int;
	var b32 : Int;
	var b33 : Int;
	var b34 : Int;
	var b35 : Int;
	var b36 : Int;
	var b37 : Int;
	var b38 : Int;
	var b39 : Int;
	var b40 : Int;
	var b41 : Int;
	var b42 : Int;
	var b43 : Int;
	var b44 : Int;
	var b45 : Int;
	var b46 : Int;
	var b47 : Int;
	var b48 : Int;
	var b49 : Int;
	var b50 : Int;
	var b51 : Int;
	var b52 : Int;
	var b53 : Int;
	var b54 : Int;
	var b55 : Int;
	var b56 : Int;
	var b57 : Int;
	var b58 : Int;
	var b59 : Int;
	var b60 : Int;
	var b61 : Int;
	var b62 : Int;
	var b63 : Int;
	public function new() {
	}
	public function toString() : String {
		return @:privateAccess String.fromUTF8(VkConst.pointer(this));
	}
	public inline function getI32( index : Int ) : Int {
		return VkConst.pointer(this).getI32(index << 2);
	}
}

@:struct class FixedBytes16 {
	var b0 : Int;
	var b1 : Int;
	var b2 : Int;
	var b3 : Int;
	public function new() {
	}
	public function toString() : String {
		return @:privateAccess String.fromUTF8(VkConst.pointer(this));
	}
	public inline function getI32( index : Int ) : Int {
		return VkConst.pointer(this).getI32(index << 2);
	}
}

@:struct class PhysicalDeviceLimits {
	public var maxImageDimension1D : Int;
	public var maxImageDimension2D : Int;
	public var maxImageDimension3D : Int;
	public var maxImageDimensionCube : Int;
	public var maxImageArrayLayers : Int;
	public var maxTexelBufferElements : Int;
	public var maxUniformBufferRange : Int;
	public var maxStorageBufferRange : Int;
	public var maxPushConstantsSize : Int;
	public var maxMemoryAllocationCount : Int;
	public var maxSamplerAllocationCount : Int;
	public var bufferImageGranularity : hl.I64;
	public var sparseAddressSpaceSize : hl.I64;
	public var maxBoundDescriptorSets : Int;
	public var maxPerStageDescriptorSamplers : Int;
	public var maxPerStageDescriptorUniformBuffers : Int;
	public var maxPerStageDescriptorStorageBuffers : Int;
	public var maxPerStageDescriptorSampledImages : Int;
	public var maxPerStageDescriptorStorageImages : Int;
	public var maxPerStageDescriptorInputAttachments : Int;
	public var maxPerStageResources : Int;
	public var maxDescriptorSetSamplers : Int;
	public var maxDescriptorSetUniformBuffers : Int;
	public var maxDescriptorSetUniformBuffersDynamic : Int;
	public var maxDescriptorSetStorageBuffers : Int;
	public var maxDescriptorSetStorageBuffersDynamic : Int;
	public var maxDescriptorSetSampledImages : Int;
	public var maxDescriptorSetStorageImages : Int;
	public var maxDescriptorSetInputAttachments : Int;
	public var maxVertexInputAttributes : Int;
	public var maxVertexInputBindings : Int;
	public var maxVertexInputAttributeOffset : Int;
	public var maxVertexInputBindingStride : Int;
	public var maxVertexOutputComponents : Int;
	public var maxTessellationGenerationLevel : Int;
	public var maxTessellationPatchSize : Int;
	public var maxTessellationControlPerVertexInputComponents : Int;
	public var maxTessellationControlPerVertexOutputComponents : Int;
	public var maxTessellationControlPerPatchOutputComponents : Int;
	public var maxTessellationControlTotalOutputComponents : Int;
	public var maxTessellationEvaluationInputComponents : Int;
	public var maxTessellationEvaluationOutputComponents : Int;
	public var maxGeometryShaderInvocations : Int;
	public var maxGeometryInputComponents : Int;
	public var maxGeometryOutputComponents : Int;
	public var maxGeometryOutputVertices : Int;
	public var maxGeometryTotalOutputComponents : Int;
	public var maxFragmentInputComponents : Int;
	public var maxFragmentOutputAttachments : Int;
	public var maxFragmentDualSrcAttachments : Int;
	public var maxFragmentCombinedOutputResources : Int;
	public var maxComputeSharedMemorySize : Int;
	public var maxComputeWorkGroupCount0 : Int;
	public var maxComputeWorkGroupCount1 : Int;
	public var maxComputeWorkGroupCount2 : Int;
	public var maxComputeWorkGroupInvocations : Int;
	public var maxComputeWorkGroupSize0 : Int;
	public var maxComputeWorkGroupSize1 : Int;
	public var maxComputeWorkGroupSize2 : Int;
	public var subPixelPrecisionBits : Int;
	public var subTexelPrecisionBits : Int;
	public var mipmapPrecisionBits : Int;
	public var maxDrawIndexedIndexValue : Int;
	public var maxDrawIndirectCount : Int;
	public var maxSamplerLodBias : Single;
	public var maxSamplerAnisotropy : Single;
	public var maxViewports : Int;
	public var maxViewportDimensions0 : Int;
	public var maxViewportDimensions1 : Int;
	public var viewportBoundsRange0 : Single;
	public var viewportBoundsRange1 : Single;
	public var viewportSubPixelBits : Int;
	public var minMemoryMapAlignment : hl.I64;
	public var minTexelBufferOffsetAlignment : hl.I64;
	public var minUniformBufferOffsetAlignment : hl.I64;
	public var minStorageBufferOffsetAlignment : hl.I64;
	public var minTexelOffset : Int;
	public var maxTexelOffset : Int;
	public var minTexelGatherOffset : Int;
	public var maxTexelGatherOffset : Int;
	public var minInterpolationOffset : Single;
	public var maxInterpolationOffset : Single;
	public var subPixelInterpolationOffsetBits : Int;
	public var maxFramebufferWidth : Int;
	public var maxFramebufferHeight : Int;
	public var maxFramebufferLayers : Int;
	public var framebufferColorSampleCounts : Int;
	public var framebufferDepthSampleCounts : Int;
	public var framebufferStencilSampleCounts : Int;
	public var framebufferNoAttachmentsSampleCounts : Int;
	public var maxColorAttachments : Int;
	public var sampledImageColorSampleCounts : Int;
	public var sampledImageIntegerSampleCounts : Int;
	public var sampledImageDepthSampleCounts : Int;
	public var sampledImageStencilSampleCounts : Int;
	public var storageImageSampleCounts : Int;
	public var maxSampleMaskWords : Int;
	public var timestampComputeAndGraphics : Int;
	public var timestampPeriod : Single;
	public var maxClipDistances : Int;
	public var maxCullDistances : Int;
	public var maxCombinedClipAndCullDistances : Int;
	public var discreteQueuePriorities : Int;
	public var pointSizeRange0 : Single;
	public var pointSizeRange1 : Single;
	public var lineWidthRange0 : Single;
	public var lineWidthRange1 : Single;
	public var pointSizeGranularity : Single;
	public var lineWidthGranularity : Single;
	public var strictLines : Int;
	public var standardSampleLocations : Int;
	public var optimalBufferCopyOffsetAlignment : hl.I64;
	public var optimalBufferCopyRowPitchAlignment : hl.I64;
	public var nonCoherentAtomSize : hl.I64;
	public function new() {
	}
}

@:struct class PhysicalDeviceSparseProperties {
	public var residencyStandard2DBlockShape : Int;
	public var residencyStandard2DMultisampleBlockShape : Int;
	public var residencyStandard3DBlockShape : Int;
	public var residencyAlignedMipSize : Int;
	public var residencyNonResidentStrict : Int;
	public function new() {
	}
}

@:struct class PhysicalDeviceProperties {
	public var apiVersion : Int;
	public var driverVersion : Int;
	public var vendorID : Int;
	public var deviceID : Int;
	public var deviceType : Int;
	@:packed public var deviceName(default, null) : FixedBytes256;
	@:packed public var pipelineCacheUUID(default, null) : FixedBytes16;
	@:packed public var limits(default, null) : PhysicalDeviceLimits;
	@:packed public var sparseProperties(default, null) : PhysicalDeviceSparseProperties;
	public function new() {
	}
}

@:struct class PhysicalDeviceProperties2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	@:packed public var properties(default, null) : PhysicalDeviceProperties;
	public function new() {
		sType = PHYSICAL_DEVICE_PROPERTIES_2;
	}
	public static function alloc( count : Int ) : hl.CArray<PhysicalDeviceProperties2> {
		var arr = hl.CArray.alloc(PhysicalDeviceProperties2, count);
		for( i in 0...count ) arr[i].sType = PHYSICAL_DEVICE_PROPERTIES_2;
		return arr;
	}
}

@:struct class PhysicalDeviceFeatures {
	public var robustBufferAccess : Int;
	public var fullDrawIndexUint32 : Int;
	public var imageCubeArray : Int;
	public var independentBlend : Int;
	public var geometryShader : Int;
	public var tessellationShader : Int;
	public var sampleRateShading : Int;
	public var dualSrcBlend : Int;
	public var logicOp : Int;
	public var multiDrawIndirect : Int;
	public var drawIndirectFirstInstance : Int;
	public var depthClamp : Int;
	public var depthBiasClamp : Int;
	public var fillModeNonSolid : Int;
	public var depthBounds : Int;
	public var wideLines : Int;
	public var largePoints : Int;
	public var alphaToOne : Int;
	public var multiViewport : Int;
	public var samplerAnisotropy : Int;
	public var textureCompressionETC2 : Int;
	public var textureCompressionASTC_LDR : Int;
	public var textureCompressionBC : Int;
	public var occlusionQueryPrecise : Int;
	public var pipelineStatisticsQuery : Int;
	public var vertexPipelineStoresAndAtomics : Int;
	public var fragmentStoresAndAtomics : Int;
	public var shaderTessellationAndGeometryPointSize : Int;
	public var shaderImageGatherExtended : Int;
	public var shaderStorageImageExtendedFormats : Int;
	public var shaderStorageImageMultisample : Int;
	public var shaderStorageImageReadWithoutFormat : Int;
	public var shaderStorageImageWriteWithoutFormat : Int;
	public var shaderUniformBufferArrayDynamicIndexing : Int;
	public var shaderSampledImageArrayDynamicIndexing : Int;
	public var shaderStorageBufferArrayDynamicIndexing : Int;
	public var shaderStorageImageArrayDynamicIndexing : Int;
	public var shaderClipDistance : Int;
	public var shaderCullDistance : Int;
	public var shaderFloat64 : Int;
	public var shaderInt64 : Int;
	public var shaderInt16 : Int;
	public var shaderResourceResidency : Int;
	public var shaderResourceMinLod : Int;
	public var sparseBinding : Int;
	public var sparseResidencyBuffer : Int;
	public var sparseResidencyImage2D : Int;
	public var sparseResidencyImage3D : Int;
	public var sparseResidency2Samples : Int;
	public var sparseResidency4Samples : Int;
	public var sparseResidency8Samples : Int;
	public var sparseResidency16Samples : Int;
	public var sparseResidencyAliased : Int;
	public var variableMultisampleRate : Int;
	public var inheritedQueries : Int;
	public function new() {
	}
}

@:struct class PhysicalDeviceFeatures2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	@:packed public var features(default, null) : PhysicalDeviceFeatures;
	public function new() {
		sType = PHYSICAL_DEVICE_FEATURES_2;
	}
	public static function alloc( count : Int ) : hl.CArray<PhysicalDeviceFeatures2> {
		var arr = hl.CArray.alloc(PhysicalDeviceFeatures2, count);
		for( i in 0...count ) arr[i].sType = PHYSICAL_DEVICE_FEATURES_2;
		return arr;
	}
}

@:struct class PhysicalDeviceVulkan11Features {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var storageBuffer16BitAccess : Int;
	public var uniformAndStorageBuffer16BitAccess : Int;
	public var storagePushConstant16 : Int;
	public var storageInputOutput16 : Int;
	public var multiview : Int;
	public var multiviewGeometryShader : Int;
	public var multiviewTessellationShader : Int;
	public var variablePointersStorageBuffer : Int;
	public var variablePointers : Int;
	public var protectedMemory : Int;
	public var samplerYcbcrConversion : Int;
	public var shaderDrawParameters : Int;
	public function new() {
		sType = PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	}
	public static function alloc( count : Int ) : hl.CArray<PhysicalDeviceVulkan11Features> {
		var arr = hl.CArray.alloc(PhysicalDeviceVulkan11Features, count);
		for( i in 0...count ) arr[i].sType = PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		return arr;
	}
}

@:struct class PhysicalDeviceVulkan12Features {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var samplerMirrorClampToEdge : Int;
	public var drawIndirectCount : Int;
	public var storageBuffer8BitAccess : Int;
	public var uniformAndStorageBuffer8BitAccess : Int;
	public var storagePushConstant8 : Int;
	public var shaderBufferInt64Atomics : Int;
	public var shaderSharedInt64Atomics : Int;
	public var shaderFloat16 : Int;
	public var shaderInt8 : Int;
	public var descriptorIndexing : Int;
	public var shaderInputAttachmentArrayDynamicIndexing : Int;
	public var shaderUniformTexelBufferArrayDynamicIndexing : Int;
	public var shaderStorageTexelBufferArrayDynamicIndexing : Int;
	public var shaderUniformBufferArrayNonUniformIndexing : Int;
	public var shaderSampledImageArrayNonUniformIndexing : Int;
	public var shaderStorageBufferArrayNonUniformIndexing : Int;
	public var shaderStorageImageArrayNonUniformIndexing : Int;
	public var shaderInputAttachmentArrayNonUniformIndexing : Int;
	public var shaderUniformTexelBufferArrayNonUniformIndexing : Int;
	public var shaderStorageTexelBufferArrayNonUniformIndexing : Int;
	public var descriptorBindingUniformBufferUpdateAfterBind : Int;
	public var descriptorBindingSampledImageUpdateAfterBind : Int;
	public var descriptorBindingStorageImageUpdateAfterBind : Int;
	public var descriptorBindingStorageBufferUpdateAfterBind : Int;
	public var descriptorBindingUniformTexelBufferUpdateAfterBind : Int;
	public var descriptorBindingStorageTexelBufferUpdateAfterBind : Int;
	public var descriptorBindingUpdateUnusedWhilePending : Int;
	public var descriptorBindingPartiallyBound : Int;
	public var descriptorBindingVariableDescriptorCount : Int;
	public var runtimeDescriptorArray : Int;
	public var samplerFilterMinmax : Int;
	public var scalarBlockLayout : Int;
	public var imagelessFramebuffer : Int;
	public var uniformBufferStandardLayout : Int;
	public var shaderSubgroupExtendedTypes : Int;
	public var separateDepthStencilLayouts : Int;
	public var hostQueryReset : Int;
	public var timelineSemaphore : Int;
	public var bufferDeviceAddress : Int;
	public var bufferDeviceAddressCaptureReplay : Int;
	public var bufferDeviceAddressMultiDevice : Int;
	public var vulkanMemoryModel : Int;
	public var vulkanMemoryModelDeviceScope : Int;
	public var vulkanMemoryModelAvailabilityVisibilityChains : Int;
	public var shaderOutputViewportIndex : Int;
	public var shaderOutputLayer : Int;
	public var subgroupBroadcastDynamicId : Int;
	public function new() {
		sType = PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	}
	public static function alloc( count : Int ) : hl.CArray<PhysicalDeviceVulkan12Features> {
		var arr = hl.CArray.alloc(PhysicalDeviceVulkan12Features, count);
		for( i in 0...count ) arr[i].sType = PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		return arr;
	}
}

@:struct class PhysicalDeviceVulkan13Features {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var robustImageAccess : Int;
	public var inlineUniformBlock : Int;
	public var descriptorBindingInlineUniformBlockUpdateAfterBind : Int;
	public var pipelineCreationCacheControl : Int;
	public var privateData : Int;
	public var shaderDemoteToHelperInvocation : Int;
	public var shaderTerminateInvocation : Int;
	public var subgroupSizeControl : Int;
	public var computeFullSubgroups : Int;
	public var synchronization2 : Int;
	public var textureCompressionASTC_HDR : Int;
	public var shaderZeroInitializeWorkgroupMemory : Int;
	public var dynamicRendering : Int;
	public var shaderIntegerDotProduct : Int;
	public var maintenance4 : Int;
	public function new() {
		sType = PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	}
	public static function alloc( count : Int ) : hl.CArray<PhysicalDeviceVulkan13Features> {
		var arr = hl.CArray.alloc(PhysicalDeviceVulkan13Features, count);
		for( i in 0...count ) arr[i].sType = PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		return arr;
	}
}

@:struct class ImageResolve2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	@:packed public var srcSubresource(default, null) : ImageSubresourceLayers;
	@:packed public var srcOffset(default, null) : Offset3D;
	@:packed public var dstSubresource(default, null) : ImageSubresourceLayers;
	@:packed public var dstOffset(default, null) : Offset3D;
	@:packed public var extent(default, null) : Offset3D;
	public function new() {
		sType = IMAGE_RESOLVE_2;
	}
	public static function alloc( count : Int ) : hl.CArray<ImageResolve2> {
		var arr = hl.CArray.alloc(ImageResolve2, count);
		for( i in 0...count ) arr[i].sType = IMAGE_RESOLVE_2;
		return arr;
	}
}

@:struct class ResolveImageInfo2 {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var srcImage : NativeImage;
	public var srcImageLayout : ImageLayout;
	public var dstImage : NativeImage;
	public var dstImageLayout : ImageLayout;
	public var regionCount : Int;
	public var pRegions : hl.CArray<ImageResolve2>;
	public function new() {
		sType = RESOLVE_IMAGE_INFO_2;
	}
	public static function alloc( count : Int ) : hl.CArray<ResolveImageInfo2> {
		var arr = hl.CArray.alloc(ResolveImageInfo2, count);
		for( i in 0...count ) arr[i].sType = RESOLVE_IMAGE_INFO_2;
		return arr;
	}
}

@:struct class CommandBufferInheritanceRenderingInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var viewMask : Int;
	public var colorAttachmentCount : Int;
	public var pColorAttachmentFormats : hl.Bytes;
	public var depthAttachmentFormat : Format;
	public var stencilAttachmentFormat : Format;
	public var rasterizationSamples : Int;
	public function new() {
		sType = COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<CommandBufferInheritanceRenderingInfo> {
		var arr = hl.CArray.alloc(CommandBufferInheritanceRenderingInfo, count);
		for( i in 0...count ) arr[i].sType = COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
		return arr;
	}
}

@:struct class CommandBufferInheritanceInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var renderPass : hl.Bytes;
	public var subpass : Int;
	public var framebuffer : hl.Bytes;
	public var occlusionQueryEnable : Int;
	public var queryFlags : Int;
	public var pipelineStatistics : Int;
	public function new() {
		sType = COMMAND_BUFFER_INHERITANCE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<CommandBufferInheritanceInfo> {
		var arr = hl.CArray.alloc(CommandBufferInheritanceInfo, count);
		for( i in 0...count ) arr[i].sType = COMMAND_BUFFER_INHERITANCE_INFO;
		return arr;
	}
}

@:struct class CommandBufferHandle {
	public var handle : CommandBuffer;
	public function new() {
	}
}

@:struct class SwapchainCreateInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var flags : Int;
	public var surface : Surface;
	public var minImageCount : Int;
	public var imageFormat : Format;
	public var imageColorSpace : ColorSpace;
	@:packed public var imageExtent(default, null) : Extent2D;
	public var imageArrayLayers : Int;
	public var imageUsage : Int;
	public var imageSharingMode : SharingMode;
	public var queueFamilyIndexCount : Int;
	public var pQueueFamilyIndices : hl.Bytes;
	public var preTransform : Int;
	public var compositeAlpha : CompositeAlpha;
	public var presentMode : PresentMode;
	public var clipped : Int;
	public var oldSwapchain : Swapchain;
	public function new() {
		sType = SWAPCHAIN_CREATE_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<SwapchainCreateInfo> {
		var arr = hl.CArray.alloc(SwapchainCreateInfo, count);
		for( i in 0...count ) arr[i].sType = SWAPCHAIN_CREATE_INFO;
		return arr;
	}
}

@:struct class Extent2D {
	public var width : Int;
	public var height : Int;
	public function new() {
	}
}

@:struct class NativeImageHandle {
	public var handle : NativeImage;
	public function new() {
	}
}

@:struct class BufferDeviceAddressInfo {
	var sType : StructureType;
	public var pNext : hl.Bytes;
	public var buffer : NativeBuffer;
	public function new() {
		sType = BUFFER_DEVICE_ADDRESS_INFO;
	}
	public static function alloc( count : Int ) : hl.CArray<BufferDeviceAddressInfo> {
		var arr = hl.CArray.alloc(BufferDeviceAddressInfo, count);
		for( i in 0...count ) arr[i].sType = BUFFER_DEVICE_ADDRESS_INFO;
		return arr;
	}
}

@:hlNative("vulkan")
class Vk {
	public static function getInstanceVersion() : Int { return 0; }

	@:hlNative("vulkan","enumerate_instance_extension_properties") public static function enumerateInstanceExtensionProperties( count : hl.Ref<Int>, props : hl.Bytes ) : Result { return cast 0; }
	@:hlNative("vulkan","enumerate_instance_layer_properties") public static function enumerateInstanceLayerProperties( count : hl.Ref<Int>, props : hl.Bytes ) : Result { return cast 0; }
	@:hlNative("vulkan","instance_create") public static function instanceCreate( info : InstanceCreateInfo ) : Result { return cast 0; }
	@:hlNative("vulkan","debug_messenger_create") public static function debugMessengerCreate( severityFlags : Int, typeFlags : Int ) : Result { return cast 0; }

	static inline var EXTENSION_PROPERTIES_SIZE = 260;
	static inline var LAYER_PROPERTIES_SIZE = 520;

	static function readNames( stride : Int, enumerate : (hl.Ref<Int>, hl.Bytes) -> Result ) : Array<String> {
		var count = 0;
		if( enumerate(count, null) != SUCCESS ) return [];
		var props = new hl.Bytes(count * stride);
		if( enumerate(count, props) != SUCCESS ) return [];
		return [for( i in 0...count ) @:privateAccess String.fromUTF8(props.offset(i * stride))];
	}

	public static function getInstanceExtensions() : Array<String> {
		return readNames(EXTENSION_PROPERTIES_SIZE, (count, props) -> enumerateInstanceExtensionProperties(count, props));
	}

	public static function getInstanceLayers() : Array<String> {
		return readNames(LAYER_PROPERTIES_SIZE, (count, props) -> enumerateInstanceLayerProperties(count, props));
	}

	public static function getDeviceExtensions( pdev : PhysicalDevice ) : Array<String> {
		return readNames(EXTENSION_PROPERTIES_SIZE, (count, props) -> enumerateDeviceExtensionProperties(pdev, count, props));
	}

	public static function makeNames( names : Array<String> ) : hl.CArray<NameHandle> {
		var arr = hl.CArray.alloc(NameHandle, names.length == 0 ? 1 : names.length);
		for( i in 0...names.length )
			arr[i].name = @:privateAccess names[i].toUtf8();
		return arr;
	}

	public static function getInstance() : Instance { return null; }
	public static function setInstanceProcAddr( proc : Proc ) : Bool { return false; }
	public static function setFunction( name : String, proc : Proc ) : Bool {
		return set_function(@:privateAccess name.toUtf8(), proc);
	}

	public static function getInstanceProcAddr() : Proc { return null; }
	public static function getDeviceProcAddr() : Proc { return null; }

	@:hlNative("vulkan","set_function") static function set_function( name : hl.Bytes, proc : Proc ) : Bool { return false; }

	#if hldx
	public static function surfaceCreateWin32( win : dx.Window ) : Bool {
		return surface_create_win32(@:privateAccess win.win);
	}
	@:hlNative("vulkan","surface_create_win32") static function surface_create_win32( win : hl.Abstract<"dx_window"> ) : Bool { return false; }
	#end

	#if hlnx
	public static function surfaceCreateVi( window : hl.Abstract<"nx_native_window"> ) : Bool { return false; }
	#end

	public static function setSurface( surface : Surface ) : Void {}
	public static function getSurface() : Surface { return null; }
	public static function surfaceDestroy() : Void {}
	@:hlNative("vulkan","surface_get_capabilities") public static function surfaceGetCapabilities( caps : SurfaceCaps ) : Result { return cast 0; }
	public static function surfaceGetFormats( out : hl.Bytes, max : Int ) : Int { return 0; }
	public static function surfaceGetPresentModes( out : hl.Bytes, max : Int ) : Int { return 0; }

	@:hlNative("vulkan","enumerate_physical_devices") public static function enumeratePhysicalDevices( count : hl.Ref<Int>, devices : hl.CArray<PhysicalDeviceHandle> ) : Result { return cast 0; }
	public static function getPhysicalDeviceProperties2( pdev : PhysicalDevice, props : PhysicalDeviceProperties2 ) : Void {}
	public static function getPhysicalDeviceFeatures2( pdev : PhysicalDevice, features : PhysicalDeviceFeatures2 ) : Void {}
	public static function getPhysicalDeviceQueueFamilyProperties( pdev : PhysicalDevice, count : hl.Ref<Int>, props : hl.CArray<QueueFamilyProperties> ) : Void {}
	public static function getPhysicalDeviceSurfaceSupport( pdev : PhysicalDevice, queueFamily : Int ) : Bool { return false; }
	public static function getPhysicalDeviceFormatProperties( pdev : PhysicalDevice, format : Format, props : FormatProperties ) : Void {}
	@:hlNative("vulkan","enumerate_device_extension_properties") public static function enumerateDeviceExtensionProperties( pdev : PhysicalDevice, count : hl.Ref<Int>, props : hl.Bytes ) : Result { return cast 0; }
	@:hlNative("vulkan","device_create") public static function deviceCreate( pdev : PhysicalDevice, info : DeviceCreateInfo ) : Result { return cast 0; }
	public static function getDeviceQueue( queueFamily : Int, index : Int ) : Queue { return null; }
	@:hlNative("vulkan","allocator_create") public static function allocatorCreate( flags : Int, vulkanApiVersion : Int ) : Result { return cast 0; }
	@:hlNative("vulkan","device_wait_idle") public static function deviceWaitIdle() : Result { return cast 0; }

	public static function getPhysicalDevices() : Array<PhysicalDevice> {
		var count = 0;
		if( enumeratePhysicalDevices(count, null) != SUCCESS || count == 0 ) return [];
		var arr = hl.CArray.alloc(PhysicalDeviceHandle, count);
		if( enumeratePhysicalDevices(count, arr) != SUCCESS ) return [];
		return [for( i in 0...count ) arr[i].handle];
	}

	public static function getPhysicalDevice() : PhysicalDevice { return null; }
	public static function getDevice() : Device { return null; }

	public static function getDeviceName() : String {
		var props = new PhysicalDeviceProperties2();
		getPhysicalDeviceProperties2(getPhysicalDevice(), props);
		return props.properties.deviceName.toString();
	}

	public static function getMemoryProperties( out : hl.Bytes ) : Void {}
	public static function getHeapBudgets( out : hl.Bytes ) : Void {}

	public static function swapchainCreate( info : SwapchainCreateInfo, result : hl.Ref<Result> ) : Swapchain { return null; }
	public static function swapchainDestroy( sc : Swapchain ) : Void {}
	@:hlNative("vulkan","get_swapchain_images") public static function getSwapchainImages( sc : Swapchain, count : hl.Ref<Int>, images : hl.CArray<NativeImageHandle> ) : Result { return cast 0; }
	@:hlNative("vulkan","acquire_next_image") public static function acquireNextImage( swapchain : Swapchain, timeout : hl.I64, sem : Semaphore, fence : Fence, index : hl.Ref<Int> ) : Result { return cast 0; }
	@:hlNative("vulkan","queue_present") public static function queuePresent( queue : Queue, info : PresentInfo ) : Result { return cast 0; }

	public static function fenceCreate( flags : Int, result : hl.Ref<Result> ) : Fence { return null; }
	public static function fenceDestroy( f : Fence ) : Void {}
	@:hlNative("vulkan","wait_for_fences") public static function waitForFences( count : Int, fences : hl.Ref<Fence>, waitAll : Bool, timeout : hl.I64 ) : Result { return cast 0; }
	@:hlNative("vulkan","get_fence_status") public static function getFenceStatus( f : Fence ) : Result { return cast 0; }
	@:hlNative("vulkan","reset_fences") public static function resetFences( count : Int, fences : hl.Ref<Fence> ) : Result { return cast 0; }
	public static function semaphoreCreate( result : hl.Ref<Result> ) : Semaphore { return null; }
	public static function semaphoreDestroy( s : Semaphore ) : Void {}
	@:hlNative("vulkan","queue_submit2") public static function queueSubmit2( queue : Queue, count : Int, submits : hl.CArray<SubmitInfo2>, fence : Fence ) : Result { return cast 0; }

	public static function commandPoolCreate( flags : Int, queueFamily : Int, result : hl.Ref<Result> ) : CommandPool { return null; }
	public static function commandPoolDestroy( p : CommandPool ) : Void {}
	@:hlNative("vulkan","command_pool_reset") public static function commandPoolReset( p : CommandPool, flags : Int ) : Result { return cast 0; }
	public static function commandBufferCreate( p : CommandPool, level : CommandBufferLevel, result : hl.Ref<Result> ) : CommandBuffer { return null; }
	@:hlNative("vulkan","begin_command_buffer") public static function beginCommandBuffer( cmd : CommandBuffer, info : CommandBufferBeginInfo ) : Result { return cast 0; }
	@:hlNative("vulkan","end_command_buffer") public static function endCommandBuffer( cmd : CommandBuffer ) : Result { return cast 0; }

	public static function bufferCreate( info : BufferCreateInfo, alloc : AllocationCreateInfo, result : hl.Ref<Result> ) : Buffer { return null; }
	public static function bufferDestroy( b : Buffer ) : Void {}
	public static function bufferGetMapped( b : Buffer ) : hl.Bytes { return null; }
	public static function bufferGetHandle( b : Buffer ) : NativeBuffer { return null; }
	public static inline function bufferSetName( b : Buffer, name : String ) {
		buffer_set_name(b, @:privateAccess name.toUtf8());
	}
	@:hlNative("vulkan","buffer_set_name") static function buffer_set_name( b : Buffer, name : hl.Bytes ) : Void {}

	public static function imageCreate( info : ImageCreateInfo, alloc : AllocationCreateInfo, result : hl.Ref<Result> ) : Image { return null; }
	public static function imageDestroy( i : Image ) : Void {}
	public static function imageGetHandle( i : Image ) : NativeImage { return null; }
	public static inline function imageSetName( i : Image, name : String ) {
		image_set_name(i, @:privateAccess name.toUtf8());
	}
	@:hlNative("vulkan","image_set_name") static function image_set_name( i : Image, name : hl.Bytes ) : Void {}
	public static function imageViewCreate( info : ImageViewCreateInfo, result : hl.Ref<Result> ) : ImageView { return null; }
	public static function imageViewDestroy( v : ImageView ) : Void {}
	public static function samplerCreate( info : SamplerCreateInfo, result : hl.Ref<Result> ) : Sampler { return null; }
	public static function samplerDestroy( s : Sampler ) : Void {}

	public static function shaderModuleCreate( info : ShaderModuleCreateInfo, result : hl.Ref<Result> ) : ShaderModule { return null; }
	public static function shaderModuleDestroy( m : ShaderModule ) : Void {}
	public static function descriptorSetLayoutCreate( info : DescriptorSetLayoutCreateInfo, result : hl.Ref<Result> ) : DescriptorSetLayout { return null; }
	public static function descriptorSetLayoutDestroy( l : DescriptorSetLayout ) : Void {}
	public static function pipelineLayoutCreate( info : PipelineLayoutCreateInfo, result : hl.Ref<Result> ) : PipelineLayout { return null; }
	public static function pipelineLayoutDestroy( l : PipelineLayout ) : Void {}
	public static function pipelineCacheCreate( info : PipelineCacheCreateInfo, result : hl.Ref<Result> ) : PipelineCache { return null; }
	public static function pipelineCacheDestroy( c : PipelineCache ) : Void {}
	@:hlNative("vulkan","get_pipeline_cache_data") public static function getPipelineCacheData( c : PipelineCache, size : hl.Ref<Int>, data : hl.Bytes ) : Result { return cast 0; }
	@:hlNative("vulkan","create_graphics_pipelines") public static function createGraphicsPipelines( cache : PipelineCache, count : Int, infos : hl.CArray<GraphicsPipelineCreateInfo>, pipelines : hl.CArray<PipelineHandle> ) : Result { return cast 0; }
	@:hlNative("vulkan","create_compute_pipelines") public static function createComputePipelines( cache : PipelineCache, count : Int, infos : hl.CArray<ComputePipelineCreateInfo>, pipelines : hl.CArray<PipelineHandle> ) : Result { return cast 0; }
	public static function pipelineDestroy( p : Pipeline ) : Void {}

	public static function descriptorPoolCreate( info : DescriptorPoolCreateInfo, result : hl.Ref<Result> ) : DescriptorPool { return null; }
	public static function descriptorPoolDestroy( p : DescriptorPool ) : Void {}
	@:hlNative("vulkan","descriptor_pool_reset") public static function descriptorPoolReset( p : DescriptorPool, flags : Int ) : Result { return cast 0; }
	@:hlNative("vulkan","allocate_descriptor_sets") public static function allocateDescriptorSets( info : DescriptorSetAllocateInfo, sets : hl.CArray<DescriptorSetHandle> ) : Result { return cast 0; }
	@:hlNative("vulkan","free_descriptor_sets") public static function freeDescriptorSets( p : DescriptorPool, count : Int, sets : hl.CArray<DescriptorSetHandle> ) : Result { return cast 0; }
	public static function updateDescriptorSets( writeCount : Int, writes : hl.CArray<WriteDescriptorSet>, copyCount : Int, copies : hl.CArray<WriteDescriptorSet> ) : Void {}
	public static function cmdBindDescriptorSets( cmd : CommandBuffer, bindPoint : PipelineBindPoint, layout : PipelineLayout, firstSet : Int, setCount : Int, sets : hl.CArray<DescriptorSetHandle>, dynamicOffsetCount : Int, dynamicOffsets : hl.Bytes ) : Void {}
	public static function cmdPushConstants( cmd : CommandBuffer, layout : PipelineLayout, stageFlags : Int, offset : Int, size : Int, values : hl.Bytes ) : Void {}

	public static function cmdPipelineBarrier2( cmd : CommandBuffer, dep : DependencyInfo ) : Void {}
	public static function cmdBeginRendering( cmd : CommandBuffer, info : RenderingInfo ) : Void {}
	public static function cmdEndRendering( cmd : CommandBuffer ) : Void {}
	public static function cmdClearAttachments( cmd : CommandBuffer, count : Int, attachments : hl.CArray<ClearAttachment>, rectCount : Int, rects : hl.CArray<ClearRect> ) : Void {}
	public static function cmdSetViewport( cmd : CommandBuffer, first : Int, count : Int, viewports : hl.CArray<Viewport> ) : Void {}
	public static function cmdSetScissor( cmd : CommandBuffer, first : Int, count : Int, scissors : hl.CArray<Rect2D> ) : Void {}
	public static function cmdSetStencilReference( cmd : CommandBuffer, faceMask : StencilFace, ref : Int ) : Void {}
	public static function cmdBindPipeline( cmd : CommandBuffer, bindPoint : PipelineBindPoint, p : Pipeline ) : Void {}
	public static function cmdBindVertexBuffers2( cmd : CommandBuffer, first : Int, count : Int, buffers : hl.CArray<BufferHandle>, offsets : hl.CArray<DeviceSize>, sizes : hl.CArray<DeviceSize>, strides : hl.CArray<DeviceSize> ) : Void {}
	public static function cmdBindIndexBuffer( cmd : CommandBuffer, b : NativeBuffer, offset : hl.I64, indexType : IndexType ) : Void {}
	public static function cmdDrawIndexed( cmd : CommandBuffer, indexCount : Int, instanceCount : Int, firstIndex : Int, vertexOffset : Int, firstInstance : Int ) : Void {}
	public static function cmdDrawIndexedIndirect( cmd : CommandBuffer, b : NativeBuffer, offset : hl.I64, drawCount : Int, stride : Int ) : Void {}
	public static function cmdDrawIndexedIndirectCount( cmd : CommandBuffer, b : NativeBuffer, offset : hl.I64, count : NativeBuffer, countOffset : hl.I64, maxDrawCount : Int, stride : Int ) : Void {}
	public static function cmdDispatch( cmd : CommandBuffer, x : Int, y : Int, z : Int ) : Void {}
	public static function cmdCopyBuffer2( cmd : CommandBuffer, info : CopyBufferInfo2 ) : Void {}
	public static function cmdCopyImage2( cmd : CommandBuffer, info : CopyImageInfo2 ) : Void {}
	public static function cmdCopyBufferToImage2( cmd : CommandBuffer, info : CopyBufferToImageInfo2 ) : Void {}
	public static function cmdCopyImageToBuffer2( cmd : CommandBuffer, info : CopyImageToBufferInfo2 ) : Void {}
	public static function cmdBlitImage2( cmd : CommandBuffer, info : BlitImageInfo2 ) : Void {}
	public static function cmdFillBuffer( cmd : CommandBuffer, b : NativeBuffer, offset : hl.I64, size : hl.I64, data : Int ) : Void {}
	public static inline function cmdBeginLabel( cmd : CommandBuffer, name : String ) {
		cmd_begin_label(cmd, @:privateAccess name.toUtf8());
	}
	@:hlNative("vulkan","cmd_begin_label") static function cmd_begin_label( cmd : CommandBuffer, name : hl.Bytes ) : Void {}
	public static function cmdEndLabel( cmd : CommandBuffer ) : Void {}

	public static function cmdDraw( cmd : CommandBuffer, vertexCount : Int, instanceCount : Int, firstVertex : Int, firstInstance : Int ) : Void {}
	public static function cmdDrawIndirect( cmd : CommandBuffer, b : NativeBuffer, offset : hl.I64, drawCount : Int, stride : Int ) : Void {}
	public static function cmdDrawIndirectCount( cmd : CommandBuffer, b : NativeBuffer, offset : hl.I64, count : NativeBuffer, countOffset : hl.I64, maxDrawCount : Int, stride : Int ) : Void {}
	public static function cmdDispatchIndirect( cmd : CommandBuffer, b : NativeBuffer, offset : hl.I64 ) : Void {}
	public static function cmdResolveImage2( cmd : CommandBuffer, info : ResolveImageInfo2 ) : Void {}
	public static function cmdClearColorImage( cmd : CommandBuffer, image : NativeImage, layout : ImageLayout, color : ClearValue, rangeCount : Int, ranges : hl.CArray<ImageSubresourceRange> ) : Void {}
	public static function cmdClearDepthStencilImage( cmd : CommandBuffer, image : NativeImage, layout : ImageLayout, value : ClearValue, rangeCount : Int, ranges : hl.CArray<ImageSubresourceRange> ) : Void {}
	public static function cmdUpdateBuffer( cmd : CommandBuffer, b : NativeBuffer, offset : hl.I64, size : hl.I64, data : hl.Bytes ) : Void {}
	public static function cmdSetDepthBias( cmd : CommandBuffer, constant : Float, clamp : Float, slope : Float ) : Void {}
	public static function cmdSetBlendConstants( cmd : CommandBuffer, constants : hl.Bytes ) : Void {}
	public static function cmdSetCullMode( cmd : CommandBuffer, mode : Int ) : Void {}
	public static function cmdSetFrontFace( cmd : CommandBuffer, face : Int ) : Void {}
	public static function cmdSetPrimitiveTopology( cmd : CommandBuffer, topology : Int ) : Void {}
	public static function cmdSetDepthTestEnable( cmd : CommandBuffer, enable : Bool ) : Void {}
	public static function cmdSetDepthWriteEnable( cmd : CommandBuffer, enable : Bool ) : Void {}
	public static function cmdSetDepthCompareOp( cmd : CommandBuffer, op : Int ) : Void {}
	public static function cmdSetDepthBiasEnable( cmd : CommandBuffer, enable : Bool ) : Void {}
	public static function cmdSetDepthBoundsTestEnable( cmd : CommandBuffer, enable : Bool ) : Void {}
	public static function cmdSetDepthBounds( cmd : CommandBuffer, minBounds : Float, maxBounds : Float ) : Void {}
	public static function cmdSetStencilTestEnable( cmd : CommandBuffer, enable : Bool ) : Void {}
	public static function cmdSetStencilOp( cmd : CommandBuffer, faceMask : StencilFace, failOp : Int, passOp : Int, depthFailOp : Int, compareOp : Int ) : Void {}
	public static function cmdSetStencilCompareMask( cmd : CommandBuffer, faceMask : StencilFace, mask : Int ) : Void {}
	public static function cmdSetStencilWriteMask( cmd : CommandBuffer, faceMask : StencilFace, mask : Int ) : Void {}
	public static function cmdSetLineWidth( cmd : CommandBuffer, width : Float ) : Void {}
	public static function cmdSetRasterizerDiscardEnable( cmd : CommandBuffer, enable : Bool ) : Void {}
	public static function cmdSetPrimitiveRestartEnable( cmd : CommandBuffer, enable : Bool ) : Void {}
	public static function getBufferDeviceAddress( info : BufferDeviceAddressInfo ) : hl.I64 { return 0; }
	public static function cmdExecuteCommands( cmd : CommandBuffer, count : Int, commands : hl.CArray<CommandBufferHandle> ) : Void {}
	@:hlNative("vulkan","reset_command_buffer") public static function resetCommandBuffer( cmd : CommandBuffer, flags : Int ) : Result { return cast 0; }
	public static function freeCommandBuffers( pool : CommandPool, count : Int, commands : hl.CArray<CommandBufferHandle> ) : Void {}
	@:hlNative("vulkan","queue_wait_idle") public static function queueWaitIdle( queue : Queue ) : Result { return cast 0; }

	public static function queryPoolCreate( queryType : QueryType, count : Int, pipelineStatistics : Int, result : hl.Ref<Result> ) : QueryPool { return null; }
	public static function queryPoolDestroy( p : QueryPool ) : Void {}
	public static function cmdResetQueryPool( cmd : CommandBuffer, p : QueryPool, first : Int, count : Int ) : Void {}
	public static function cmdWriteTimestamp2( cmd : CommandBuffer, stage : hl.I64, p : QueryPool, index : Int ) : Void {}
	public static function cmdBeginQuery( cmd : CommandBuffer, p : QueryPool, query : Int, flags : Int ) : Void {}
	public static function cmdEndQuery( cmd : CommandBuffer, p : QueryPool, query : Int ) : Void {}
	public static function cmdCopyQueryPoolResults( cmd : CommandBuffer, p : QueryPool, first : Int, count : Int, dst : NativeBuffer, dstOffset : hl.I64, stride : hl.I64, flags : Int ) : Void {}
	public static function resetQueryPool( p : QueryPool, first : Int, count : Int ) : Void {}
	@:hlNative("vulkan","get_query_pool_results") public static function getQueryPoolResults( p : QueryPool, first : Int, count : Int, dataSize : Int, data : hl.Bytes, stride : hl.I64, flags : Int ) : Result { return cast 0; }
}
