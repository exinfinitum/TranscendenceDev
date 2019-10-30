#version 330 core
out vec4 FragColor;
  
in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    FragColor = vec4(ourColor, 1.0) * 0.2 + texture(ourTexture, vec2(-TexCoord[0], TexCoord[1]));
}