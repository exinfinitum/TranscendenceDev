#include "OpenGL.h"
#include "OpenGLInstancedBatchImpl.h"
#include "PreComp.h"
#include <mutex>

const int MAX_DEPTH_LEVELS = 10000000;
const float OpenGLMasterRenderQueue::m_fDepthDelta = float(1.0 / MAX_DEPTH_LEVELS); // Up to 100k different depth levels
const float OpenGLMasterRenderQueue::m_fDepthStart = 1.0f - float(1.0 / MAX_DEPTH_LEVELS); // Up to 100k different depth levels

namespace {

} // namespace 

OpenGLMasterRenderQueue::OpenGLMasterRenderQueue(void)
{
	// Initialize the VAO.
	initializeCanvasVAO();
	// Depth level starts at one minus the delta.
	m_fDepthLevel = m_fDepthStart - m_fDepthDelta;
	m_iCurrentTick = 0;
	// Initialize the FBO.
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);
	m_pGlowmapShader = std::make_unique<OpenGLShader>("./shaders/glowmap_vertex_shader.glsl", "./shaders/glowmap_fragment_shader.glsl");
	m_pObjectTextureShader = std::make_unique<OpenGLShader>("./shaders/instanced_vertex_shader.glsl", "./shaders/instanced_fragment_shader.glsl");
	m_pRayShader = std::make_unique<OpenGLShader>("./shaders/ray_vertex_shader.glsl", "./shaders/ray_fragment_shader.glsl");
	m_pOrbShader = std::make_unique<OpenGLShader>("./shaders/orb_vertex_shader.glsl", "./shaders/orb_fragment_shader.glsl");
	m_pPerlinNoiseShader = std::make_unique<OpenGLShader>("./shaders/fbm_vertex_shader.glsl", "./shaders/fbm_fragment_shader.glsl");
	m_pPerlinNoiseTexture = std::make_unique<OpenGLAnimatedNoise>(1024, 1024, 64);
	m_pPerlinNoiseTexture->populateTexture3D(fbo, m_pCanvasVAO, m_pPerlinNoiseShader.get());
	m_pActiveRenderLayer = &m_renderLayers[0];
	m_renderLayers[layerStations + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderProper);
	m_renderLayers[layerFGWeaponFire + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderProper);
	m_renderLayers[layerBGWeaponFire + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderProper);
	m_renderLayers[layerShips + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderTextureFirst);
	m_renderLayers[layerOverhang + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderTextureFirst);
	m_renderLayers[layerEffects + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setMaxProperRenderOrderDrawCalls(30);
	m_renderLayers[layerEffects + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderProper);
#if defined(OPENGL_FPS_COUNTER_ENABLE) || defined(OPENGL_OBJ_COUNTER_ENABLE) || defined(OPENGL_DC_COUNTER_ENABLE)
	m_pOpenGLIndicatorFont = std::make_unique<CG16bitFont>();
	m_pOpenGLIndicatorFont->Create(CONSTLIT("Arial"), -16);
#endif
}

OpenGLMasterRenderQueue::~OpenGLMasterRenderQueue(void)
{
	clear();
	//deinitCanvasVAO();
	// TODO: Delete render queues, delete VAOs properly (specifically the instanced VAO bits)
	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &rbo);
}

void OpenGLMasterRenderQueue::initializeCanvasVAO()
{
	// Prepare the background canvas.

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
	//m_pCanvasVAO->setShader(m_pPerlinNoiseShader);

}

void OpenGLMasterRenderQueue::deinitCanvasVAO(void)
{
	// TODO(heliogenesis): Move this VAO to the parent class once it's done
	unsigned int *canvasVAO = m_pCanvasVAO->getinstancedVBO();
	glDeleteBuffers(16, &canvasVAO[0]);
	delete[] m_pCanvasVAO;
}

void OpenGLMasterRenderQueue::addTextureToRenderQueue(int startPixelX, int startPixelY, int sizePixelX,
	int sizePixelY, int posPixelX, int posPixelY, float rotationInDegrees, OpenGLTexture* image, int texWidth, int texHeight,
	int texQuadWidth, int texQuadHeight, int numFramesPerRow, int numFramesPerCol, int spriteSheetStartX, int spriteSheetStartY, float alphaStrength,
	float glowR, float glowG, float glowB, float glowA, float glowNoise, int glowRadius, bool useDepthTesting, OpenGLRenderLayer::textureRenderCategory textureRenderType, OpenGLRenderLayer::blendMode blendMode,
	glm::ivec4 glowDecay, OpenGLTexture* mask)
	{
	glm::vec2 vTexPositions((float)startPixelX / (float)texWidth, (float)startPixelY / (float)texHeight);
	glm::vec2 vSpriteSheetPositions((float)spriteSheetStartX / (float)texWidth, (float)spriteSheetStartY / (float)texHeight);
	glm::vec2 vCanvasQuadSizes((float)sizePixelX, (float)sizePixelY);
	glm::vec2 vCanvasPositions((float)posPixelX, (float)posPixelY);
	glm::vec2 vTextureQuadSizes((float)texQuadWidth / (float)texWidth, (float)texQuadHeight / (float)texHeight);
	glm::vec4 glowColor(glowR, glowG, glowB, glowA);

	// If this is a glow color, and a glowmap is defined, then adjust coordinates

	if (glowColor[3] > 0.0) {
		auto glowmapTile = GlowmapTile(
			vSpriteSheetPositions[0],
			vSpriteSheetPositions[1],
			float(numFramesPerRow * vTextureQuadSizes[0]),
			float(numFramesPerCol * vTextureQuadSizes[1]),
			vTextureQuadSizes[0], vTextureQuadSizes[1],
			numFramesPerRow, numFramesPerCol, glowRadius * 2
		);
		auto* glowMap = image->getGlowMap(glowmapTile);
		int padSize = glowMap != nullptr ? glowMap->getPadSize() : 0;
		glm::vec2 vPadSizesInPixels(float(2 * padSize), float(2 * padSize));
		glm::vec2 vPadSizes(float(2 * padSize), float(2 * padSize));

		vCanvasQuadSizes = vCanvasQuadSizes + vPadSizesInPixels;
		vCanvasPositions = vCanvasPositions - (vPadSizes / 2.0f);
	}
	float aspectRatio = vCanvasQuadSizes[0] / vCanvasQuadSizes[1];
	glm::vec4 relativeGlowDecay(
		// Note we divide both components of glow decay point by iQuadWidth - this is because the shader uses the aspect ratio to calculate
		// the glow decay point location in a unit that is common across both quad dimensions so we can handle nonsquare images
		// Values 1 and 2 are the (x, y) coordinates of the decay origin point in pixels offset from the quad centre, value 3 is
		// the minimum radius from decay origin point at which glow occurs, and value 4 is the maximum radius from that point at which
		// glow occurs - glow is only nonzero between those two radii, and peaks at the average radius
		float(glowDecay[0]) / vCanvasQuadSizes[0],
		float(glowDecay[1]) / vCanvasQuadSizes[1],
		float(glowDecay[2]) / vCanvasQuadSizes[0],
		float(glowDecay[3]) / vCanvasQuadSizes[0]
	);

	//if (m_bPrevObjAddedIsParticle) {
	//	m_fDepthLevel -= m_fDepthDelta;
	//	m_bPrevObjAddedIsParticle = false;
	//}
	m_pActiveRenderLayer->addTextureToRenderQueue(vTexPositions, vSpriteSheetPositions, vCanvasQuadSizes, vCanvasPositions, rotationInDegrees, vTextureQuadSizes, glowColor, alphaStrength,
		glowNoise, numFramesPerRow, numFramesPerCol, image, useDepthTesting, m_fDepthLevel, aspectRatio, textureRenderType, blendMode, glowRadius, relativeGlowDecay, mask);
	m_fDepthLevel -= m_fDepthDelta;
	}

void OpenGLMasterRenderQueue::addTextToRenderQueue(int startPixelX, int startPixelY, int sizePixelX, int sizePixelY,
	int posPixelX, int posPixelY, OpenGLTexture* image, int texWidth, int texHeight, int texQuadWidth, int texQuadHeight,
	std::tuple<int, int, int> textColor, bool useDepthTesting)
{
	addTextureToRenderQueue(startPixelX, startPixelY, sizePixelX, sizePixelY, posPixelX, posPixelY, 0, image, texWidth, texHeight,
		texQuadWidth, texQuadHeight, 1, 1, 0, 0, 1.0f,
		float(std::get<0>(textColor)) / 255.0f, float(std::get<1>(textColor)) / 255.0f, float(std::get<2>(textColor)) / 255.0f,
		0.0f, 0.0f, 0, useDepthTesting, OpenGLRenderLayer::textureRenderCategory::text);
}

void OpenGLMasterRenderQueue::addRayToEffectRenderQueue(int posPixelX, int posPixelY, int sizePixelX, int sizePixelY, int canvasSizeX, int canvasSizeY, float rotation,
	int iColorTypes, int iOpacityTypes, int iWidthAdjType, int iReshape, int iTexture, std::tuple<int, int, int> primaryColor,
	std::tuple<int, int, int> secondaryColor, int iIntensity, float waveCyclePos, int opacityAdj, OpenGLRenderLayer::blendMode blendMode, float secondaryOpacity)
{
	glm::vec3 vPrimaryColor = glm::vec3(std::get<0>(primaryColor), std::get<1>(primaryColor), std::get<2>(primaryColor)) / float(255.0);
	glm::vec3 vSecondaryColor = glm::vec3(std::get<0>(secondaryColor), std::get<1>(secondaryColor), std::get<2>(secondaryColor)) / float(255.0);

	glm::vec4 sizeAndPosition(float(sizePixelX), float(sizePixelY) + 0.5,
		float(posPixelX) / float(canvasSizeX), float(posPixelY) / float(canvasSizeY));
	glm::ivec4 shapes(iWidthAdjType, iReshape, 0, 0);
	glm::vec3 intensitiesAndCycles(float(iIntensity), waveCyclePos, float(opacityAdj) / 255.0f);
	glm::ivec4 styles(iColorTypes, iOpacityTypes, iTexture, 0);
	// TODO(heliogenesis): Handle the case when a ray is part of a particle system.
	//if (m_bPrevObjAddedIsParticle) {
	//	m_fDepthLevel -= m_fDepthDelta;
	//	m_bPrevObjAddedIsParticle = false;
	//}
	m_pActiveRenderLayer->addRayToEffectRenderQueue(vPrimaryColor, vSecondaryColor, sizeAndPosition, shapes, intensitiesAndCycles, styles, rotation, m_fDepthLevel, blendMode, secondaryOpacity);
	m_fDepthLevel -= m_fDepthDelta;
}

void OpenGLMasterRenderQueue::addOrbToEffectRenderQueue(
	int posPixelX, int posPixelY, int sizePixelX, int sizePixelY, int canvasSizeX, int canvasSizeY,
	float rotation,
	float intensity,
	float opacity,
	int animation,
	int style,
	int detail,
	int distortion,
	int animationSeed,
	int lifetime,
	int currFrame,
	glm::vec3 primaryColor,
	glm::vec3 secondaryColor,
    float secondaryOpacity,
	OpenGLRenderLayer::blendMode blendMode)
	{
	glm::vec4 sizeAndPosition((float)sizePixelX, (float)sizePixelY,
		(float)posPixelX / (float)canvasSizeX, (float)posPixelY / (float)canvasSizeY);
	m_pActiveRenderLayer->addOrbToEffectRenderQueue(sizeAndPosition, rotation, intensity, opacity, animation, style, detail, distortion, animationSeed, lifetime, currFrame, primaryColor, secondaryColor, secondaryOpacity,
		m_fDepthLevel, blendMode);
	m_fDepthLevel -= m_fDepthDelta;
	}

void OpenGLMasterRenderQueue::addParticleToEffectRenderQueue(int posPixelX, int posPixelY, int sizePixelX, int sizePixelY, int canvasSizeX, int canvasSizeY,
	float rotation,
	float opacity,
	int style,
	int lifetime,
	int currFrame,
	int destiny,
	float minRadius,
	float maxRadius,
	std::tuple<int, int, int> primaryColor,
	std::tuple<int, int, int> secondaryColor,
	OpenGLRenderLayer::blendMode blendMode)
	{
	glm::vec4 sizeAndPosition((float)sizePixelX * 2.0f, (float)sizePixelY * 2.0f,
		(float)posPixelX / (float)canvasSizeX, (float)posPixelY / (float)canvasSizeY);
	glm::vec3 vPrimaryColor = glm::vec3(std::get<0>(primaryColor), std::get<1>(primaryColor), std::get<2>(primaryColor)) / float(255.0);
	glm::vec3 vSecondaryColor = glm::vec3(std::get<0>(secondaryColor), std::get<1>(secondaryColor), std::get<2>(secondaryColor)) / float(255.0);
	m_pActiveRenderLayer->addParticleToEffectRenderQueue(sizeAndPosition, rotation, opacity, style, lifetime, currFrame, destiny, minRadius * 2.0f, maxRadius * 2.0f, vPrimaryColor, vSecondaryColor, m_fDepthLevel, blendMode);
	m_fDepthLevel -= m_fDepthDelta;
	// Note that we do not change the depth level when adding particles.
	//m_bPrevObjAddedIsParticle = true;
	}

int OpenGLMasterRenderQueue::renderQueueByLayerNumberIndex(int index)
{
	int iNumDrawCalls = m_renderLayers[index].renderAllQueues(m_fDepthLevel, m_fDepthDelta, m_iCurrentTick, glm::ivec2(m_iCanvasWidth, m_iCanvasHeight),
		m_pObjectTextureShader.get(), m_pRayShader.get(), m_pGlowmapShader.get(), m_pOrbShader.get(), fbo, m_pCanvasVAO, m_pPerlinNoiseTexture.get());
	m_fDepthLevel = m_fDepthStart - m_fDepthDelta;
	return iNumDrawCalls;
}

void OpenGLMasterRenderQueue::deleteUnusedTextures(void)
{
	std::unique_lock<std::mutex> lck(m_deleteTextureMutex);
	for (auto& texture : m_texturesForDeletion) {
		::kernelDebugLogPattern("[OpenGL] Preparing to delete texture at addr: %x", int(texture.get()));
	}
	if (m_texturesForDeletion.size() > 0) ::kernelDebugLogPattern("[OpenGL] Deleting %d textures using thread %d.", m_texturesForDeletion.size(), GetCurrentThreadId());
	m_texturesForDeletion.clear();
}

int OpenGLMasterRenderQueue::renderAllQueues(void)
{
	int iNumDrawCalls = 0;
	for (size_t i = 0; i < m_renderLayers.size(); i++) {
		iNumDrawCalls += renderQueueByLayerNumberIndex(i);
	}
	deleteUnusedTextures();
	return iNumDrawCalls;
}
void OpenGLMasterRenderQueue::renderToGlowmaps(void)
{
	for (OpenGLRenderLayer& renderLayer : m_renderLayers) {
		renderLayer.GenerateGlowmaps(fbo, m_pCanvasVAO, m_pGlowmapShader.get());
	}
}
void OpenGLMasterRenderQueue::clear(void)
{
	// Clear our queue vectors, and our textures.
	// TODO: Do this...
}
