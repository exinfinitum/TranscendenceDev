//	CCInteger.cpp
//
//	Implements CCInteger class

#include "PreComp.h"

CCInteger::CCInteger (void) : 
		m_iValue(0)

//	CCInteger constructor

	{
	}

ICCItem *CCInteger::Clone (CCodeChain *pCC)

//	Clone
//
//	Returns a new item with a single ref-count

	{
	ICCItem *pResult;
	CCInteger *pClone;
	
	pResult = pCC->CreateInteger(m_iValue);
	if (pResult->IsError())
		return pResult;

	pClone = (CCInteger *)pResult;
	pClone->CloneItem(this);

	return pClone;
	}

void CCInteger::DestroyItem (void)

//	DestroyItem
//
//	Destroys the item

	{
	CCodeChain::DestroyInteger(this);
	}

CString CCInteger::Print (DWORD dwFlags) const

//	Print
//
//	Returns a text representation of this item

	{
	//	If this is an error code, translate it

	if (IsError())
		{
		switch (m_iValue)
			{
			case CCRESULT_NOTFOUND:
				return strPatternSubst(LITERAL("[%d] Item not found."), m_iValue);

			case CCRESULT_CANCEL:
				return strPatternSubst(LITERAL("[%d] Operation canceled."), m_iValue);

			case CCRESULT_DISKERROR:
				return strPatternSubst(LITERAL("[%d] Disk error."), m_iValue);

			default:
				return strPatternSubst(LITERAL("[%d] Unknown error."), m_iValue);
			}
		}

	//	Otherwise, just print the integer value

	else
		return strFromInt(m_iValue);
	}

void CCInteger::Reset (void)

//	Reset
//
//	Reset to initial conditions

	{
	ASSERT(m_dwRefCount == 0);
	m_iValue = 0;
	}

