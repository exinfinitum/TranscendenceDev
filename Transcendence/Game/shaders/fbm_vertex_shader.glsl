#version 410 core
uniform mat4 rotationMatrix;

in vec3 aPos;

out vec2 TexCoord;

void main()
{
    gl_Position = rotationMatrix * vec4(aPos, 1.0);
    TexCoord = vec2(aPos.x, aPos.y);
}