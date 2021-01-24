//	CGenericType.cpp
//
//	CGenericType class
//	Copyright (c) 2017 Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

#define COMMUNICATIONS_TAG						CONSTLIT("Communications")

CString CGenericType::GetNamePattern (DWORD dwNounFormFlags, DWORD *retdwFlags) const

//	GetNamePattern
//
//	Get the name pattern.

	{
	CCodeChainCtx CCX(GetUniverse());

	//	core.namePattern can be used to set this value.

	ICCItemPtr pValue = GetProperty(CCX, PROPERTY_CORE_NAME_PATTERN);

	if (retdwFlags)
		*retdwFlags = 0;

	return pValue->GetStringValue();
	}

void CGenericType::OnAccumulateXMLMergeFlags (TSortMap<DWORD, DWORD> &MergeFlags) const

//	OnAccumulateXMLMergeFlags
//
//	Handle merging.

	{
	//	We know how to handle these tags through the inheritance hierarchy.

	MergeFlags.SetAt(CXMLElement::GetKeywordID(COMMUNICATIONS_TAG), CXMLElement::MERGE_OVERRIDE);
	}

ALERROR CGenericType::OnCreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc)

//	OnCreateFromXML
//
//	Load from XML

	{
	ALERROR error;
	if (error = m_Comms.InitFromXML(Ctx, pDesc->GetContentElementByTag(COMMUNICATIONS_TAG)))
		return ComposeLoadError(Ctx, Ctx.sError);

	return NOERROR;
	}

void CGenericType::OnUnbindDesign (void)

//	OnUbindDesign
//
//	Unbind design in preparation for a new bind

	{
	m_Comms.Unbind();
	}
