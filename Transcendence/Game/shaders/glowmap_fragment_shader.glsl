#version 410 core
out vec4 FragColor;
  
layout (location = 0) in vec2 TexCoord;

uniform vec2 gridSquareSize;
uniform sampler2D ourTexture;
uniform int kernelSize;
uniform int use_x_axis;
uniform int second_pass;

float gaussian(float x, float mu, float sigma)
{
	float sqrt_2pi = 2.50662827463;
	float e = 2.718281828;
	float exp_term = (x - mu) / (sigma);
	float top = pow(e, (-(exp_term*exp_term)/2.0f));
	float bottom = sqrt_2pi * sigma;
	return top / bottom;
}

int manhattan_distance(int center_x, int center_y, int query_x, int query_y)
{
    int delta_x = abs(center_x - query_x);
    int delta_y = abs(center_y - query_y);
    
    return delta_x + delta_y;
}

float getSumGauss(int kernel_size)
{
	float percent_std_dev = 0.66;
	float sum_gauss = 0.0;
	int center = int(kernel_size / 2);
	// First find out the total value under the gaussian curve
	for (int i = 0; i < kernel_size; i++)
	{
        float gauss_val = gaussian(float(i), float(center), float(kernel_size) * percent_std_dev);
        sum_gauss = sum_gauss + gauss_val;
	}
	return sum_gauss;
}

ivec2 getPixelGridSquare(vec2 coords) {
    int x_coord = int(coords[0] / gridSquareSize[0]);
	int y_coord = int(coords[1] / gridSquareSize[1]);
	return ivec2(x_coord, y_coord);
}

vec4 getGlowBoundaries_variable(float epsilon, vec2 texture_uv, sampler2D obj_texture, vec2 texture_size, int glow_size, float sum_gauss, bool using_x_axis, bool additive)
{
	float percent_std_dev = 0.66;
    vec2 onePixel = vec2(1.0, 1.0) / texture_size;
    vec2 quadSize_float = gridSquareSize;// / texture_size;
	ivec2 quad_index = getPixelGridSquare(texture_uv);
    int center = int(glow_size / 2);
	vec4 glowBoundaries = vec4(0.0, 0.0, 0.0, 0.0);
	for (int i = 0; i < glow_size; i++)
	{
	    float sample_point = float(i - center);
		float gauss_val = gaussian(float(i), float(center), float(glow_size) * percent_std_dev);
		vec2 offsetX = vec2(onePixel[0] * sample_point, onePixel[1] * 0.0);
		vec2 offsetY = vec2(onePixel[0] * 0.0, onePixel[1] * sample_point);
        vec2 offset0 = (float(using_x_axis) * offsetX) + (float(!using_x_axis) * offsetY);
        vec2 texCoords0 =  vec2(texture_uv[0], texture_uv[1]) + offset0;
        float texAlpha0 = float(texture(obj_texture, texCoords0)[3]);
        bool texInBounds0 = getPixelGridSquare(texCoords0) == quad_index && texCoords0[0] > 0.0f && texCoords0[0] < 1.0f && texCoords0[1] > 0.0f && texCoords0[1] < 1.0f;
        float sampleR = min(1.0, float(texture(obj_texture, texCoords0)[0])) * (gauss_val / sum_gauss) * float(texInBounds0);
		float sampleG = min(1.0, float(texture(obj_texture, texCoords0)[1])) * (gauss_val / sum_gauss) * float(texInBounds0);
		float sampleB = min(1.0, float(texture(obj_texture, texCoords0)[2])) * (gauss_val / sum_gauss) * float(texInBounds0);
		float sampleA = min(1.0, float(texture(obj_texture, texCoords0)[3] > epsilon)) * (gauss_val / sum_gauss) * float(texInBounds0);
		sampleA = (float(!additive) * sampleA) + (float(additive) * (sampleA * texture(obj_texture, texCoords0)[3]));
		glowBoundaries = (vec4(sampleR, sampleG, sampleB, sampleA) + glowBoundaries);
	}
	return vec4(glowBoundaries[0], glowBoundaries[1], glowBoundaries[2], min(1.0, 2.0 * glowBoundaries[3]));
	
}

void main()
{
	float epsilon = 0.0001;
	vec2 texture_size = textureSize(ourTexture, 0);
	float sumGauss = getSumGauss(kernelSize);
	vec4 glowValue = getGlowBoundaries_variable(epsilon, TexCoord, ourTexture, texture_size, kernelSize, sumGauss, (float(use_x_axis) > epsilon), bool(second_pass));
	FragColor = glowValue;
	//FragColor = vec4(glowValue, glowValue, glowValue, glowValue);
}