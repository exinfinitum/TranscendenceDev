//	CDockScreenCustomList.cpp
//
//	CDockScreenCustomList class
//	Copyright (c) 2014 by Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"
#include "Transcendence.h"

ALERROR CDockScreenCustomList::OnInitList (SInitCtx &Ctx, const SDisplayOptions &Options, CString *retsError)

//	OnInitList
//
//	Initialize list

	{
	//	See if we define a custom row height

	if (!Options.sRowHeightCode.IsBlank())
		{
		CString sResult;
		if (!EvalString(Options.sRowHeightCode, false, eventNone, &sResult))
			{
			*retsError = sResult;
			return ERR_FAIL;
			}

		int cyRow = strToInt(sResult, -1);
		if (cyRow > 0)
			m_pItemListControl->SetRowHeight(cyRow);
		}

	m_pItemListControl->SetIconHeight(Options.cyIcon);
	m_pItemListControl->SetIconWidth(Options.cxIcon);
	m_pItemListControl->SetIconScale(Options.rIconScale);

	//	Get the list to show

	ICCItemPtr pExp = CCodeChain::LinkCode(Options.sCode);

	//	Evaluate the function

	CCodeChainCtx CCCtx(GetUniverse());
	CCCtx.SetScreen(&m_DockScreen);
	CCCtx.DefineContainingType(m_DockScreen.GetRoot());
	CCCtx.SaveAndDefineSourceVar(m_pLocation);
	CCCtx.SaveAndDefineDataVar(m_pData);

	ICCItemPtr pResult = CCCtx.RunCode(pExp);	//	LATER:Event

	if (pResult->IsError())
		{
		*retsError = pResult->GetStringValue();
		return ERR_FAIL;
		}

	//	Set this expression as the list

	m_pItemListControl->SetList(pResult);

	//	Position the cursor on the next relevant item

	SelectNextItem();

	//	Give the screen a chance to start at a different item (other
	//	than the first)

	if (!Options.sInitialItemCode.IsBlank())
		{
		bool bMore = IsCurrentItemValid();
		while (bMore)
			{
			bool bResult;
			if (!EvalBool(Options.sInitialItemCode, &bResult, retsError))
				return ERR_FAIL;

			if (bResult)
				break;

			bMore = SelectNextItem();
			}
		}

	return NOERROR;
	}
