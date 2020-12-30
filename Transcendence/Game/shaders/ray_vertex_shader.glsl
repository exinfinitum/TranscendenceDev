#version 410 core

// The shader used in RayRasterizer comprises the following:
// -Width specification. The RayRasterizer draws a line in an arbitrary direction, then extends it width-wise.
// What we can do is to transform a pixel back to a given rotational frame using the rotation, then using the distance
// from the axis to get the width. Note, the C code uses a WidthCount and LengthCount to divide the ray into cells.
// We can use modulos and similar methods to determine which cell our pixel is in.
// -Length specification. There is an array of lengths in the C code; we can just pass the specified one directly into
// our VBO. It is specified as a number of length "cells" in the C code, but this is what is meant (?)
// -Start and end points. Since we're using OpenGL we can just generate the horizontal ray, and rotate the quad ourselves at
// the end. Simpler! No need to transform pixels back on a per-pixel basis; we handle this in vertex shader!

// Vertex shader should handle only the sizing and rotation aspects. All the magic happens in fragment shader.
// Note, it seems that numLengthCells is just the length of the ray in pixels...

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aSizeAndPosition;
layout (location = 2) in float aRotation;
layout (location = 3) in ivec4 aShapes;
layout (location = 4) in ivec4 aStyles;
layout (location = 5) in vec4 aFloatParams;
layout (location = 6) in vec3 aPrimaryColor;
layout (location = 7) in vec3 aSecondaryColor;
layout (location = 8) in float aSeed;
layout (location = 9) in int aEffectType;
layout (location = 10) in int aBlendMode;
layout (location = 11) in float aDepth;

uniform vec2 aCanvasAdjustedDimensions;

layout (location = 0) out vec2 quadPos;
layout (location = 1) flat out int rayReshape;
layout (location = 2) flat out int rayWidthAdjType;
layout (location = 3) flat out int rayOpacity;
layout (location = 4) flat out int rayGrainyTexture;
layout (location = 5) out float depth;
layout (location = 6) out float intensity;
layout (location = 7) out vec3 primaryColor;
layout (location = 8) out vec3 secondaryColor;
layout (location = 9) out float waveCyclePos;
layout (location = 10) flat out int rayColorTypes;
layout (location = 11) out float opacityAdj;
layout (location = 12) flat out int effectType;
layout (location = 13) out float seed;
layout (location = 14) out vec2 quadSize;
layout (location = 15) flat out int orbAnimation; //
layout (location = 16) flat out int orbStyle; //
layout (location = 17) flat out int orbDistortion;
layout (location = 18) flat out int orbDetail;
layout (location = 19) out float orbSecondaryOpacity;
layout (location = 20) flat out int orbLifetime;
layout (location = 21) flat out int orbCurrFrame;
layout (location = 22) flat out int blendMode;
layout (location = 23) flat out float rotation;
layout (location = 24) flat out int particleDestiny;
layout (location = 25) flat out float particleMinRadius;

// This should match enum effectType in opengl.h.
// Note that flares are under the ray category.

int effectTypeRay = 0;
int effectTypeLightning = 1;
int effectTypeOrb = 2;
int effectTypeFlare = 3;
int effectTypeParticle = 4;

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

mat4 scalingMatrix2D(float scaleX, float scaleY)
{
    // Generates a rotation matrix that rotates a vector counterclockwise in the XY plane by the specified rotation
    vec4 v1 = vec4(scaleX, 0, 0, 0);
    vec4 v2 = vec4(0, scaleY, 0, 0);
    vec4 v3 = vec4(0, 0, 1, 0);
    vec4 v4 = vec4(0, 0, 0, 1);
    return mat4(v1, v2, v3, v4);
}

mat4 translationMatrix2D(float transX, float transY)
{
    // Generates a rotation matrix that rotates a vector counterclockwise in the XY plane by the specified rotation
    vec4 v1 = vec4(1, 0, 0, transX);
    vec4 v2 = vec4(0, 1, 0, transY);
    vec4 v3 = vec4(0, 0, 1, 0);
    vec4 v4 = vec4(0, 0, 0, 1);
    return mat4(v1, v2, v3, v4);
}

void main(void)
{
    vec2 aSize = vec2(aSizeAndPosition[0], aSizeAndPosition[1]) * ((2.0*float(aEffectType == effectTypeOrb)) + (float(aEffectType != effectTypeOrb)));
    vec2 aPosOnCanvas = (vec2(aSizeAndPosition[2], aSizeAndPosition[3]) - vec2(0.5, 0.5)) * 2.0;

	// Note that the order is Scale->Rotate->Scale->Translate. The first scaling is to size the quad in absolute terms properly,
	// the second scaling is to get the proper size relative to the canvas
	vec4 final_pos = aPos * scalingMatrix2D(aSize[0], aSize[1]) * rotationMatrix2D(aRotation) * scalingMatrix2D(1.0 / aCanvasAdjustedDimensions[0], 1.0 / aCanvasAdjustedDimensions[1]) * translationMatrix2D(aPosOnCanvas[0], -aPosOnCanvas[1]);

	quadPos = vec2(aPos[0], aPos[1]) * 2.0;

    // For rays and lightning, aShapes, aStyles and aFloatParams are, in order:
	// aShapes: widthAdjType, reshape, blank, blank
	// aStyles: colorTypes, opacity, graintyTexture, blank
	// aFloatParams: intensity, waveCyclePos, opacityAdj, blank

	// For orbs, aShapes, aStyles and aFloatParams are, in order:
	// aShapes: orbLifetime, orbCurrFrame, orbDistortion, orbDetail
	// aStyles: orbStyle, orbAnimation, opacity, blank
	// aFloatParams: intensity, orbSecondaryOpacity, opacityAdj, blank

	// For particles, aShapes, aStyles and aFloatParams are, in order:
	// aShapes: orbLifetime, orbCurrFrame, orbDistortion, blank
	// aStyles: orbStyle, particleDestiny, opacity, blank
	// aFloatParams: minRadius, maxRadius, opacityAdj, blank
	// Note, rotation is already supplied separately as aRotation, and orbRadius is reused as maxRadius, along with orbStyle as style
	// and orbLifetime/orbCurrFrame as lifetime/currFrame respectively.
	// orbRadius, particleMinRadius, primaryColor, secondaryColor, opacityAdj, lifetime, currFrame, rotation, style, particleDestiny


    rayWidthAdjType = aShapes[0];
    rayReshape = aShapes[1];
    rayColorTypes = aStyles[0];
    rayOpacity = aStyles[1];
    rayGrainyTexture = aStyles[2];
    particleDestiny = aStyles[1];
    particleMinRadius = aFloatParams[0];
    orbLifetime = aShapes[0];
    orbCurrFrame = aShapes[1];
    orbDistortion = aShapes[2];
    orbDetail = aShapes[3];
    orbStyle = aStyles[0];
    orbAnimation = aStyles[1];
    orbSecondaryOpacity = aFloatParams[1];

    depth = aDepth;
    rotation = aRotation;
    intensity = aFloatParams[0];
    opacityAdj = aFloatParams[2];
    waveCyclePos = aFloatParams[3];
    primaryColor = aPrimaryColor;
    secondaryColor = aSecondaryColor;
    quadSize = aSize;
    gl_Position = final_pos;
	seed = aSeed;
	effectType = aEffectType;
	blendMode = aBlendMode;
}
