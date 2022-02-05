#define HL_NAME(n) sdl_##n
#include <hl.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <shaderc/shaderc.h>

static VkInstance instance = NULL;

VkInstance vk_get_instance() {
	return instance;
}

HL_PRIM bool HL_NAME(vk_init)( bool enable_validation ) {
	if( instance )
		return true;
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "HashLink Vulkan",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Heaps.io",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};
	VkInstanceCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = enable_validation ? 1 : 0,
		.enabledExtensionCount = enable_validation ? 3 : 2,
		.ppEnabledLayerNames = (const char* const[]) { "VK_LAYER_KHRONOS_validation" },
		.ppEnabledExtensionNames = (const char* const[]) { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME },
	};
	
	return vkCreateInstance(&info, NULL, &instance) == VK_SUCCESS;
}

vbyte *HL_NAME(vk_make_ref)( vdynamic *v ) {
	if( v->t->kind != HSTRUCT ) hl_error("assert");
	return v->v.ptr;
}

vbyte *HL_NAME(vk_make_array)( varray *a ) {
	if( a->size == 0 )
		return NULL;
	if( a->at->kind == HABSTRACT )
		return hl_copy_bytes(hl_aptr(a,vbyte), a->size * sizeof(void*));
	if( a->at->kind == HI32 )
		return hl_copy_bytes(hl_aptr(a,vbyte), a->size * sizeof(int));
#ifdef HL_DEBUG
	if( a->at->kind != HSTRUCT ) hl_error("assert");
#endif
	int size = a->at->obj->rt->size;
	vbyte *ptr = hl_alloc_bytes(size * a->size);
	int i;
	for(i=0;i<a->size;i++)
		memcpy(ptr + i * size, hl_aptr(a,vbyte*)[i], size);
	return ptr;
}

// ------------------------------------------ CONTEXT INIT --------------------------------------------

typedef struct _VkContext {
	VkSurfaceKHR surface;
	VkPhysicalDevice pdevice;
	VkPhysicalDeviceMemoryProperties memProps;
	VkDevice device;
	VkQueue queue;
	VkSwapchainKHR swapchain;
} *VkContext;

VkContext HL_NAME(vk_init_context)( VkSurfaceKHR surface, int *outQueue ) {
	VkContext ctx = (VkContext)malloc(sizeof(struct _VkContext));
	memset(ctx,0,sizeof(struct _VkContext));
	ctx->surface = surface;

	int queueFamily = 0;
	int physicalDeviceCount;
#	define MAX_DEVICE_COUNT 16
#	define MAX_QUEUE_COUNT 16
	VkPhysicalDevice deviceHandles[MAX_DEVICE_COUNT];
	VkQueueFamilyProperties queueFamilyProperties[MAX_QUEUE_COUNT];
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, 0);
	physicalDeviceCount = physicalDeviceCount > MAX_DEVICE_COUNT ? MAX_DEVICE_COUNT : physicalDeviceCount;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, deviceHandles);

	for(int i=0; i<physicalDeviceCount; i++) {
		int queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(deviceHandles[i], &queueFamilyCount, NULL);
		queueFamilyCount = queueFamilyCount > MAX_QUEUE_COUNT ? MAX_QUEUE_COUNT : queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(deviceHandles[i], &queueFamilyCount, queueFamilyProperties);
		vkGetPhysicalDeviceProperties(deviceHandles[i], &deviceProperties);
		vkGetPhysicalDeviceFeatures(deviceHandles[i], &deviceFeatures);
		for(int j=0; j<queueFamilyCount; j++) {
			VkBool32 supportsPresent = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(deviceHandles[i], j, ctx->surface, &supportsPresent);
			if (supportsPresent && (queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				queueFamily = j;
				ctx->pdevice = deviceHandles[i];
				break;
			}
		}
		if( ctx->pdevice ) break;
	}

	VkDeviceQueueCreateInfo qinf = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamily,
		.queueCount = 1,
		.pQueuePriorities = (const float[]) { 1.0f }
	};
	VkDeviceCreateInfo dinf = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = 0,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = 0,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = (const char* const[]) { VK_KHR_SWAPCHAIN_EXTENSION_NAME },
		.pEnabledFeatures = 0,
		.pQueueCreateInfos = &qinf,
	};

	if( vkCreateDevice(ctx->pdevice, &dinf, NULL, &ctx->device) != VK_SUCCESS )
		return NULL;

	vkGetDeviceQueue(ctx->device, queueFamily, 0, &ctx->queue);
	vkGetPhysicalDeviceMemoryProperties(ctx->pdevice, &ctx->memProps);
	*outQueue = queueFamily;
	return ctx;
}

int HL_NAME(vk_find_memory_type)( VkContext ctx, int allowed, int req ) {
	unsigned int i;
    for(i=0;i<ctx->memProps.memoryTypeCount;i++) {
		if( (allowed & (1<< i)) && (ctx->memProps.memoryTypes[i].propertyFlags & req) == req )
			return i;
    }
    return -1;
}

bool HL_NAME(vk_init_swapchain)( VkContext ctx, int width, int height, varray *outImages, VkFormat *outFormat ) {

	vkDeviceWaitIdle(ctx->device);

	if( ctx->swapchain ) {
		vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
		ctx->swapchain = NULL;
	}

	int formatCount = 1;
	VkSurfaceFormatKHR format;
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->pdevice, ctx->surface, &formatCount, 0);
	formatCount = 1;
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->pdevice, ctx->surface, &formatCount, &format);
	format.format = format.format == VK_FORMAT_UNDEFINED ? VK_FORMAT_B8G8R8A8_UNORM : format.format;

	int modeCount = 0;
#	define MAX_PRESENT_MODE_COUNT 16
	VkPresentModeKHR modes[MAX_PRESENT_MODE_COUNT];

	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->pdevice, ctx->surface, &modeCount, NULL);
	modeCount = modeCount > MAX_PRESENT_MODE_COUNT ? MAX_PRESENT_MODE_COUNT : modeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->pdevice, ctx->surface, &modeCount, modes);

	VkSurfaceCapabilitiesKHR scaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->pdevice, ctx->surface, &scaps);

	VkExtent2D swapchainExtent = scaps.currentExtent;
#	define clamp(v,min,max) ( ((v) < (min)) ? (min) : ((v) < (max)) ? (max) : (v) )
	if( swapchainExtent.width == UINT32_MAX ) {
		swapchainExtent.width = clamp((unsigned)width, scaps.minImageExtent.width, scaps.maxImageExtent.width);
		swapchainExtent.height = clamp((unsigned)height, scaps.minImageExtent.height, scaps.maxImageExtent.height);
	}

	VkSwapchainCreateInfoKHR swapInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = ctx->surface,
		.minImageCount = outImages->size,
		.imageFormat = format.format,
		.imageColorSpace = format.colorSpace,
		.imageExtent = swapchainExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = scaps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR, // always supported
		.clipped = VK_TRUE,
	};

	if( vkCreateSwapchainKHR(ctx->device, &swapInfo, 0, &ctx->swapchain) != VK_SUCCESS )
		return false;

	vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &outImages->size, NULL);
	vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &outImages->size, hl_aptr(outImages,void));
	*outFormat = format.format;

	return true;
}

VkShaderModule HL_NAME(vk_create_shader_module)( VkContext ctx, vbyte *data, int len ) {
	VkShaderModule module = NULL;
	VkShaderModuleCreateInfo inf = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = len,
		.pCode = (const uint32_t*)data,
	};
	vkCreateShaderModule(ctx->device,&inf,NULL,&module);
	return module;
}

VkPipelineLayout HL_NAME(vk_create_pipeline_layout)( VkContext ctx, VkPipelineLayoutCreateInfo *info ) {
	VkPipelineLayout p = NULL;
	vkCreatePipelineLayout(ctx->device, info, NULL, &p);
	return p;
}

VkPipeline HL_NAME(vk_create_graphics_pipeline)( VkContext ctx, VkGraphicsPipelineCreateInfo *info ) {
	VkPipeline p = NULL;
	vkCreateGraphicsPipelines(ctx->device, NULL, 1, info, NULL, &p);
	return p;
}

VkRenderPass HL_NAME(vk_create_render_pass)( VkContext ctx, VkRenderPassCreateInfo *info ) {
	VkRenderPass p = NULL;
	vkCreateRenderPass(ctx->device, info, NULL, &p);
	return p;
}

VkImageView HL_NAME(vk_create_image_view)( VkContext ctx, VkImageViewCreateInfo *info ) {
	VkImageView i = NULL;
	vkCreateImageView(ctx->device, info, NULL, &i);
	return i;
}

VkFramebuffer HL_NAME(vk_create_framebuffer)( VkContext ctx, VkFramebufferCreateInfo *info ) {
	VkFramebuffer b = NULL;
	vkCreateFramebuffer(ctx->device, info, NULL, &b);
	return b;
}

VkDescriptorSetLayout HL_NAME(vk_create_descriptor_set_layout)( VkContext ctx, VkDescriptorSetLayoutCreateInfo *info ) {
	VkDescriptorSetLayout p = NULL;
	vkCreateDescriptorSetLayout(ctx->device, info, NULL, &p);
	return p;
}

VkBuffer HL_NAME(vk_create_buffer)( VkContext ctx, VkBufferCreateInfo *info ) {
	VkBuffer b = NULL;
	vkCreateBuffer(ctx->device, info, NULL, &b);
	return b;
}

void HL_NAME(vk_get_buffer_memory_requirements)( VkContext ctx, VkBuffer buf, VkMemoryRequirements *info ) {
	vkGetBufferMemoryRequirements(ctx->device,buf,info);
}

VkDeviceMemory HL_NAME(vk_allocate_memory)( VkContext ctx, VkMemoryAllocateInfo *inf ) {
	VkDeviceMemory m = NULL;
	vkAllocateMemory(ctx->device, inf, NULL, &m);
	return m;
}

vbyte *HL_NAME(vk_map_memory)( VkContext ctx, VkDeviceMemory mem, int offset, int size, int flags ) {
	void *ptr = NULL;
	vkMapMemory(ctx->device, mem, offset, size, flags, &ptr);
	return ptr;
}

void HL_NAME(vk_unmap_memory)( VkContext ctx, VkDeviceMemory mem ) {
	vkUnmapMemory(ctx->device, mem);
}

bool HL_NAME(vk_bind_buffer_memory)( VkContext ctx, VkBuffer buf, VkDeviceMemory mem, int offset ) {
	return vkBindBufferMemory(ctx->device, buf, mem, offset) == VK_SUCCESS;
}

VkCommandPool HL_NAME(vk_create_command_pool)( VkContext ctx, VkCommandPoolCreateInfo *inf ) {
	VkCommandPool pool = NULL;
	vkCreateCommandPool(ctx->device,inf,NULL,&pool);
	return pool;
}

bool HL_NAME(vk_allocate_command_buffers)( VkContext ctx, VkCommandBufferAllocateInfo *inf, varray *buffers ) {
	return vkAllocateCommandBuffers(ctx->device, inf, hl_aptr(buffers,void)) == VK_SUCCESS;
}

VkFence HL_NAME(vk_create_fence)( VkContext ctx, VkFenceCreateInfo *inf ) {
	VkFence fence = NULL;
	vkCreateFence(ctx->device,inf,NULL,&fence);
	return fence;
}

VkSemaphore HL_NAME(vk_create_semaphore)( VkContext ctx, VkSemaphoreCreateInfo *inf ) {
	VkSemaphore s = NULL;
	vkCreateSemaphore(ctx->device,inf,NULL,&s);
	return s;
}

void HL_NAME(vk_reset_fence)( VkContext ctx, VkFence f ) {
	vkResetFences(ctx->device,1,&f);
}

bool HL_NAME(vk_wait_for_fence)( VkContext ctx, VkFence f, double timeout ) {
	uint64_t t = (uint64_t)timeout;
	return vkWaitForFences(ctx->device, 1, &f, TRUE, t) == VK_SUCCESS;
}

int HL_NAME(vk_get_next_image_index)( VkContext ctx, VkSemaphore lock ) {
	int image = -1;
	if( vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, lock, VK_NULL_HANDLE, &image) != VK_SUCCESS )
		return -1;
	return image;
}

void HL_NAME(vk_queue_submit)( VkContext ctx, VkSubmitInfo *inf, VkFence fence ) {
	vkQueueSubmit(ctx->queue, 1, inf, fence);
}

void HL_NAME(vk_present)( VkContext ctx, VkSemaphore sem, int image ) {
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &sem,
		.swapchainCount = 1,
		.pSwapchains = &ctx->swapchain,
		.pImageIndices = &image,
	};
	vkQueuePresentKHR(ctx->queue, &presentInfo);
}

#define _VCTX _ABSTRACT(vk_context)
#define _SHADER_MODULE _ABSTRACT(vk_shader_module)
#define _GPIPELINE _ABSTRACT(vk_gpipeline)
#define _PIPELAYOUT _ABSTRACT(vk_pipeline_layout)
#define _RENDERPASS _ABSTRACT(vk_render_pass)
#define _IMAGE _ABSTRACT(vk_image)
#define _IMAGE_VIEW _ABSTRACT(vk_image_view)
#define _FRAMEBUFFER _ABSTRACT(vk_framebuffer)
#define _DESCRIPTOR_SET _ABSTRACT(vk_descriptor_set)
#define _BUFFER _ABSTRACT(vk_buffer)
#define _MEMORY _ABSTRACT(vk_device_memory)
#define _CMD _ABSTRACT(vk_command_buffer)
#define _CMD_POOL _ABSTRACT(vk_command_pool)
#define _FENCE _ABSTRACT(vk_fence)
#define _SEMAPHORE _ABSTRACT(vk_semaphore)

DEFINE_PRIM(_BOOL, vk_init, _BOOL);
DEFINE_PRIM(_VCTX, vk_init_context, _BYTES _REF(_I32));
DEFINE_PRIM(_BOOL, vk_init_swapchain, _VCTX _I32 _I32 _ARR _REF(_I32));
DEFINE_PRIM(_BYTES, vk_make_array, _ARR);
DEFINE_PRIM(_BYTES, vk_make_ref, _DYN);
DEFINE_PRIM(_I32, vk_find_memory_type, _VCTX _I32 _I32);
DEFINE_PRIM(_SHADER_MODULE, vk_create_shader_module, _VCTX _BYTES _I32 );
DEFINE_PRIM(_GPIPELINE, vk_create_graphics_pipeline, _VCTX _STRUCT);
DEFINE_PRIM(_PIPELAYOUT, vk_create_pipeline_layout, _VCTX _STRUCT);
DEFINE_PRIM(_RENDERPASS, vk_create_render_pass, _VCTX _STRUCT);
DEFINE_PRIM(_IMAGE_VIEW, vk_create_image_view, _VCTX _STRUCT);
DEFINE_PRIM(_FENCE, vk_create_fence, _VCTX _STRUCT);
DEFINE_PRIM(_BOOL, vk_wait_for_fence, _VCTX _FENCE _F64);
DEFINE_PRIM(_VOID, vk_reset_fence, _VCTX _FENCE);
DEFINE_PRIM(_CMD_POOL, vk_create_command_pool, _VCTX _STRUCT);
DEFINE_PRIM(_BOOL, vk_allocate_command_buffers, _VCTX _STRUCT _ARR);
DEFINE_PRIM(_FRAMEBUFFER, vk_create_framebuffer, _VCTX _STRUCT);
DEFINE_PRIM(_DESCRIPTOR_SET, vk_create_descriptor_set_layout, _VCTX _STRUCT);
DEFINE_PRIM(_BUFFER, vk_create_buffer, _VCTX _STRUCT);
DEFINE_PRIM(_VOID, vk_get_buffer_memory_requirements, _VCTX _BUFFER _STRUCT);
DEFINE_PRIM(_MEMORY, vk_allocate_memory, _VCTX _STRUCT);
DEFINE_PRIM(_BYTES, vk_map_memory, _VCTX _MEMORY _I32 _I32 _I32);
DEFINE_PRIM(_VOID, vk_unmap_memory, _VCTX _MEMORY);
DEFINE_PRIM(_BOOL, vk_bind_buffer_memory, _VCTX _BUFFER _MEMORY _I32);
DEFINE_PRIM(_SEMAPHORE, vk_create_semaphore, _VCTX _STRUCT);
DEFINE_PRIM(_I32, vk_get_next_image_index, _VCTX _SEMAPHORE);
DEFINE_PRIM(_VOID, vk_queue_submit, _VCTX _STRUCT _FENCE);
DEFINE_PRIM(_VOID, vk_present, _VCTX _SEMAPHORE _I32);

// ------ COMMAND BUFFER OPERATIONS -----------------------

HL_PRIM void HL_NAME(vk_command_begin)( VkCommandBuffer out, VkCommandBufferBeginInfo *inf ) {
	vkBeginCommandBuffer(out,inf);
}

HL_PRIM void HL_NAME(vk_command_end)( VkCommandBuffer out ) {
	vkEndCommandBuffer(out);
}

HL_PRIM void HL_NAME(vk_clear_color_image)( VkCommandBuffer out, VkImage img, VkImageLayout layout, VkClearColorValue *colors, int count, VkImageSubresourceRange *range) {
	vkCmdClearColorImage(out, img, layout, colors, count, range);
}

HL_PRIM void HL_NAME(vk_clear_attachments)( VkCommandBuffer out, int count, VkClearAttachment *attachs, int rectCount, VkClearRect *rects ) {
	vkCmdClearAttachments(out, count, attachs, rectCount, rects);
}

HL_PRIM void HL_NAME(vk_clear_depth_stencil_image)( VkCommandBuffer out, VkImage img, VkImageLayout layout, VkClearDepthStencilValue *values, int count, VkImageSubresourceRange *range) {
	vkCmdClearDepthStencilImage(out, img, layout, values, count, range);
}

HL_PRIM void HL_NAME(vk_draw_indexed)( VkCommandBuffer out, int indexCount, int instanceCount, int firstIndex, int vertexOffset, int firstInstance ) {
	vkCmdDrawIndexed(out,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
}

HL_PRIM void HL_NAME(vk_bind_pipeline)( VkCommandBuffer out, int bindPoint, VkPipeline pipeline ) {
	vkCmdBindPipeline(out, (VkPipelineBindPoint)bindPoint, pipeline);
}

HL_PRIM void HL_NAME(vk_begin_render_pass)( VkCommandBuffer out, VkRenderPassBeginInfo *info, int contents ) {
	vkCmdBeginRenderPass(out, info, (VkSubpassContents)contents);
}

HL_PRIM void HL_NAME(vk_bind_index_buffer)( VkCommandBuffer out, VkBuffer buf, int offset, int type ) {
	vkCmdBindIndexBuffer(out, buf, offset, type);
}

HL_PRIM void HL_NAME(vk_bind_vertex_buffers)( VkCommandBuffer out, int first, int count, VkBuffer *buffers, VkDeviceSize *sizes ) {
	vkCmdBindVertexBuffers(out, first, count, buffers, sizes);
}

HL_PRIM void HL_NAME(vk_end_render_pass)( VkCommandBuffer out ) {
	vkCmdEndRenderPass(out);
}

DEFINE_PRIM(_VOID, vk_command_begin, _CMD _STRUCT);
DEFINE_PRIM(_VOID, vk_command_end, _CMD);
DEFINE_PRIM(_VOID, vk_clear_color_image, _CMD _IMAGE _I32 _BYTES _I32 _STRUCT);
DEFINE_PRIM(_VOID, vk_clear_depth_stencil_image, _CMD _IMAGE _I32 _BYTES _I32 _STRUCT);
DEFINE_PRIM(_VOID, vk_clear_attachments, _CMD _I32 _BYTES _I32 _BYTES);
DEFINE_PRIM(_VOID, vk_draw_indexed, _CMD _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, vk_bind_pipeline, _CMD _I32 _GPIPELINE);
DEFINE_PRIM(_VOID, vk_begin_render_pass, _CMD _STRUCT _I32);
DEFINE_PRIM(_VOID, vk_bind_index_buffer, _CMD _BUFFER _I32 _I32);
DEFINE_PRIM(_VOID, vk_bind_vertex_buffers, _CMD _I32 _I32 _BYTES _BYTES);
DEFINE_PRIM(_VOID, vk_end_render_pass, _CMD);

// ------ SHADER COMPILATION ------------------------------

HL_PRIM vbyte *HL_NAME(vk_compile_shader)( vbyte *source, vbyte *shaderFile, vbyte *mainFunction, int shaderKind, int *outSize ) {
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compile_options_t opts = shaderc_compile_options_initialize();
	shaderc_compile_options_set_optimization_level(opts, shaderc_optimization_level_size);
	shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, source, strlen(source), shaderKind, shaderFile, mainFunction, opts);
	shaderc_compiler_release(compiler);
	shaderc_compile_options_release(opts);
	
	if( shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success ) {
		const char *str = shaderc_result_get_error_message(result);
		vbyte *error = hl_copy_bytes(str, (int)strlen(str)+1);
		shaderc_result_release(result);
		*outSize = -1;
		return error;
	}

	int size = (int)shaderc_result_get_length(result);
	vbyte *data = hl_alloc_bytes(size);
	memcpy(data, shaderc_result_get_bytes(result), size);
	shaderc_result_release(result);
	
	*outSize = size;
	return data;
}

DEFINE_PRIM(_BYTES, vk_compile_shader, _BYTES _BYTES _BYTES _I32 _REF(_I32));
