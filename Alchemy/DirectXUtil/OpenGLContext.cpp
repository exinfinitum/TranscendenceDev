#include "OpenGL.h"
#include "PreComp.h"
#define OPENGL_COLOR_BITS 32;
#define OPENGL_DEPTH_BITS 32;

OpenGLContext::OpenGLContext (HWND hwnd) :
	m_iWindowWidth(0),
	m_iWindowHeight(0),
	m_bResized(false)
{
}

bool OpenGLContext::initOpenGL (HWND hwnd, HDC hdc)
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
	::kernelDebugLogPattern("[OpenGL] Initializing...");

	int iPixelFormat = ChoosePixelFormat(m_deviceContext, &pfd);
	if (iPixelFormat == 0)
		{
		::kernelDebugLogPattern("[OpenGL] Failed to choose pixel format.");
		getWGLError();
		return false;
		}
	if (!SetPixelFormat(m_deviceContext, iPixelFormat, &pfd))
		{
		::kernelDebugLogPattern("[OpenGL] Failed to set pixel format");
		getWGLError();
		return false;
		}
	// Create an OpenGL 2.1 render context so we can initialize GLEW
	HGLRC tempOpenGLContext = wglCreateContext(m_deviceContext);
	if (!wglMakeCurrent(m_deviceContext, tempOpenGLContext))
		{
		::kernelDebugLogPattern("[OpenGL] Failed to make temp OpenGL context current");
		getWGLError();
		}

	int attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		// enable hardware MSAA
		//WGL_SAMPLE_BUFFERS_ARB, TRUE,
		//WGL_SAMPLES_ARB, 4,
		0
	};

	GLenum glewErr = glewInit();
	if (glewErr != GLEW_OK)
	{
		::kernelDebugLogPattern("[OpenGL] Failed to initialize GLEW. Error: %s", CString((LPCSTR)glewGetErrorString(glewErr)));
		return false;
	}

	if (wglewIsSupported("WGL_ARB_create_context") == 1) {
		m_renderContext = wglCreateContextAttribsARB(m_deviceContext, NULL, attributes);
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(tempOpenGLContext);
		if (!wglMakeCurrent(m_deviceContext, m_renderContext))
			{
			::kernelDebugLogPattern("[OpenGL] Failed to make OpenGL context current");
			getWGLError();
			}
	}
	else {
		m_renderContext = tempOpenGLContext;
		::kernelDebugLogPattern("[OpenGL] Unable to use WGL_ARB_create_context, falling back to 2.1 render context...");
	}

	const GLubyte *glVersionString = glGetString(GL_VERSION);
	int glVersion[2] = { -1, -1 };
	glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
	glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

	::kernelDebugLogPattern("[OpenGL] OpenGL successfully initialized, version: %d.%d", glVersion[0], glVersion[1]);
	int iMaxTexturesPerShader, iMaxTextures, iMaxVertexAttribs;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &iMaxTexturesPerShader);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_iMaxTextureSize);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &iMaxTextures);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &iMaxVertexAttribs);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	::kernelDebugLogPattern("[OpenGL] Using graphics device: %s", CString((LPCSTR)renderer));
	::kernelDebugLogPattern("[OpenGL] Maximum %d textures at resolution %dx%d, max %d per shader", iMaxTextures, m_iMaxTextureSize, m_iMaxTextureSize, iMaxTexturesPerShader);
	::kernelDebugLogPattern("[OpenGL] Maximum %d vertex attributes", iMaxVertexAttribs);
	if (iMaxVertexAttribs < 16) {
		throw CException(ERR_FAIL, CONSTLIT("[OpenGL] Unable to use OpenGL on this machine, device must support at least 16 vertex attributes."));
	}
	prepSquareCanvas();
	setBlendMode();

	//glEnable(GL_MULTISAMPLE);
	return true;
	}

void OpenGLContext::setBlendMode ()
	{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// TODO: Make this a function in the instanced render queue.
	glBlendEquation(GL_FUNC_ADD); // Requires KHR_blend_equation_advanced; https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_blend_equation_advanced.txt

	}

void OpenGLContext::prepSquareCanvas ()
{
	// Prepare the background canvas.
	//OpenGLShader* pTestShader = new OpenGLShader("./shaders/test_vertex_shader.glsl", "./shaders/test_fragment_shader.glsl");
	m_pCanvasShader = std::make_unique<OpenGLShader>("./shaders/texture_vertex_shader.glsl", "./shaders/texture_fragment_shader.glsl");
	float fSize = 1.0f;
	float posZ = 0.999999f;

	std::vector<float> vertices {
		fSize, fSize, posZ,
		fSize, -fSize, posZ,
		-fSize, -fSize, posZ,
		-fSize, fSize, posZ,
	};
	
	std::vector<float> colors {
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f
	};

	std::vector<float> texcoords{
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f,
	};

	std::vector<unsigned int> indices {
		0, 1, 3,
		1, 2, 3
	};

	std::vector<std::vector<float>> vbos{ vertices, colors };
	std::vector<std::vector<float>> texcoord_arr{ texcoords };
	std::vector<std::vector<unsigned int>> ebos{ indices, indices, indices };

	OpenGLVAO* vao = new OpenGLVAO(vbos, ebos, texcoord_arr);
	vaos.push_back(vao);

	}

void OpenGLContext::prepTestScene ()
	{
	m_pTestShader = new OpenGLShader("./shaders/test_vertex_shader.glsl", "./shaders/test_fragment_shader.glsl");
	// Create our square
	// Declare vertices for our square - 18 floats (6 vertices, xyz for each)
	float* vertices = new float[18];
	float* colors = new float[18];
	float square_dimensions = 1.0;

	// Fill out our array of vertices
	vertices[0] = -square_dimensions; vertices[1] = -square_dimensions; vertices[2] = 0.0; // Bottom left corner
	vertices[3] = -square_dimensions; vertices[4] = square_dimensions; vertices[5] = 0.0; // Top left corner
	vertices[6] = square_dimensions; vertices[7] = square_dimensions; vertices[8] = 0.0; // Top Right corner
	vertices[9] = square_dimensions; vertices[10] = -square_dimensions; vertices[11] = 0.0; // Bottom right corner
	vertices[12] = -square_dimensions; vertices[13] = -square_dimensions; vertices[14] = 0.0; // Bottom left corner
	vertices[15] = square_dimensions; vertices[16] = square_dimensions; vertices[17] = 0.0; // Top Right corner

																							// Fill out our array of colors
	colors[0] = 1.0; colors[1] = 1.0; colors[2] = 1.0; // Bottom left corner
	colors[3] = 1.0; colors[4] = 0.0; colors[5] = 0.0; // Top left corner
	colors[6] = 0.0; colors[7] = 1.0; colors[8] = 0.0; // Top Right corner
	colors[9] = 0.0; colors[10] = 0.0; colors[11] = 1.0; // Bottom right corner
	colors[12] = 1.0; colors[13] = 1.0; colors[14] = 1.0; // Bottom left corner
	colors[15] = 0.0; colors[16] = 1.0; colors[17] = 0.0; // Top Right corner

														  // Now that the vertices are filled in, we need to create a VAO with a call to
														  // glGenVertexArrays, then bind it so we can use it.
														  // Generate 2 vertex arrays, one for colors and one for vertices
	glGenVertexArrays(1, &vaoID[0]);
	glBindVertexArray(vaoID[0]);

	// Generate and bind our Vertex Buffer Objects.
	glGenBuffers(2, vboID);
	glBindBuffer(GL_ARRAY_BUFFER, vboID[0]);

	// Now, fill the size and data of our VBO and set it to static_draw.
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	// Set up our vertex attribute pointer.
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Repeat for colors.
	glBindBuffer(GL_ARRAY_BUFFER, vboID[1]); // Bind our second Vertex Buffer Object
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(GLfloat), colors, GL_STATIC_DRAW); // Set the size and data of our VBO and set it to STATIC_DRAW
	glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0); // Set up our vertex attributes pointer
	glEnableVertexAttribArray(1); // Enable the second vertex attribute array

								  // Disable our VAO and VBO. In that order.
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	// Delete our vertices once we're done with them
	// Unlike machine learning/CUDA applications, the vertices go straight from GPU to our screen
	// so no use keeping them here in our memory
	delete[] vertices;
	delete[] colors;
	}

void OpenGLContext::resize (int w, int h)
	{
	m_iWindowWidth = w;
	m_iWindowHeight = h;
	::kernelDebugLogPattern("Resolution change: %d, %d", m_iWindowWidth, m_iWindowHeight);
	}

void OpenGLContext::testRender ()
	{
	static float red = 0.0f;
	static float blue = 0.0f;
	static float green = 0.0f;
	glClearColor(red, green, blue, 0.0f);
	glViewport(0, 0, m_iWindowWidth, m_iWindowHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	red = fmod((red + 0.05f), 1.0f);
	blue = fmod((blue + 0.01f), 1.0f);
	green = fmod((green + 0.1f), 1.0f);
	}

void OpenGLContext::renderCanvasBackground ()
	{
	// TODO: Remove this function
	// Create our new shader
	
	// Set up our perspective matrix
	// glm::perspective is the equivalent of old gluPerspective
	// 10 deg FOV in y direction, 0.1 distance to near clipping plane, 100 distance to far clipping plane
	// Note, FOV is in radians!
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(90.0f), (float)m_iWindowWidth / (float)m_iWindowHeight, 0.1f, 100.0f);

	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	glViewport(0, 0, m_iWindowWidth, m_iWindowHeight); // Set the viewport size to fill the window
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear required buffers

																				// Set up model and view matrices
																				// glm::mat4(1.0f) seems to make an identity matrix?
																				// View matrix translates 5 units back into the scene

	glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	// Model matrix rotates by 45 degrees
	static float rotation = 0.0f;
	glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
	rotation += 5.0f;

	m_pCanvasShader->bind(); // Bind our shader
	// TODO: Put the rest of these thingies into the VAO class...
					// Get the location of the matrix variables inside our shaders
	int projectionMatrixLocation = glGetUniformLocation(m_pCanvasShader->id(), "projectionMatrix");
	int viewMatrixLocation = glGetUniformLocation(m_pCanvasShader->id(), "viewMatrix");
	int modelMatrixLocation = glGetUniformLocation(m_pCanvasShader->id(), "modelMatrix");

	// Send our matrices into the shader variables
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

	glBindVertexArray((vaos[0]->getVAO())[0]); // Bind our Vertex Array Object
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0); // Unbind our Vertex Array Object

	m_pCanvasShader->unbind(); // Unbind our shader
	}

void OpenGLContext::renderCanvasBackgroundFromTexture (OpenGLTexture* texture, float depth, bool clear)
{

	// Create our new shader

	// Set up our perspective matrix
	// glm::perspective is the equivalent of old gluPerspective
	// 10 deg FOV in y direction, 0.1 distance to near clipping plane, 100 distance to far clipping plane
	// Note, FOV is in radians!
	//glm::mat4 projectionMatrix = glm::perspective(glm::radians(90.0f), (float)m_iWindowWidth / (float)m_iWindowHeight, 0.1f, 100.0f);
	glm::mat4 projectionMatrix = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, 0.1f, 256.0f);

	if (clear) {
		glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
		glViewport(0, 0, m_iWindowWidth, m_iWindowHeight); // Set the viewport size to fill the window
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear required buffers
	}


	// Set up model and view matrices
	// glm::mat4(1.0f) seems to make an identity matrix?
	// View matrix translates 5 units back into the scene


	// Model matrix rotates by 45 degrees
	static float rotation = 180.0f;
	glm::mat4 rotationMatrix = glm::mat4(glm::vec4(-1.0, 0.0, 0.0, 0.0), glm::vec4(0.0, -1.0, 0.0, 0.0), glm::vec4(0.0, 0.0, 1.0, 0.0), glm::vec4(0.0, 0.0, 0.0, 1.0));


	m_pCanvasShader->bind(); // Bind our shader
					 // TODO: Put the rest of these thingies into the VAO class...
					 // Get the location of the matrix variables inside our shaders

	int rotationMatrixLocation = glGetUniformLocation(m_pCanvasShader->id(), "rotationMatrix");
	glUniform1i(glGetUniformLocation(m_pCanvasShader->id(), "ourTexture"), 0);
	glUniform1f(glGetUniformLocation(m_pCanvasShader->id(), "depth"), depth);

	// Send our matrices into the shader variables
	glUniformMatrix4fv(rotationMatrixLocation, 1, GL_FALSE, &rotationMatrix[0][0]);
	texture->bindTexture2D(GL_TEXTURE0);

	glBindVertexArray((vaos[0]->getVAO())[0]); // Bind our Vertex Array Object

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0); // Unbind our Vertex Array Object
	texture->unbindTexture2D();

	m_pCanvasShader->unbind(); // Unbind our shader

}

void OpenGLContext::swapBuffers (HWND hwnd)
	{
	HDC hDC = ::GetDC(hwnd);
	SwapBuffers(hDC);
	::ReleaseDC(hwnd, hDC);
	}

void OpenGLContext::getWGLError ()
	{
	LPCSTR lpMsgBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process
	::kernelDebugLogPattern("[OpenGL] Error code %d: %s", dw, CString(lpMsgBuf));
	}

void OpenGLContext::getWGLSwapError ()
	{
	LPCSTR lpMsgBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process
	::kernelDebugLogPattern("[OpenGL Swap] Error code %d: %s", dw, CString(lpMsgBuf));
	}
