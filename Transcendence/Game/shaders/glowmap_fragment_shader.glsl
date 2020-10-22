#version 410 core
out vec4 FragColor;
  
layout (location = 0) in vec2 TexCoord;

uniform vec2 aTexStartPoint;
uniform vec2 aTexQuadSizes;
uniform vec2 gridSquareSize;
uniform sampler2D ourTexture;
uniform int kernelSize;
uniform int pad_pixels_per_grid_square;
uniform int pixel_decimal_place_per_channel_for_linear_glowmap;
uniform ivec2 num_grid_squares;
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

ivec2 getPixelGridSquareUnpadded(vec2 coords) {
    vec2 adjusted_coords = coords - aTexStartPoint;
    int x_coord = int(adjusted_coords[0] / gridSquareSize[0]);
	int y_coord = int(adjusted_coords[1] / gridSquareSize[1]);
	return ivec2(x_coord, y_coord);
}

int obtainPixelDistanceFromFloatVec(vec3 dist_channels) {
	float pixelDistanceIColumn = dist_channels[2] * 100.0;
	float pixelDistanceCColumn = dist_channels[1] * 100.0;
	float pixelDistance10KColumn = dist_channels[0] * 100.0;
	return int((pixelDistance10KColumn * 10000.0) + (pixelDistanceCColumn * 100.0) + (pixelDistanceIColumn));
}

vec4 getGlowBoundaries_variable(float epsilon, vec2 texture_uv, sampler2D obj_texture, vec2 texture_size_input, int glow_size, float sum_gauss, bool using_x_axis, bool additive, vec2 pad_fraction_of_output_image, vec2 texture_size_output)
{
	float percent_std_dev = 0.66;
    vec2 onePixel = vec2(1.0, 1.0) / texture_size_input;
    vec2 quadSize_float = gridSquareSize;//
	ivec2 quad_index = getPixelGridSquareUnpadded(texture_uv);
    int center = int(glow_size / 2);
	vec4 glowBoundaries = vec4(0.0, 0.0, 0.0, 0.0);
	int distanceToNearestNonZeroAlphaPixel = int(max(texture_size_input[0], texture_size_input[1]));
	for (int i = 0; i < glow_size; i++)
	{
		// Given the sum of the pixel values under the kernel, blur this pixel using a gaussian blur technique.
		// Note that this is a normalized gaussian blur, so we take the weighted average of all pixels under this kernel.
	    float sample_point = float(i - center);
		float gauss_val = gaussian(float(i), float(center), float(glow_size) * percent_std_dev);
		vec2 offsetX = vec2(onePixel[0] * sample_point, onePixel[1] * 0.0);
		vec2 offsetY = vec2(onePixel[0] * 0.0, onePixel[1] * sample_point);
		// Offset indicates which pixel under this gaussian kernel we are currently considering
        vec2 offset0 = (float(using_x_axis) * offsetX) + (float(!using_x_axis) * offsetY);
		// To get the texture coordinates: note the scale between the input and the output image.
		// If the UV coordinates after offset lie in a padded region, then the sample result should be zero.
		// Otherwise, if the UV coordinates after offset do NOT lie in a padded region, then get the padded pixel
		// grid square location (xp, yp). Subtract((2 * xp) + 1, (2 * yp) + 1) * padSizeInPixels from the pixel coordinates of the
		// output; this is the pixel coordinates we need to sample from in the input. Note that if we apply the subtraction
		// and apply zero minimum bound, then padded pixels in output image will map to row or column zero in the input image
		// or map to out-of-quad locations, so they will effectively be zero.
        vec2 texCoords0 =  vec2(texture_uv[0], texture_uv[1]);
		texCoords0 = texCoords0 - aTexStartPoint;
		texCoords0 = texCoords0 * texture_size_output;
		texCoords0 = texCoords0 - (((2 * quad_index) + vec2(1.0, 1.0)) * pad_pixels_per_grid_square);
		texCoords0 = texCoords0 / texture_size_input;
		texCoords0 = texCoords0 + aTexStartPoint;
		texCoords0 = texCoords0 + offset0;
		vec2 texEndPoint = aTexStartPoint + aTexQuadSizes;
        bool texInBounds0 = getPixelGridSquareUnpadded(texCoords0) == quad_index && texCoords0[0] > aTexStartPoint[0] && texCoords0[0] < texEndPoint[0] && texCoords0[1] > aTexStartPoint[1] && texCoords0[1] < texEndPoint[1];
		vec4 sample_value = texture(obj_texture, texCoords0);

		// For linear glow map, what we do is the following:
		// For each pixel, in the first pass, simply obtain the distance to the nearest pixel with non-zero
		// alpha value in the axis of choice.
		// For the second pass, do the same, but this time check the distance to the pixel that has a marked
		// distance to non-zero alpha pixel in the first axis. Use normalized vector to get the total distance.
		int distanceToNearestNonZeroAlphaPixelPreviousPass = obtainPixelDistanceFromFloatVec(vec3(sample_value[0], sample_value[1], sample_value[2]));
		int candidateDistanceToNearestNonZeroAlphaPixel = abs(i - center);
		candidateDistanceToNearestNonZeroAlphaPixel = (
			(int(!additive) * candidateDistanceToNearestNonZeroAlphaPixel) +
			(int(additive) * int(round(length(ivec2(candidateDistanceToNearestNonZeroAlphaPixel, distanceToNearestNonZeroAlphaPixelPreviousPass)))))
		);
		bool reducedDistanceToNonZeroPixel = sample_value[3] > epsilon;
		distanceToNearestNonZeroAlphaPixel = (
			(int(reducedDistanceToNonZeroPixel) * min(distanceToNearestNonZeroAlphaPixel, candidateDistanceToNearestNonZeroAlphaPixel)) +
			(int(!reducedDistanceToNonZeroPixel) * distanceToNearestNonZeroAlphaPixel)
			);

        float sampleR = min(1.0, float(sample_value[0])) * (gauss_val / sum_gauss) * float(texInBounds0);
		float sampleG = min(1.0, float(sample_value[1])) * (gauss_val / sum_gauss) * float(texInBounds0);
		float sampleB = min(1.0, float(sample_value[2])) * (gauss_val / sum_gauss) * float(texInBounds0);
        float sampleRF = min(1.0, float(texture(obj_texture, texCoords0 - offset0)[0])) * float(texInBounds0);
		float sampleGF = min(1.0, float(texture(obj_texture, texCoords0 - offset0)[1])) * float(texInBounds0);
		float sampleBF = min(1.0, float(texture(obj_texture, texCoords0 - offset0)[2])) * float(texInBounds0);
		float sampleA = min(1.0, float(sample_value[3] > epsilon)) * (gauss_val / sum_gauss) * float(texInBounds0);
		sampleA = (float(!additive) * sampleA) + (float(additive) * (sampleA * sample_value[3]));
		glowBoundaries = (vec4(sampleR, sampleG, sampleB, sampleA) + glowBoundaries);
	}
	float pixelDistanceIColumn = float(mod(distanceToNearestNonZeroAlphaPixel, 100)) / 100.0;
	float pixelDistanceCColumn = float(mod(distanceToNearestNonZeroAlphaPixel / 100, 100)) / 100.0;
	float pixelDistance10KColumn = float(mod(distanceToNearestNonZeroAlphaPixel / 10000, 100)) / 100.0;
	return vec4(pixelDistance10KColumn, pixelDistanceCColumn, pixelDistanceIColumn, float(distanceToNearestNonZeroAlphaPixel < max(texture_size_input[0], texture_size_input[1])));
	//return vec4(glowBoundaries[0], glowBoundaries[1], glowBoundaries[2], min(1.0, 2.0 * glowBoundaries[3]));
	
}

void main()
{
	float epsilon = 0.0001;
	vec2 texture_size_input = textureSize(ourTexture, 0);
	vec2 texture_size_output = (textureSize(ourTexture, 0)) + ((1.0 / gridSquareSize) * (2 * pad_pixels_per_grid_square));
	vec2 pad_fraction_of_output_image = pad_pixels_per_grid_square / texture_size_output;
	float sumGauss = getSumGauss(kernelSize);
	vec4 glowValue = getGlowBoundaries_variable(epsilon, TexCoord, ourTexture, texture_size_input, kernelSize, sumGauss, (float(use_x_axis) > epsilon), bool(second_pass), pad_fraction_of_output_image, texture_size_output);
	FragColor = glowValue;
}