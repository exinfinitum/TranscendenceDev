#version 410 core
layout (location = 0) in vec2 texture_uv;
layout (location = 1) in vec2 texture_pos;
layout (location = 2) in vec2 texture_size;
layout (location = 3) in vec2 fragment_pos;
layout (location = 4) in float alpha_strength;
layout (location = 5) in float depth;
layout (location = 6) in vec4 glow_color;
layout (location = 7) in float glow_noise;
layout (location = 8) in vec2 texture_bounds_min;
layout (location = 9) in vec2 texture_bounds_max;
layout (location = 10) flat in int render_category;
layout (location = 11) in vec2 texture_raw_pos;
layout (location = 12) flat in int blendMode;


out vec4 out_color;

uniform sampler2D obj_texture;
uniform sampler2D glow_map;
uniform int current_tick;
uniform int glowmap_pad_size;
uniform sampler3D perlin_noise;

const int renderCategoryObjectCartesian = 0;
const int renderCategoryText = 1;
const int renderCategoryObjectPolar = 2;
const int renderCategoryObjectCartesianGrayscale = 3;
const float PI = 3.14159;

// This should match enum blendMode in opengl.h.

const int blendNormal = 0;      //      Normal drawing
const int blendMultiply = 1;      //      Darkens images
const int blendOverlay = 2;      //      Combine multiply/screen
const int blendScreen = 3;      //      Brightens images
const int blendHardLight = 4;
const int blendCompositeNormal = 5;

//	Classic Perlin 3D Noise 
//	by Stefan Gustavson
//
//  Found at https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}
vec3 fade(vec3 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}

float cnoise(vec3 P){
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod(Pi0, 289.0);
  Pi1 = mod(Pi1, 289.0);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 / 7.0;
  vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 / 7.0;
  vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
  return 2.2 * n_xyz;
}

ivec2 getPixelGridSquareUnpadded(vec2 coords, vec2 texture_size) {
    int x_coord = int(coords[0] / texture_size[0]);
	int y_coord = int(coords[1] / texture_size[1]);
	return ivec2(x_coord, y_coord);
}

vec4 sampleFromGlowMap(vec2 texture_uv, vec2 texture_size)
{
	// Convert the raw (unpadded) fragment pos to properly deal with glowmap padding
	ivec2 quad_index = getPixelGridSquareUnpadded(texture_uv, texture_size);
	vec2 glowmap_fragment_pos = texture_uv;
	glowmap_fragment_pos = glowmap_fragment_pos * textureSize(obj_texture, 0);
	glowmap_fragment_pos = glowmap_fragment_pos + (((2 * quad_index) + vec2(1.0, 1.0)) * glowmap_pad_size);
	glowmap_fragment_pos = glowmap_fragment_pos / textureSize(glow_map, 0);
	return texture(glow_map, glowmap_fragment_pos);
}

vec4 getGlowColor_PerlinNoise(float glowNoisePeriodXY, float alphaNoiseTimeAxis, float epsilon, vec4 color, vec2 texture_size, vec2 texture_uv, sampler2D obj_texture, float perlinNoiseGlow)
{
	float glowBoundaries = texture(glow_map, vec2(texture_uv[0], texture_uv[1]))[3];
	
	vec4 glowColor = vec4(glow_color[0], glow_color[1], glow_color[2], glow_color[3] * ((perlinNoiseGlow / 2.0) + 0.5)) * glowBoundaries;
	
	return glowColor;
}

vec4 getGlowColor_Static(float epsilon, vec4 color, vec2 texture_size, vec2 texture_uv, sampler2D obj_texture)
{
	float glowBoundaries = texture(glow_map, vec2(texture_uv[0], texture_uv[1]))[3];
	
	vec4 glowColor = vec4(glow_color[0], glow_color[1], glow_color[2], glow_color[3]) * glowBoundaries;
	
	return glowColor;
}

vec4 getTextColor(vec4 color, vec2 texture_uv)
{
	float textAlpha = texture(obj_texture, vec2(texture_uv[0], texture_uv[1]))[0];

	vec4 textColor = vec4(color[0], color[1], color[2], textAlpha);

	return textColor;
}

float sampleNoiseFBM(vec3 sampler) {
    return texture(perlin_noise, vec3(sampler[0], sampler[1], sampler[2]))[0];
}

float rand(vec2 co){
  // Canonical PRNG from https://stackoverflow.com/questions/12964279/whats-the-origin-of-this-glsl-rand-one-liner
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float sampleNoisePerlin(vec3 sampler) {
    //return texture(perlin_noise, vec3(sampler[0], sampler[1], abs(mod(sampler[2], 2.0) - 1.0)))[1];
    return texture(perlin_noise, vec3(sampler[0], sampler[1], sampler[2]))[1];
}

float fbm(vec2 p, float time)
{
    // Fractal Brownian Motion using Perlin noise
     float v = 0.0;
     float amp = 0.3;
     float freq = 20;
     int octaves = 3;
     for (int i = 0; i < octaves; i++)
         v += cnoise(vec3(((p + vec2(1000.0, 1000.0)) * (i * freq)), time / 100)) * (amp / (i + 1));

     return v;
}

float stackedPerlin(vec2 p, float time)
{
    // Fractal Brownian Motion using Perlin noise
    float v = 1.0;
    int octaves = 5;
	float dir_multiplier = 0.25;
    for (int i = 0; i < octaves; i++) {
        int rseed = i + 123;
        vec2 rand_dir = normalize(vec2(rand(vec2(rseed, rseed)), rand(vec2(rseed+1, rseed+1)))) * dir_multiplier;
        v *= sampleNoiseFBM(vec3((p * 5 * (rand(vec2(rseed+2, rseed-2)) - 0.5)) + time*rand_dir, 0.0)) * 2.0;
    }

    return v / 2.0;
}

vec4 sampleCircular(sampler2D tex_sample) {
    // This is used for shockwaves. Works just like regular texture painting, except we use the following UV mapping:
	// x_tex to quad angle, and y_tex to quad distance from center
	float distanceFromCenter = length(vec2(texture_raw_pos[0], texture_raw_pos[1]));
	vec2 polarTexPos = vec2(atan(texture_raw_pos[1], texture_raw_pos[0]) / (2 * PI), 0.5 - (2.0 * distanceFromCenter));
	vec2 fixedTexPos = texture_bounds_min;
	vec2 texPositionOffset = (texture_bounds_max - fixedTexPos) / 2.0;
	vec2 polarTexPos2d = vec2(polarTexPos[0] * texture_size[0], polarTexPos[1] * texture_size[1]) + fixedTexPos + texPositionOffset;
	//return vec4(polarTexPos2d[0], 0.0, polarTexPos2d[1], 1.0);
	return vec4(texture(tex_sample, polarTexPos2d)) * float(distanceFromCenter < 0.5);
}

float stackedPerlin(vec3 p) {
    return stackedPerlin(vec2(p.x, p.y), p.z);
}

void main(void)
{		
	float epsilon = 0.01;
	float alphaNoisePeriodTime = glow_noise;
	float alphaNoisePeriodXY = 1000.0f;
	float glowNoisePeriodXY = 10.0f;

	vec4 realColorCartesian = texture(obj_texture, vec2(texture_uv[0], texture_uv[1]));
	vec4 realColorPolar = sampleCircular(obj_texture);
	vec4 realColor = (
		(realColorCartesian * float(render_category == renderCategoryObjectCartesian || render_category == renderCategoryObjectCartesianGrayscale)) +
        (realColorPolar * float(render_category == renderCategoryObjectPolar))
	);

	float alphaNoiseTimeAxis = (float(current_tick) / max(alphaNoisePeriodTime, epsilon));
	float perlinNoise = (sampleNoiseFBM(vec3(fragment_pos[0] * alphaNoisePeriodXY, fragment_pos[1] * alphaNoisePeriodXY, alphaNoiseTimeAxis)) + 0.0f);
	float alphaNoise = 1.0;
	//float alphaNoise = perlinNoise + float(alpha_strength > 0.9999);
	//alphaNoise = (alpha_strength + (alphaNoise * alpha_strength));
	//alphaNoise = (float(alphaNoise > 0.5) * 2) - 1;
	
	vec4 glowColorPerlin = getGlowColor_PerlinNoise(20.0f, alphaNoiseTimeAxis, epsilon, realColor, texture_size, texture_uv, obj_texture, perlinNoise);
	vec4 glowColorStatic = getGlowColor_Static(epsilon, realColor, texture_size, texture_uv, obj_texture);
	vec4 textColor = getTextColor(glow_color, texture_uv);
	bool useStaticNoise = (glow_noise < epsilon);
	vec4 glowColor = (float(useStaticNoise) * glowColorStatic) + (float(!useStaticNoise) * glowColorPerlin);
	
	// If a glow color is specified, use glow instead of using the ship texture
	// Also add noise to the glow as appropriate
	// Glow size is controlled via the quad size; we use separate quads for glow and standard ship texture
	bool useGlow = (glow_color[3] > epsilon);
	
	
	vec4 textureColor = vec4(realColor[0], realColor[1], realColor[2], realColor[3] * alphaNoise * alpha_strength);
	//float ftime = float(current_tick) / 60.0;
	//vec4 perlin_noise_color = vec4(sampleNoiseFBM(vec3(fragment_pos[0], fragment_pos[1], ftime)));
	//perlin_noise_color[3] = 1.0;
	//out_color = perlin_noise_color;
	vec4 objectColor = (float(!useGlow) * textureColor) + (float(useGlow) * glowColor);
	float grayscaleIntensity = length(vec3(objectColor[0], objectColor[1], objectColor[2]));
	vec4 grayscaleColor = vec4(grayscaleIntensity, grayscaleIntensity, grayscaleIntensity, objectColor[3]);

	bool usePreMultipliedAlpha = (
		(blendMode == blendScreen)
	);

    vec4 finalColor = (
        (objectColor * float(render_category == renderCategoryObjectCartesian || render_category == renderCategoryObjectPolar)) +
        (textColor * float(render_category == renderCategoryText)) +
		(grayscaleColor * float(render_category == renderCategoryObjectCartesianGrayscale))
	);

	bool alphaIsZero = finalColor[3] < epsilon;
	gl_FragDepth = depth + float(alphaIsZero && (glowColor[3] < epsilon));

	vec3 finalColorRGB = (vec3(finalColor[0], finalColor[1], finalColor[2]) * float(!usePreMultipliedAlpha)) + (vec3(finalColor[0], finalColor[1], finalColor[2]) * float(usePreMultipliedAlpha) * finalColor[3]);

	out_color = vec4(finalColorRGB[0], finalColorRGB[1], finalColorRGB[2], finalColor[3]);
}