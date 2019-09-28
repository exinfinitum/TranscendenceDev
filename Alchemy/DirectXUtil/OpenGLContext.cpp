#include "OpenGL.h"
#include "PreComp.h"
#define OPENGL_COLOR_BITS 32;
#define OPENGL_DEPTH_BITS 32;

OpenGLContext::OpenGLContext(HWND hwnd) {
	initOpenGL(hwnd, GetDC(hwnd));
}

bool OpenGLContext::initOpenGL(HWND hwnd, HDC hdc)
{
	this->m_windowID = hwnd;
	m_deviceContext = hdc;
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int iPixelFormat = ChoosePixelFormat(m_deviceContext, &pfd);
	if (iPixelFormat == 0)
		{
		::kernelDebugLogPattern("[OpenGL] Failed to choose pixel format.");
		return false;
		}
	if (!SetPixelFormat(m_deviceContext, iPixelFormat, &pfd))
		{
		::kernelDebugLogPattern("[OpenGL] Failed to set pixel format");
		return false;
		}
	// Create an OpenGL 2.1 render context so we can initialize GLEW
	HGLRC tempOpenGLContext = wglCreateContext(m_deviceContext);
	wglMakeCurrent(m_deviceContext, tempOpenGLContext);
	if (glewInit() != GLEW_OK)
		{
		::kernelDebugLogPattern("[OpenGL] Failed to initialize GLEW.");
		return false;
		}

	int attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};
	if (wglewIsSupported("WGL_ARB_create_context") == 1) {
		m_renderContext = wglCreateContextAttribsARB(m_deviceContext, NULL, attributes);
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(tempOpenGLContext);
		wglMakeCurrent(m_deviceContext, m_renderContext);
	}
	else {
		m_renderContext = tempOpenGLContext;
	}


	const GLubyte *glVersionString = glGetString(GL_VERSION);
	int glVersion[2] = { -1, -1 };
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

	::kernelDebugLogPattern("OpenGL successfully initialized, version: %d.%d", glVersion[0], glVersion[1]);

	return true;
	}

void OpenGLContext::reshapeWindow(int w, int h) {
	m_iWindowWidth = w;
	m_iWindowHeight = h;
}

void OpenGLContext::swapBuffers()
{
	SwapBuffers(m_deviceContext);
}