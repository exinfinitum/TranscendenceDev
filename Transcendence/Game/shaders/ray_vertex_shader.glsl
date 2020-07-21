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
layout (location = 3) in ivec2 aShapes;
layout (location = 4) in ivec3 aStyles;
layout (location = 5) in vec3 aFloatParams;
layout (location = 6) in vec3 aPrimaryColor;
layout (location = 7) in vec3 aSecondaryColor;
layout (location = 8) in float aDepth;

uniform vec2 aCanvasAdjustedDimensions;

layout (location = 0) out vec2 quadPos;
layout (location = 1) flat out int reshape;
layout (location = 2) flat out int widthAdjType;
layout (location = 3) flat out int opacity;
layout (location = 4) flat out int grainyTexture;
layout (location = 5) out float depth;
layout (location = 6) out float intensity;
layout (location = 7) out vec3 primaryColor;
layout (location = 8) out vec3 secondaryColor;
layout (location = 9) out float waveCyclePos;
layout (location = 10) flat out int colorTypes;
layout (location = 11) out float opacityAdj;
layout (location = 12) out vec2 quadSize;

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
    vec2 aSize = vec2(aSizeAndPosition[0], aSizeAndPosition[1]);
    vec2 aPosOnCanvas = (vec2(aSizeAndPosition[2], aSizeAndPosition[3]) - vec2(0.5, 0.5)) * 2.0;
	
	// Fix positions and sizes
	//vec2 fixedSize = aCanvasQuadSizes * 2.0;
	//vec2 fixedTexSize = aTexQuadSizes * 1.0;
	//vec2 positionOffset = vec2(fixedSize[0], -fixedSize[1]) / 2.0;
	//vec2 fixedCanvPos = vec2((aCanvasPositions[0] * 2.0) - 1.0, (aCanvasPositions[1] * -2.0) + 1.0);
	//vec2 fixedTexPos = vec2((aTexPositions[0] * 1.0), (aTexPositions[1] * 1.0));
	//fixedCanvPos = fixedCanvPos + positionOffset;
	//vec2 pos2d = vec2(aPos[0] * fixedSize[0], aPos[1] * fixedSize[1]) + fixedCanvPos;

    //vec4 final_pos = aPos * translationMatrix2D(aPosOnCanvas[0], -aPosOnCanvas[1]);
	//vec4 final_pos = aPos * scalingMatrix2D(aSize[0] * aCanvasAdjustedDimensions[0], aSize[1] * aCanvasAdjustedDimensions[1]) * rotationMatrix2D(aRotation) * translationMatrix2D(aPosOnCanvas[0], -aPosOnCanvas[1]);
	vec4 final_pos = aPos * scalingMatrix2D(aSize[0], aSize[1]) * rotationMatrix2D(aRotation) * scalingMatrix2D(1.0 / aCanvasAdjustedDimensions[0], 1.0 / aCanvasAdjustedDimensions[1]) * translationMatrix2D(aPosOnCanvas[0], -aPosOnCanvas[1]);
	//vec4 final_pos = aPos * translationMatrix2D(aPosOnCanvas[0], -aPosOnCanvas[1]) * rotationMatrix2D(aRotation) * scalingMatrix2D(aSize[0], aSize[1]);

    //vec4 quadPosV4 = (vec4(aPos[0], aPos[1], 0.0, 0.5) * 2.0) * scalingMatrix2D(aSize[0], aSize[1]) * rotationMatrix2D(-aRotation);
	quadPos = vec2(aPos[0], aPos[1]) * 2.0;
	// Note, the iVec values are casted to float but treated as int for some weird reason. This means that the
	// iVecs will contain bits equal to the float representation of their value, but are of type int. We should use intBitsToFloat
	// to fix this.
    widthAdjType = aShapes[0];
    reshape = aShapes[1];
    colorTypes = aStyles[0];
    opacity = aStyles[1];
    grainyTexture = aStyles[2];
    depth = aDepth;
    intensity = aFloatParams[0];
    waveCyclePos = aFloatParams[1];
    opacityAdj = aFloatParams[2];
    primaryColor = aPrimaryColor;
    secondaryColor = aSecondaryColor;
    quadSize = aSize;
    gl_Position = final_pos;
    
}