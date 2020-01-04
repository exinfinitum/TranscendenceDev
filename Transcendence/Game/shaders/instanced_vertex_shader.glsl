#version 330 core

in vec3 aPos;
in vec2 aTexPositions;
in vec2 aCanvasQuadSizes;
in vec2 aCanvasPositions;
in vec2 aTexQuadSizes;
in float aAlphaStrength;
in float aDepth;
in vec4 aGlowColor;
in float aGlowNoise;

out vec2 texture_uv;
out vec2 texture_pos;
out vec2 texture_size;
out vec2 fragment_pos;
out float alpha_strength;
out float depth;
out vec4 glow_color;
out float glow_noise;
out vec2 texture_bounds_min;
out vec2 texture_bounds_max;
void main(void)
{
	// Fix positions and sizes
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
	fragment_pos = vec2(aPos[0] + 0.5f, aPos[1] + 0.5f);
	alpha_strength = aAlphaStrength;
	glow_color = aGlowColor;
	glow_noise = aGlowNoise;
	texture_bounds_min = fixedTexPos;
	texture_bounds_max = fixedTexPos + texPositionOffset*2;
}