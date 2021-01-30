//	SFXImage.cpp
//
//	Paints a simple image

#include "PreComp.h"

#define RANDOM_START_FRAME_ATTRIB				CONSTLIT("randomStartFrame")
#define ROTATE_IMAGE_ATTRIB						CONSTLIT("rotateImage")
#define VARIANTS_ATTRIB							CONSTLIT("imageVariants")
#define LIFETIME_ATTRIB					        CONSTLIT("lifetime")
#define ROTATION_COUNT_ATTRIB					CONSTLIT("rotationCount")

const int COMPUTE_LIFETIME_CONSTANT = -100;
const int ALWAYS_LOOP_CONSTANT = -101;

class CImagePainter : public IEffectPainter
	{
	public:
		CImagePainter (CImageEffectCreator *pCreator);

		//	IEffectPainter virtuals
		virtual CEffectCreator *GetCreator (void) { return m_pCreator; }
		virtual const CObjectImageArray &GetImage (int iRotation, int *retiRotationFrameIndex = NULL) const;
		virtual bool GetParticlePaintDesc (SParticlePaintDesc *retDesc);
		virtual void GetRect (RECT *retRect) const;
		virtual int GetVariants (void) const;
		virtual void Paint (CG32bitImage &Dest, int x, int y, SViewportPaintCtx &Ctx);
		virtual bool PointInImage (int x, int y, int iTick, int iVariant = 0, int iRotation = 0) const;

	protected:
		virtual void OnReadFromStream (SLoadCtx &Ctx);
		virtual void OnWriteToStream (IWriteStream *pStream);

	private:
		CImageEffectCreator *m_pCreator;
		CCompositeImageSelector m_Sel;
	};

IEffectPainter *CImageEffectCreator::OnCreatePainter (CCreatePainterCtx &Ctx)

//	CreatePainter
//
//	Returns a painter
	
	{
	if (m_Image.IsConstant())
		return this;
	else
		return new CImagePainter(this);
	}

const CObjectImageArray &CImageEffectCreator::GetImage (int iRotation, int *retiRotationFrameIndex) const

//	GetImage
//
//	Return the image.

	{
	SGetImageCtx ImageCtx(GetUniverse());

	//	If this image has a rotation count, then we get the rotation from the
	//	context block.

	if (m_Image.IsRotatable() && m_bDirectional)
		{
		CCompositeImageModifiers Modifiers;
		Modifiers.SetRotation(iRotation);

		CObjectImageArray &Image = m_Image.GetImage(ImageCtx, CCompositeImageSelector(), Modifiers);

		if (retiRotationFrameIndex)
			*retiRotationFrameIndex = 0;

		return Image;
		}

	//	Otherwise, if we've been asked to rotate the image procedurally, do that.

	else if (m_bRotateImage)
		{
		CObjectImageArray &Image = m_Image.GetImage(ImageCtx, CCompositeImageSelector());

		if (retiRotationFrameIndex)
			*retiRotationFrameIndex = 0;

		return Image;
		}

	//	Otherwise we assume we have the entire image

	else
		{
		CObjectImageArray &Image = m_Image.GetImage(ImageCtx, CCompositeImageSelector());

		if (retiRotationFrameIndex)
			*retiRotationFrameIndex = (m_bDirectional ? Angle2Direction(iRotation, m_iVariants) : 0);

		return Image;
		}
	}

bool CImageEffectCreator::GetParticlePaintDesc (SParticlePaintDesc *retDesc)

//	GetParticlePaintDesc
//
//	Returns particle painting descriptor for optimized painting

	{
	SGetImageCtx Ctx(GetUniverse());

	retDesc->iStyle = paintImage;
	retDesc->pImage = &m_Image.GetImage(Ctx, CCompositeImageSelector());
	retDesc->iVariants = m_iVariants;
	retDesc->bDirectional = m_bDirectional;
	retDesc->bRandomStartFrame = m_bRandomStartFrame;

	return true;
	}

void CImageEffectCreator::GetRect (RECT *retRect) const

//	GetRect
//
//	Returns the image rect
	
	{
	SGetImageCtx Ctx(GetUniverse());

	CObjectImageArray &Image = m_Image.GetImage(Ctx, CCompositeImageSelector());
	*retRect = Image.GetImageRect();
	}

ALERROR CImageEffectCreator::OnEffectCreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, const CString &sUNID)

//	OnEffectCreateFromXML
//
//	Initialize type-specific data

	{
	ALERROR error;

	if (error = m_Image.InitFromXML(Ctx, pDesc))
		return error;

    if (!pDesc->FindAttributeInteger(LIFETIME_ATTRIB, &m_iLifetime))
        m_iLifetime = (Ctx.bLoopImages ? ALWAYS_LOOP_CONSTANT : COMPUTE_LIFETIME_CONSTANT);

	m_bRandomStartFrame = pDesc->GetAttributeBool(RANDOM_START_FRAME_ATTRIB);

	//	Variants & Rotation

	m_bRotateImage = pDesc->GetAttributeBool(ROTATE_IMAGE_ATTRIB);
	if (m_bRotateImage)
		{
		m_iVariants = 1;
		m_bDirectional = false;
		}
	else
		{
		m_iVariants = pDesc->GetAttributeInteger(ROTATION_COUNT_ATTRIB);
		m_bDirectional = (m_iVariants > 1);

		if (m_iVariants == 0)
			m_iVariants = pDesc->GetAttributeInteger(VARIANTS_ATTRIB);

		if (m_iVariants <= 0)
			m_iVariants = 1;
		}

	return NOERROR;
	}

ALERROR CImageEffectCreator::OnEffectBindDesign (SDesignLoadCtx &Ctx)

//	OnEffectBindDesign
//
//	Resolve loading

	{
	ALERROR error;

	if (error = m_Image.OnDesignLoadComplete(Ctx))
		return error;

    if (m_iLifetime == COMPUTE_LIFETIME_CONSTANT)
        m_iLifetime = m_Image.GetMaxLifetime();

    else if (m_iLifetime == ALWAYS_LOOP_CONSTANT)
        m_iLifetime = -1;

	return NOERROR;
	}

void CImageEffectCreator::Paint (CG32bitImage &Dest, int x, int y, SViewportPaintCtx &Ctx)

//	Paint
//
//	Paint the effect

	{
	SGetImageCtx ImageCtx(GetUniverse());

	//	Calculate the tick.

	int iTick = Ctx.iTick;
	if (m_bRandomStartFrame)
		iTick += Ctx.iDestiny;

	//	If this image has a rotation count, then we get the rotation from the
	//	context block.

	if (m_Image.IsRotatable() && m_bDirectional)
		{
		CCompositeImageModifiers Modifiers;
		Modifiers.SetRotation(Ctx.iRotation);

		CObjectImageArray &Image = m_Image.GetImage(ImageCtx, CCompositeImageSelector(), Modifiers);

		if (Ctx.byShimmer)
			Image.PaintImageShimmering(Dest, x, y, iTick, 0, Ctx.byShimmer, 0.0f, float(1.0 / g_ZoomScale));
		else
			Image.PaintImage(Dest, x, y, iTick, 0, false, 0.0f, float(1.0 / g_ZoomScale));
		}

	//	Otherwise, if we've been asked to rotate the image procedurally, do that.

	else if (m_bRotateImage)
		{
		CObjectImageArray &Image = m_Image.GetImage(ImageCtx, CCompositeImageSelector());
		Image.PaintRotatedImage(Dest, x, y, iTick, Ctx.iRotation);
		}

	//	Otherwise we assume we have the entire image

	else
		{
		CObjectImageArray &Image = m_Image.GetImage(ImageCtx, CCompositeImageSelector());
		int iFrame = (m_bDirectional ? Angle2Direction(Ctx.iRotation, m_iVariants) : (Ctx.iVariant % m_iVariants));

		if (Ctx.byShimmer)
			Image.PaintImageShimmering(Dest, x, y, iTick, iFrame, Ctx.byShimmer);
		else
			Image.PaintImage(Dest, x, y, iTick, iFrame);
		}
	}

void CImageEffectCreator::PaintComposite (CG32bitImage &Dest, int x, int y, SViewportPaintCtx &Ctx)

//	Paint
//
//	Paint the effect

	{
	SGetImageCtx ImageCtx(GetUniverse());

	CObjectImageArray &Image = m_Image.GetImage(ImageCtx, CCompositeImageSelector());

	int iTick = Ctx.iTick;
	if (m_bRandomStartFrame)
		iTick += Ctx.iDestiny;

	if (m_bRotateImage)
		Image.PaintRotatedImage(Dest, x, y, iTick, Ctx.iRotation, true);
	else
		{
		int iFrame = (m_bDirectional ? Angle2Direction(Ctx.iRotation, m_iVariants) : (Ctx.iVariant % m_iVariants));

		Image.PaintImage(Dest, x, y, iTick, iFrame, true);
		}
	}

bool CImageEffectCreator::PointInImage (int x, int y, int iTick, int iVariant, int iRotation) const

//	PointInImage
//
//	Returns TRUE if the given point is in the image

	{
	SGetImageCtx ImageCtx(GetUniverse());

	CObjectImageArray &Image = m_Image.GetImage(ImageCtx, CCompositeImageSelector());
	return Image.PointInImage(x, y, iTick, (iVariant % m_iVariants));
	}

void CImageEffectCreator::SetVariants (int iVariants)

//	SetVariants
//
//	Sets the number of variants
//	This function is only called because directionally used to be set at the
//	object level. We only honor this if we are not already directional

	{
	if (!m_bDirectional)
		{
		m_bDirectional = true;
		m_iVariants = iVariants;
		}
	}

//	CImagePainter --------------------------------------------------------------

CImagePainter::CImagePainter (CImageEffectCreator *pCreator) : m_pCreator(pCreator)

//	CImagePainter constructor
	
	{
	SSelectorInitCtx InitCtx;

	m_pCreator->GetImage().InitSelector(InitCtx, &m_Sel);
	}

const CObjectImageArray &CImagePainter::GetImage (int iRotation, int *retiRotationFrameIndex) const

//	GetImage
//
//	Returns the image.

	{
	return m_pCreator->GetImage(iRotation, retiRotationFrameIndex);
	}

bool CImagePainter::GetParticlePaintDesc (SParticlePaintDesc *retDesc)

//	GetParticlePaintDesc
//
//	Returns the particle paint descriptor

	{
	SGetImageCtx ImageCtx(PainterGetUniverse());
	CObjectImageArray &Image = m_pCreator->GetImage().GetImage(ImageCtx, m_Sel);

	retDesc->iStyle = paintImage;
	retDesc->pImage = &Image;
	retDesc->iVariants = m_pCreator->GetVariants();
	retDesc->bDirectional = m_pCreator->IsDirectional();
	retDesc->bRandomStartFrame = m_pCreator->HasRandomStartFrame();

	return true;
	}

void CImagePainter::GetRect (RECT *retRect) const

//	GetRect
//
//	Returns the rect of the image

	{
	SGetImageCtx ImageCtx(PainterGetUniverse());
	CObjectImageArray &Image = m_pCreator->GetImage().GetImage(ImageCtx, m_Sel);
	*retRect = Image.GetImageRect();
	}

int CImagePainter::GetVariants (void) const

//	GetVariants
//
//	Returns the number of variants

	{
	return m_pCreator->GetVariants();
	}

void CImagePainter::OnReadFromStream (SLoadCtx &Ctx)

//	OnReadFromStream
//
//	Load from stream

	{
	//	The constructor initializes this to a default value; we need to clear
	//	it out because ReadFromStream ASSERTS that it is empty.

	m_Sel.DeleteAll();

	m_Sel.ReadFromStream(Ctx);
	}

void CImagePainter::OnWriteToStream (IWriteStream *pStream)

//	OnWriteToStream
//
//	Write to stream

	{
	m_Sel.WriteToStream(pStream);
	}

void CImagePainter::Paint (CG32bitImage &Dest, int x, int y, SViewportPaintCtx &Ctx)

//	Paint
//
//	Paint image

	{
	SGetImageCtx ImageCtx(PainterGetUniverse());
	CObjectImageArray &Image = m_pCreator->GetImage().GetImage(ImageCtx, m_Sel);

	int iTick = Ctx.iTick;
	if (m_pCreator->HasRandomStartFrame())
		iTick += Ctx.iDestiny;

	if (m_pCreator->ImageRotationNeeded())
		Image.PaintRotatedImage(Dest, x, y, iTick, Ctx.iRotation);
	else
		{
		int iVariants = m_pCreator->GetVariants();
		int iFrame = (m_pCreator->IsDirectional() ? Angle2Direction(Ctx.iRotation, iVariants) : (Ctx.iVariant % iVariants));

		Image.PaintImage(Dest, x, y, iTick, iFrame);
		}
	}

bool CImagePainter::PointInImage (int x, int y, int iTick, int iVariant, int iRotation) const

//	PointInImage
//
//	Returns TRUE if point is in the image

	{
	SGetImageCtx ImageCtx(PainterGetUniverse());
	CObjectImageArray &Image = m_pCreator->GetImage().GetImage(ImageCtx, m_Sel);
	int iVariants = m_pCreator->GetVariants();
	return Image.PointInImage(x, y, iTick, (iVariant % iVariants));
	}

