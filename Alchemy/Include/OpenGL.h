#pragma once

#include "OpenGLIncludes.h"
#include "OpenGLShader.h"
#include <vector>

class OpenGLVAO {
public:
	OpenGLVAO(void) { };
	OpenGLVAO(std::vector<std::vector<float>> vbos,
		std::vector<std::vector<unsigned int>> ebos);
	~OpenGLVAO(void);
	void initVAO(std::vector<std::vector<float>> vbos,
		std::vector<std::vector<unsigned int>> ebos);
	void setShader(Shader* shader) { m_pShader = shader; }
	Shader* getShader(void) { return m_pShader; }
	unsigned int* getVAO(void) { return vaoID; }

private:
	Shader *m_pShader;
	unsigned int m_iNumArrays;
	unsigned int vaoID[1];
	unsigned int vboID[1];
	unsigned int eboID[1];
};

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

	std::vector<OpenGLVAO*> vaos;

	unsigned int vaoID[1];
	unsigned int vboID[1];
	unsigned int eboID[1];

protected:
	HGLRC m_renderContext;
	HDC m_deviceContext;
	HWND m_windowID;
};
