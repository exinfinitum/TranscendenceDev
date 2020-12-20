#pragma once
#include "OpenGLIncludes.h"
#include "OpenGLVAO.h"
#include <unordered_set>
#include <map>

// Helper class to define bounds
class GlowmapTile {
public:
	GlowmapTile(float upleft_X, float upleft_Y, float size_X, float size_Y, float gridsize_X, float gridsize_Y, int numFramesPerRow, int numFramesPerCol, int iGlowSize):
		value(std::make_tuple(upleft_X, upleft_Y, size_X, size_Y, gridsize_X, gridsize_Y, numFramesPerRow, numFramesPerCol)),
		m_iGlowSize(iGlowSize) {};
	~GlowmapTile(void) { };
	friend bool operator == (const GlowmapTile& lhs, const GlowmapTile& rhs) {
		return (std::get<0>(lhs.value) == std::get<0>(rhs.value)
			&& std::get<1>(lhs.value) == std::get<1>(rhs.value)
			&& std::get<2>(lhs.value) == std::get<2>(rhs.value)
			&& std::get<3>(lhs.value) == std::get<3>(rhs.value)
			&& std::get<4>(lhs.value) == std::get<4>(rhs.value)
			&& std::get<5>(lhs.value) == std::get<5>(rhs.value)
			&& std::get<6>(lhs.value) == std::get<6>(rhs.value)
			&& std::get<7>(lhs.value) == std::get<7>(rhs.value));
	}
	size_t getHash() const {
		return std::hash<float>{}(std::get<0>(value))
			^ (std::hash<float>{}(std::get<1>(value)) << 1)
			^ (std::hash<float>{}(std::get<2>(value)) << 2)
			^ (std::hash<float>{}(std::get<3>(value)) << 3)
			^ (std::hash<float>{}(std::get<4>(value)) << 4)
			^ (std::hash<float>{}(std::get<5>(value)) << 5)
			^ (std::hash<int>{}(std::get<6>(value)) << 6)
			^ (std::hash<int>{}(std::get<7>(value)) << 7);
	}
	std::tuple<float, float, float, float, float, float, int, int> getValue() const { return value; }
	int getGlowSize() const { return m_iGlowSize; }
private:
	std::tuple<float, float, float, float, float, float, int, int> value;
	int m_iGlowSize;
};

class QBEquals {
public:
	size_t operator()(const GlowmapTile &qb1, const GlowmapTile &qb2) const {
		return qb1.getHash() == qb2.getHash();
	};
};


class QBHash {
public:
	size_t operator()(const GlowmapTile &quadBounds) const {
		return quadBounds.getHash();
	};
};


class OpenGLTexture {
public:
	static const int PIXEL_DECIMAL_PLACE_PER_CHANNEL_FOR_LINEAR_GLOWMAP = 100;
	OpenGLTexture(void* texture, int width, int height, bool isOpaque);
	OpenGLTexture(int width, int height);
	virtual ~OpenGLTexture(void);
	void initTexture2D(int width, int height);
	void initTexture2D(void* texture, int width, int height);
	void bindTexture2D (GLenum glTexture) const { glActiveTexture(glTexture); glBindTexture(GL_TEXTURE_2D, m_pTextureID[0]); }
	void unbindTexture2D (void) const { glBindTexture(GL_TEXTURE_2D, 0); }
	void updateTexture2D(void* texture, int width, int height);
	void initTextureFromOpenGLThread(void);
	unsigned int* getTexture(void) { return m_pTextureID; }
	virtual OpenGLTexture *getGlowMap(float upleft_X, float upleft_Y, float size_X, float size_Y, float gridsize_X, float gridsize_Y, int numFramesPerRow, int numFramesPerCol, int iGlowSize) { return nullptr; }
	OpenGLTexture *getGlowMap(GlowmapTile glowmapTile) {
		auto [texStart_x, texStart_y, texQuadSize_x, texQuadSize_y, texGridSize_x, texGridSize_y, numFramesPerRow, numFramesPerCol] = glowmapTile.getValue();
		auto iGlowSize = glowmapTile.getGlowSize();
		return this->getGlowMap(texStart_x, texStart_y, texQuadSize_x, texQuadSize_y, texGridSize_x, texGridSize_y, numFramesPerRow, numFramesPerCol, iGlowSize);
	}
	virtual void requestGlowmapTile(float upleft_X, float upleft_Y, float size_X, float size_Y, float gridsize_X, float gridsize_Y, int numFramesPerRow, int numFramesPerCol, int iGlowSize) {};
	void requestGlowmapTile(GlowmapTile glowmapTile) {
		auto [texStart_x, texStart_y, texQuadSize_x, texQuadSize_y, texGridSize_x, texGridSize_y, numFramesPerRow, numFramesPerCol] = glowmapTile.getValue();
		auto iGlowSize = glowmapTile.getGlowSize();
		this->requestGlowmapTile(texStart_x, texStart_y, texQuadSize_x, texQuadSize_y, texGridSize_x, texGridSize_y, numFramesPerRow, numFramesPerCol, iGlowSize);
	};
	virtual void populateGlowmaps(unsigned int fbo, OpenGLVAO* vao, OpenGLShader* shader) {};
	virtual int getPadSize() { return 0; };
	virtual int getGlowSize() { return 0; };
	virtual GLint getInternalFormat() = 0;
	virtual GLenum getTexSubImageFormat() = 0;
	virtual GLenum getTexSubImageType() = 0;
	virtual int getNumberOfChannels() = 0;
protected:
	unsigned int m_iWidth;
	unsigned int m_iHeight;
	static uint64_t m_iNumPixelsAllocd;
private:
	unsigned int m_pTextureID[1];
	unsigned int pboID[2];
	GLint m_pixelFormat;
	GLint m_pixelType;
	void* m_pTextureToInitFrom = nullptr;
	bool m_isOpaque;
	bool m_bIsInited = false;
};

class OpenGLTextureGlowmapRGBA32 : public OpenGLTexture {
public:
	OpenGLTextureGlowmapRGBA32(void* texture, int width, int height, bool isOpaque) : OpenGLTexture(texture, width, height, isOpaque) {}
	OpenGLTextureGlowmapRGBA32(int width, int height) : OpenGLTexture(width, height) {}
	~OpenGLTextureGlowmapRGBA32(void) {};
	GLint getInternalFormat() override {
		return GL_RGBA8;
	}
	GLenum getTexSubImageFormat() override {
		return GL_BGRA;
	}
	GLenum getTexSubImageType() override {
		return GL_UNSIGNED_INT_8_8_8_8_REV;
	}
	int getNumberOfChannels() override {
		return 4;
	}
	int getPadSize() override { return m_iPadSize; };
	int getGlowSize() override { return m_iGlowSize; };
protected:
	void setPadSize(int padSize) { m_iPadSize = padSize; }
	void setGlowSize(int glowSize) { m_iGlowSize = glowSize; }
private:
	int m_iPadSize = 0;
	int m_iGlowSize = 0;

	friend class OpenGLTextureRGBA32;
};

class OpenGLTextureRGBA32 : public OpenGLTexture {
public:
	OpenGLTextureRGBA32(void* texture, int width, int height, bool isOpaque) : OpenGLTexture(texture, width, height, isOpaque) {}
	OpenGLTextureRGBA32(int width, int height) : OpenGLTexture(width, height) {}
	~OpenGLTextureRGBA32(void) {};
	GLint getInternalFormat() override {
		return GL_RGBA8;
	}
	GLenum getTexSubImageFormat() override {
		return GL_BGRA;
	}
	GLenum getTexSubImageType() override {
		return GL_UNSIGNED_INT_8_8_8_8_REV;
	}
	int getNumberOfChannels() override {
		return 4;
	}
	void requestGlowmapTile(float upleft_X, float upleft_Y, float size_X, float size_Y, float gridsize_X, float gridsize_Y, int numFramesPerRow, int numFramesPerCol, int iGlowSize) override {
		// This function should be called when we request a render with this texture
		// We should not add a request if it is already in the completed queue
		auto glowmapTile = GlowmapTile(upleft_X, upleft_Y, size_X, size_Y, gridsize_X, gridsize_Y, numFramesPerRow, numFramesPerCol, iGlowSize);
		bool tileAlreadyRendered = m_pGlowMaps.count(glowmapTile) > 0;
		bool requestingLargerGlowmapSize = tileAlreadyRendered ? m_pGlowMaps.at(glowmapTile)->getGlowSize() < iGlowSize : iGlowSize > 0;
		// TODO(heliogenesis): Maintain one glowmap per glowmap tile. The glowmap should be of the minimum size needed
		// to hold the glowmap for that glowmap tile. This is because we can't just draw glowmaps on a texture the same
		// size as the input texture due to padding.
		if (requestingLargerGlowmapSize) {
			m_GlowmapTilesToRender.insert(glowmapTile);
		}
	}
	void populateGlowmaps(unsigned int fbo, OpenGLVAO* vao, OpenGLShader* shader) override {
		for (const auto& tileToRender : m_GlowmapTilesToRender) {
			if (m_pGlowMaps.count(tileToRender) > 0) {
				m_pGlowMaps.erase(tileToRender);
			}
			auto [texStart_x, texStart_y, texQuadSize_x, texQuadSize_y, texGridSize_x, texGridSize_y, numFramesPerRow, numFramesPerCol] = tileToRender.getValue();
			glm::vec2 texStart(texStart_x, texStart_y); // Starting point of this texture sprite sheet, 1.0 being bottom/rightmost and 0.0 being top/leftmost
			glm::vec2 texSize(texQuadSize_x, texQuadSize_y); // Size of the ENTIRE sprite sheet, 1.0 being 100% of the quad in X/Y dir
			glm::vec2 texGridSize(texGridSize_x, texGridSize_y); // Size of each frame of this sprite sheet, 1.0 being 100% of the quad in X/Y dir
			auto iGlowSize = tileToRender.getGlowSize();
			auto glowmap = GenerateGlowMap(fbo, vao, shader, texSize, texStart, texGridSize, numFramesPerRow, numFramesPerCol, iGlowSize);
			m_pGlowMaps[tileToRender] = std::move(glowmap);
		}
		m_GlowmapTilesToRender.clear();
	}
	OpenGLTexture* getGlowMap(float upleft_X, float upleft_Y, float size_X, float size_Y, float gridsize_X, float gridsize_Y, int numFramesPerRow, int numFramesPerCol, int iGlowSize) override {
		auto glowmapTile = GlowmapTile(upleft_X, upleft_Y, size_X, size_Y, gridsize_X, gridsize_Y, numFramesPerRow, numFramesPerCol, iGlowSize);
		if (m_pGlowMaps.count(glowmapTile) > 0) {
			return m_pGlowMaps[glowmapTile].get();
		}
		else {
			return nullptr;
		}
	}
private:
	std::unique_ptr<OpenGLTextureGlowmapRGBA32> GenerateGlowMap(unsigned int fbo, OpenGLVAO* vao, OpenGLShader* shader, const glm::vec2 texQuadSize, const glm::vec2 texStartPoint, const glm::vec2 texGridSize, int numFramesPerRow, int numFramesPerCol, int iGlowSize);
	std::unordered_set <GlowmapTile, QBHash, QBEquals> m_CompletedGlowmapTiles;
	std::unordered_set <GlowmapTile, QBHash, QBEquals> m_GlowmapTilesToRender;
	std::unordered_map<GlowmapTile, std::unique_ptr<OpenGLTextureGlowmapRGBA32>, QBHash, QBEquals> m_pGlowMaps;
};

// Grayscale version for fonts. Note that RED is the channel used as the font alpha, all other channels can be ignored.
class OpenGLTextureRGBA32GrayscaleSrc : public OpenGLTexture {
public:
	OpenGLTextureRGBA32GrayscaleSrc(void* texture, int width, int height) : OpenGLTexture(texture, width, height, false) {}
	OpenGLTextureRGBA32GrayscaleSrc(int width, int height) : OpenGLTexture(width, height) {}
	~OpenGLTextureRGBA32GrayscaleSrc(void) {};
	GLint getInternalFormat() override {
		return GL_RGBA8;
	}
	GLenum getTexSubImageFormat() override {
		return GL_RED;
	}
	GLenum getTexSubImageType() override {
		return GL_UNSIGNED_BYTE;
	}
	int getNumberOfChannels() override {
		return 4;
	}
};

class OpenGLTextureRGB16 : public OpenGLTexture {
public:
	OpenGLTextureRGB16(void* texture, int width, int height) : OpenGLTexture(texture, width, height, false) {}
	OpenGLTextureRGB16(int width, int height) : OpenGLTexture(width, height) {}
	~OpenGLTextureRGB16(void) {};
	GLint getInternalFormat() override {
		return GL_RGB;
	}
	GLenum getTexSubImageFormat() override {
		return GL_BGR;
	}
	GLenum getTexSubImageType() override {
		return GL_UNSIGNED_SHORT_5_6_5_REV;
	}
	int getNumberOfChannels() override {
		return 3;
	}
};
