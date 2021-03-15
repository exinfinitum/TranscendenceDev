#include "PreComp.h"

#include "OpenGLShader.h"
#include <string.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>

using namespace std;

string textFileRead (const char *fileName) {
	ifstream in(fileName);
	string contents((istreambuf_iterator<char>(in)),
		istreambuf_iterator<char>());
	return contents;
}

OpenGLShader::OpenGLShader () {

}

OpenGLShader::OpenGLShader(const std::string vertexCode, const std::string fragmentCode) {
	Init(vertexCode, fragmentCode);
}

OpenGLShader::OpenGLShader (const char *vsFile, const char *fsFile) {
	InitFromFile(vsFile, fsFile);
}

void OpenGLShader::Init(const std::string vertexCode, const std::string fragmentCode) {
	char err_log[512];
	shader_vp = glCreateShader(GL_VERTEX_SHADER);
	shader_fp = glCreateShader(GL_FRAGMENT_SHADER);
	const char* vsText = vertexCode.c_str();
	const char* fsText = fragmentCode.c_str();

	glShaderSource(shader_vp, 1, &vsText, 0);
	glShaderSource(shader_fp, 1, &fsText, 0);

	int success;
	glCompileShader(shader_vp);
	glGetShaderiv(shader_vp, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader_vp, 512, NULL, err_log);
		string error_msg = ("Vertex shader failed to compile!\n" + string(err_log));
		::kernelDebugLogPattern(CString(error_msg.c_str()));
		throw(runtime_error(error_msg));
	};
	glCompileShader(shader_fp);
	glGetShaderiv(shader_fp, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader_fp, 512, NULL, err_log);
		string error_msg = ("Fragment shader failed to compile!\n" + string(err_log));
		::kernelDebugLogPattern(CString(error_msg.c_str()));
		throw(runtime_error(error_msg));
	};

	shader_id = glCreateProgram();
	glBindAttribLocation(shader_id, 0, "in_Position"); // Bind a constant attribute location for positions of vertices
	glBindAttribLocation(shader_id, 1, "in_Color"); // Bind another constant attribute location, this time for color
	glAttachShader(shader_id, shader_fp);
	glAttachShader(shader_id, shader_vp);
	glLinkProgram(shader_id);
}

void OpenGLShader::InitFromFile (const char *vsFile, const char *fsFile) {
	char err_log[512];
	shader_vp = glCreateShader(GL_VERTEX_SHADER);
	shader_fp = glCreateShader(GL_FRAGMENT_SHADER);

	// We must get the strings into separate local variables first,
	// otherwise we'll get garbage characters in our const char*s
	// See https://stackoverflow.com/questions/27627413/why-does-calling-c-str-on-a-function-that-returns-a-string-not-work
	string vsString = textFileRead(vsFile);
	string fsString = textFileRead(fsFile);
	const char* vsText = vsString.c_str();
	const char* fsText = fsString.c_str();

	if (vsText == NULL || fsText == NULL) {
		::kernelDebugLogPattern("Either vertex shader or fragment shader file not found.");
		return;
	}

	glShaderSource(shader_vp, 1, &vsText, 0);
	glShaderSource(shader_fp, 1, &fsText, 0);

	int success;
	glCompileShader(shader_vp);
	glGetShaderiv(shader_vp, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader_vp, 512, NULL, err_log);
		string error_msg = (string(vsFile) + ": Vertex shader failed to compile!\n" + string(err_log));
		::kernelDebugLogPattern(CString(error_msg.c_str()));
		throw(runtime_error(error_msg));
	};
	glCompileShader(shader_fp);
	glGetShaderiv(shader_fp, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader_fp, 512, NULL, err_log);
		string error_msg = (string(fsFile) + ": Fragment shader failed to compile!\n" + string(err_log));
		::kernelDebugLogPattern(CString(error_msg.c_str()));
		throw(runtime_error(error_msg));
	};

	shader_id = glCreateProgram();
	glBindAttribLocation(shader_id, 0, "in_Position"); // Bind a constant attribute location for positions of vertices
	glBindAttribLocation(shader_id, 1, "in_Color"); // Bind another constant attribute location, this time for color
	glAttachShader(shader_id, shader_fp);
	glAttachShader(shader_id, shader_vp);
	glLinkProgram(shader_id);
}

OpenGLShader::~OpenGLShader () {
	glDetachShader(shader_id, shader_fp);
	glDetachShader(shader_id, shader_vp);

	glDeleteShader(shader_fp);
	glDeleteShader(shader_vp);
	glDeleteProgram(shader_id);
}

unsigned int OpenGLShader::Id () const {
	return shader_id;
}

void OpenGLShader::Bind () const {
	glUseProgram(shader_id);
}

void OpenGLShader::Unbind () const {
	glUseProgram(0);
}

