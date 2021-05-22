//	SFXGlow.cpp
//
//	CGlowEffectCreator class
//	Copyright (c) 2019 Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

#define ANIMATE_ATTRIB					CONSTLIT("animate")
#define BLEND_MODE_ATTRIB				CONSTLIT("blendMode")
#define LIFETIME_ATTRIB					CONSTLIT("lifetime")
#define PRIMARY_COLOR_ATTRIB			CONSTLIT("primaryColor")
#define RADIUS_ATTRIB					CONSTLIT("radius")
#define SECONDARY_COLOR_ATTRIB			CONSTLIT("secondaryColor")
#define OPACITY_ATTRIB					CONSTLIT("opacity")
#define STYLE_ATTRIB					CONSTLIT("style")
#define USE_SOURCE_OBJ_ATTRIB			CONSTLIT("useSourceObj")
#define RADIUS_AROUND_IMPACT_ATTRIB		CONSTLIT("radiusAroundImpact")
#define WAVE_WIDTH_ATTRIB				CONSTLIT("waveWidth")

class CGlowEffectPainter : public IEffectPainter
	{
	public:
		CGlowEffectPainter (CEffectCreator *pCreator);
		~CGlowEffectPainter (void);

		//	IEffectPainter virtuals

		virtual CEffectCreator *GetCreator (void) override { return m_pCreator; }
		virtual int GetLifetime (void) override { return m_iLifetime; }
		virtual bool GetParam (const CString &sParam, CEffectParamDesc *retValue) const override;
		virtual bool GetParamList (TArray<CString> *retList) const override;
		virtual void GetRect (RECT *retRect) const override;
		virtual void Paint (CG32bitImage &Dest, int x, int y, SViewportPaintCtx &Ctx) override;
		virtual bool PointInImage (int x, int y, int iTick, int iVariant = 0, int iRotation = 0) const override;

	protected:
		virtual bool OnSetParam (CCreatePainterCtx &Ctx, const CString &sParam, const CEffectParamDesc &Value) override;

	private:
		enum EAnimationTypes
			{
			//	Do not change order; IDs saved in save file

			animateNone =			0,

			animateExplode =		1,
			animateFade =			2,
			animateChargeUp =		3,

			animateMax =			3,
			};

		enum EStyles
			{
			//	Do not change order; IDs saved in save file

			styleUnknown =			0,

			styleFull =				1,
			styleOutline =			2,
			styleRipple =			3,
			styleRippleWave =		4,
			styleRippleNegative =	5,

			styleMax =				5,
			};

		struct SCacheEntry
			{
			CG32bitImage GlowImage;
			int xOffset;
			int yOffset;
			};
		
		bool CalcIntermediates (void) const;
		const SCacheEntry &GetGlowImage (const CSpaceObject &Source) const;
		void Reset (void) const;

		static DWORDLONG MakeCacheKey (DWORD dwSourceID, int iSourceRotation);

		CEffectCreator *m_pCreator = NULL;

		EStyles m_iStyle = styleFull;
		int m_iRadius = 10;
		CG32bitPixel m_rgbPrimaryColor = CG32bitPixel(255, 255, 255);
		CG32bitPixel m_rgbSecondaryColor = CG32bitPixel(128, 128, 128);
		CGDraw::EBlendModes m_iBlendMode = CGDraw::blendNormal;

		int m_iLifetime = 0;
		EAnimationTypes m_iAnimate = animateNone;
		const CSpaceObject* m_pSource = nullptr;
		CVector m_vHitVector;
		bool m_bUseDamageCtxSource = false;
		int m_iOpacity = 0;
		int m_iRadiusAroundImpact = -1;
		int m_iWaveWidth = 0;

		//	Temporary variables during painting

		mutable bool m_bInitialized = false;

		mutable TSortMap<DWORDLONG, SCacheEntry> m_Cache;
		mutable TArray<BYTE> m_OpacityTable;
	};

static LPCSTR ANIMATION_TABLE[] =
	{
	//	Must be same order as EAnimationTypes
		"",

		"explode",
		"fade",
		"chargeUp",

		NULL
	};

static LPCSTR STYLE_TABLE[] =
	{
	//	Must be same order as EStyles
		"",

		"full",
		"outline",
		"ripple",
		"rippleWave",
		"rippleNegative",

		NULL,
	};


CGlowEffectCreator::~CGlowEffectCreator (void)

//	CGlowEffectCreator destructor

	{
	if (m_pSingleton)
		delete m_pSingleton;
	}

IEffectPainter *CGlowEffectCreator::OnCreatePainter (CCreatePainterCtx &Ctx)

//	CreatePainter
//
//	Creates a new painter
	
	{
	//	If we have a singleton, return that

	if (m_pSingleton && Ctx.CanCreateSingleton())
		return m_pSingleton;

	//	Otherwise we need to create a painter with the actual
	//	parameters.

	IEffectPainter *pPainter = new CGlowEffectPainter(this);

	//	Initialize the painter parameters

	pPainter->SetParam(Ctx, ANIMATE_ATTRIB, m_Animate);
	pPainter->SetParam(Ctx, BLEND_MODE_ATTRIB, m_BlendMode);
	pPainter->SetParam(Ctx, LIFETIME_ATTRIB, m_Lifetime);
	pPainter->SetParam(Ctx, PRIMARY_COLOR_ATTRIB, m_PrimaryColor);
	pPainter->SetParam(Ctx, RADIUS_ATTRIB, m_Radius);
	pPainter->SetParam(Ctx, SECONDARY_COLOR_ATTRIB, m_SecondaryColor);
	pPainter->SetParam(Ctx, STYLE_ATTRIB, m_Style);
	pPainter->SetParam(Ctx, OPACITY_ATTRIB, m_Opacity);
	pPainter->SetParam(Ctx, USE_SOURCE_OBJ_ATTRIB, m_UseSourceObj);
	pPainter->SetParam(Ctx, RADIUS_AROUND_IMPACT_ATTRIB, m_RadiusAroundImpact);
	pPainter->SetParam(Ctx, WAVE_WIDTH_ATTRIB, m_WaveWidth);

	//	Initialize via GetParameters, if necessary

	InitPainterParameters(Ctx, pPainter);

	//	If we are a singleton, then we only need to create this once.

	if (GetInstance() == instGame
			&& Ctx.CanCreateSingleton())
		{
		pPainter->SetSingleton(true);
		m_pSingleton = pPainter;
		}

	return pPainter;
	}

ALERROR CGlowEffectCreator::OnEffectCreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, const CString &sUNID)

//	OnEffectCreateFromXML
//
//	Initialize type-specific data

	{
	ALERROR error;

	if (error = m_Animate.InitIdentifierFromXML(Ctx, pDesc->GetAttribute(ANIMATE_ATTRIB), ANIMATION_TABLE))
		return error;

	if (error = m_BlendMode.InitBlendModeFromXML(Ctx, pDesc->GetAttribute(BLEND_MODE_ATTRIB)))
		return error;

	if (error = m_Lifetime.InitIntegerFromXML(Ctx, pDesc->GetAttribute(LIFETIME_ATTRIB)))
		return error;

	if (error = m_PrimaryColor.InitColorFromXML(Ctx, pDesc->GetAttribute(PRIMARY_COLOR_ATTRIB)))
		return error;

	if (error = m_Radius.InitIntegerFromXML(Ctx, pDesc->GetAttribute(RADIUS_ATTRIB)))
		return error;

	if (error = m_SecondaryColor.InitColorFromXML(Ctx, pDesc->GetAttribute(SECONDARY_COLOR_ATTRIB)))
		return error;

	if (error = m_Opacity.InitIntegerFromXML(Ctx, pDesc->GetAttribute(OPACITY_ATTRIB)))
		return error;

	if (error = m_RadiusAroundImpact.InitIntegerFromXML(Ctx, pDesc->GetAttribute(RADIUS_AROUND_IMPACT_ATTRIB)))
		return error;

	if (error = m_WaveWidth.InitIntegerFromXML(Ctx, pDesc->GetAttribute(WAVE_WIDTH_ATTRIB)))
		return error;

	if (error = m_Style.InitIdentifierFromXML(Ctx, pDesc->GetAttribute(STYLE_ATTRIB), STYLE_TABLE))
		return error;

	m_UseSourceObj = pDesc->GetAttributeBool(USE_SOURCE_OBJ_ATTRIB);

	return NOERROR;
	}

ALERROR CGlowEffectCreator::OnEffectBindDesign (SDesignLoadCtx &Ctx)

//	OnEffectBindDesign
//
//	Resolve loading

	{
	//	Clean up, because we might want to recompute for next time.

	if (m_pSingleton)
		{
		delete m_pSingleton;
		m_pSingleton = NULL;
		}

	return NOERROR;
	}

//	CGlowEffectPainter ---------------------------------------------------------

CGlowEffectPainter::CGlowEffectPainter (CEffectCreator *pCreator) : 
		m_pCreator(pCreator)

//	CGlowEffectPainter constructor

	{
	}

CGlowEffectPainter::~CGlowEffectPainter (void)

//	CGlowEffectPainter destructor

	{
	}

bool CGlowEffectPainter::CalcIntermediates (void) const

//	CalcIntermediates
//
//	Calculates some intermediate values, such as color tables before painting.
//	Returns FALSE if we do not need to paint.

	{
	if (m_bInitialized)
		return true;

	//	Reset tables

	Reset();

	//	Calculate the opacity table

	switch (m_iAnimate)
		{
		case animateChargeUp:
			{
			int iLifetime = Max(1, m_iLifetime);
			CStepIncrementor Opacity(CStepIncrementor::styleLinear, 0, 255, iLifetime);

			m_OpacityTable.InsertEmpty(iLifetime);
			for (int i = 0; i < iLifetime; i++)
				m_OpacityTable[i] = (BYTE)mathRound(Opacity.GetAt(i));

			break;
			}

		case animateExplode:
			{
			int iLifetime = Max(1, m_iLifetime);
			int iPeak = mathRound(0.2 * iLifetime);
			CStepIncrementor RampUp(CStepIncrementor::styleSquareRoot, 0, 255, iPeak);

			m_OpacityTable.InsertEmpty(iLifetime);
			for (int i = 0; i < iPeak; i++)
				m_OpacityTable[i] = (BYTE)mathRound(RampUp.GetAt(i));

			CStepIncrementor RampDown(CStepIncrementor::styleSquareRoot, 255, 0, iLifetime - iPeak);
			for (int i = iPeak; i < iLifetime; i++)
				m_OpacityTable[i] = (BYTE)mathRound(RampDown.GetAt(i - iPeak));

			break;
			}

		case animateFade:
			{
			int iLifetime = Max(1, m_iLifetime);
			CStepIncrementor Opacity(CStepIncrementor::styleSquareRoot, 255, 0, iLifetime);

			m_OpacityTable.InsertEmpty(iLifetime);
			for (int i = 0; i < iLifetime; i++)
				m_OpacityTable[i] = (BYTE)mathRound(Opacity.GetAt(i));

			break;
			}
		}

	//	Done

	m_bInitialized = true;
	return true;
	}

const CGlowEffectPainter::SCacheEntry &CGlowEffectPainter::GetGlowImage (const CSpaceObject &Source) const

//	GetGlowImage
//
//	Returns an 8-bit alpha image representing the glow.

	{
	//	Get the source object

	DWORD dwSourceID = Source.GetID();
	int iSourceRotation = Source.GetRotationFrameIndex();

	//	Look for the image in the cache.

	bool bNew;
	SCacheEntry *pCache = m_Cache.SetAt(MakeCacheKey(dwSourceID, iSourceRotation), &bNew);
	if (!bNew)
		return *pCache;

	//	If not found, we need to create the glow image. Start by getting the
	//	source image and all its details.

	const CObjectImageArray &SourceImage = Source.GetImage();
	const CG32bitImage &RawImage = SourceImage.GetImage(CONSTLIT("SFXGlow"));
	RECT rcSourceImage = SourceImage.GetImageRect(0, iSourceRotation, &pCache->xOffset, &pCache->yOffset);

	//	Create an 8-bit image that is bigger than the source by the given 
	//	radius.

	int iEdge = Max(0, m_iRadius);
	int cxWidth = RectWidth(rcSourceImage) + 2 * iEdge;
	int cyHeight = RectHeight(rcSourceImage) + 2 * iEdge;

	CG8bitImage GlowMask;
	GlowMask.Create(cxWidth, cyHeight);
	GlowMask.CopyChannel(channelAlpha, RawImage, rcSourceImage.left, rcSourceImage.top, RectWidth(rcSourceImage), RectHeight(rcSourceImage), iEdge, iEdge);

	//	Blur the mask

	RECT rcGlowMask;
	rcGlowMask.left = 0;
	rcGlowMask.top = 0;
	rcGlowMask.right = cxWidth;
	rcGlowMask.bottom = cyHeight;

#if 1
	int iBlur = Max(0, m_iRadius);
	CGFilter::Blur(GlowMask, rcGlowMask, iBlur, GlowMask);
#else
	CGFilter::MaxPixel(GlowMask, rcGlowMask, m_iRadius, GlowMask);
#endif

	//	Generate a color table based on primary and secondary color.

	//	Now generate the final 32-bit image by combining the mask and the color
	//	table.

	pCache->GlowImage.Create(cxWidth, cyHeight, CG32bitImage::alpha8);

	BYTE *pSrcRow = GlowMask.GetPixelPos(0, 0);
	BYTE *pSrcRowEnd = GlowMask.GetPixelPos(0, cyHeight);
	CG32bitPixel *pDestRow = pCache->GlowImage.GetPixelPos(0, 0);
	while (pSrcRow < pSrcRowEnd)
		{
		BYTE *pSrcPos = pSrcRow;
		BYTE *pSrcPosEnd = pSrcRow + cxWidth;
		CG32bitPixel *pDestPos = pDestRow;
		while (pSrcPos < pSrcPosEnd)
			{
			*pDestPos = CG32bitPixel(*pSrcPos, *pSrcPos, *pSrcPos, *pSrcPos);
			pSrcPos++;
			pDestPos++;
			}

		pSrcRow = GlowMask.NextRow(pSrcRow);
		pDestRow = pCache->GlowImage.NextRow(pDestRow);
		}

	//	GetImageRect returns the center of the image relative to RawImage. We
	//	need to convert to an offset for the upper-left corner of the new image.

	pCache->xOffset = pCache->xOffset - (rcSourceImage.left + (RectWidth(rcSourceImage) / 2)) - (cxWidth / 2);
	pCache->yOffset = pCache->yOffset - (rcSourceImage.top + (RectHeight(rcSourceImage) / 2)) - (cyHeight / 2);

	//	Done

	return *pCache;
	}

bool CGlowEffectPainter::GetParam (const CString &sParam, CEffectParamDesc *retValue) const

//	GetParam
//
//	Returns the parameter

	{
	if (strEquals(sParam, ANIMATE_ATTRIB))
		retValue->InitInteger(m_iAnimate);

	else if (strEquals(sParam, BLEND_MODE_ATTRIB))
		retValue->InitInteger(m_iBlendMode);

	else if (strEquals(sParam, LIFETIME_ATTRIB))
		retValue->InitInteger(m_iLifetime);

	else if (strEquals(sParam, PRIMARY_COLOR_ATTRIB))
		retValue->InitColor(m_rgbPrimaryColor);

	else if (strEquals(sParam, RADIUS_ATTRIB))
		retValue->InitInteger(m_iRadius);

	else if (strEquals(sParam, SECONDARY_COLOR_ATTRIB))
		retValue->InitColor(m_rgbSecondaryColor);
	
	else if (strEquals(sParam, STYLE_ATTRIB))
		retValue->InitInteger(m_iStyle);

	else if (strEquals(sParam, OPACITY_ATTRIB))
		retValue->InitInteger(m_iOpacity);

	else if (strEquals(sParam, RADIUS_AROUND_IMPACT_ATTRIB))
		retValue->InitInteger(m_iRadiusAroundImpact);

	else if (strEquals(sParam, WAVE_WIDTH_ATTRIB))
		retValue->InitInteger(m_iWaveWidth);

	else
		return false;

	return true;
	}

bool CGlowEffectPainter::GetParamList (TArray<CString> *retList) const

//	GetParamList
//
//	Returns a list of value parameter names

	{
	retList->DeleteAll();
	retList->InsertEmpty(7);
	retList->GetAt(0) = BLEND_MODE_ATTRIB;
	retList->GetAt(1) = LIFETIME_ATTRIB;
	retList->GetAt(2) = PRIMARY_COLOR_ATTRIB;
	retList->GetAt(3) = RADIUS_ATTRIB;
	retList->GetAt(4) = SECONDARY_COLOR_ATTRIB;
	retList->GetAt(5) = OPACITY_ATTRIB;
	retList->GetAt(6) = RADIUS_AROUND_IMPACT_ATTRIB;

	return true;
	}

void CGlowEffectPainter::GetRect (RECT *retRect) const

//	GetRect
//
//	Returns the rect bounds of the image

	{
	int iSize = m_iRadius;

	retRect->left = -iSize;
	retRect->right = iSize;
	retRect->top = -iSize;
	retRect->bottom = iSize;
	}

DWORDLONG CGlowEffectPainter::MakeCacheKey (DWORD dwSourceID, int iSourceRotation)

//	MakeCacheKey
//
//	Generates a cache key for the glow image source.

	{
	return ((DWORDLONG)(DWORD)iSourceRotation << 32) | (DWORDLONG)dwSourceID;
	}

void CGlowEffectPainter::Paint (CG32bitImage &Dest, int x, int y, SViewportPaintCtx &Ctx)

//	Paint
//
//	Paint the effect

	{
	//	Figure out which object we're going to make glow.

	const CSpaceObject *pSource = m_pSource ? m_pSource : Ctx.pObj;
	if (pSource->IsDestroyed())
		{
		//  Mark the glow effect painter's source as destroyed, so we don't try to access a wild pointer and segfault.
		m_pSource = NULL;
		return;
		}
	if (pSource == NULL)
		return;

	//	Compute some values, such as color table

	if (!CalcIntermediates())
		return;

	//	Compute some values

	BYTE byOpacity;
	if (m_OpacityTable.GetCount() > 0)
		byOpacity = m_OpacityTable[Ctx.iTick % m_OpacityTable.GetCount()];
	else
		byOpacity = 0xff;

	//  Use OpenGL if we have it enabled
	OpenGLMasterRenderQueue* pRenderQueue = Dest.GetMasterRenderQueue();
	if (pRenderQueue) {
		//glm::vec4 glowDecay()
		int hitX, hitY, minRadiusAroundImpact, maxRadiusAroundImpact;
		if (m_bUseDamageCtxSource) {
			Ctx.XForm.Transform(m_vHitVector, &hitX, &hitY);
			int waveFrontPos = (m_iRadiusAroundImpact * (Ctx.iTick % m_iLifetime)) / m_iLifetime;
			switch (m_iStyle) {
				case styleRipple:
					maxRadiusAroundImpact =  waveFrontPos;
					minRadiusAroundImpact = -1;
					break;
				case styleRippleNegative:
					maxRadiusAroundImpact =  (pSource->GetImage().GetImageHeight() + pSource->GetImage().GetImageWidth());
					minRadiusAroundImpact = waveFrontPos;
					break;
				case styleRippleWave:
					maxRadiusAroundImpact = waveFrontPos + (m_iWaveWidth / 2);
					minRadiusAroundImpact = waveFrontPos - (m_iWaveWidth / 2);
					break;
				default:
					maxRadiusAroundImpact = m_iRadiusAroundImpact;
					minRadiusAroundImpact = -1;
			}
		} else {
			hitX = 0;
			hitY = 0;
			maxRadiusAroundImpact = -1;
			minRadiusAroundImpact = -1;
		}
		int relativeHitX = hitX - x;
		int relativeHitY = hitY - y;
		float scale = float(1.0 / Ctx.rZoomScale);
		pSource->GetImage().PaintImageGlowUsingOpenGL(Dest, x, y, Ctx.iTick, pSource->GetRotationFrameIndex(), m_rgbPrimaryColor,
			(float(byOpacity) / 255.0f) * (float(m_iOpacity) / 255.0f), m_iRadius, (float(byOpacity) / 255.0f) * (float(m_iOpacity) / 255.0f), 0.0, m_iBlendMode,
			glm::ivec4(relativeHitX / scale, relativeHitY / scale, maxRadiusAroundImpact == -1 ? -1 : int(maxRadiusAroundImpact), minRadiusAroundImpact == -1 ? -1 : int(minRadiusAroundImpact)), 0.0f, scale, scale);
		return;
	}

	//	Get the glow image, creating if necessary.

	const SCacheEntry &Entry = GetGlowImage(*pSource);
	if (Entry.GlowImage.IsEmpty())
		return;



	//	Paint the glow

	CGDraw::Blt(Dest, x + Entry.xOffset, y + Entry.yOffset, Entry.GlowImage, 0, 0, Entry.GlowImage.GetWidth(), Entry.GlowImage.GetHeight(), byOpacity, m_iBlendMode);
	}

bool CGlowEffectPainter::PointInImage (int x, int y, int iTick, int iVariant, int iRotation) const

//	PointInImage
//
//	Returns TRUE if the given point is in the image

	{
	//	We only intersect if we are inside a box around the center of 1/2 the
	//	width (since a lot of the width is taken up by glow effects).

	int iSize = (m_iRadius / 2);
	return (Absolute(x) <= iSize && Absolute(y) <= iSize);
	}

bool CGlowEffectPainter::OnSetParam (CCreatePainterCtx &Ctx, const CString &sParam, const CEffectParamDesc &Value)

//	SetParam
//
//	Sets parameters

	{
	if (strEquals(sParam, ANIMATE_ATTRIB))
		m_iAnimate = (EAnimationTypes)Value.EvalIdentifier(ANIMATION_TABLE, animateMax, animateNone);

	else if (strEquals(sParam, BLEND_MODE_ATTRIB))
		m_iBlendMode = Value.EvalBlendMode();

	else if (strEquals(sParam, LIFETIME_ATTRIB))
		m_iLifetime = Value.EvalIntegerBounded(0, -1, 0);

	else if (strEquals(sParam, PRIMARY_COLOR_ATTRIB))
		m_rgbPrimaryColor = Value.EvalColor();

	else if (strEquals(sParam, RADIUS_ATTRIB))
		m_iRadius = Value.EvalIntegerBounded(1, -1, (int)(STD_SECONDS_PER_UPDATE * LIGHT_SECOND / KLICKS_PER_PIXEL));

	else if (strEquals(sParam, SECONDARY_COLOR_ATTRIB))
		m_rgbSecondaryColor = Value.EvalColor();

	else if (strEquals(sParam, STYLE_ATTRIB))
		m_iStyle = (EStyles)Value.EvalIdentifier(STYLE_TABLE, styleMax, styleFull);

	else if (strEquals(sParam, OPACITY_ATTRIB))
		m_iOpacity = Value.EvalIntegerBounded(0, 255, 0);

	else if (strEquals(sParam, RADIUS_AROUND_IMPACT_ATTRIB))
		m_iRadiusAroundImpact = Value.EvalIntegerBounded(1, -1, -1);

	else if (strEquals(sParam, WAVE_WIDTH_ATTRIB))
		m_iWaveWidth = Value.EvalIntegerBounded(1, -1, 0);

	else if (strEquals(sParam, USE_SOURCE_OBJ_ATTRIB)) {
		if (Value.EvalBool()) {
			const CSpaceObject* pCtxSource = Ctx.GetSource();
			const CSpaceObject* pDamageCtxSource = Ctx.GetDamageCtx() ? Ctx.GetDamageCtx()->pObj : nullptr;
			const CVector vHitVector = Ctx.GetDamageCtx() ? Ctx.GetDamageCtx()->vHitPos : CVector(0, 0);
			m_vHitVector = vHitVector;
			m_bUseDamageCtxSource = Ctx.GetDamageCtx() ? true : false;
			m_pSource = pDamageCtxSource ? pDamageCtxSource : pCtxSource;
		}
	}

	else
		return false;

	m_bInitialized = false;
	return true;
	}

void CGlowEffectPainter::Reset (void) const

//	Reset
//
//	Reset the cached intermediates so they can be recomputed.

	{
	m_OpacityTable.DeleteAll();
	m_Cache.DeleteAll();
	}
