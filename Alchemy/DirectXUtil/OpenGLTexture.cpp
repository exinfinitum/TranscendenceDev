#include "OpenGL.h"
#include "PreComp.h"

OpenGLTexture::OpenGLTexture(void* texture, int width, int height)
	{
	initTexture2D(texture, width, height);
	}

void OpenGLTexture::initTexture2D(void* texture, int width, int height)
	{
	glGenTextures(1, &m_pTextureID[0]);
	glBindTexture(GL_TEXTURE_2D, m_pTextureID[0]);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, texture);
	// glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	}

void OpenGLTexture::updateTexture2D(void* texture, int width, int height)
	{
	glBindTexture(GL_TEXTURE_2D, m_pTextureID[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, texture);
	}

OpenGLTexture::~OpenGLTexture()
	{
	glDeleteTextures(1, &m_pTextureID[0]);
	}


