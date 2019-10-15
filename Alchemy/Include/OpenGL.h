#pragma once

#include "OpenGLIncludes.h"
#include "OpenGLShader.h"

class OpenGLContext {
public:
	OpenGLContext(void) { };
	OpenGLContext(HWND hwnd);
	~OpenGLContext(void) { };
	bool initOpenGL(HWND hwnd, HDC hdc);
	void setupQuads(void);
	void resize(int w, int h);
	void renderScene(void);
	void testRender();
	void testShaders();
	void prepTestScene();
	void swapBuffers(HWND hwnd);
	void getWGLError();
	void getWGLSwapError();

private:
	int m_iWindowWidth;
	int m_iWindowHeight;

	Shader *m_pTestShader;

	// Projection, view and model matrices respectively
	glm::mat4 m_pMatrix;
	glm::mat4 m_vMatrix;
	glm::mat4 m_mMatrix;

	unsigned int vaoID[1];
	unsigned int vboID[1];

protected:
	HGLRC m_renderContext;
	HDC m_deviceContext;
	HWND m_windowID;
};
