#pragma once

#include "OpenGLIncludes.h"
#include "OpenGLShader.h"
#include <vector>

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
	void prepSquareCanvas();
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
	unsigned int eboID[1];

protected:
	HGLRC m_renderContext;
	HDC m_deviceContext;
	HWND m_windowID;
};

class OpenGLVAO {
public:
	OpenGLVAO(void) { };
	OpenGLVAO(std::vector<std::vector<float>> vbos,
		std::vector<std::vector<unsigned int>> ebos);
	~OpenGLVAO(void) { };
	void initVAO(std::vector<std::vector<float>> vbos,
		std::vector<std::vector<unsigned int>> ebos);
	void setShader(Shader* shader) { m_pTestShader = shader; }

private:
	Shader *m_pTestShader;
	unsigned int vaoID[1];
	unsigned int vboID[1];
	unsigned int eboID[1];
};
