//	SFXImageFracture.cpp
//
//	Particle explosion effect

#include "PreComp.h"

#define IMAGE_TAG								(CONSTLIT("Image"))

ALERROR CImageFractureEffectCreator::CreateEffect (CSystem *pSystem,
												   CSpaceObject *pAnchor,
												   const CVector &vPos,
												   const CVector &vVel,
												   int iRotation,
												   int iVariant,
												   ICCItem *pData,
												   CSpaceObject **retpEffect)

//	CreateEffect
//
//	Creates the effect object

	{
	ALERROR error;

	if (pSystem == NULL)
		return ERR_FAIL;

	//	Create the effect

	CFractureEffect *pObj = NULL;
	if (m_Image.IsLoaded())
		{
		if (error = CFractureEffect::CreateExplosion(*pSystem,
				vPos,
				vVel,
				m_Image,
				0,
				0,
				&pObj))
			return error;

		//	Play Sound

		PlaySound(pObj);
		}

	if (retpEffect)
		*retpEffect = pObj;

	return NOERROR;
	}

ALERROR CImageFractureEffectCreator::OnEffectCreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, const CString &sUNID)

//	OnEffectCreateFromXML
//
//	Load from XML

	{
	ALERROR error;

	CXMLElement *pImage = pDesc->GetContentElementByTag(IMAGE_TAG);
	if (pImage)
		{
		if (error = m_Image.InitFromXML(Ctx, *pImage))
			return error;
		}

	return NOERROR;
	}

ALERROR CImageFractureEffectCreator::OnEffectBindDesign (SDesignLoadCtx &Ctx)

//	OnEffectBindDesign
//
//	Resolve loading

	{
	ALERROR error;

	if (error = m_Image.OnDesignLoadComplete(Ctx))
		return error;

	return NOERROR;
	}

