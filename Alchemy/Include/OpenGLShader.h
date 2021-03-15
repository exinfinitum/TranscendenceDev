#pragma once

#include "OpenGLIncludes.h"
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/GLU.h>

#include <string>

class OpenGLShader {
public:
	OpenGLShader ();
	OpenGLShader (const char *vsFile, const char *fsFile);
	~OpenGLShader ();

	void Init (const char *vsFile, const char *fsFile);

	void Bind () const;
	void Unbind () const;

	unsigned int Id () const;


private:
	unsigned int shader_id;
	unsigned int shader_vp;
	unsigned int shader_fp;
};

