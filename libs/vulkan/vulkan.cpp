#define HL_NAME(n) vulkan_##n
#include <hl.h>
#undef _GUID

#ifndef HL_64
#	error "vulkan.hdll requires a 64-bit build"
#endif

#if defined(HL_WIN)
#	define VK_USE_PLATFORM_WIN32_KHR
#elif defined(HL_NX)
#	define VK_USE_PLATFORM_VI_NN
#endif
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#if defined(HL_NX)
// the loader is linked statically (vulkan.nso)
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr( VkInstance instance, const char *pName );
#elif !defined(HL_WIN) && !defined(HL_CONSOLE)
#	include <dlfcn.h>
#endif

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable:4100 4127 4189 4324 4505)
#elif defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wnullability-completeness"
#	pragma clang diagnostic ignored "-Wunused-private-field"
#	pragma clang diagnostic ignored "-Wunused-variable"
#endif
#include <vma/vk_mem_alloc.h>
#if defined(_MSC_VER)
#	pragma warning(pop)
#elif defined(__clang__)
#	pragma clang diagnostic pop
#endif

#ifdef HL_WIN
#include <dxcapi.h>
#endif

#define VK_GLOBAL_FUNCS \
	VKF(vkCreateInstance) \
	VKF(vkEnumerateInstanceExtensionProperties) \
	VKF(vkEnumerateInstanceLayerProperties) \
	VKF(vkEnumerateInstanceVersion)

#define VK_INSTANCE_FUNCS \
	VKF(vkDestroyInstance) \
	VKF(vkEnumeratePhysicalDevices) \
	VKF(vkGetPhysicalDeviceProperties2) \
	VKF(vkGetPhysicalDeviceFeatures2) \
	VKF(vkGetPhysicalDeviceQueueFamilyProperties) \
	VKF(vkGetPhysicalDeviceFormatProperties) \
	VKF(vkCreateDevice) \
	VKF(vkGetDeviceProcAddr) \
	VKF(vkEnumerateDeviceExtensionProperties) \
	VKF(vkDestroySurfaceKHR) \
	VKF(vkGetPhysicalDeviceSurfaceSupportKHR) \
	VKF(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
	VKF(vkGetPhysicalDeviceSurfaceFormatsKHR) \
	VKF(vkGetPhysicalDeviceSurfacePresentModesKHR)

// these can be NULL if the debug utils extension is not available
#define VK_INSTANCE_OPT_FUNCS \
	VKF(vkCreateDebugUtilsMessengerEXT) \
	VKF(vkDestroyDebugUtilsMessengerEXT) \
	VKF(vkSetDebugUtilsObjectNameEXT) \
	VKF(vkCmdBeginDebugUtilsLabelEXT) \
	VKF(vkCmdEndDebugUtilsLabelEXT)

#define VK_DEVICE_FUNCS \
	VKF(vkDestroyDevice) \
	VKF(vkGetDeviceQueue) \
	VKF(vkDeviceWaitIdle) \
	VKF(vkQueueSubmit2) \
	VKF(vkCreateSwapchainKHR) \
	VKF(vkDestroySwapchainKHR) \
	VKF(vkGetSwapchainImagesKHR) \
	VKF(vkAcquireNextImageKHR) \
	VKF(vkQueuePresentKHR) \
	VKF(vkCreateCommandPool) \
	VKF(vkDestroyCommandPool) \
	VKF(vkResetCommandPool) \
	VKF(vkAllocateCommandBuffers) \
	VKF(vkBeginCommandBuffer) \
	VKF(vkEndCommandBuffer) \
	VKF(vkCreateFence) \
	VKF(vkDestroyFence) \
	VKF(vkWaitForFences) \
	VKF(vkResetFences) \
	VKF(vkGetFenceStatus) \
	VKF(vkCreateSemaphore) \
	VKF(vkDestroySemaphore) \
	VKF(vkCreateImageView) \
	VKF(vkDestroyImageView) \
	VKF(vkCreateSampler) \
	VKF(vkDestroySampler) \
	VKF(vkCreateShaderModule) \
	VKF(vkDestroyShaderModule) \
	VKF(vkCreateDescriptorSetLayout) \
	VKF(vkDestroyDescriptorSetLayout) \
	VKF(vkCreatePipelineLayout) \
	VKF(vkDestroyPipelineLayout) \
	VKF(vkCreatePipelineCache) \
	VKF(vkDestroyPipelineCache) \
	VKF(vkGetPipelineCacheData) \
	VKF(vkCreateGraphicsPipelines) \
	VKF(vkCreateComputePipelines) \
	VKF(vkDestroyPipeline) \
	VKF(vkCreateDescriptorPool) \
	VKF(vkDestroyDescriptorPool) \
	VKF(vkResetDescriptorPool) \
	VKF(vkAllocateDescriptorSets) \
	VKF(vkUpdateDescriptorSets) \
	VKF(vkFreeDescriptorSets) \
	VKF(vkCmdPushConstants) \
	VKF(vkCreateQueryPool) \
	VKF(vkDestroyQueryPool) \
	VKF(vkGetQueryPoolResults) \
	VKF(vkCmdResetQueryPool) \
	VKF(vkCmdWriteTimestamp2) \
	VKF(vkCmdPipelineBarrier2) \
	VKF(vkCmdBeginRendering) \
	VKF(vkCmdEndRendering) \
	VKF(vkCmdClearAttachments) \
	VKF(vkCmdSetViewport) \
	VKF(vkCmdSetScissor) \
	VKF(vkCmdSetStencilReference) \
	VKF(vkCmdBindPipeline) \
	VKF(vkCmdBindDescriptorSets) \
	VKF(vkCmdBindVertexBuffers2) \
	VKF(vkCmdBindIndexBuffer) \
	VKF(vkCmdDrawIndexed) \
	VKF(vkCmdDrawIndexedIndirect) \
	VKF(vkCmdDrawIndexedIndirectCount) \
	VKF(vkCmdDispatch) \
	VKF(vkCmdCopyBuffer2) \
	VKF(vkCmdCopyBufferToImage2) \
	VKF(vkCmdCopyImageToBuffer2) \
	VKF(vkCmdCopyImage2) \
	VKF(vkCmdBlitImage2) \
	VKF(vkCmdFillBuffer) \
	VKF(vkCmdDraw) \
	VKF(vkCmdDrawIndirect) \
	VKF(vkCmdDrawIndirectCount) \
	VKF(vkCmdDispatchIndirect) \
	VKF(vkCmdResolveImage2) \
	VKF(vkCmdClearColorImage) \
	VKF(vkCmdClearDepthStencilImage) \
	VKF(vkCmdUpdateBuffer) \
	VKF(vkCmdBeginQuery) \
	VKF(vkCmdEndQuery) \
	VKF(vkCmdCopyQueryPoolResults) \
	VKF(vkResetQueryPool) \
	VKF(vkCmdSetDepthBias) \
	VKF(vkCmdSetBlendConstants) \
	VKF(vkCmdSetCullMode) \
	VKF(vkCmdSetFrontFace) \
	VKF(vkCmdSetPrimitiveTopology) \
	VKF(vkCmdSetDepthTestEnable) \
	VKF(vkCmdSetDepthWriteEnable) \
	VKF(vkCmdSetDepthCompareOp) \
	VKF(vkCmdSetDepthBiasEnable) \
	VKF(vkCmdSetDepthBoundsTestEnable) \
	VKF(vkCmdSetDepthBounds) \
	VKF(vkCmdSetStencilTestEnable) \
	VKF(vkCmdSetStencilOp) \
	VKF(vkCmdSetStencilCompareMask) \
	VKF(vkCmdSetStencilWriteMask) \
	VKF(vkCmdSetLineWidth) \
	VKF(vkCmdSetRasterizerDiscardEnable) \
	VKF(vkCmdSetPrimitiveRestartEnable) \
	VKF(vkGetBufferDeviceAddress) \
	VKF(vkCmdExecuteCommands) \
	VKF(vkResetCommandBuffer) \
	VKF(vkFreeCommandBuffers) \
	VKF(vkQueueWaitIdle)

#define VKF(name) static PFN_##name name = NULL;
static PFN_vkGetInstanceProcAddr getInstanceProcAddr = NULL;
VK_GLOBAL_FUNCS
VK_INSTANCE_FUNCS
VK_INSTANCE_OPT_FUNCS
VK_DEVICE_FUNCS
#undef VKF

typedef struct {
	const char *name;
	PFN_vkVoidFunction *ptr;
} vk_function;

#define VKF(name) { #name, (PFN_vkVoidFunction*)&name },
static vk_function vk_functions[] = {
	VK_GLOBAL_FUNCS
	VK_INSTANCE_FUNCS
	VK_INSTANCE_OPT_FUNCS
	VK_DEVICE_FUNCS
};
#undef VKF

typedef struct {
	VkBuffer buffer;
	VmaAllocation alloc;
	void *mapped;
	VkDeviceSize size;
} vk_buffer;

typedef struct {
	VkImage image;
	VmaAllocation alloc;
} vk_image;

typedef struct {
	VkInstance instance;
	VkPhysicalDevice pdev;
	VkDevice device;
	VkSurfaceKHR surface;
	VmaAllocator allocator;
	VkDebugUtilsMessengerEXT messenger;
} vk_context;

static vk_context ctx = {0};
#if defined(HL_WIN)
static HMODULE vk_lib = NULL;
#elif !defined(HL_CONSOLE)
static void *vk_lib = NULL;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback( VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *data, void *user ) {
	const char *level = (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) ? "ERROR" : "WARNING";
	printf("[Vulkan %s] %s\n", level, data->pMessage);
	fflush(stdout);
	return VK_FALSE;
}

static bool load_global_functions() {
	if( !getInstanceProcAddr ) return false;
#	define VKF(name) name = (PFN_##name)getInstanceProcAddr(NULL, #name); if( !name ) return false;
	VK_GLOBAL_FUNCS
#	undef VKF
	return true;
}

static bool load_vulkan() {
	if( getInstanceProcAddr ) return true;
#if defined(HL_WIN)
	vk_lib = LoadLibraryA("vulkan-1.dll");
	if( !vk_lib ) return false;
	getInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vk_lib, "vkGetInstanceProcAddr");
#elif defined(HL_NX)
	getInstanceProcAddr = vkGetInstanceProcAddr;
#elif !defined(HL_CONSOLE)
#	ifdef HL_MAC
	vk_lib = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
#	else
	vk_lib = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
#	endif
	if( !vk_lib ) return false;
	getInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vk_lib, "vkGetInstanceProcAddr");
#endif
	return load_global_functions();
}

HL_PRIM bool HL_NAME(set_instance_proc_addr)( PFN_vkGetInstanceProcAddr proc ) {
	if( ctx.instance ) hl_error("Vulkan instance already created");
	getInstanceProcAddr = proc;
	return load_global_functions();
}

HL_PRIM bool HL_NAME(set_function)( vbyte *name, PFN_vkVoidFunction proc ) {
	for(size_t i=0;i<sizeof(vk_functions)/sizeof(vk_function);i++)
		if( strcmp(vk_functions[i].name, (char*)name) == 0 ) {
			*vk_functions[i].ptr = proc;
			return true;
		}
	return false;
}

HL_PRIM int HL_NAME(get_instance_version)() {
	if( !load_vulkan() ) return 0;
	uint32_t version = 0;
	vkEnumerateInstanceVersion(&version);
	return (int)version;
}

HL_PRIM int HL_NAME(enumerate_instance_extension_properties)( int *count, vbyte *props ) {
	if( !load_vulkan() ) return VK_ERROR_INITIALIZATION_FAILED;
	uint32_t c = props ? *count : 0;
	VkResult r = vkEnumerateInstanceExtensionProperties(NULL, &c, (VkExtensionProperties*)props);
	*count = (int)c;
	return r;
}

HL_PRIM int HL_NAME(enumerate_instance_layer_properties)( int *count, vbyte *props ) {
	if( !load_vulkan() ) return VK_ERROR_INITIALIZATION_FAILED;
	uint32_t c = props ? *count : 0;
	VkResult r = vkEnumerateInstanceLayerProperties(&c, (VkLayerProperties*)props);
	*count = (int)c;
	return r;
}

HL_PRIM int HL_NAME(instance_create)( VkInstanceCreateInfo *inf ) {
	if( ctx.instance ) hl_error("Vulkan instance already created");
	if( !load_vulkan() ) return VK_ERROR_INITIALIZATION_FAILED;
	VkResult r = vkCreateInstance(inf, NULL, &ctx.instance);
	if( r != VK_SUCCESS ) return r;
#	define VKF(name) name = (PFN_##name)getInstanceProcAddr(ctx.instance, #name); if( !name ) return VK_ERROR_INCOMPATIBLE_DRIVER;
	VK_INSTANCE_FUNCS
#	undef VKF
	for(uint32_t i=0;i<inf->enabledExtensionCount;i++)
		if( strcmp(inf->ppEnabledExtensionNames[i], VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0 ) {
#			define VKF(name) name = (PFN_##name)getInstanceProcAddr(ctx.instance, #name);
			VK_INSTANCE_OPT_FUNCS
#			undef VKF
		}
	return VK_SUCCESS;
}

HL_PRIM int HL_NAME(debug_messenger_create)( int severityFlags, int typeFlags ) {
	if( !vkCreateDebugUtilsMessengerEXT ) return VK_ERROR_EXTENSION_NOT_PRESENT;
	VkDebugUtilsMessengerCreateInfoEXT inf = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	inf.messageSeverity = (VkDebugUtilsMessageSeverityFlagsEXT)severityFlags;
	inf.messageType = (VkDebugUtilsMessageTypeFlagsEXT)typeFlags;
	inf.pfnUserCallback = vk_debug_callback;
	return vkCreateDebugUtilsMessengerEXT(ctx.instance, &inf, NULL, &ctx.messenger);
}

HL_PRIM VkInstance HL_NAME(get_instance)() {
	return ctx.instance;
}

#define _INSTANCE _ABSTRACT(vk_instance)
#define _PROC _ABSTRACT(vk_proc)
DEFINE_PRIM(_BOOL, set_instance_proc_addr, _PROC);
DEFINE_PRIM(_BOOL, set_function, _BYTES _PROC);
DEFINE_PRIM(_I32, get_instance_version, _NO_ARG);
DEFINE_PRIM(_I32, enumerate_instance_extension_properties, _REF(_I32) _BYTES);
DEFINE_PRIM(_I32, enumerate_instance_layer_properties, _REF(_I32) _BYTES);
DEFINE_PRIM(_I32, instance_create, _STRUCT);
DEFINE_PRIM(_I32, debug_messenger_create, _I32 _I32);
DEFINE_PRIM(_INSTANCE, get_instance, _NO_ARG);

#ifdef HL_WIN
HL_PRIM bool HL_NAME(surface_create_win32)( HWND wnd ) {
	PFN_vkCreateWin32SurfaceKHR create = (PFN_vkCreateWin32SurfaceKHR)getInstanceProcAddr(ctx.instance, "vkCreateWin32SurfaceKHR");
	if( !create ) return false;
	VkWin32SurfaceCreateInfoKHR inf = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	inf.hinstance = GetModuleHandle(NULL);
	inf.hwnd = wnd;
	return create(ctx.instance, &inf, NULL, &ctx.surface) == VK_SUCCESS;
}
DEFINE_PRIM(_BOOL, surface_create_win32, _ABSTRACT(dx_window));
#endif

#ifdef HL_NX
// window : nn::vi::NativeWindowHandle
HL_PRIM bool HL_NAME(surface_create_vi)( void *window ) {
	PFN_vkCreateViSurfaceNN create = (PFN_vkCreateViSurfaceNN)getInstanceProcAddr(ctx.instance, "vkCreateViSurfaceNN");
	if( !create ) return false;
	VkViSurfaceCreateInfoNN inf = { VK_STRUCTURE_TYPE_VI_SURFACE_CREATE_INFO_NN };
	inf.window = window;
	return create(ctx.instance, &inf, NULL, &ctx.surface) == VK_SUCCESS;
}
DEFINE_PRIM(_BOOL, surface_create_vi, _ABSTRACT(nx_native_window));
#endif

HL_PRIM void HL_NAME(set_surface)( VkSurfaceKHR surface ) {
	ctx.surface = surface;
}

HL_PRIM VkSurfaceKHR HL_NAME(get_surface)() {
	return ctx.surface;
}

HL_PRIM void HL_NAME(surface_destroy)() {
	if( ctx.surface ) vkDestroySurfaceKHR(ctx.instance, ctx.surface, NULL);
	ctx.surface = VK_NULL_HANDLE;
}

HL_PRIM int HL_NAME(surface_get_capabilities)( VkSurfaceCapabilitiesKHR *caps ) {
	return vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.pdev, ctx.surface, caps);
}

HL_PRIM int HL_NAME(surface_get_formats)( vbyte *out, int max ) {
	uint32_t count = out ? max : 0;
	VkResult r = vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.pdev, ctx.surface, &count, (VkSurfaceFormatKHR*)out);
	return r == VK_SUCCESS || r == VK_INCOMPLETE ? (int)count : 0;
}

HL_PRIM int HL_NAME(surface_get_present_modes)( vbyte *out, int max ) {
	uint32_t count = out ? max : 0;
	VkResult r = vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.pdev, ctx.surface, &count, (VkPresentModeKHR*)out);
	return r == VK_SUCCESS || r == VK_INCOMPLETE ? (int)count : 0;
}

#define _SURFACE _ABSTRACT(vk_surface)
DEFINE_PRIM(_VOID, set_surface, _SURFACE);
DEFINE_PRIM(_SURFACE, get_surface, _NO_ARG);
DEFINE_PRIM(_VOID, surface_destroy, _NO_ARG);
DEFINE_PRIM(_I32, surface_get_capabilities, _STRUCT);
DEFINE_PRIM(_I32, surface_get_formats, _BYTES _I32);
DEFINE_PRIM(_I32, surface_get_present_modes, _BYTES _I32);

HL_PRIM int HL_NAME(enumerate_physical_devices)( int *count, VkPhysicalDevice *devices ) {
	uint32_t c = devices ? *count : 0;
	VkResult r = vkEnumeratePhysicalDevices(ctx.instance, &c, devices);
	*count = (int)c;
	return r;
}

HL_PRIM void HL_NAME(get_physical_device_properties2)( VkPhysicalDevice pdev, VkPhysicalDeviceProperties2 *props ) {
	vkGetPhysicalDeviceProperties2(pdev, props);
}

HL_PRIM void HL_NAME(get_physical_device_features2)( VkPhysicalDevice pdev, VkPhysicalDeviceFeatures2 *features ) {
	vkGetPhysicalDeviceFeatures2(pdev, features);
}

HL_PRIM void HL_NAME(get_physical_device_queue_family_properties)( VkPhysicalDevice pdev, int *count, VkQueueFamilyProperties *props ) {
	uint32_t c = props ? *count : 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pdev, &c, props);
	*count = (int)c;
}

HL_PRIM bool HL_NAME(get_physical_device_surface_support)( VkPhysicalDevice pdev, int queueFamily ) {
	VkBool32 supported = VK_FALSE;
	vkGetPhysicalDeviceSurfaceSupportKHR(pdev, queueFamily, ctx.surface, &supported);
	return supported == VK_TRUE;
}

HL_PRIM void HL_NAME(get_physical_device_format_properties)( VkPhysicalDevice pdev, int format, VkFormatProperties *props ) {
	vkGetPhysicalDeviceFormatProperties(pdev, (VkFormat)format, props);
}

HL_PRIM int HL_NAME(enumerate_device_extension_properties)( VkPhysicalDevice pdev, int *count, vbyte *props ) {
	uint32_t c = props ? *count : 0;
	VkResult r = vkEnumerateDeviceExtensionProperties(pdev, NULL, &c, (VkExtensionProperties*)props);
	*count = (int)c;
	return r;
}

static PFN_vkVoidFunction get_device_proc( const char *name, bool aliases ) {
	PFN_vkVoidFunction f = vkGetDeviceProcAddr(ctx.device, name);
	const char *suffixes[] = { "KHR", "EXT" };
	char alias[128];
	for( int i = 0; !f && aliases && i < 2; i++ ) {
		snprintf(alias, sizeof(alias), "%s%s", name, suffixes[i]);
		f = vkGetDeviceProcAddr(ctx.device, alias);
	}
	return f;
}

HL_PRIM int HL_NAME(device_create)( VkPhysicalDevice pdev, VkDeviceCreateInfo *inf ) {
	if( ctx.device ) hl_error("Vulkan device already created");
	VkResult r = vkCreateDevice(pdev, inf, NULL, &ctx.device);
	if( r != VK_SUCCESS ) return r;
	ctx.pdev = pdev;
	VkPhysicalDeviceProperties2 props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	vkGetPhysicalDeviceProperties2(pdev, &props);
	bool aliases = props.properties.apiVersion < VK_API_VERSION_1_3;
#	define VKF(name) name = (PFN_##name)get_device_proc(#name, aliases); if( !name ) return VK_ERROR_INCOMPATIBLE_DRIVER;
	VK_DEVICE_FUNCS
#	undef VKF
	return VK_SUCCESS;
}

HL_PRIM VkQueue HL_NAME(get_device_queue)( int queueFamily, int index ) {
	VkQueue q = NULL;
	vkGetDeviceQueue(ctx.device, queueFamily, index, &q);
	return q;
}

HL_PRIM int HL_NAME(allocator_create)( int flags, int vulkanApiVersion ) {
	VmaVulkanFunctions funcs = {};
	funcs.vkGetInstanceProcAddr = getInstanceProcAddr;
	funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	VmaAllocatorCreateInfo inf = {};
	inf.flags = (VmaAllocatorCreateFlags)flags;
	inf.physicalDevice = ctx.pdev;
	inf.device = ctx.device;
	inf.instance = ctx.instance;
	inf.vulkanApiVersion = (uint32_t)vulkanApiVersion;
	inf.pVulkanFunctions = &funcs;
	return vmaCreateAllocator(&inf, &ctx.allocator);
}

HL_PRIM void HL_NAME(get_memory_properties)( vbyte *out ) {
	const VkPhysicalDeviceMemoryProperties *mem;
	vmaGetMemoryProperties(ctx.allocator, &mem);
	memcpy(out, mem, sizeof(VkPhysicalDeviceMemoryProperties));
}

HL_PRIM void HL_NAME(get_heap_budgets)( vbyte *out ) {
	vmaGetHeapBudgets(ctx.allocator, (VmaBudget*)out);
}

HL_PRIM VkPhysicalDevice HL_NAME(get_physical_device)() {
	return ctx.pdev;
}

HL_PRIM VkDevice HL_NAME(get_device)() {
	return ctx.device;
}

HL_PRIM PFN_vkGetInstanceProcAddr HL_NAME(get_instance_proc_addr)() {
	return getInstanceProcAddr;
}

HL_PRIM PFN_vkGetDeviceProcAddr HL_NAME(get_device_proc_addr)() {
	return vkGetDeviceProcAddr;
}

// returns a VkResult
HL_PRIM int HL_NAME(device_wait_idle)() {
	return vkDeviceWaitIdle(ctx.device);
}

#define _PHYSICAL_DEVICE _ABSTRACT(vk_physical_device)
DEFINE_PRIM(_I32, enumerate_physical_devices, _REF(_I32) _ABSTRACT(hl_carray));
DEFINE_PRIM(_VOID, get_physical_device_properties2, _PHYSICAL_DEVICE _STRUCT);
DEFINE_PRIM(_VOID, get_physical_device_features2, _PHYSICAL_DEVICE _STRUCT);
DEFINE_PRIM(_VOID, get_physical_device_queue_family_properties, _PHYSICAL_DEVICE _REF(_I32) _ABSTRACT(hl_carray));
DEFINE_PRIM(_BOOL, get_physical_device_surface_support, _PHYSICAL_DEVICE _I32);
DEFINE_PRIM(_VOID, get_physical_device_format_properties, _PHYSICAL_DEVICE _I32 _STRUCT);
DEFINE_PRIM(_I32, enumerate_device_extension_properties, _PHYSICAL_DEVICE _REF(_I32) _BYTES);
DEFINE_PRIM(_I32, device_create, _PHYSICAL_DEVICE _STRUCT);
DEFINE_PRIM(_ABSTRACT(vk_queue), get_device_queue, _I32 _I32);
DEFINE_PRIM(_I32, allocator_create, _I32 _I32);
DEFINE_PRIM(_VOID, get_memory_properties, _BYTES);
DEFINE_PRIM(_VOID, get_heap_budgets, _BYTES);
DEFINE_PRIM(_PHYSICAL_DEVICE, get_physical_device, _NO_ARG);
DEFINE_PRIM(_ABSTRACT(vk_device), get_device, _NO_ARG);
DEFINE_PRIM(_PROC, get_instance_proc_addr, _NO_ARG);
DEFINE_PRIM(_PROC, get_device_proc_addr, _NO_ARG);
DEFINE_PRIM(_I32, device_wait_idle, _NO_ARG);

HL_PRIM VkSwapchainKHR HL_NAME(swapchain_create)( VkSwapchainCreateInfoKHR *inf, int *result ) {
	VkSwapchainKHR sc = VK_NULL_HANDLE;
	*result = vkCreateSwapchainKHR(ctx.device, inf, NULL, &sc);
	return sc;
}

HL_PRIM void HL_NAME(swapchain_destroy)( VkSwapchainKHR sc ) {
	vkDestroySwapchainKHR(ctx.device, sc, NULL);
}

HL_PRIM int HL_NAME(get_swapchain_images)( VkSwapchainKHR sc, int *count, VkImage *images ) {
	uint32_t c = images ? *count : 0;
	VkResult r = vkGetSwapchainImagesKHR(ctx.device, sc, &c, images);
	*count = (int)c;
	return r;
}

HL_PRIM int HL_NAME(acquire_next_image)( VkSwapchainKHR swapchain, int64 timeout, VkSemaphore sem, VkFence fence, int *index ) {
	uint32_t idx = 0;
	VkResult r = vkAcquireNextImageKHR(ctx.device, swapchain, (uint64_t)timeout, sem, fence, &idx);
	*index = (int)idx;
	return r;
}

HL_PRIM int HL_NAME(queue_present)( VkQueue queue, VkPresentInfoKHR *inf ) {
	return vkQueuePresentKHR(queue, inf);
}

#define _SEMAPHORE _ABSTRACT(vk_semaphore)
#define _IMAGE _ABSTRACT(vk_image)
#define _FENCE _ABSTRACT(vk_fence)
#define _QUEUE _ABSTRACT(vk_queue)
#define _SWAPCHAIN _ABSTRACT(vk_swapchain)
DEFINE_PRIM(_SWAPCHAIN, swapchain_create, _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, swapchain_destroy, _SWAPCHAIN);
DEFINE_PRIM(_I32, get_swapchain_images, _SWAPCHAIN _REF(_I32) _ABSTRACT(hl_carray));
DEFINE_PRIM(_I32, acquire_next_image, _SWAPCHAIN _I64 _SEMAPHORE _FENCE _REF(_I32));
DEFINE_PRIM(_I32, queue_present, _QUEUE _STRUCT);

HL_PRIM VkFence HL_NAME(fence_create)( int flags, int *result ) {
	VkFenceCreateInfo inf = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	inf.flags = (VkFenceCreateFlags)flags;
	VkFence f = VK_NULL_HANDLE;
	*result = vkCreateFence(ctx.device, &inf, NULL, &f);
	return f;
}

HL_PRIM void HL_NAME(fence_destroy)( VkFence f ) {
	vkDestroyFence(ctx.device, f, NULL);
}

HL_PRIM int HL_NAME(wait_for_fences)( int count, VkFence *fences, bool waitAll, int64 timeout ) {
	return vkWaitForFences(ctx.device, count, fences, waitAll, (uint64_t)timeout);
}

HL_PRIM int HL_NAME(get_fence_status)( VkFence f ) {
	return vkGetFenceStatus(ctx.device, f);
}

HL_PRIM int HL_NAME(reset_fences)( int count, VkFence *fences ) {
	return vkResetFences(ctx.device, count, fences);
}

HL_PRIM VkSemaphore HL_NAME(semaphore_create)( int *result ) {
	VkSemaphoreCreateInfo inf = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkSemaphore s = VK_NULL_HANDLE;
	*result = vkCreateSemaphore(ctx.device, &inf, NULL, &s);
	return s;
}

HL_PRIM void HL_NAME(semaphore_destroy)( VkSemaphore s ) {
	vkDestroySemaphore(ctx.device, s, NULL);
}

HL_PRIM int HL_NAME(queue_submit2)( VkQueue queue, int count, VkSubmitInfo2 *submits, VkFence fence ) {
	return vkQueueSubmit2(queue, count, submits, fence);
}

#define _CMD _ABSTRACT(vk_command_buffer)
DEFINE_PRIM(_FENCE, fence_create, _I32 _REF(_I32));
DEFINE_PRIM(_VOID, fence_destroy, _FENCE);
DEFINE_PRIM(_I32, wait_for_fences, _I32 _REF(_FENCE) _BOOL _I64);
DEFINE_PRIM(_I32, get_fence_status, _FENCE);
DEFINE_PRIM(_I32, reset_fences, _I32 _REF(_FENCE));
DEFINE_PRIM(_SEMAPHORE, semaphore_create, _REF(_I32));
DEFINE_PRIM(_VOID, semaphore_destroy, _SEMAPHORE);
DEFINE_PRIM(_I32, queue_submit2, _QUEUE _I32 _ABSTRACT(hl_carray) _FENCE);

HL_PRIM VkCommandPool HL_NAME(command_pool_create)( int flags, int queueFamily, int *result ) {
	VkCommandPoolCreateInfo inf = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	inf.flags = (VkCommandPoolCreateFlags)flags;
	inf.queueFamilyIndex = queueFamily;
	VkCommandPool p = VK_NULL_HANDLE;
	*result = vkCreateCommandPool(ctx.device, &inf, NULL, &p);
	return p;
}

HL_PRIM void HL_NAME(command_pool_destroy)( VkCommandPool p ) {
	vkDestroyCommandPool(ctx.device, p, NULL);
}

HL_PRIM int HL_NAME(command_pool_reset)( VkCommandPool p, int flags ) {
	return vkResetCommandPool(ctx.device, p, (VkCommandPoolResetFlags)flags);
}

HL_PRIM VkCommandBuffer HL_NAME(command_buffer_create)( VkCommandPool p, int level, int *result ) {
	VkCommandBufferAllocateInfo inf = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	inf.commandPool = p;
	inf.level = (VkCommandBufferLevel)level;
	inf.commandBufferCount = 1;
	VkCommandBuffer cmd = NULL;
	*result = vkAllocateCommandBuffers(ctx.device, &inf, &cmd);
	return cmd;
}

HL_PRIM int HL_NAME(begin_command_buffer)( VkCommandBuffer cmd, VkCommandBufferBeginInfo *inf ) {
	return vkBeginCommandBuffer(cmd, inf);
}

HL_PRIM int HL_NAME(end_command_buffer)( VkCommandBuffer cmd ) {
	return vkEndCommandBuffer(cmd);
}

#define _CMD_POOL _ABSTRACT(vk_command_pool)
DEFINE_PRIM(_CMD_POOL, command_pool_create, _I32 _I32 _REF(_I32));
DEFINE_PRIM(_VOID, command_pool_destroy, _CMD_POOL);
DEFINE_PRIM(_I32, command_pool_reset, _CMD_POOL _I32);
DEFINE_PRIM(_CMD, command_buffer_create, _CMD_POOL _I32 _REF(_I32));
DEFINE_PRIM(_I32, begin_command_buffer, _CMD _STRUCT);
DEFINE_PRIM(_I32, end_command_buffer, _CMD);

HL_PRIM vk_buffer *HL_NAME(buffer_create)( VkBufferCreateInfo *inf, VmaAllocationCreateInfo *ainf, int *result ) {
	VkBuffer buf;
	VmaAllocation alloc;
	VmaAllocationInfo info;
	VkResult r = vmaCreateBuffer(ctx.allocator, inf, ainf, &buf, &alloc, &info);
	*result = r;
	if( r != VK_SUCCESS ) return NULL;
	vk_buffer *b = (vk_buffer*)malloc(sizeof(vk_buffer));
	b->buffer = buf;
	b->alloc = alloc;
	b->mapped = info.pMappedData;
	b->size = inf->size;
	return b;
}

HL_PRIM void HL_NAME(buffer_destroy)( vk_buffer *b ) {
	if( !b ) return;
	vmaDestroyBuffer(ctx.allocator, b->buffer, b->alloc);
	free(b);
}

HL_PRIM vbyte *HL_NAME(buffer_get_mapped)( vk_buffer *b ) {
	return (vbyte*)b->mapped;
}

HL_PRIM VkBuffer HL_NAME(buffer_get_handle)( vk_buffer *b ) {
	return b->buffer;
}

HL_PRIM void HL_NAME(buffer_set_name)( vk_buffer *b, vbyte *name ) {
	if( !vkSetDebugUtilsObjectNameEXT || !b ) return;
	VkDebugUtilsObjectNameInfoEXT inf = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
	inf.objectType = VK_OBJECT_TYPE_BUFFER;
	inf.objectHandle = (uint64_t)b->buffer;
	inf.pObjectName = (char*)name;
	vkSetDebugUtilsObjectNameEXT(ctx.device, &inf);
}

#define _BUFFER _ABSTRACT(vk_buffer)
DEFINE_PRIM(_BUFFER, buffer_create, _STRUCT _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, buffer_destroy, _BUFFER);
DEFINE_PRIM(_BYTES, buffer_get_mapped, _BUFFER);
DEFINE_PRIM(_VOID, buffer_set_name, _BUFFER _BYTES);
DEFINE_PRIM(_ABSTRACT(vk_native_buffer), buffer_get_handle, _BUFFER);

HL_PRIM vk_image *HL_NAME(image_create)( VkImageCreateInfo *inf, VmaAllocationCreateInfo *ainf, int *result ) {
	VkImage img;
	VmaAllocation alloc;
	VkResult r = vmaCreateImage(ctx.allocator, inf, ainf, &img, &alloc, NULL);
	*result = r;
	if( r != VK_SUCCESS ) return NULL;
	vk_image *i = (vk_image*)malloc(sizeof(vk_image));
	i->image = img;
	i->alloc = alloc;
	return i;
}

HL_PRIM void HL_NAME(image_destroy)( vk_image *i ) {
	if( !i ) return;
	vmaDestroyImage(ctx.allocator, i->image, i->alloc);
	free(i);
}

HL_PRIM VkImage HL_NAME(image_get_handle)( vk_image *i ) {
	return i->image;
}

HL_PRIM void HL_NAME(image_set_name)( vk_image *i, vbyte *name ) {
	if( !vkSetDebugUtilsObjectNameEXT || !i ) return;
	VkDebugUtilsObjectNameInfoEXT inf = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
	inf.objectType = VK_OBJECT_TYPE_IMAGE;
	inf.objectHandle = (uint64_t)i->image;
	inf.pObjectName = (char*)name;
	vkSetDebugUtilsObjectNameEXT(ctx.device, &inf);
}

HL_PRIM VkImageView HL_NAME(image_view_create)( VkImageViewCreateInfo *inf, int *result ) {
	VkImageView v = VK_NULL_HANDLE;
	*result = vkCreateImageView(ctx.device, inf, NULL, &v);
	return v;
}

HL_PRIM void HL_NAME(image_view_destroy)( VkImageView v ) {
	vkDestroyImageView(ctx.device, v, NULL);
}

HL_PRIM VkSampler HL_NAME(sampler_create)( VkSamplerCreateInfo *inf, int *result ) {
	VkSampler s = VK_NULL_HANDLE;
	*result = vkCreateSampler(ctx.device, inf, NULL, &s);
	return s;
}

HL_PRIM void HL_NAME(sampler_destroy)( VkSampler s ) {
	vkDestroySampler(ctx.device, s, NULL);
}

#define _VIEW _ABSTRACT(vk_image_view)
#define _SAMPLER _ABSTRACT(vk_sampler)
DEFINE_PRIM(_IMAGE, image_create, _STRUCT _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, image_destroy, _IMAGE);
DEFINE_PRIM(_VOID, image_set_name, _IMAGE _BYTES);
DEFINE_PRIM(_ABSTRACT(vk_native_image), image_get_handle, _IMAGE);
DEFINE_PRIM(_VIEW, image_view_create, _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, image_view_destroy, _VIEW);
DEFINE_PRIM(_SAMPLER, sampler_create, _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, sampler_destroy, _SAMPLER);

HL_PRIM VkShaderModule HL_NAME(shader_module_create)( VkShaderModuleCreateInfo *inf, int *result ) {
	VkShaderModule m = VK_NULL_HANDLE;
	*result = vkCreateShaderModule(ctx.device, inf, NULL, &m);
	return m;
}

HL_PRIM void HL_NAME(shader_module_destroy)( VkShaderModule m ) {
	vkDestroyShaderModule(ctx.device, m, NULL);
}

HL_PRIM VkDescriptorSetLayout HL_NAME(descriptor_set_layout_create)( VkDescriptorSetLayoutCreateInfo *inf, int *result ) {
	VkDescriptorSetLayout l = VK_NULL_HANDLE;
	*result = vkCreateDescriptorSetLayout(ctx.device, inf, NULL, &l);
	return l;
}

HL_PRIM void HL_NAME(descriptor_set_layout_destroy)( VkDescriptorSetLayout l ) {
	vkDestroyDescriptorSetLayout(ctx.device, l, NULL);
}

HL_PRIM VkPipelineLayout HL_NAME(pipeline_layout_create)( VkPipelineLayoutCreateInfo *inf, int *result ) {
	VkPipelineLayout l = VK_NULL_HANDLE;
	*result = vkCreatePipelineLayout(ctx.device, inf, NULL, &l);
	return l;
}

HL_PRIM void HL_NAME(pipeline_layout_destroy)( VkPipelineLayout l ) {
	vkDestroyPipelineLayout(ctx.device, l, NULL);
}

HL_PRIM VkPipelineCache HL_NAME(pipeline_cache_create)( VkPipelineCacheCreateInfo *inf, int *result ) {
	VkPipelineCache c = VK_NULL_HANDLE;
	*result = vkCreatePipelineCache(ctx.device, inf, NULL, &c);
	return c;
}

HL_PRIM void HL_NAME(pipeline_cache_destroy)( VkPipelineCache c ) {
	vkDestroyPipelineCache(ctx.device, c, NULL);
}

HL_PRIM int HL_NAME(get_pipeline_cache_data)( VkPipelineCache c, int *size, vbyte *data ) {
	size_t sz = (size_t)*size;
	VkResult r = vkGetPipelineCacheData(ctx.device, c, &sz, data);
	*size = (int)sz;
	return r;
}

HL_PRIM int HL_NAME(create_graphics_pipelines)( VkPipelineCache cache, int count, VkGraphicsPipelineCreateInfo *infos, VkPipeline *pipelines ) {
	hl_blocking(true);
	VkResult r = vkCreateGraphicsPipelines(ctx.device, cache, count, infos, NULL, pipelines);
	hl_blocking(false);
	return r;
}

HL_PRIM int HL_NAME(create_compute_pipelines)( VkPipelineCache cache, int count, VkComputePipelineCreateInfo *infos, VkPipeline *pipelines ) {
	hl_blocking(true);
	VkResult r = vkCreateComputePipelines(ctx.device, cache, count, infos, NULL, pipelines);
	hl_blocking(false);
	return r;
}

HL_PRIM void HL_NAME(pipeline_destroy)( VkPipeline p ) {
	vkDestroyPipeline(ctx.device, p, NULL);
}

#define _SHADER_MODULE _ABSTRACT(vk_shader_module)
#define _SET_LAYOUT _ABSTRACT(vk_descriptor_set_layout)
#define _PIPELINE_LAYOUT _ABSTRACT(vk_pipeline_layout)
#define _PIPELINE _ABSTRACT(vk_pipeline)
DEFINE_PRIM(_SHADER_MODULE, shader_module_create, _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, shader_module_destroy, _SHADER_MODULE);
DEFINE_PRIM(_SET_LAYOUT, descriptor_set_layout_create, _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, descriptor_set_layout_destroy, _SET_LAYOUT);
DEFINE_PRIM(_PIPELINE_LAYOUT, pipeline_layout_create, _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, pipeline_layout_destroy, _PIPELINE_LAYOUT);
#define _PIPELINE_CACHE _ABSTRACT(vk_pipeline_cache)
DEFINE_PRIM(_PIPELINE_CACHE, pipeline_cache_create, _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, pipeline_cache_destroy, _PIPELINE_CACHE);
DEFINE_PRIM(_I32, get_pipeline_cache_data, _PIPELINE_CACHE _REF(_I32) _BYTES);
DEFINE_PRIM(_I32, create_graphics_pipelines, _PIPELINE_CACHE _I32 _ABSTRACT(hl_carray) _ABSTRACT(hl_carray));
DEFINE_PRIM(_I32, create_compute_pipelines, _PIPELINE_CACHE _I32 _ABSTRACT(hl_carray) _ABSTRACT(hl_carray));
DEFINE_PRIM(_VOID, pipeline_destroy, _PIPELINE);

HL_PRIM VkDescriptorPool HL_NAME(descriptor_pool_create)( VkDescriptorPoolCreateInfo *inf, int *result ) {
	VkDescriptorPool p = VK_NULL_HANDLE;
	*result = vkCreateDescriptorPool(ctx.device, inf, NULL, &p);
	return p;
}

HL_PRIM void HL_NAME(descriptor_pool_destroy)( VkDescriptorPool p ) {
	vkDestroyDescriptorPool(ctx.device, p, NULL);
}

HL_PRIM int HL_NAME(descriptor_pool_reset)( VkDescriptorPool p, int flags ) {
	return vkResetDescriptorPool(ctx.device, p, (VkDescriptorPoolResetFlags)flags);
}

HL_PRIM int HL_NAME(allocate_descriptor_sets)( VkDescriptorSetAllocateInfo *inf, VkDescriptorSet *sets ) {
	return vkAllocateDescriptorSets(ctx.device, inf, sets);
}

HL_PRIM int HL_NAME(free_descriptor_sets)( VkDescriptorPool p, int count, VkDescriptorSet *sets ) {
	return vkFreeDescriptorSets(ctx.device, p, count, sets);
}

HL_PRIM void HL_NAME(update_descriptor_sets)( int writeCount, VkWriteDescriptorSet *writes, int copyCount, VkCopyDescriptorSet *copies ) {
	vkUpdateDescriptorSets(ctx.device, writeCount, writes, copyCount, copies);
}

HL_PRIM void HL_NAME(cmd_bind_descriptor_sets)( VkCommandBuffer cmd, int bindPoint, VkPipelineLayout layout, int firstSet, int setCount, VkDescriptorSet *sets, int dynamicOffsetCount, vbyte *dynamicOffsets ) {
	vkCmdBindDescriptorSets(cmd, (VkPipelineBindPoint)bindPoint, layout, firstSet, setCount, sets, dynamicOffsetCount, (uint32_t*)dynamicOffsets);
}

HL_PRIM void HL_NAME(cmd_push_constants)( VkCommandBuffer cmd, VkPipelineLayout layout, int stageFlags, int offset, int size, vbyte *values ) {
	vkCmdPushConstants(cmd, layout, (VkShaderStageFlags)stageFlags, offset, size, values);
}

#define _DESC_POOL _ABSTRACT(vk_descriptor_pool)
#define _DESC_SET _ABSTRACT(vk_descriptor_set)
DEFINE_PRIM(_DESC_POOL, descriptor_pool_create, _STRUCT _REF(_I32));
DEFINE_PRIM(_VOID, descriptor_pool_destroy, _DESC_POOL);
DEFINE_PRIM(_I32, descriptor_pool_reset, _DESC_POOL _I32);
DEFINE_PRIM(_I32, allocate_descriptor_sets, _STRUCT _ABSTRACT(hl_carray));
DEFINE_PRIM(_I32, free_descriptor_sets, _DESC_POOL _I32 _ABSTRACT(hl_carray));
DEFINE_PRIM(_VOID, update_descriptor_sets, _I32 _ABSTRACT(hl_carray) _I32 _ABSTRACT(hl_carray));
DEFINE_PRIM(_VOID, cmd_bind_descriptor_sets, _CMD _I32 _PIPELINE_LAYOUT _I32 _I32 _ABSTRACT(hl_carray) _I32 _BYTES);
DEFINE_PRIM(_VOID, cmd_push_constants, _CMD _PIPELINE_LAYOUT _I32 _I32 _I32 _BYTES);

HL_PRIM void HL_NAME(cmd_pipeline_barrier2)( VkCommandBuffer cmd, VkDependencyInfo *dep ) {
	vkCmdPipelineBarrier2(cmd, dep);
}

HL_PRIM void HL_NAME(cmd_begin_rendering)( VkCommandBuffer cmd, VkRenderingInfo *inf ) {
	vkCmdBeginRendering(cmd, inf);
}

HL_PRIM void HL_NAME(cmd_end_rendering)( VkCommandBuffer cmd ) {
	vkCmdEndRendering(cmd);
}

HL_PRIM void HL_NAME(cmd_clear_attachments)( VkCommandBuffer cmd, int count, VkClearAttachment *attachments, int rectCount, VkClearRect *rects ) {
	vkCmdClearAttachments(cmd, count, attachments, rectCount, rects);
}

HL_PRIM void HL_NAME(cmd_set_viewport)( VkCommandBuffer cmd, int first, int count, VkViewport *viewports ) {
	vkCmdSetViewport(cmd, first, count, viewports);
}

HL_PRIM void HL_NAME(cmd_set_scissor)( VkCommandBuffer cmd, int first, int count, VkRect2D *scissors ) {
	vkCmdSetScissor(cmd, first, count, scissors);
}

HL_PRIM void HL_NAME(cmd_set_stencil_reference)( VkCommandBuffer cmd, int faceMask, int ref ) {
	vkCmdSetStencilReference(cmd, (VkStencilFaceFlags)faceMask, ref);
}

HL_PRIM void HL_NAME(cmd_bind_pipeline)( VkCommandBuffer cmd, int bindPoint, VkPipeline p ) {
	vkCmdBindPipeline(cmd, (VkPipelineBindPoint)bindPoint, p);
}

// buffers, offsets, sizes & strides : arrays of count elements, sizes & strides can be NULL
HL_PRIM void HL_NAME(cmd_bind_vertex_buffers2)( VkCommandBuffer cmd, int first, int count, VkBuffer *buffers, VkDeviceSize *offsets, VkDeviceSize *sizes, VkDeviceSize *strides ) {
	vkCmdBindVertexBuffers2(cmd, first, count, buffers, offsets, sizes, strides);
}

HL_PRIM void HL_NAME(cmd_bind_index_buffer)( VkCommandBuffer cmd, VkBuffer b, int64 offset, int indexType ) {
	vkCmdBindIndexBuffer(cmd, b, (VkDeviceSize)offset, (VkIndexType)indexType);
}

HL_PRIM void HL_NAME(cmd_draw_indexed)( VkCommandBuffer cmd, int indexCount, int instanceCount, int firstIndex, int vertexOffset, int firstInstance ) {
	vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

HL_PRIM void HL_NAME(cmd_draw_indexed_indirect)( VkCommandBuffer cmd, VkBuffer b, int64 offset, int drawCount, int stride ) {
	vkCmdDrawIndexedIndirect(cmd, b, (VkDeviceSize)offset, drawCount, stride);
}

HL_PRIM void HL_NAME(cmd_draw_indexed_indirect_count)( VkCommandBuffer cmd, VkBuffer b, int64 offset, VkBuffer count, int64 countOffset, int maxDrawCount, int stride ) {
	vkCmdDrawIndexedIndirectCount(cmd, b, (VkDeviceSize)offset, count, (VkDeviceSize)countOffset, maxDrawCount, stride);
}

HL_PRIM void HL_NAME(cmd_dispatch)( VkCommandBuffer cmd, int x, int y, int z ) {
	vkCmdDispatch(cmd, x, y, z);
}

HL_PRIM void HL_NAME(cmd_copy_buffer2)( VkCommandBuffer cmd, VkCopyBufferInfo2 *inf ) {
	vkCmdCopyBuffer2(cmd, inf);
}

HL_PRIM void HL_NAME(cmd_copy_image2)( VkCommandBuffer cmd, VkCopyImageInfo2 *inf ) {
	vkCmdCopyImage2(cmd, inf);
}

HL_PRIM void HL_NAME(cmd_copy_buffer_to_image2)( VkCommandBuffer cmd, VkCopyBufferToImageInfo2 *inf ) {
	vkCmdCopyBufferToImage2(cmd, inf);
}

HL_PRIM void HL_NAME(cmd_copy_image_to_buffer2)( VkCommandBuffer cmd, VkCopyImageToBufferInfo2 *inf ) {
	vkCmdCopyImageToBuffer2(cmd, inf);
}

HL_PRIM void HL_NAME(cmd_blit_image2)( VkCommandBuffer cmd, VkBlitImageInfo2 *inf ) {
	vkCmdBlitImage2(cmd, inf);
}

HL_PRIM void HL_NAME(cmd_fill_buffer)( VkCommandBuffer cmd, VkBuffer b, int64 offset, int64 size, int data ) {
	vkCmdFillBuffer(cmd, b, (VkDeviceSize)offset, (VkDeviceSize)size, (uint32_t)data);
}

HL_PRIM void HL_NAME(cmd_begin_label)( VkCommandBuffer cmd, vbyte *name ) {
	if( !vkCmdBeginDebugUtilsLabelEXT ) return;
	VkDebugUtilsLabelEXT l = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	l.pLabelName = (char*)name;
	vkCmdBeginDebugUtilsLabelEXT(cmd, &l);
}

HL_PRIM void HL_NAME(cmd_end_label)( VkCommandBuffer cmd ) {
	if( !vkCmdEndDebugUtilsLabelEXT ) return;
	vkCmdEndDebugUtilsLabelEXT(cmd);
}

#define _CARRAY _ABSTRACT(hl_carray)
DEFINE_PRIM(_VOID, cmd_pipeline_barrier2, _CMD _STRUCT);
DEFINE_PRIM(_VOID, cmd_begin_rendering, _CMD _STRUCT);
DEFINE_PRIM(_VOID, cmd_end_rendering, _CMD);
DEFINE_PRIM(_VOID, cmd_clear_attachments, _CMD _I32 _CARRAY _I32 _CARRAY);
DEFINE_PRIM(_VOID, cmd_set_viewport, _CMD _I32 _I32 _CARRAY);
DEFINE_PRIM(_VOID, cmd_set_scissor, _CMD _I32 _I32 _CARRAY);
DEFINE_PRIM(_VOID, cmd_set_stencil_reference, _CMD _I32 _I32);
DEFINE_PRIM(_VOID, cmd_bind_pipeline, _CMD _I32 _PIPELINE);
#define _NATIVE_BUFFER _ABSTRACT(vk_native_buffer)
DEFINE_PRIM(_VOID, cmd_bind_vertex_buffers2, _CMD _I32 _I32 _CARRAY _CARRAY _CARRAY _CARRAY);
DEFINE_PRIM(_VOID, cmd_bind_index_buffer, _CMD _NATIVE_BUFFER _I64 _I32);
DEFINE_PRIM(_VOID, cmd_draw_indexed, _CMD _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, cmd_draw_indexed_indirect, _CMD _NATIVE_BUFFER _I64 _I32 _I32);
DEFINE_PRIM(_VOID, cmd_draw_indexed_indirect_count, _CMD _NATIVE_BUFFER _I64 _NATIVE_BUFFER _I64 _I32 _I32);
DEFINE_PRIM(_VOID, cmd_dispatch, _CMD _I32 _I32 _I32);
DEFINE_PRIM(_VOID, cmd_copy_buffer2, _CMD _STRUCT);
DEFINE_PRIM(_VOID, cmd_copy_image2, _CMD _STRUCT);
DEFINE_PRIM(_VOID, cmd_copy_buffer_to_image2, _CMD _STRUCT);
DEFINE_PRIM(_VOID, cmd_copy_image_to_buffer2, _CMD _STRUCT);
DEFINE_PRIM(_VOID, cmd_blit_image2, _CMD _STRUCT);
DEFINE_PRIM(_VOID, cmd_fill_buffer, _CMD _NATIVE_BUFFER _I64 _I64 _I32);
DEFINE_PRIM(_VOID, cmd_begin_label, _CMD _BYTES);
DEFINE_PRIM(_VOID, cmd_end_label, _CMD);

HL_PRIM void HL_NAME(cmd_draw)( VkCommandBuffer cmd, int vertexCount, int instanceCount, int firstVertex, int firstInstance ) {
	vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

HL_PRIM void HL_NAME(cmd_draw_indirect)( VkCommandBuffer cmd, VkBuffer b, int64 offset, int drawCount, int stride ) {
	vkCmdDrawIndirect(cmd, b, (VkDeviceSize)offset, drawCount, stride);
}

HL_PRIM void HL_NAME(cmd_draw_indirect_count)( VkCommandBuffer cmd, VkBuffer b, int64 offset, VkBuffer count, int64 countOffset, int maxDrawCount, int stride ) {
	vkCmdDrawIndirectCount(cmd, b, (VkDeviceSize)offset, count, (VkDeviceSize)countOffset, maxDrawCount, stride);
}

HL_PRIM void HL_NAME(cmd_dispatch_indirect)( VkCommandBuffer cmd, VkBuffer b, int64 offset ) {
	vkCmdDispatchIndirect(cmd, b, (VkDeviceSize)offset);
}

HL_PRIM void HL_NAME(cmd_resolve_image2)( VkCommandBuffer cmd, VkResolveImageInfo2 *inf ) {
	vkCmdResolveImage2(cmd, inf);
}

HL_PRIM void HL_NAME(cmd_clear_color_image)( VkCommandBuffer cmd, VkImage image, int layout, VkClearColorValue *color, int rangeCount, VkImageSubresourceRange *ranges ) {
	vkCmdClearColorImage(cmd, image, (VkImageLayout)layout, color, rangeCount, ranges);
}

HL_PRIM void HL_NAME(cmd_clear_depth_stencil_image)( VkCommandBuffer cmd, VkImage image, int layout, VkClearDepthStencilValue *value, int rangeCount, VkImageSubresourceRange *ranges ) {
	vkCmdClearDepthStencilImage(cmd, image, (VkImageLayout)layout, value, rangeCount, ranges);
}

HL_PRIM void HL_NAME(cmd_update_buffer)( VkCommandBuffer cmd, VkBuffer b, int64 offset, int64 size, vbyte *data ) {
	vkCmdUpdateBuffer(cmd, b, (VkDeviceSize)offset, (VkDeviceSize)size, data);
}

HL_PRIM void HL_NAME(cmd_set_depth_bias)( VkCommandBuffer cmd, double constant, double clamp, double slope ) {
	vkCmdSetDepthBias(cmd, (float)constant, (float)clamp, (float)slope);
}

HL_PRIM void HL_NAME(cmd_set_blend_constants)( VkCommandBuffer cmd, vbyte *constants ) {
	vkCmdSetBlendConstants(cmd, (float*)constants);
}

HL_PRIM void HL_NAME(cmd_set_cull_mode)( VkCommandBuffer cmd, int mode ) {
	vkCmdSetCullMode(cmd, (VkCullModeFlags)mode);
}

HL_PRIM void HL_NAME(cmd_set_front_face)( VkCommandBuffer cmd, int face ) {
	vkCmdSetFrontFace(cmd, (VkFrontFace)face);
}

HL_PRIM void HL_NAME(cmd_set_primitive_topology)( VkCommandBuffer cmd, int topology ) {
	vkCmdSetPrimitiveTopology(cmd, (VkPrimitiveTopology)topology);
}

HL_PRIM void HL_NAME(cmd_set_depth_test_enable)( VkCommandBuffer cmd, bool enable ) {
	vkCmdSetDepthTestEnable(cmd, enable);
}

HL_PRIM void HL_NAME(cmd_set_depth_write_enable)( VkCommandBuffer cmd, bool enable ) {
	vkCmdSetDepthWriteEnable(cmd, enable);
}

HL_PRIM void HL_NAME(cmd_set_depth_compare_op)( VkCommandBuffer cmd, int op ) {
	vkCmdSetDepthCompareOp(cmd, (VkCompareOp)op);
}

HL_PRIM void HL_NAME(cmd_set_depth_bias_enable)( VkCommandBuffer cmd, bool enable ) {
	vkCmdSetDepthBiasEnable(cmd, enable);
}

HL_PRIM void HL_NAME(cmd_set_depth_bounds_test_enable)( VkCommandBuffer cmd, bool enable ) {
	vkCmdSetDepthBoundsTestEnable(cmd, enable);
}

HL_PRIM void HL_NAME(cmd_set_depth_bounds)( VkCommandBuffer cmd, double minBounds, double maxBounds ) {
	vkCmdSetDepthBounds(cmd, (float)minBounds, (float)maxBounds);
}

HL_PRIM void HL_NAME(cmd_set_stencil_test_enable)( VkCommandBuffer cmd, bool enable ) {
	vkCmdSetStencilTestEnable(cmd, enable);
}

HL_PRIM void HL_NAME(cmd_set_stencil_op)( VkCommandBuffer cmd, int faceMask, int failOp, int passOp, int depthFailOp, int compareOp ) {
	vkCmdSetStencilOp(cmd, (VkStencilFaceFlags)faceMask, (VkStencilOp)failOp, (VkStencilOp)passOp, (VkStencilOp)depthFailOp, (VkCompareOp)compareOp);
}

HL_PRIM void HL_NAME(cmd_set_stencil_compare_mask)( VkCommandBuffer cmd, int faceMask, int mask ) {
	vkCmdSetStencilCompareMask(cmd, (VkStencilFaceFlags)faceMask, (uint32_t)mask);
}

HL_PRIM void HL_NAME(cmd_set_stencil_write_mask)( VkCommandBuffer cmd, int faceMask, int mask ) {
	vkCmdSetStencilWriteMask(cmd, (VkStencilFaceFlags)faceMask, (uint32_t)mask);
}

HL_PRIM void HL_NAME(cmd_set_line_width)( VkCommandBuffer cmd, double width ) {
	vkCmdSetLineWidth(cmd, (float)width);
}

HL_PRIM void HL_NAME(cmd_set_rasterizer_discard_enable)( VkCommandBuffer cmd, bool enable ) {
	vkCmdSetRasterizerDiscardEnable(cmd, enable);
}

HL_PRIM void HL_NAME(cmd_set_primitive_restart_enable)( VkCommandBuffer cmd, bool enable ) {
	vkCmdSetPrimitiveRestartEnable(cmd, enable);
}

HL_PRIM int64 HL_NAME(get_buffer_device_address)( VkBufferDeviceAddressInfo *inf ) {
	return (int64)vkGetBufferDeviceAddress(ctx.device, inf);
}

HL_PRIM void HL_NAME(cmd_execute_commands)( VkCommandBuffer cmd, int count, VkCommandBuffer *commands ) {
	vkCmdExecuteCommands(cmd, count, commands);
}

HL_PRIM int HL_NAME(reset_command_buffer)( VkCommandBuffer cmd, int flags ) {
	return vkResetCommandBuffer(cmd, (VkCommandBufferResetFlags)flags);
}

HL_PRIM void HL_NAME(free_command_buffers)( VkCommandPool pool, int count, VkCommandBuffer *commands ) {
	vkFreeCommandBuffers(ctx.device, pool, count, commands);
}

HL_PRIM int HL_NAME(queue_wait_idle)( VkQueue queue ) {
	return vkQueueWaitIdle(queue);
}

DEFINE_PRIM(_VOID, cmd_draw, _CMD _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, cmd_draw_indirect, _CMD _NATIVE_BUFFER _I64 _I32 _I32);
DEFINE_PRIM(_VOID, cmd_draw_indirect_count, _CMD _NATIVE_BUFFER _I64 _NATIVE_BUFFER _I64 _I32 _I32);
DEFINE_PRIM(_VOID, cmd_dispatch_indirect, _CMD _NATIVE_BUFFER _I64);
DEFINE_PRIM(_VOID, cmd_resolve_image2, _CMD _STRUCT);
DEFINE_PRIM(_VOID, cmd_clear_color_image, _CMD _ABSTRACT(vk_native_image) _I32 _STRUCT _I32 _CARRAY);
DEFINE_PRIM(_VOID, cmd_clear_depth_stencil_image, _CMD _ABSTRACT(vk_native_image) _I32 _STRUCT _I32 _CARRAY);
DEFINE_PRIM(_VOID, cmd_update_buffer, _CMD _NATIVE_BUFFER _I64 _I64 _BYTES);
DEFINE_PRIM(_VOID, cmd_set_depth_bias, _CMD _F64 _F64 _F64);
DEFINE_PRIM(_VOID, cmd_set_blend_constants, _CMD _BYTES);
DEFINE_PRIM(_VOID, cmd_set_cull_mode, _CMD _I32);
DEFINE_PRIM(_VOID, cmd_set_front_face, _CMD _I32);
DEFINE_PRIM(_VOID, cmd_set_primitive_topology, _CMD _I32);
DEFINE_PRIM(_VOID, cmd_set_depth_test_enable, _CMD _BOOL);
DEFINE_PRIM(_VOID, cmd_set_depth_write_enable, _CMD _BOOL);
DEFINE_PRIM(_VOID, cmd_set_depth_compare_op, _CMD _I32);
DEFINE_PRIM(_VOID, cmd_set_depth_bias_enable, _CMD _BOOL);
DEFINE_PRIM(_VOID, cmd_set_depth_bounds_test_enable, _CMD _BOOL);
DEFINE_PRIM(_VOID, cmd_set_depth_bounds, _CMD _F64 _F64);
DEFINE_PRIM(_VOID, cmd_set_stencil_test_enable, _CMD _BOOL);
DEFINE_PRIM(_VOID, cmd_set_stencil_op, _CMD _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, cmd_set_stencil_compare_mask, _CMD _I32 _I32);
DEFINE_PRIM(_VOID, cmd_set_stencil_write_mask, _CMD _I32 _I32);
DEFINE_PRIM(_VOID, cmd_set_line_width, _CMD _F64);
DEFINE_PRIM(_VOID, cmd_set_rasterizer_discard_enable, _CMD _BOOL);
DEFINE_PRIM(_VOID, cmd_set_primitive_restart_enable, _CMD _BOOL);
DEFINE_PRIM(_I64, get_buffer_device_address, _STRUCT);
DEFINE_PRIM(_VOID, cmd_execute_commands, _CMD _I32 _CARRAY);
DEFINE_PRIM(_I32, reset_command_buffer, _CMD _I32);
DEFINE_PRIM(_VOID, free_command_buffers, _CMD_POOL _I32 _CARRAY);
DEFINE_PRIM(_I32, queue_wait_idle, _QUEUE);

HL_PRIM VkQueryPool HL_NAME(query_pool_create)( int queryType, int count, int pipelineStatistics, int *result ) {
	VkQueryPoolCreateInfo inf = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
	inf.queryType = (VkQueryType)queryType;
	inf.queryCount = count;
	inf.pipelineStatistics = (VkQueryPipelineStatisticFlags)pipelineStatistics;
	VkQueryPool p = VK_NULL_HANDLE;
	*result = vkCreateQueryPool(ctx.device, &inf, NULL, &p);
	return p;
}

HL_PRIM void HL_NAME(query_pool_destroy)( VkQueryPool p ) {
	vkDestroyQueryPool(ctx.device, p, NULL);
}

HL_PRIM void HL_NAME(cmd_reset_query_pool)( VkCommandBuffer cmd, VkQueryPool p, int first, int count ) {
	vkCmdResetQueryPool(cmd, p, first, count);
}

HL_PRIM void HL_NAME(cmd_begin_query)( VkCommandBuffer cmd, VkQueryPool p, int query, int flags ) {
	vkCmdBeginQuery(cmd, p, query, (VkQueryControlFlags)flags);
}

HL_PRIM void HL_NAME(cmd_end_query)( VkCommandBuffer cmd, VkQueryPool p, int query ) {
	vkCmdEndQuery(cmd, p, query);
}

HL_PRIM void HL_NAME(cmd_copy_query_pool_results)( VkCommandBuffer cmd, VkQueryPool p, int first, int count, VkBuffer dst, int64 dstOffset, int64 stride, int flags ) {
	vkCmdCopyQueryPoolResults(cmd, p, first, count, dst, (VkDeviceSize)dstOffset, (VkDeviceSize)stride, (VkQueryResultFlags)flags);
}

HL_PRIM void HL_NAME(reset_query_pool)( VkQueryPool p, int first, int count ) {
	vkResetQueryPool(ctx.device, p, first, count);
}

HL_PRIM void HL_NAME(cmd_write_timestamp2)( VkCommandBuffer cmd, int64 stage, VkQueryPool p, int index ) {
	vkCmdWriteTimestamp2(cmd, (VkPipelineStageFlags2)stage, p, index);
}

HL_PRIM int HL_NAME(get_query_pool_results)( VkQueryPool p, int first, int count, int dataSize, vbyte *data, int64 stride, int flags ) {
	return vkGetQueryPoolResults(ctx.device, p, first, count, (size_t)dataSize, data, (VkDeviceSize)stride, (VkQueryResultFlags)flags);
}

#define _QUERY_POOL _ABSTRACT(vk_query_pool)
DEFINE_PRIM(_QUERY_POOL, query_pool_create, _I32 _I32 _I32 _REF(_I32));
DEFINE_PRIM(_VOID, query_pool_destroy, _QUERY_POOL);
DEFINE_PRIM(_VOID, cmd_reset_query_pool, _CMD _QUERY_POOL _I32 _I32);
DEFINE_PRIM(_VOID, cmd_write_timestamp2, _CMD _I64 _QUERY_POOL _I32);
DEFINE_PRIM(_VOID, cmd_begin_query, _CMD _QUERY_POOL _I32 _I32);
DEFINE_PRIM(_VOID, cmd_end_query, _CMD _QUERY_POOL _I32);
DEFINE_PRIM(_VOID, cmd_copy_query_pool_results, _CMD _QUERY_POOL _I32 _I32 _NATIVE_BUFFER _I64 _I64 _I32);
DEFINE_PRIM(_VOID, reset_query_pool, _QUERY_POOL _I32 _I32);
DEFINE_PRIM(_I32, get_query_pool_results, _QUERY_POOL _I32 _I32 _I32 _BYTES _I64 _I32);

#ifdef HL_WIN

typedef struct {
	IDxcLibrary *library;
	IDxcCompiler *compiler;
} vk_compiler;

HL_PRIM vk_compiler *HL_NAME(compiler_create)() {
	vk_compiler *comp = (vk_compiler*)hl_gc_alloc_raw(sizeof(vk_compiler));
	memset(comp,0,sizeof(vk_compiler));
	if( DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&comp->library)) != S_OK || DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&comp->compiler)) != S_OK )
		hl_error("Failed to create DXC compiler (dxcompiler.dll missing?)");
	return comp;
}

HL_PRIM vbyte *HL_NAME(compiler_compile)( vk_compiler *comp, uchar *source, uchar *profile, varray *args, int *dataLen ) {
	IDxcBlobEncoding *blob = NULL;
	IDxcOperationResult *result = NULL;
	comp->library->CreateBlobWithEncodingFromPinned(source,(int)ustrlen(source)*2,1200/*DXC_CP_UTF16*/,&blob);
	if( blob == NULL )
		hl_error("Could not create blob");
	hl_blocking(true);
	comp->compiler->Compile(blob,L"",L"main",profile,hl_aptr(args,LPCWSTR),args->size,NULL,0,NULL,&result);
	hl_blocking(false);
	HRESULT hr;
	result->GetStatus(&hr);
	if( !SUCCEEDED(hr) ) {
		IDxcBlobEncoding *error = NULL;
		result->GetErrorBuffer(&error);
		uchar *c = hl_to_utf16((char*)error->GetBufferPointer());
		blob->Release();
		result->Release();
		error->Release();
		hl_error("%s",c);
	}
	IDxcBlob *out = NULL;
	result->GetResult(&out);
	*dataLen = (int)out->GetBufferSize();
	vbyte *bytes = hl_copy_bytes((vbyte*)out->GetBufferPointer(), *dataLen);
	out->Release();
	blob->Release();
	result->Release();
	return bytes;
}

#else

typedef struct _vk_compiler vk_compiler;

HL_PRIM vk_compiler *HL_NAME(compiler_create)() {
	return NULL;
}

HL_PRIM vbyte *HL_NAME(compiler_compile)( vk_compiler *comp, uchar *source, uchar *profile, varray *args, int *dataLen ) {
	hl_error("No shader compiler available on this platform");
	return NULL;
}

#endif

#define _COMPILER _ABSTRACT(vk_compiler)
DEFINE_PRIM(_COMPILER, compiler_create, _NO_ARG);
DEFINE_PRIM(_BYTES, compiler_compile, _COMPILER _BYTES _BYTES _ARR _REF(_I32));
