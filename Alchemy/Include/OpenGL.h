#pragma once

/*
DESIGN DOC FOR OPENGL'ING TRANS!

-First, start with something simple. Say, the spaceObject PAINT function. Either enable batched painting for specific object classes on a class by class basis,
or modify the Image.PaintImage function (CObjectImageArray.cpp::1352) to blt to an OpenGL quad instead of the canvas. First is recommended, since that is ultimately what we want to do.

-We will need to come up with the following things:
1. Render queue to organize all the things we need to paint, and how to best paint them (i.e. instancing)
2. Function to replace CObjectImageArray::PaintImage

The simple and hacky way to go is to attach an OpenGL render queue object (which we'll have to define ourselves) to the CG32bitImage class. Note that the Dest for most spaceobject renderings is the same
as the canvas object we blt to a quad in the render phase, so we can use OpenGL to do batched or naive rendering as we see fit. Once we have the render queue object, we can then decide what to do with our
objects to render. Start with a simple naive render of 'load texture, render, load next texture'. In the Paint functions, we can generate and attach objects to the render queues.

For spaceobjects with image textures, we will probably need a custom OpenGLVAO object. Features we may need include: rotation index vao, vao specifying texture rows/columns, bool vao specifying whether to
go by row order or column order (or maybe use different shader for that), vao specifying texture size, 

Things needed per spobject:
-Start position pixel (supplied by function)
-Size of the ship object in pixels
-Position of the ship object in pixel coords
-Size of the screen in pixels
-Texture to use for the ship

The spobject render queue contains the following globals:
-Vector with order of textures to render
-Subqueue containing which texture to use for the ship 

The master render queue will need the following:
-Sub render queues, which are vectors of OpenGLInstancedRenderQueues, one for each layer
-For each render queue, a parallel vector that contains RenderSpecs. These RenderSpecs contain references to which shaders and textures (if applicable) are to be used to render a given render queue
-The master quad which is used for all instanced rendering
-A set of textures, lazy loaded.

We should probably include the shimmering and glowing effects in the texture shader used for the texture class. For the effects, we can either go with a mega shader (nightmare to maintain) or multiple small shaders (much more modular).
Maybe start with a single megashader for now, just make sure to put things in functions (should be quite simple, since this shader simply reads things and writes them) and hope we don't exceed the max number of vbos...

The cleaner solution (and the one that's probably implemented right now) for non-texture effects is to have a separate shader for each one, and a "render spec" for these shaders (one per shader).

We'll need a separate texture queue for effects that don't use textures...

For special effects that use textures (such as glow), what we can do is use a separate instanced render queue that will make use of the glow shader, rendered after the texture itself is done.

*/
#include "OpenGLIncludes.h"
#include "OpenGLTexture.h"
#include "OpenGLInstancedBatchImpl.h"
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <mutex>
//#define OPENGL_FPS_COUNTER_ENABLE // Uncomment this line to enable the OpenGL FPS counter (which specifically notes how much time is spent in graphics)

/*
class OpenGLMasterRenderQueue {
public:
	OpenGLTextureRenderQueue(void) { }
	~OpenGLTextureRenderQueue(void);
	void pushSpaceObjectOntoQueue();
private:

};
*/

//typedef OpenGLInstancedBatch<std::tuple<float, glm::vec2>, glm::vec4, float, glm::ivec2, float, glm::vec3, glm::vec3> OpenGLInstancedBatchLightning;
//typedef OpenGLInstancedBatch<std::tuple<float, glm::vec2>, glm::vec4, float, glm::ivec2, glm::ivec3, glm::vec3, glm::vec3, glm::vec3> OpenGLInstancedBatchRay;
//typedef OpenGLInstancedBatch<std::tuple<OpenGLTexture*, OpenGLTexture*, int>, glm::vec2, glm::vec2, glm::vec2, glm::vec2, float, glm::vec4, float> OpenGLInstancedBatchTexture;

const int NUM_OPENGL_OBJECT_LAYERS = 6;

class OpenGLContext {
public:
	OpenGLContext (void) { };
	OpenGLContext (HWND hwnd);
	~OpenGLContext (void) { };
	bool initOpenGL (HWND hwnd, HDC hdc);
	void setupQuads (void);
	void resize (int w, int h);
	void ackResize () { m_bResized = false; };
	bool pollResize () { return m_bResized; };
	void renderScene (void);
	void testRender ();
	void renderCanvasBackground ();
	void renderCanvasBackgroundFromTexture (OpenGLTexture *texture);
	void prepSquareCanvas ();
	void prepTextureCanvas ();
	void prepTestScene ();
	void swapBuffers (HWND hwnd);
	void getWGLError ();
	void getWGLSwapError ();
	void setBlendMode ();

private:
	int m_iWindowWidth;
	int m_iWindowHeight;
	bool m_bResized;

	OpenGLShader *m_pTestShader;

	// Projection, view and model matrices respectively
	glm::mat4 m_pMatrix;
	glm::mat4 m_vMatrix;
	glm::mat4 m_mMatrix;

	std::vector<OpenGLVAO*> vaos;
	std::vector<OpenGLTexture*> textures;

	unsigned int vaoID[1];
	unsigned int vboID[1];
	unsigned int eboID[1];

protected:
	HGLRC m_renderContext;
	HDC m_deviceContext;
	HWND m_windowID;
};

class OpenGLMasterRenderQueue {
public:
	OpenGLMasterRenderQueue (void);
	~OpenGLMasterRenderQueue (void);
	void renderAllQueues (void);
	void setObjectTextureShader (OpenGLShader *shader) { m_pObjectTextureShader = shader; }
	OpenGLShader* getObjectTextureShader (void) { return m_pObjectTextureShader; }
	void addShipToRenderQueue (int startPixelX, int startPixelY, int sizePixelX, int sizePixelY,
 int posPixelX,

 int posPixelY,
		int canvasHeight, int canvasWidth, OpenGLTexture *image, int texWidth, int texHeight, int texQuadWidth, int texQuadHeight, int numFramesPerRow, int numFramesPerCol, int spriteSheetStartX, int spriteSheetStartY, float alphaStrength = 1.0, float glowR = 0.0, float glowG = 0.0, float glowB = 0.0, float glowA = 0.0, float glowNoise = 0.0);
	void addRayToEffectRenderQueue (int posPixelX, int posPixelY, int sizePixelX, int sizePixelY, int canvasSizeX, int canvasSizeY, float rotation,
		int iColorTypes, int iOpacityTypes, int iWidthAdjType, int iReshape, int iTexture, std::tuple<int, int, int> primaryColor,
		std::tuple<int, int, int> secondaryColor, int iIntensity, float waveCyclePos, int opacityAdj);
	void addLightningToEffectRenderQueue (int posPixelX, int posPixelY, int sizePixelX, int sizePixelY, int canvasSizeX, int canvasSizeY, float rotation,
		int iWidthAdjType, int iReshape, std::tuple<int, int, int> primaryColor, std::tuple<int, int, int> secondaryColor, float seed);
	void setCurrentTick (int currTick) { m_iCurrentTick = currTick; }
	void setCanvasDimensions(int width, int height) { m_iCanvasHeight = height; m_iCanvasWidth = width; }
	void handOffTextureForDeletion(std::shared_ptr<OpenGLTexture> texPtr) {
		m_texturesForDeletion.push_back(std::move(texPtr));
	 }
	void setPointerToCanvas(void* canvas) { m_pCanvas = canvas; }
	void *getPointerToCanvas() { return m_pCanvas; }
#ifdef OPENGL_FPS_COUNTER_ENABLE
	CG16bitFont& getOpenGLIndicatorFont() {
		return *(m_pOpenGLIndicatorFont.get());
	}
#endif
private:
	void initializeCanvasVAO (void);
	void deinitCanvasVAO (void);
	void clear (void);
	OpenGLVAO* m_pCanvasVAO;
	OpenGLShader *m_pObjectTextureShader;
	OpenGLShader *m_pGlowmapShader;
	OpenGLShader *m_pRayShader;
	OpenGLShader *m_pLightningShader;
	// TODO: Maybe use filenames of texture images as the key rather than pointer to OpenGLTextures? Using pointers as map keys is not reliable.
	std::map<OpenGLTexture*, OpenGLInstancedBatchTexture*> m_shipRenderQueues;
	OpenGLInstancedBatchRay m_shipEffectRayRenderQueue;
	OpenGLInstancedBatchRay m_effectRayRenderQueue;
	OpenGLInstancedBatchLightning m_effectLightningRenderQueue;
	OpenGLInstancedBatchLightning m_shipEffectLightningRenderQueue;
	//OpenGLInstancedRenderQueue* m_shipEffectOrbRenderQueues;
	//OpenGLInstancedRenderQueue* m_effectOrbRenderQueues;
	// TODO: CPU images should own corresponding OpenGL textures. We need to permanently map our RAM textures to GPU textures, which means avoiding
	// the dictionary we have below as CPU memory addresses can change so the dictionary method is not reliable.
	std::map<GLvoid*, OpenGLTexture*> m_textures;
	//std::map<OpenGLTexture*, std::vector<GlowmapTile>> m_glowmapTiles;
	std::vector<std::shared_ptr<OpenGLTexture>> m_texturesForDeletion;
	//std::vector<std::shared_ptr<OpenGLTexture>> m_texturesNeedingInitialization;
	float m_fDepthLevel;
	static const float m_fDepthDelta;
	static const float m_fDepthStart;
	unsigned int m_iCurrentTick; // Used instead of ticks
	unsigned int m_iCanvasWidth;
	unsigned int m_iCanvasHeight;
	unsigned int fbo;
	unsigned int rbo;
	std::mutex m_shipRenderQueueAddMutex;
	void* m_pCanvas;
#ifdef OPENGL_FPS_COUNTER_ENABLE
	std::unique_ptr<CG16bitFont> m_pOpenGLIndicatorFont;

	//OpenGLInstancedBatchRay m_FPSCounterRayRenderQueue;
	//std::chrono::duration<double> last_frame_time;
	//void DrawHex7Digit(float pos_x, float pos_y, int num) {
//		const float bar_width = 0.002;
//	}
#endif

};
