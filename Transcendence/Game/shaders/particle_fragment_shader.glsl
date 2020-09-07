#version 410 core


layout (location = 0) in vec2 quadPos;
layout (location = 1) flat in int style; //
layout (location = 2) in float depth; //
layout (location = 3) in vec3 primaryColor; //
layout (location = 4) in vec3 secondaryColor; //
layout (location = 5) in float opacityAdj;
layout (location = 6) in vec2 quadSize;
layout (location = 7) in float orbRadius;
layout (location = 8) flat in int lifetime;
layout (location = 9) flat in int currFrame;
layout (location = 10) flat in int destiny;
layout (location = 11) flat in float rotation;
layout (location = 12) flat in int blendMode; // Note that most particles use NORMAL blending mode
layout (location = 13) flat in float minRadius;


uniform float current_tick;

out vec4 fragColor;

float pixelsDistanceFromCenter = length(quadPos) * (quadSize[0] / 2);

// This should match enum blendMode in opengl.h.
const float pi = 3.14159;

const int blendNormal = 0;      //      Normal drawing
const int blendMultiply = 1;      //      Darkens images
const int blendOverlay = 2;      //      Combine multiply/screen
const int blendScreen = 3;      //      Brightens images
const int blendHardLight = 4;
const int blendCompositeNormal = 5;

const int particleTypeGaseous = 1;
const int particleTypeFire = 2;
const int particleTypeSmoke = 3;
const int particleTypeImage = 4; // This is not handled here; it is handled by the texture shader
const int particleTypeLine = 5; // This is not handled here; it is handled by the ray shader
const int particleTypeGlitter = 6;


float AngleRange (float fMinAngle, float fMaxAngle) {
    bool useCase1 = fMinAngle > fMaxAngle;
    float case1 = fMaxAngle + (2 * pi) - fMinAngle;
    float case2 = fMaxAngle - fMinAngle;
    return (float(useCase1) * case1) + (float(!useCase1) * case2);
}

float AngleBearing (float fDir, float fTarget) {
    float fBearing = AngleRange(fDir, fTarget);
    bool over360 = (fBearing > pi);
    return (float(over360) * (fBearing - (2 * pi))) + (float(!over360) * fBearing);
}

vec4 calcParticleColorGlitter(float fMaxRadius, float fMinRadius, vec3 vPrimaryColor, vec3 vSecondaryColor, float fOpacity) {
    const float HIGHLIGHT_ANGLE = (2 * pi) / 135;
    const float HIGHLIGHT_RANGE = (2 * pi) / 30;
    float fBearing = abs(AngleBearing(HIGHLIGHT_ANGLE, mod(rotation, (2*pi))));
    bool bFaded = (fBearing < HIGHLIGHT_RANGE);
    vec3 rgbColor = mix(vSecondaryColor, vPrimaryColor, min(1.0, fBearing / HIGHLIGHT_RANGE));
    vec3 finalRgbColor = (rgbColor * float(bFaded)) + (vPrimaryColor * float(!bFaded));
    
    float alpha = fOpacity * float(pixelsDistanceFromCenter < fMaxRadius);
    return vec4(finalRgbColor, alpha);
}


vec4 calcParticleColorFlame(float fMaxRadius, float fMinRadius, float fOpacity, float fCore, float fFlame, float fSmoke, float fSmokeBrightness, int iLifetime, int iCurrFrame) {
    vec3 FLAME_CORE_COLOR = vec3(1.0, 0.94, 0.90);
    vec3 FLAME_MIDDLE_COLOR = vec3(1.0, 0.82, 0.0);
    vec3 FLAME_OUTER_COLOR = vec3(1.0, 0.23, 0.11);

    float fLifetime = float(max(1, iLifetime));
	float fCurrFrame = float(iCurrFrame);
    float fLifeLeft = fLifetime - fCurrFrame;
	
    float fFade = max(20.0 / 255.0, min(1.0, (1.0 * fLifeLeft / fLifetime)));
    float fFade2 = 0.0;
    float fCurrRadius = fMinRadius + ((fMaxRadius - fMinRadius) * fCurrFrame / fLifetime);
    float alpha = mix(fFade, fFade2, pixelsDistanceFromCenter / fCurrRadius) * float(pixelsDistanceFromCenter <= fCurrRadius);
    float fSmokeColor = min(1.0, fSmokeBrightness + (2.0 * (1.0 / 255.0) * float(mod(destiny, 25))));
    vec3 rgbSmokeColor = vec3(fSmokeColor);
    bool useFlameOuterColor = (mod(destiny, 4) != 0);
    vec3 rgbOuterColor = (float(useFlameOuterColor) * FLAME_OUTER_COLOR) + (float(!useFlameOuterColor) * rgbSmokeColor);
    
    bool useCoreColor = fCurrFrame <= fCore;
    vec3 coreColor = mix(FLAME_CORE_COLOR, FLAME_MIDDLE_COLOR, float(fCurrFrame) / fCore) * float(useCoreColor);
    bool useFlameColor = (fCurrFrame <= fFlame) && !useCoreColor;
    vec3 flameColor = mix(FLAME_MIDDLE_COLOR, rgbOuterColor, max(0.0, float(fCurrFrame - fCore) / (fFlame - fCore))) * float(useFlameColor);
    bool useSmokeColor = (fCurrFrame <= fSmoke) && (!useCoreColor) && (!useFlameColor);
    vec3 smokeColor = mix(rgbOuterColor, rgbSmokeColor, max(0.0, float(fCurrFrame - fFlame) / (fSmoke - fFlame))) * float(useSmokeColor);
    bool useFadeColor = fCurrFrame > fSmoke;
    vec3 fadeColor = rgbSmokeColor * float(useFadeColor);
    
    vec3 color = coreColor + flameColor + smokeColor + fadeColor;
    return vec4(color, alpha);
}

vec4 calcParticleColorSmoke(float fMaxRadius, float fMinRadius, float fOpacity, int iLifetime, int iCurrFrame) {
float fLifetime = float(lifetime);
return calcParticleColorFlame(fMaxRadius, fMinRadius, fOpacity, 1.0, max(1.0, fLifetime / 6.0), max(1.0, fLifetime / 4.0), 60.0, iLifetime, iCurrFrame);
}

vec4 calcParticleColorFire(float fMaxRadius, float fMinRadius, float fOpacity, int iLifetime, int iCurrFrame) {
float fLifetime = float(lifetime);
return calcParticleColorFlame(fMaxRadius, fMinRadius, fOpacity, max(1.0, fLifetime / 6.0), max(1.0, fLifetime / 2.0), max(1.0, 3.0 * (fLifetime / 4.0)), 80.0, iLifetime, iCurrFrame);
}

vec4 calcParticleColorGaseous(float fMaxRadius, float fMinRadius, vec3 vPrimaryColor, vec3 vSecondaryColor, float fOpacity, int iLifetime, int iCurrFrame) {
	float fLifetime = float(max(1, iLifetime));
	float fCurrFrame = float(iCurrFrame);
    float fLifeLeft = fLifetime - fCurrFrame;
	
	
    float fFade = max(20.0 / 255.0, min(1.0, (1.0 * fLifeLeft / fLifetime)));
    float fFade2 = fFade / 2.0;
    //float fFade2 = 0.0;
    float fCurrRadius = fMinRadius + ((fMaxRadius - fMinRadius) * fCurrFrame / fLifetime);
    //float fCurrRadius = 500.0;
    float alpha = mix(fFade, fFade2, min(4.0, pixelsDistanceFromCenter) / fCurrRadius) * float(pixelsDistanceFromCenter <= fCurrRadius);
    //float alpha = max(0.0, fOpacity - ((pixelsDistanceFromCenter * fOpacity) / fRadius));
    vec3 color = mix(vPrimaryColor, vSecondaryColor, fCurrFrame / fLifetime) * float(pixelsDistanceFromCenter <= fCurrRadius);
    return vec4(color, fFade) * float(pixelsDistanceFromCenter <= fCurrRadius);
}

void main(void)
{
    vec4 finalColor = (
		(calcParticleColorGaseous(orbRadius, minRadius, primaryColor, secondaryColor, opacityAdj, lifetime, currFrame) * float(style == particleTypeGaseous)) +
		(calcParticleColorGlitter(orbRadius, minRadius, primaryColor, secondaryColor, opacityAdj) * float(style == particleTypeGlitter)) +
		(calcParticleColorFire(orbRadius, minRadius, opacityAdj, lifetime, currFrame) * float(style == particleTypeFire)) +
		(calcParticleColorSmoke(orbRadius, minRadius, opacityAdj, lifetime, currFrame) * float(style == particleTypeSmoke))
	);
    bool usePreMultipliedAlpha = (
        (blendMode == blendScreen)
    );

    vec3 finalColorRGB = (vec3(finalColor[0], finalColor[1], finalColor[2]) * float(!usePreMultipliedAlpha)) + (vec3(finalColor[0], finalColor[1], finalColor[2]) * float(usePreMultipliedAlpha) * finalColor[3]);
    //finalColorRGB = (vec3(float(lifetime)));
    fragColor = vec4(finalColorRGB, finalColor[3]);
    //fragColor = vec4(finalColor);
}
