//	SFXSingleParticle.cpp
//
//	Single particle SFX

#include "PreComp.h"

#define STYLE_ATTRIB							CONSTLIT("style")
#define MIN_WIDTH_ATTRIB						CONSTLIT("minWidth")
#define MAX_WIDTH_ATTRIB						CONSTLIT("maxWidth")
#define PRIMARY_COLOR_ATTRIB					(CONSTLIT("primaryColor"))
#define SECONDARY_COLOR_ATTRIB					(CONSTLIT("secondaryColor"))

#define STYLE_PLAIN								CONSTLIT("plain")
#define STYLE_FLAME								CONSTLIT("flame")
#define STYLE_SMOKE								CONSTLIT("smoke")
#define STYLE_LINE								CONSTLIT("line")

const int DEFAULT_MIN_WIDTH =					1;
const int DEFAULT_MAX_WIDTH =					8;

class CSingleParticlePainter : public IEffectPainter
	{
	public:
		CSingleParticlePainter (CSingleParticleEffectCreator *pCreator);

		//	IEffectPainter virtuals
		virtual CEffectCreator *GetCreator (void) override { return m_pCreator; }
		virtual bool GetParam (const CString &sParam, CEffectParamDesc *retValue) const override;
		virtual bool GetParamList (TArray<CString> *retList) const override;
		virtual bool GetParticlePaintDesc (SParticlePaintDesc *retDesc) override;
		virtual void Paint (CG32bitImage &Dest, int x, int y, SViewportPaintCtx &Ctx) override { };

	protected:
		virtual void OnReadFromStream (SLoadCtx &Ctx) override;
		virtual bool OnSetParam (CCreatePainterCtx &Ctx, const CString &sParam, const CEffectParamDesc &Value) override;

	private:
		CSingleParticleEffectCreator *m_pCreator;

		ParticlePaintStyles m_iStyle;
		int m_iMaxWidth;
		int m_iMinWidth;
		CG32bitPixel m_rgbPrimaryColor;
		CG32bitPixel m_rgbSecondaryColor;
	};

static LPCSTR STYLE_TABLE[] =
	{
	//	Must be same order as ParticlePaintStyles
		"",

		"plain",
		"flame",
		"smoke",
		"image",
		"line",
		"glitter",

		NULL,
	};

//	CSingleParticleEffectCreator object

CSingleParticleEffectCreator::~CSingleParticleEffectCreator (void)

//	CShockwaveEffectCreator destructor

	{
	if (m_pSingleton)
		delete m_pSingleton;
	}

IEffectPainter *CSingleParticleEffectCreator::OnCreatePainter (CCreatePainterCtx &Ctx)

//	CreatePainter
//
//	Creates a new painter

	{
	//	If we have a singleton, return that

	if (m_pSingleton && Ctx.CanCreateSingleton())
		return m_pSingleton;

	//	Otherwise we need to create a painter with the actual
	//	parameters.

	IEffectPainter *pPainter = new CSingleParticlePainter(this);

	//	Initialize the painter parameters

	pPainter->SetParam(Ctx, MAX_WIDTH_ATTRIB, m_MaxWidth);
	pPainter->SetParam(Ctx, MIN_WIDTH_ATTRIB, m_MinWidth);
	pPainter->SetParam(Ctx, PRIMARY_COLOR_ATTRIB, m_PrimaryColor);
	pPainter->SetParam(Ctx, SECONDARY_COLOR_ATTRIB, m_SecondaryColor);
	pPainter->SetParam(Ctx, STYLE_ATTRIB, m_Style);

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

ALERROR CSingleParticleEffectCreator::OnEffectCreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, const CString &sUNID)

//	OnEffectCreateFromXML
//
//	Initializes from XML

	{
	ALERROR error;

	if (error = m_MaxWidth.InitIntegerFromXML(Ctx, pDesc->GetAttribute(MAX_WIDTH_ATTRIB)))
		return error;

	if (error = m_MinWidth.InitIntegerFromXML(Ctx, pDesc->GetAttribute(MIN_WIDTH_ATTRIB)))
		return error;

	if (error = m_PrimaryColor.InitColorFromXML(Ctx, pDesc->GetAttribute(PRIMARY_COLOR_ATTRIB)))
		return error;

	if (error = m_SecondaryColor.InitColorFromXML(Ctx, pDesc->GetAttribute(SECONDARY_COLOR_ATTRIB)))
		return error;

	if (error = m_Style.InitIdentifierFromXML(Ctx, pDesc->GetAttribute(STYLE_ATTRIB), STYLE_TABLE))
		return error;

	return NOERROR;
	}

//	CSingleParticlePainter object

CSingleParticlePainter::CSingleParticlePainter (CSingleParticleEffectCreator *pCreator) : 
		m_pCreator(pCreator),
		m_iStyle(paintPlain),
		m_iMaxWidth(6),
		m_iMinWidth(1),
		m_rgbPrimaryColor(CG32bitPixel(255, 255, 255)),
		m_rgbSecondaryColor(CG32bitPixel(128, 128, 128))

//	CSingleParticlePainter constructor

	{
	}

bool CSingleParticlePainter::GetParam (const CString &sParam, CEffectParamDesc *retValue) const

//	GetParam
//
//	Returns a parameter

	{
	if (strEquals(sParam, MAX_WIDTH_ATTRIB))
		retValue->InitInteger(m_iMaxWidth);
	else if (strEquals(sParam, MIN_WIDTH_ATTRIB))
		retValue->InitInteger(m_iMinWidth);
	else if (strEquals(sParam, PRIMARY_COLOR_ATTRIB))
		retValue->InitColor(m_rgbPrimaryColor);
	else if (strEquals(sParam, SECONDARY_COLOR_ATTRIB))
		retValue->InitColor(m_rgbSecondaryColor);
	else if (strEquals(sParam, STYLE_ATTRIB))
		retValue->InitInteger(m_iStyle);
	else
		return false;

	return true;
	}

bool CSingleParticlePainter::GetParamList (TArray<CString> *retList) const

//	GetParamList
//
//	Returns a list of value parameter names

	{
	retList->DeleteAll();
	retList->InsertEmpty(5);
	retList->GetAt(0) = MAX_WIDTH_ATTRIB;
	retList->GetAt(1) = MIN_WIDTH_ATTRIB;
	retList->GetAt(2) = PRIMARY_COLOR_ATTRIB;
	retList->GetAt(3) = SECONDARY_COLOR_ATTRIB;
	retList->GetAt(4) = STYLE_ATTRIB;

	return true;
	}

bool CSingleParticlePainter::GetParticlePaintDesc (SParticlePaintDesc *retDesc)

//	GetParticlePaintDesc
//
//	Returns a structure describing how particles should be painted

	{
	retDesc->iStyle = m_iStyle;
	retDesc->iMinWidth = m_iMinWidth;
	retDesc->iMaxWidth = m_iMaxWidth;
	retDesc->rgbPrimaryColor = m_rgbPrimaryColor;
	retDesc->rgbSecondaryColor = (m_rgbSecondaryColor.IsNull() ? m_rgbPrimaryColor : m_rgbSecondaryColor);

	return true;
	}

void CSingleParticlePainter::OnReadFromStream (SLoadCtx &Ctx)

//	OnReadFromStream
//
//	Load from stream

	{
	if (Ctx.dwVersion >= 122)
		IEffectPainter::OnReadFromStream(Ctx);
	else
		{
		Ctx.pStream->Read((char *)&m_iMinWidth, sizeof(DWORD));
		Ctx.pStream->Read((char *)&m_iMaxWidth, sizeof(DWORD));
		}
	}

bool CSingleParticlePainter::OnSetParam (CCreatePainterCtx &Ctx, const CString &sParam, const CEffectParamDesc &Value)

//	OnSetParam
//
//	Sets parameters

	{
	if (strEquals(sParam, MAX_WIDTH_ATTRIB))
		m_iMaxWidth = Value.EvalIntegerBounded(0, -1, 6);
	else if (strEquals(sParam, MIN_WIDTH_ATTRIB))
		m_iMinWidth = Value.EvalIntegerBounded(0, -1, 1);
	else if (strEquals(sParam, PRIMARY_COLOR_ATTRIB))
		m_rgbPrimaryColor = Value.EvalColor(CG32bitPixel(255, 255, 255));
	else if (strEquals(sParam, SECONDARY_COLOR_ATTRIB))
		m_rgbSecondaryColor = Value.EvalColor();
	else if (strEquals(sParam, STYLE_ATTRIB))
		m_iStyle = (ParticlePaintStyles)Value.EvalIdentifier(STYLE_TABLE, paintMax, paintPlain);
	else
		return false;

	return true;
	}
