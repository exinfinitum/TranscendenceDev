//	CStaticEffect.cpp
//
//	CStaticEffect class

#include "PreComp.h"

CStaticEffect::CStaticEffect (CUniverse &Universe) : TSpaceObjectImpl(Universe),
		m_pPainter(NULL)

//	CStaticEffect constructor

	{
	}

CStaticEffect::~CStaticEffect (void)

//	CStaticEffect destructor

	{
	if (m_pPainter)
		m_pPainter->Delete();
	}

ALERROR CStaticEffect::Create (CEffectCreator *pType,
				CSystem &System,
				const CVector &vPos,
				CStaticEffect **retpEffect)

//	Create
//
//	Creates a new effects object

	{
	ALERROR error;
	CStaticEffect *pEffect;

	pEffect = new CStaticEffect(System.GetUniverse());
	if (pEffect == NULL)
		return ERR_MEMORY;

	pEffect->Place(vPos);

	ASSERT(pType);
	CCreatePainterCtx CreateCtx;
	pEffect->m_pPainter = pType->CreatePainter(CreateCtx);

	//	Set the size of the object

	if (pEffect->m_pPainter)
		pEffect->SetBounds(pEffect->m_pPainter);

	//	Add to system

	if (error = pEffect->AddToSystem(System))
		{
		delete pEffect;
		return error;
		}

	//	Done

	if (retpEffect)
		*retpEffect = pEffect;

	return NOERROR;
	}

void CStaticEffect::OnPaint (CG32bitImage &Dest, int x, int y, SViewportPaintCtx &Ctx)

//	OnPaint
//
//	Paints the effect

	{
	CViewportPaintCtxSmartSave Save(Ctx);
	Ctx.iTick = 0;
	Ctx.iVariant = GetDestiny();
	Ctx.iRotation = 0;
	Ctx.iDestiny = GetDestiny();

	m_pPainter->Paint(Dest, x, y, Ctx);
	}

void CStaticEffect::OnReadFromStream (SLoadCtx &Ctx)

//	OnReadFromStream
//
//	Read object data from a stream
//
//	DWORD		IEffectPainter

	{
#ifdef DEBUG_LOAD
	::OutputDebugString("CStaticEffect::OnReadFromStream\n");
#endif
	m_pPainter = CEffectCreator::CreatePainterFromStream(Ctx);
	}

void CStaticEffect::OnWriteToStream (IWriteStream *pStream)

//	OnWriteToStream
//
//	Write the object's data to stream
//
//	DWORD		IEffectPainter

	{
	CEffectCreator::WritePainterToStream(pStream, m_pPainter);
	}

