#include "OpenGL.h"
#include "OpenGLInstancedBatchImpl.h"
#include "PreComp.h"
#include <mutex>

const int MAX_DEPTH_LEVELS = 100000;
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
	// TODO: Replace m_p*Shader with unique_ptrs
	m_pGlowmapShader = new OpenGLShader("./shaders/glowmap_vertex_shader.glsl", "./shaders/glowmap_fragment_shader.glsl");
	m_pObjectTextureShader = new OpenGLShader("./shaders/instanced_vertex_shader.glsl", "./shaders/instanced_fragment_shader.glsl");
	m_pRayShader = new OpenGLShader("./shaders/ray_vertex_shader.glsl", "./shaders/ray_fragment_shader.glsl");
	m_pOrbShader = new OpenGLShader("./shaders/orb_vertex_shader.glsl", "./shaders/orb_fragment_shader.glsl");
	m_pPerlinNoiseShader = new OpenGLShader("./shaders/fbm_vertex_shader.glsl", "./shaders/fbm_fragment_shader.glsl");
	m_pPerlinNoiseTexture = std::make_unique<OpenGLAnimatedNoise>(512, 512, 64);
	m_pPerlinNoiseTexture->populateTexture3D(fbo, m_pCanvasVAO, m_pPerlinNoiseShader);
	m_pActiveRenderLayer = &m_renderLayers[0];
	m_renderLayers[layerStations + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderProper);
	m_renderLayers[layerFGWeaponFire + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderSimplified);
	m_renderLayers[layerBGWeaponFire + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderSimplified);
	m_renderLayers[layerShips + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderTextureFirst);
	m_renderLayers[layerOverhang + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderTextureFirst);
	m_renderLayers[layerEffects + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setMaxProperRenderOrderDrawCalls(30);
	m_renderLayers[layerEffects + NUM_OPENGL_BACKGROUND_OBJECT_LAYERS].setRenderOrder(OpenGLRenderLayer::renderOrder::renderOrderSimplified);
#if defined(OPENGL_FPS_COUNTER_ENABLE) || defined(OPENGL_OBJ_COUNTER_ENABLE)
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
	delete m_pGlowmapShader;
	delete m_pObjectTextureShader;
	delete m_pRayShader;
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
 int sizePixelY, int posPixelX, int posPixelY, int canvasHeight, int canvasWidth, OpenGLTexture *image, int texWidth, int texHeight, 
	int texQuadWidth, int texQuadHeight, int numFramesPerRow, int numFramesPerCol, int spriteSheetStartX, int spriteSheetStartY, float alphaStrength,
	float glowR, float glowG, float glowB, float glowA, float glowNoise, int glowRadius, bool useDepthTesting, OpenGLRenderLayer::textureRenderCategory textureRenderType, OpenGLRenderLayer::blendMode blendMode,
	glm::vec4 glowDecay)
	{
	glm::vec2 vTexPositions((float)startPixelX / (float)texWidth, (float)startPixelY / (float)texHeight);
	glm::vec2 vSpriteSheetPositions((float)spriteSheetStartX / (float)texWidth, (float)spriteSheetStartY / (float)texHeight);
	glm::vec2 vCanvasQuadSizes((float)sizePixelX / (float)canvasWidth, (float)sizePixelY / (float)canvasHeight);
	glm::vec2 vCanvasPositions((float)posPixelX / (float)canvasWidth, (float)posPixelY / (float)canvasHeight);
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
		glm::vec2 vPadSizes(float(2 * padSize) / float(canvasWidth), float(2 * padSize) / float(canvasHeight));

		vCanvasQuadSizes = vCanvasQuadSizes + vPadSizes;
		vCanvasPositions = vCanvasPositions - (vPadSizes / 2.0f);
	}

	if (m_bPrevObjAddedIsParticle) {
		m_fDepthLevel -= m_fDepthDelta;
		m_bPrevObjAddedIsParticle = false;
	}
	m_pActiveRenderLayer->addTextureToRenderQueue(vTexPositions, vSpriteSheetPositions, vCanvasQuadSizes, vCanvasPositions, vTextureQuadSizes, glowColor, alphaStrength,
		glowNoise, numFramesPerRow, numFramesPerCol, image, useDepthTesting, m_fDepthLevel, textureRenderType, blendMode, glowRadius, glowDecay);
	m_fDepthLevel -= m_fDepthDelta;
	}

void OpenGLMasterRenderQueue::addTextToRenderQueue(int startPixelX, int startPixelY, int sizePixelX, int sizePixelY,
	int posPixelX, int posPixelY, int canvasHeight, int canvasWidth, OpenGLTexture* image, int texWidth, int texHeight, int texQuadWidth, int texQuadHeight,
	std::tuple<int, int, int> textColor, bool useDepthTesting)
{
	addTextureToRenderQueue(startPixelX, startPixelY, sizePixelX, sizePixelY, posPixelX, posPixelY, canvasHeight, canvasWidth, image, texWidth, texHeight,
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
	if (m_bPrevObjAddedIsParticle) {
		m_fDepthLevel -= m_fDepthDelta;
		m_bPrevObjAddedIsParticle = false;
	}
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
	if (m_bPrevObjAddedIsParticle) {
		m_fDepthLevel -= m_fDepthDelta;
		m_bPrevObjAddedIsParticle = false;
	}
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
	// Note that we do not change the depth level when adding particles.
	m_bPrevObjAddedIsParticle = true;
	}

void OpenGLMasterRenderQueue::renderAllQueues(void)
{
	for (OpenGLRenderLayer &renderLayer : m_renderLayers) {
		renderLayer.renderAllQueues(m_fDepthLevel, m_fDepthDelta, m_iCurrentTick, glm::ivec2(m_iCanvasWidth, m_iCanvasHeight),
		m_pObjectTextureShader, m_pRayShader, m_pGlowmapShader, m_pOrbShader, fbo, m_pCanvasVAO, m_pPerlinNoiseTexture.get());
		m_fDepthLevel = m_fDepthStart - m_fDepthDelta;
	}

}
void OpenGLMasterRenderQueue::renderToGlowmaps(void)
{
	for (OpenGLRenderLayer& renderLayer : m_renderLayers) {
		renderLayer.GenerateGlowmaps(fbo, m_pCanvasVAO, m_pGlowmapShader);
	}
}
void OpenGLMasterRenderQueue::clear(void)
{
	// Clear our queue vectors, and our textures.
	// TODO: Do this...
}
