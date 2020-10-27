#version 410 core
uniform mat4 rotationMatrix;
uniform vec2 aTexStartPoint;
uniform vec2 aTexQuadSizes;

layout (location = 0) in vec3 aPos;

layout (location = 0) out vec2 TexCoord;
//layout (location = 1) out vec2 TexStartPoint;

void main()
{
	// Read coords span from 0 to 1, POSITIVE!!!!
    vec2 fixedTexSize = vec2(aTexQuadSizes[0], aTexQuadSizes[1]);
    vec2 texPositionOffset = vec2(fixedTexSize[0], fixedTexSize[1]) / 1.0;
    vec2 fixedTexPos = vec2(aTexStartPoint[0], aTexStartPoint[1]);
	vec2 quadPosFromZeroToOne = (vec2(aPos[0], aPos[1]) / 2) + vec2(0.5, 0.5);
	vec2 texPos2dRead = vec2(quadPosFromZeroToOne[0] * fixedTexSize[0], quadPosFromZeroToOne[1] * fixedTexSize[1]) + fixedTexPos;

    // Write coords span from -1 to 1
	vec2 positionOffset = vec2(1.0, -1.0) * 1.0f;
	vec2 fixedCanvPos = vec2((0.0 * 2.0) - 1.0, -((0.0 * 2.0) - 1.0));
	fixedCanvPos = fixedCanvPos + positionOffset;
	vec2 texPos2dWrite = vec2(aPos[0] * 1.0, -aPos[1] * 1.0) + fixedCanvPos;

    gl_Position = vec4(texPos2dWrite[0], texPos2dWrite[1], 0.1, 1.0);
	// Note that this inverts the image; since we do two passes and the second pass is over the entirety
	// of an image this should be OK as it inverts the image twice to gives us back a right side up image
	TexCoord = vec2(texPos2dRead[0], texPos2dRead[1]);
	//TexStartPoint = aTexStartPoint;
    //TexCoord = aTexCoord;
}