#include "glfw3webgpu.h"

#if defined(__EMSCRIPTEN__)
#define WGPU_TARGET_EMSCRIPTEN 1
#elif defined(_WIN32)
#define WGPU_TARGET_WINDOWS 1
#elif defined(__APPLE__)
#define WGPU_TARGET_MACOS 1
#elif defined(_GLFW_WAYLAND)
#define WGPU_TARGET_LINUX_WAYLAND 1
#else
#define WGPU_TARGET_LINUX_X11 1
#endif

#if defined(WGPU_TARGET_MACOS)
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

#if defined(WGPU_TARGET_MACOS)
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(WGPU_TARGET_LINUX_X11)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(WGPU_TARGET_LINUX_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#elif defined(WGPU_TARGET_WINDOWS)
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#if !defined(__EMSCRIPTEN__)
#include <GLFW/glfw3native.h>
#endif

WGPUSurface glfwGetWGPUSurface(WGPUInstance instance, GLFWwindow* window) {
#if defined(WGPU_TARGET_MACOS)
	id metalLayer = [CAMetalLayer layer];
	NSWindow* nsWindow = glfwGetCocoaWindow(window);
	[nsWindow.contentView setWantsLayer:YES];
	[nsWindow.contentView setLayer:metalLayer];

	WGPUSurfaceSourceMetalLayer source;
	source.chain.next = NULL;
	source.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
	source.layer = metalLayer;

	WGPUSurfaceDescriptor desc;
	desc.nextInChain = &source.chain;
	desc.label.data = NULL;
	desc.label.length = 0;
	return wgpuInstanceCreateSurface(instance, &desc);
#elif defined(WGPU_TARGET_LINUX_X11)
	Display* x11Display = glfwGetX11Display();
	Window x11Window = glfwGetX11Window(window);

	WGPUSurfaceSourceXlibWindow source;
	source.chain.next = NULL;
	source.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
	source.display = x11Display;
	source.window = x11Window;

	WGPUSurfaceDescriptor desc;
	desc.nextInChain = &source.chain;
	desc.label.data = NULL;
	desc.label.length = 0;
	return wgpuInstanceCreateSurface(instance, &desc);
#elif defined(WGPU_TARGET_LINUX_WAYLAND)
	struct wl_display* waylandDisplay = glfwGetWaylandDisplay();
	struct wl_surface* waylandSurface = glfwGetWaylandWindow(window);

	WGPUSurfaceSourceWaylandSurface source;
	source.chain.next = NULL;
	source.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
	source.display = waylandDisplay;
	source.surface = waylandSurface;

	WGPUSurfaceDescriptor desc;
	desc.nextInChain = &source.chain;
	desc.label.data = NULL;
	desc.label.length = 0;
	return wgpuInstanceCreateSurface(instance, &desc);
#elif defined(WGPU_TARGET_WINDOWS)
	HWND hwnd = glfwGetWin32Window(window);
	HINSTANCE hinstance = GetModuleHandle(NULL);

	WGPUSurfaceSourceWindowsHWND source;
	source.chain.next = NULL;
	source.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
	source.hinstance = hinstance;
	source.hwnd = hwnd;

	WGPUSurfaceDescriptor desc;
	desc.nextInChain = &source.chain;
	desc.label.data = NULL;
	desc.label.length = 0;
	return wgpuInstanceCreateSurface(instance, &desc);
#elif defined(WGPU_TARGET_EMSCRIPTEN)
	WGPUEmscriptenSurfaceSourceCanvasHTMLSelector source;
	source.chain.next = NULL;
	source.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
	source.selector.data = "#canvas";
	source.selector.length = WGPU_STRLEN;

	WGPUSurfaceDescriptor desc;
	desc.nextInChain = &source.chain;
	desc.label.data = NULL;
	desc.label.length = 0;
	return wgpuInstanceCreateSurface(instance, &desc);
#else
#error Unsupported platform for glfwGetWGPUSurface
#endif
}
