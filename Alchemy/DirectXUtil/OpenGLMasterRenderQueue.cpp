#include "OpenGL.h"
#include "PreComp.h"
#include <mutex>

const float OpenGLMasterRenderQueue::m_fDepthDelta = 0.000001f; // Up to one million different depth levels
const float OpenGLMasterRenderQueue::m_fDepthStart = 0.999998f; // Up to one million different depth levels

OpenGLMasterRenderQueue::OpenGLMasterRenderQueue(void)
{
	// Initialize the VAO.
	initializeVAO();
	initializeCanvasVAO();
	// Depth level starts at one minus the delta.
	m_fDepthLevel = m_fDepthStart - m_fDepthDelta;
	m_iCurrentTick = 0;
	// Initialize the FBO.
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);
	m_pGlowmapShader = new OpenGLShader("./shaders/glowmap_vertex_shader.glsl", "./shaders/glowmap_fragment_shader.glsl");
	m_pObjectTextureShader = new OpenGLShader("./shaders/instanced_vertex_shader.glsl", "./shaders/instanced_fragment_shader.glsl");
	m_pRayShader = new OpenGLShader("./shaders/ray_vertex_shader.glsl", "./shaders/ray_fragment_shader.glsl");
	m_pLightningShader = new OpenGLShader("./shaders/lightning_vertex_shader.glsl", "./shaders/lightning_fragment_shader.glsl");

}

OpenGLMasterRenderQueue::~OpenGLMasterRenderQueue(void)
{
	clear();
	deinitVAO();
	deinitCanvasVAO();
	// TODO: Delete render queues
	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &rbo);
	delete m_pGlowmapShader;
	delete m_pObjectTextureShader;
	delete m_pRayShader;
}

void OpenGLMasterRenderQueue::initializeCanvasVAO()
{
	// Prepare the background canvas.
	//OpenGLShader* pTestShader = new OpenGLShader("./shaders/test_vertex_shader.glsl", "./shaders/test_fragment_shader.glsl");

	float fSize = 1.0f;
	float posZ = 0.999999f;

	std::vector<float> vertices{
		fSize, fSize, posZ,
		fSize, -fSize, posZ,
		-fSize, -fSize, posZ,
		-fSize, fSize, posZ,
	};

	std::vector<float> colors{
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

	std::vector<unsigned int> indices{
		0, 1, 3,
		1, 2, 3
	};

	std::vector<std::vector<float>> vbos{ vertices, colors };
	std::vector<std::vector<float>> texcoord_arr{ texcoords };
	std::vector<std::vector<unsigned int>> ebos{ indices, indices, indices };

	m_pCanvasVAO = new OpenGLVAO(vbos, ebos, texcoord_arr);
	m_pCanvasVAO->setShader(m_pGlowmapShader);

}

void OpenGLMasterRenderQueue::deinitCanvasVAO(void)
{
	// TODO(heliogenesis): Move this VAO to the parent class once it's done
	unsigned int *canvasVAO = m_pCanvasVAO->getinstancedVBO();
	glDeleteBuffers(16, &canvasVAO[0]);
	delete[] m_pCanvasVAO;
}

void OpenGLMasterRenderQueue::deinitVAO(void)
{
	// TODO(heliogenesis): Move this VAO to the parent class once it's done
	unsigned int *instancedVBO = m_pVao->getinstancedVBO();
	glDeleteBuffers(16, &instancedVBO[0]);
	delete[] m_pVao;
}

void OpenGLMasterRenderQueue::addShipToRenderQueue(int startPixelX, int startPixelY, int sizePixelX, int sizePixelY, int posPixelX,
	int posPixelY, int canvasHeight, int canvasWidth, OpenGLTexture *image, int texWidth, int texHeight, int texQuadWidth, int texQuadHeight,
	float alphaStrength, float glowR, float glowG, float glowB, float glowA, float glowNoise)
	{
	// Note, image is a pointer to the CG32bitPixel* we want to use as a texture. We can use CG32bitPixel->GetPixelArray() to get this pointer.
	// To get the width and height, we can use pSource->GetWidth() and pSource->GetHeight() respectively.
	// TODO: This can be called from different threads; we need to make sure that OGL textures are only initialized on a single thread
	// Note, the texture doesn't need to be initialized until we render, although the texture must be initialized when we generate glow maps
	// We only need the glow maps at render time, not at add to queue time, so we can defer generation until render time
	//m_texturesNeedingInitialization.push_back()

	// Initialize the texture if necessary; we do this here because all OpenGL calls must be made on the same thread
//	image->initTextureFromOpenGLThread();

	// Generate a glow map for this texture if needed.
	// TODO: Store the texture quad width and height on the OpenGLTexture object temporarily, so glowmap can be created later
//	if (!image->getGlowMap()) {
		//image->GenerateGlowMap(fbo, m_pCanvasVAO, m_pGlowmapShader, glm::vec2(float(texQuadWidth), float(texQuadHeight)));
	//}

	const std::lock_guard<std::mutex> lock(m_shipRenderQueueAddMutex);

	// Check to see if we have a render queue with that texture already loaded.
	if (!m_shipRenderQueues.count(image))
		{
		// If we don't have a render queue with that texture loaded, then add one.
		m_shipRenderQueues[image] = new OpenGLInstancedBatchTexture();
		}
	// Add this quad to the render queue.
	glm::vec2 vTexPositions((float)startPixelX / (float)texWidth, (float)startPixelY / (float)texHeight);
	glm::vec2 vCanvasQuadSizes((float)sizePixelX / (float)canvasWidth, (float)sizePixelY / (float)canvasHeight);
	glm::vec2 vCanvasPositions((float)posPixelX / (float)canvasWidth, (float)posPixelY / (float)canvasHeight);
	glm::vec2 vTextureQuadSizes((float)texQuadWidth / (float)texWidth, (float)texQuadHeight / (float)texHeight);
	glm::vec4 glowColor(glowR, glowG, glowB, glowA);

	m_shipRenderQueues[image]->addObjToRender(vTexPositions, vCanvasQuadSizes, vCanvasPositions, vTextureQuadSizes, alphaStrength, glowColor, glowNoise);
	//m_shipRenderQueues[image]->addObjToRender(startPixelX, startPixelY, sizePixelX, sizePixelY, posPixelX, posPixelY, canvasHeight, canvasWidth, texHeight, texWidth, texQuadWidth, texQuadHeight, alphaStrength, glow, glowNoise);
	}

void OpenGLMasterRenderQueue::addRayToEffectRenderQueue(int posPixelX, int posPixelY, int sizePixelX, int sizePixelY, int canvasSizeX, int canvasSizeY, float rotation,
	                                                    int iColorTypes, int iOpacityTypes, int iWidthAdjType, int iReshape, int iTexture, std::tuple<int, int, int> primaryColor,
	                                                    std::tuple<int, int, int> secondaryColor, int iIntensity, float waveCyclePos, int opacityAdj)
	{
	glm::vec3 vPrimaryColor = glm::vec3(std::get<0>(primaryColor), std::get<1>(primaryColor), std::get<2>(primaryColor)) / float(255.0);
	glm::vec3 vSecondaryColor = glm::vec3(std::get<0>(secondaryColor), std::get<1>(secondaryColor), std::get<2>(secondaryColor)) / float(255.0);

	glm::vec4 sizeAndPosition((float)sizePixelX, (float)sizePixelY,
		(float)posPixelX / (float)canvasSizeX, (float)posPixelY / (float)canvasSizeY);
	glm::ivec2 shapes(iWidthAdjType, iReshape);
	glm::vec3 intensitiesAndCycles(float(iIntensity), waveCyclePos, float(opacityAdj) / 255.0f);
	glm::ivec3 styles(iColorTypes, iOpacityTypes, iTexture);

	m_effectRayRenderQueue.addObjToRender(sizeAndPosition, rotation, shapes, styles, intensitiesAndCycles, vPrimaryColor, vSecondaryColor);

	//m_effectRayRenderQueue->addObjToRender(sizePixelX, sizePixelY, posPixelX, posPixelY, canvasSizeX, canvasSizeY, rotation, iColorTypes, iOpacityTypes, iWidthAdjType, iReshape, iTexture,
	//	vPrimaryColor, vSecondaryColor, iIntensity, waveCyclePos, opacityAdj);
	//m_effectRayRenderQueue->addObjToRender(200, 200, 500, 500, 1024, 768, 0, 1, 1, 1, 1, 1,
		//glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 0.0, 1.0), 255, 0);
	}

void OpenGLMasterRenderQueue::addLightningToEffectRenderQueue(int posPixelX, int posPixelY, int sizePixelX, int sizePixelY, int canvasSizeX, int canvasSizeY, float rotation,
	int iWidthAdjType, int iReshape, std::tuple<int, int, int> primaryColor,
	std::tuple<int, int, int> secondaryColor, float seed)
{
	glm::vec3 vPrimaryColor = glm::vec3(std::get<0>(primaryColor), std::get<1>(primaryColor), std::get<2>(primaryColor)) / float(255.0);
	glm::vec3 vSecondaryColor = glm::vec3(std::get<0>(secondaryColor), std::get<1>(secondaryColor), std::get<2>(secondaryColor)) / float(255.0);
	glm::vec4 sizeAndPosition((float)sizePixelX, (float)sizePixelY,
		(float)posPixelX / (float)canvasSizeX, (float)posPixelY / (float)canvasSizeY);
	glm::ivec2 shapes(iWidthAdjType, iReshape);

	m_effectLightningRenderQueue.addObjToRender(sizeAndPosition, rotation, shapes, seed, vPrimaryColor, vSecondaryColor);
	//m_effectLightningRenderQueue->addObjToRender(sizePixelX, sizePixelY, posPixelX, posPixelY, canvasSizeX, canvasSizeY, rotation, iWidthAdjType, iReshape,
	//	vPrimaryColor, vSecondaryColor, seed);
	//m_effectRayRenderQueue->addObjToRender(200, 200, 500, 500, 1024, 768, 0, 1, 1, 1, 1, 1,
		//glm::vec3(1.0, 1.0, 1.0), glm::vec3(1.0, 0.0, 1.0), 255, 0);
}

void OpenGLMasterRenderQueue::renderAllQueues(void)
{
	// Delete textures scheduled for deletion.
	if (m_texturesForDeletion.size() > 0)
		for (const std::shared_ptr<OpenGLTexture> textureToDelete : m_texturesForDeletion) {
			// If any value exists here, delete it from m_shipRenderQueues if it exists there
			if (m_shipRenderQueues.find(textureToDelete.get()) != m_shipRenderQueues.end()) {
				m_shipRenderQueues.erase(textureToDelete.get());
			}
		}
	m_texturesForDeletion.clear();
	// For each render queue in the ships render queue, render that render queue. We need to set the texture and do a glBindTexture before doing so.
	for (const auto &p : m_shipRenderQueues)
	{
		OpenGLTexture *pTextureToUse = p.first;
		//OpenGLInstancedRenderQueue *pInstancedRenderQueue = p.second;
		OpenGLInstancedBatchTexture *pInstancedRenderQueue = p.second;
		// TODO: Set the depths here before rendering. This will ensure that we always render from back to front, which should solve most issues with blending.

		// Initialize the texture if necessary; we do this here because all OpenGL calls must be made on the same thread
		pTextureToUse->initTextureFromOpenGLThread();

		// Generate a glow map for this texture if needed.
		// TODO: Store the texture quad width and height on the OpenGLTexture object temporarily, so glowmap can be created later
		if (!pTextureToUse->getGlowMap()) {
			::kernelDebugLogPattern("[OpenGL] Creating glowmap for texture %d", pTextureToUse);
			pTextureToUse->GenerateGlowMap(fbo, m_pCanvasVAO, m_pGlowmapShader, glm::vec2(float(10), float(10)));
		}

		// If we can't get a glowmap then this texture is blank; do not render it
		if (pTextureToUse->getGlowMap()) {
			float depthLevel = m_fDepthLevel;
			std::array<std::string, 3> textureUniformNames = { "obj_texture", "glow_map", "current_tick" };
			pInstancedRenderQueue->setUniforms(textureUniformNames, pTextureToUse, pTextureToUse->getGlowMap(), m_iCurrentTick);
			pInstancedRenderQueue->Render(m_pObjectTextureShader, depthLevel, m_fDepthDelta, m_iCurrentTick);
			m_fDepthLevel = depthLevel;
		}
	}

	std::array<std::string, 2> rayAndLightningUniformNames = { "current_tick", "aCanvasAdjustedDimensions" };

	m_effectRayRenderQueue.setUniforms(rayAndLightningUniformNames, float(m_iCurrentTick), glm::ivec2(m_iCanvasWidth, m_iCanvasHeight));
	m_effectRayRenderQueue.Render(m_pRayShader, m_fDepthLevel, m_fDepthDelta, m_iCurrentTick);
	m_effectLightningRenderQueue.setUniforms(rayAndLightningUniformNames, float(m_iCurrentTick), glm::ivec2(m_iCanvasWidth, m_iCanvasHeight));
	m_effectLightningRenderQueue.Render(m_pLightningShader, m_fDepthLevel, m_fDepthDelta, m_iCurrentTick);
	// Reset the depth level.
	m_fDepthLevel = m_fDepthStart - m_fDepthDelta;
}

void OpenGLMasterRenderQueue::initializeVAO(void)
{
	// First, we need to initialize the quad's vertices. Create a single VAO that will
	// be the basis for all of our quads rendered using this queue.
	// TODO(heliogenesis): Allow passing in of the texture for loading. Maybe move
	// this VAO to the parent class once it's done?
	float fSize = 0.5f;
	float posZ = 0.9f; // TODO(heliogenesis): Fix this

	std::vector<float> vertices{
		fSize, fSize, posZ,
		fSize, -fSize, posZ,
		-fSize, -fSize, posZ,
		-fSize, fSize, posZ,
	};

	std::vector<float> colors{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f
	};

	std::vector<unsigned int> indices{
		0, 1, 3,
		1, 2, 3
	};

	std::vector<std::vector<float>> vbos{ vertices };
	std::vector<std::vector<unsigned int>> ebos{ indices };

	m_pVao = new OpenGLVAO(vbos, ebos);
	unsigned int iVAOID = m_pVao->getVAO()[0];
	unsigned int *instancedVBO = m_pVao->getinstancedVBO();
	glBindVertexArray(iVAOID);
	glGenBuffers(16, &instancedVBO[0]);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[0]);
	glVertexAttribPointer((GLuint)1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[1]);
	glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[2]);
	glVertexAttribPointer((GLuint)3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(4);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[3]);
	glVertexAttribPointer((GLuint)4, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(5);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[4]);
	glVertexAttribPointer((GLuint)5, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(6);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[5]);
	glVertexAttribPointer((GLuint)6, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(7);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[6]);
	glVertexAttribPointer((GLuint)7, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(8);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[7]);
	glVertexAttribPointer((GLuint)8, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
	// Ones below these lines are placeholders...
	glEnableVertexAttribArray(9);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[8]);
	glVertexAttribPointer((GLuint)9, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(10);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[9]);
	glVertexAttribPointer((GLuint)10, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(11);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[10]);
	glVertexAttribPointer((GLuint)11, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(12);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[11]);
	glVertexAttribPointer((GLuint)12, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(13);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[12]);
	glVertexAttribPointer((GLuint)13, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(14);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[13]);
	glVertexAttribPointer((GLuint)14, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(15);
	glBindBuffer(GL_ARRAY_BUFFER, instancedVBO[14]);
	glVertexAttribPointer((GLuint)15, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);
	glVertexAttribDivisor(8, 1);
	glVertexAttribDivisor(9, 1);
	glVertexAttribDivisor(10, 1);
	glVertexAttribDivisor(11, 1);
	glVertexAttribDivisor(12, 1);
	glVertexAttribDivisor(13, 1);
	glVertexAttribDivisor(14, 1);
	glVertexAttribDivisor(15, 1);
	glBindVertexArray(0);
}

void OpenGLMasterRenderQueue::clear(void)
{
	// Clear our queue vectors, and our textures.
	// TODO: Do this...
}
