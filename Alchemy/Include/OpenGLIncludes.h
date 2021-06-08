#pragma once
// To compile statically linked TOGL, "glew32.lib" must NOT be present in vcpkg\installed\x86-windows\lib - use "glew32s.lib" instead of "glew32.lib".
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/wglew.h>
//#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib")

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
