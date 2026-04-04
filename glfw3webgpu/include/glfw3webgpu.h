#ifndef GLFW3_WEBGPU_H
#define GLFW3_WEBGPU_H

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C" {
#endif

WGPUSurface glfwGetWGPUSurface(WGPUInstance instance, GLFWwindow* window);

#ifdef __cplusplus
}
#endif

#endif
