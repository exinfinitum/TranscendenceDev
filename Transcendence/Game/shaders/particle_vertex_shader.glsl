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
layout (location = 3) in vec3 aPrimaryColor;
layout (location = 4) in vec3 aSecondaryColor;
layout (location = 5) in int aStyle;
layout (location = 6) in int aDestiny;
layout (location = 7) in int aLifetime;
layout (location = 8) in int aCurrFrame;
layout (location = 9) in float aMaxRadius;
layout (location = 10) in float aMinRadius;
layout (location = 11) in float aOpacity;
layout (location = 12) in int aBlendMode;
layout (location = 13) in float aDepth;

uniform vec2 aCanvasAdjustedDimensions;

layout (location = 0) out vec2 quadPos; //
layout (location = 1) flat out int style;
layout (location = 2) out float depth; //
layout (location = 3) out vec3 primaryColor; //
layout (location = 4) out vec3 secondaryColor; //
layout (location = 5) out float opacityAdj;
layout (location = 6) out vec2 quadSize; //
layout (location = 7) out float orbRadius;
layout (location = 8) flat out int lifetime; //
layout (location = 9) flat out int currFrame; //
layout (location = 10) flat out int destiny; //
layout (location = 11) flat out float rotation; //
layout (location = 12) flat out int blendMode; //
layout (location = 13) flat out float minRadius;

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
    // Note that we normally have to multiply aSize by 2 for orb effects, which particles are based off of, but since "width" in particles refers
	// to diameter, we don't in this case
    vec2 aSize = vec2(aSizeAndPosition[0], aSizeAndPosition[1]) * 12.0;
    vec2 aPosOnCanvas = (vec2(aSizeAndPosition[2], aSizeAndPosition[3]) - vec2(0.5, 0.5)) * 2.0;
	vec4 final_pos = aPos * scalingMatrix2D(aSize[0], aSize[1]) * rotationMatrix2D(aRotation) * scalingMatrix2D(1.0 / aCanvasAdjustedDimensions[0], 1.0 / aCanvasAdjustedDimensions[1]) * translationMatrix2D(aPosOnCanvas[0], -aPosOnCanvas[1]);

	quadPos = vec2(aPos[0], aPos[1]) * 2.0;


    opacityAdj = aOpacity;
	orbRadius = aMaxRadius * 1.5;
	minRadius = aMinRadius * 1.5;

	style = aStyle;
	lifetime = aLifetime;
	currFrame = aCurrFrame;
	rotation = aRotation;
	destiny = aDestiny;
    depth = aDepth;
    primaryColor = aPrimaryColor;
    secondaryColor = aSecondaryColor;
    quadSize = aSize;
    gl_Position = final_pos;
	blendMode = aBlendMode;
}
