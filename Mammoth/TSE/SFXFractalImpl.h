//	SFXFractalImpl.h
//
//	Classes to implement explosion effects
//	Copyright (c) 2015 by Kronosaur Productions, LLC. All Rights Reserved

#pragma once

//	CSphericalTextureMapper
//
//	This helper class is used to paint a spherical UV texture to the points in 
//	a circle, given an angle and radius.

class CSphericalTextureMapper
	{
	public:
		CSphericalTextureMapper (void) :
				m_pTexture(NULL)
			{ }

		BYTE GetPixel (int iAngle, int iRadius) const
			{
			return (m_pTexture ? m_pTexture->GetPixel(m_AngleToX[iAngle], m_RadiusToY[iRadius]) : 0);
			}

		void Init (CFractalTextureLibrary::ETextureTypes iTexture, int iFrame, int iRadius, int iAngleRange);

	private:
		const CG8bitImage *m_pTexture;
		TArray<int> m_AngleToX;
		TArray<int> m_RadiusToY;
	};

//	CCircleRadiusDisruptor
//
//	This helper class is used to distort the edges of a circle by adjusting
//	its radius at each angle.

class CCircleRadiusDisruptor
	{
	public:
		int GetAdjustedRadius (int iAngle, int iRadius) const
			{
			return (int)(m_RadiusAdj[iAngle] * iRadius);
			}

		void Init (Metric rDisruption, int iRadius, int iAngleRange);

	private:
		void InitSegment (int iStart, int iCount, Metric rEndAdj, Metric rDisruption);
		Metric RandomPoint (Metric rRange) { return mathRandomGaussian() * rRange; }

		TArray<Metric> m_RadiusAdj;
		TArray<Metric> m_FullRadiusAdj;
	};

//	CExplosionColorizer
//
//	This helper class generates static tables for the color of an explosion 
//	pixel at various radii and intensities.
//
//	We use a normal probability density function to simulate the distribution
//	of heat across the explosion. That is, we start with a very narrow bell 
//	curve (center is very hot) and proceed to wider and wider versions as the
//	heat distributes to the rest of the fireball.

class CExplosionColorizer
	{
	public:
		CG32bitPixel GetPixel (int iRadius, int iMaxRadius, int iIntensity, CG32bitPixel rgbPrimary, CG32bitPixel rgbSecondary) const;
		void Init (void);

	private:
		static constexpr int RADIUS_COUNT =		100;
		static constexpr int INTENSITY_COUNT =	101;	//	0 to 100

		TArray<TArray<Metric>> m_Heat;
	};

//	CCloudCirclePainter
//
//	We paint a spherical fractal cloud with a color table keyed to radius.
//	rFrame is from 0.0 to 1.0. Use (Metric){frame} / {frameCount}.

template <class BLENDER> class CCloudCirclePainter : public TCirclePainter32<CCloudCirclePainter<BLENDER>, BLENDER>
	{
	public:
		CCloudCirclePainter (CFractalTextureLibrary::ETextureTypes iTexture) :
				m_iTexture(iTexture),
				m_pRadiusTable(NULL),
				m_pPixelTable(NULL)
			{
			}

		virtual void SetParam (const CString &sParam, const TArray<CG32bitPixel> &ColorTable)
			{
			if (strEquals(sParam, CONSTLIT("radiusTable")))
				m_pRadiusTable = &ColorTable;
			else if (strEquals(sParam, CONSTLIT("pixelTable")))
				m_pPixelTable = &ColorTable;
			}

	private:
		using TCirclePainter32<CCloudCirclePainter<BLENDER>, BLENDER>::m_iAngleRange;
		using TCirclePainter32<CCloudCirclePainter<BLENDER>, BLENDER>::m_iFrame;
		using TCirclePainter32<CCloudCirclePainter<BLENDER>, BLENDER>::m_iRadius;

		bool BeginDraw (void)
			{
			//	Must have both tables, or else this won't work.

			if (m_pRadiusTable == NULL || m_pPixelTable == NULL || m_pPixelTable->GetCount() < 256)
				return false;

			//	We need enough angular resolution to reach the pixel level (but
			//	no more).

			m_iAngleRange = (int)(m_iRadius * 2.0 * PI);

			//	Set the texture based on the frame

			m_Texture.Init(m_iTexture, m_iFrame, m_iRadius, m_iAngleRange);

			//	Success

			return true;
			}

		CG32bitPixel GetColorAt (int iAngle, int iRadius) const 

		//	GetColorAt
		//
		//	Returns the color at the given position in the circle. NOTE: We must
		//	return a pre-multiplied pixel.

			{
			//	Get the alpha value at the texture position.

			BYTE byAlpha = m_Texture.GetPixel(iAngle, iRadius);

			//	Get the color from the pixel table.

			CG32bitPixel rgbColor = m_pPixelTable->GetAt(byAlpha);

			//	Blend with alpha from the radius table

			rgbColor = CG32bitPixel(rgbColor, CG32bitPixel::BlendAlpha(rgbColor.GetAlpha(), m_pRadiusTable->GetAt(iRadius).GetAlpha()));

			//	Done

			return CG32bitPixel::PreMult(rgbColor);
			}

		//	Context

		CFractalTextureLibrary::ETextureTypes m_iTexture;
		const TArray<CG32bitPixel> *m_pRadiusTable;
		const TArray<CG32bitPixel> *m_pPixelTable;

		//	Run time parameters for drawing a single frame.

		CSphericalTextureMapper m_Texture;

		friend TCirclePainter32;
	};

//	CDiffractionCirclePainter
//
//	We draw a set of dotted diffraction circles

template <class BLENDER> class CDiffractionCirclePainter : public TCirclePainter32<CDiffractionCirclePainter<BLENDER>, BLENDER>
	{
	public:
		CDiffractionCirclePainter (void) :
				m_pColorTable(NULL)
			{
			}

		virtual void SetParam (const CString &sParam, const TArray<CG32bitPixel> &ColorTable)
			{
			if (strEquals(sParam, CONSTLIT("colorTable")))
				m_pColorTable = &ColorTable;
			}

	private:
		bool BeginDraw (void)
			{
			return true;
			}

		CG32bitPixel GetColorAt (int iAngle, int iRadius) const 

		//	GetColorAt
		//
		//	Returns the color at the given position in the circle. NOTE: We must
		//	return a pre-multiplied pixel.

			{
			CG32bitPixel rgbColor = (m_pColorTable ? m_pColorTable->GetAt(iRadius) : CG32bitPixel(255, 0, 0));

			//	We create concentric circles

			if ((iRadius % 2) == 1)
				rgbColor = CG32bitPixel(CG32bitPixel::Blend(CG32bitPixel(0, 0, 0), rgbColor, (BYTE)128), rgbColor.GetAlpha());
			else
				rgbColor = CG32bitPixel(CG32bitPixel::Blend(rgbColor, CG32bitPixel(255, 255, 255), (BYTE)128), rgbColor.GetAlpha());

			//	Done

			return CG32bitPixel::PreMult(rgbColor);
			}

		const TArray<CG32bitPixel> *m_pColorTable;

		friend TCirclePainter32;
	};

//	CFireblastCirclePainter
//
//	A fireball has two layers: the bottom layer is a radially symmetric glow 
//	effect (using a given color table). On top we paint a fractal cloud wrapped
//	on a sphere (to simulate the fractal smoke patterns in an explosion). This
//	second layer has its own color table (also keyed to radius).
//
//	For efficiency we paint both layers at the same time, using the fractal 
//	cloud pattern as the discriminator.

template <class BLENDER> class CFireblastCirclePainter : public TCirclePainter32<CFireblastCirclePainter<BLENDER>, BLENDER>
	{
	public:
		CFireblastCirclePainter (CFractalTextureLibrary::ETextureTypes iTexture, Metric rDistortion = 0.0) :
				m_iTexture(iTexture),
				m_rDistortion(rDistortion),
				m_pExplosionTable(NULL),
				m_pSmokeTable(NULL)
			{
			}

		virtual void SetParam (const CString &sParam, const TArray<CG32bitPixel> &ColorTable)
			{
			if (strEquals(sParam, CONSTLIT("explosionTable")))
				m_pExplosionTable = &ColorTable;
			else if (strEquals(sParam, CONSTLIT("smokeTable")))
				m_pSmokeTable = &ColorTable;
			}

	private:
		using TCirclePainter32<CFireblastCirclePainter<BLENDER>, BLENDER>::m_iAngleRange;
		using TCirclePainter32<CFireblastCirclePainter<BLENDER>, BLENDER>::m_iFrame;
		using TCirclePainter32<CFireblastCirclePainter<BLENDER>, BLENDER>::m_iRadius;

		bool BeginDraw (void)
			{
			//	We need enough angular resolution to reach the pixel level (but
			//	no more).

			m_iAngleRange = (int)(m_iRadius * 2.0 * PI);

			//	Set the texture based on the frame

			m_Texture.Init(m_iTexture, m_iFrame, m_iRadius, m_iAngleRange);

			//	Initialize the disruptor

			m_Disruptor.Init(m_rDistortion, m_iRadius, m_iAngleRange);

			//	Success

			return true;
			}

		CG32bitPixel GetColorAt (int iAngle, int iRadius) const 

		//	GetColorAt
		//
		//	Returns the color at the given position in the circle. NOTE: We must
		//	return a pre-multiplied pixel.

			{
			//	Adjust the radius based on the disruptor

			int iNewRadius = m_Disruptor.GetAdjustedRadius(iAngle, iRadius);
			if (iNewRadius >= m_iRadius)
				return CG32bitPixel::Null();

			//	Get the alpha value at the texture position.

			BYTE byAlpha = m_Texture.GetPixel(iAngle, iNewRadius);

			//	Combine the explosion with the smoke, using the texture as the
			//	discriminator.

			CG32bitPixel rgbColor = CG32bitPixel::Interpolate(m_pExplosionTable->GetAt(iNewRadius), m_pSmokeTable->GetAt(iNewRadius), byAlpha);
				
			//	Return the value (premultiplied)

			return CG32bitPixel::PreMult(rgbColor);
			}

	private:
		CFractalTextureLibrary::ETextureTypes m_iTexture;
		Metric m_rDistortion;
		const TArray<CG32bitPixel> *m_pExplosionTable;
		const TArray<CG32bitPixel> *m_pSmokeTable;

		//	Run time parameters for drawing a single frame.

		CSphericalTextureMapper m_Texture;
		CCircleRadiusDisruptor m_Disruptor;

		friend TCirclePainter32<CFireblastCirclePainter<BLENDER>, BLENDER>;
	};
