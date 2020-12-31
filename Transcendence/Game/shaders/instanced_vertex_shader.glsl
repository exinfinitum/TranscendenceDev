#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexPositions;
layout (location = 2) in vec2 aCanvasQuadSizes;
layout (location = 3) in vec2 aCanvasPositions;
layout (location = 4) in float aRotationInDegrees;
layout (location = 5) in vec2 aTexQuadSizes;
layout (location = 6) in vec2 aTexStartPoint;
layout (location = 7) in ivec2 aNumberOfFrames;
layout (location = 8) in float aAlphaStrength;
layout (location = 9) in vec4 aGlowColor;
layout (location = 10) in float aGlowNoise;
layout (location = 11) in vec4 aDecayPointInfo;
layout (location = 12) in ivec2 aRenderModes;
layout (location = 13) in int aGlowRadius;
layout (location = 14) in float aCanvasQuadAspectRatio; // ratio of width to height
layout (location = 15) in float aDepth;

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
layout (location = 14) flat out int glowRadius;
layout (location = 15) flat out int blendMode;
layout (location = 16) out vec2 decayPoint;
layout (location = 17) out float decayMaxRadius;
layout (location = 18) out float decayMinRadius;
layout (location = 19) out vec2 canvPosition;

uniform vec2 aCanvasAdjustedDimensions;

const int renderCategoryObject = 0;
const int renderCategoryText = 1;
const float PI = 3.14159;

mat4 rotationMatrix2D(float rotation)
{
    // Generates a rotation matrix that rotates a vector counterclockwise in the XY plane by the specified rotation
    float sterm = sin(rotation);
    float cterm = cos(rotation);
    vec4 v1 = vec4(cterm, -sterm, 0, 0);
    vec4 v2 = vec4(sterm, cterm, 0, 0);
    vec4 v3 = vec4(0, 0, 1, 0);
    vec4 v4 = vec4(0, 0, 0, 1);
    return mat4(v1, v2, v3, v4);
}

vec2 rotateVector2D(vec2 in_vec, float rotation_in_rad)
{
	mat4 rot_mat = rotationMatrix2D(rotation_in_rad);
	vec4 vec_to_rotate = vec4(in_vec[0], in_vec[1], 0, 1.0);
	vec4 rot_result = rot_mat * vec_to_rotate;
	return vec2(rot_result.x, rot_result.y);
}

void main(void)
{
	// Fix positions and sizes
	vec2 fragmentPos = vec2(aPos[0] + 0.5f, aPos[1] + 0.5f);
	vec2 fixedSize = aCanvasQuadSizes * 2.0;
	vec2 fixedTexSize = aTexQuadSizes * 1.0;
	vec2 positionOffset = vec2(fixedSize[0], -fixedSize[1]) / 2.0;
	vec2 texPositionOffset = vec2(fixedTexSize[0], fixedTexSize[1]) / 2.0;
	vec2 proportionalCanvasPositions = aCanvasPositions / aCanvasAdjustedDimensions;
	vec2 fixedCanvPos = vec2((proportionalCanvasPositions[0] * 2.0) - 1.0, (proportionalCanvasPositions[1] * -2.0) + 1.0);
	vec2 fixedTexPos = vec2((aTexPositions[0] * 1.0), (aTexPositions[1] * 1.0));
	fixedCanvPos = fixedCanvPos + (positionOffset / aCanvasAdjustedDimensions);
	// Note that the order is Scale->Rotate->Scale->Translate. The first scaling is to size the quad in absolute terms properly,
	// the second scaling is to get the proper size relative to the canvas
	vec2 pos2d = (rotateVector2D(vec2(aPos[0], aPos[1]) * fixedSize, -(aRotationInDegrees * PI) / 180) / aCanvasAdjustedDimensions) + fixedCanvPos;
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
	render_category = aRenderModes[0];
	texture_raw_pos = vec2(aPos[0], aPos[1]);
	texture_start_point = aTexStartPoint;
	num_frames = aNumberOfFrames;
	blendMode = aRenderModes[1];
	glowRadius = aGlowRadius;
	decayPoint = vec2(aDecayPointInfo[0], aDecayPointInfo[1] / aCanvasQuadAspectRatio);
	decayMaxRadius = aDecayPointInfo[2];
	decayMinRadius = aDecayPointInfo[3];
	canvPosition = vec2(aPos[0], -aPos[1] / aCanvasQuadAspectRatio);
}