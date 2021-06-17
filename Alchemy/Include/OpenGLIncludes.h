#pragma once
// To compile statically linked TOGL, "glew32.lib" must be present in vcpkg\installed\x86-windows\lib for debug, and "glew32s.lib" for release.
// Debug builds cannot be compiled statically linked.
// You can do this by running ./vcpkg install glew:x86-windows-static and ./vcpkg install glew:x86-windows in your vcpkg folder.
#ifndef DEBUG
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <GL/wglew.h>
//#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib")

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
