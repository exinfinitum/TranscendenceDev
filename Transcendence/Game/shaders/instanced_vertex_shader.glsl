#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexPositions;
layout (location = 2) in vec2 aCanvasQuadSizes;
layout (location = 3) in vec2 aCanvasPositions;
layout (location = 4) in vec2 aTexQuadSizes;
layout (location = 5) in vec2 aTexStartPoint;
layout (location = 6) in ivec2 aNumberOfFrames;
layout (location = 7) in float aAlphaStrength;
layout (location = 8) in vec4 aGlowColor;
layout (location = 9) in float aGlowNoise;
layout (location = 10) in int aRenderCategory;
layout (location = 11) in int aBlendMode;
layout (location = 12) in float aDepth;

layout (location = 0) out vec2 texture_uv;
layout (location = 1) out vec2 texture_pos;
layout (location = 2) out vec2 texture_size;
layout (location = 3) out vec2 fragment_pos;
layout (location = 4) out vec2 texture_start_point;
layout (location = 5) flat out ivec2 num_frames;
layout (location = 6) out float alpha_strength;
layout (location = 7) out float depth;
layout (location = 8) out vec4 glow_color;
layout (location = 9) out float glow_noise;
layout (location = 10) out vec2 texture_bounds_min;
layout (location = 11) out vec2 texture_bounds_max;
layout (location = 12) flat out int render_category;
layout (location = 13) out vec2 texture_raw_pos;
layout (location = 14) flat out int blendMode;


const int renderCategoryObject = 0;
const int renderCategoryText = 1;
const float PI = 3.14159;

void main(void)
{
	// Fix positions and sizes
	vec2 fragmentPos = vec2(aPos[0] + 0.5f, aPos[1] + 0.5f);
	vec2 fixedSize = aCanvasQuadSizes * 2.0;
	vec2 fixedTexSize = aTexQuadSizes * 1.0;
	vec2 positionOffset = vec2(fixedSize[0], -fixedSize[1]) / 2.0;
	vec2 texPositionOffset = vec2(fixedTexSize[0], fixedTexSize[1]) / 2.0;
	vec2 fixedCanvPos = vec2((aCanvasPositions[0] * 2.0) - 1.0, (aCanvasPositions[1] * -2.0) + 1.0);
	vec2 fixedTexPos = vec2((aTexPositions[0] * 1.0), (aTexPositions[1] * 1.0));
	fixedCanvPos = fixedCanvPos + positionOffset;
	vec2 pos2d = vec2(aPos[0] * fixedSize[0], aPos[1] * fixedSize[1]) + fixedCanvPos;
	vec2 texPos2d = vec2(aPos[0] * fixedTexSize[0], -aPos[1] * fixedTexSize[1]) + fixedTexPos + texPositionOffset;
	
    gl_Position = vec4(pos2d, aDepth, 1.0);
	depth = aDepth;
    texture_uv = texPos2d;
	texture_pos = aTexPositions;
	texture_size = fixedTexSize;
	fragment_pos = fragmentPos;
	alpha_strength = aAlphaStrength;
	glow_color = aGlowColor;
	glow_noise = aGlowNoise;
	texture_bounds_min = fixedTexPos;
	texture_bounds_max = fixedTexPos + texPositionOffset*2;
	render_category = aRenderCategory;
	texture_raw_pos = vec2(aPos[0], aPos[1]);
	texture_start_point = aTexStartPoint;
	num_frames = aNumberOfFrames;
	blendMode = aBlendMode;
}