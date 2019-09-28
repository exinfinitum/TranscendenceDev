//	CEventHandler.cpp
//
//	CEventHandler object

#include "PreComp.h"

static TStaticStringTable<SSimpleStringEntry, 4> DIAGNOSTIC_EVENTS = {
	"OnGlobalEndDiagnostics",
	"OnGlobalRunDiagnostics",
	"OnGlobalStartDiagnostics",
	"OnGlobalSystemDiagnostics",
	};

CEventHandler::CEventHandler (void)

//	CEventHandler constructor

	{
	}

CEventHandler::~CEventHandler (void)

//	CEventHandler destructor

	{
	DeleteAll();
	}

CEventHandler &CEventHandler::operator= (const CEventHandler &Src)

//	CEventHandler equals operator

	{
	int i;

	//	Copy the data

	DeleteAll();
	m_Handlers = Src.m_Handlers;

	//	Add a reference to every item

	for (i = 0; i < m_Handlers.GetCount(); i++)
		{
		if (m_Handlers[i])
			m_Handlers[i] = m_Handlers[i]->Reference();
		}

	return *this;
	}

void CEventHandler::AddEvent (const CString &sEvent, ICCItem *pCode)

//	AddEvent
//
//	Adds an event

	{
	m_Handlers.Insert(sEvent, pCode);
	}

ALERROR CEventHandler::AddEvent (CXMLElement *pEventXML, CString *retsError)

//	AddEvent
//
//	Adds an event defined in XML.

	{
	//	If NULL, then that's OK. It means it is an optional event.

	if (pEventXML == NULL)
		return NOERROR;

	return AddEvent(pEventXML->GetTag(), pEventXML->GetContentText(0), retsError);
	}

ALERROR CEventHandler::AddEvent (const CString &sEvent, const CString &sCode, CString *retsError)

//	AddEvent
//
//	Adds an event

	{
	CCodeChainCtx Ctx(*g_pUniverse);

	ICCItemPtr pCode = Ctx.LinkCode(sCode);
	if (pCode->IsError())
		{
		if (retsError)
			*retsError = pCode->GetStringValue();
		return ERR_FAIL;
		}

	m_Handlers.Insert(sEvent, pCode->Reference());

	return NOERROR;
	}

void CEventHandler::DeleteAll (void)

//	DeleteAll
//
//	Delete all events

	{
	for (int i = 0; i < m_Handlers.GetCount(); i++)
		{
		ICCItem *pItem = m_Handlers[i];
		pItem->Discard();
		}

	m_Handlers.DeleteAll();
	}

bool CEventHandler::FindEvent (const CString &sEvent, ICCItem **retpCode) const

//	FindEvent
//
//	Finds the event handler by name

	{
	ICCItem * const *pCode = m_Handlers.GetAt(sEvent);
	if (pCode)
		{
		if (retpCode)
			*retpCode = *pCode;

		return true;
		}

	return false;
	}

const CString &CEventHandler::GetEvent (int iIndex, ICCItem **retpCode) const

//	GetEvent
//
//	Returns the event by index

	{
	if (retpCode)
		*retpCode = m_Handlers[iIndex];

	return m_Handlers.GetKey(iIndex);
	}

bool CEventHandler::IsDiagnosticsEvent (const CString &sEvent)

//	IsDiagnosticsEvent
//
//	Returns TRUE if this is a diagnostics event.

	{
	return DIAGNOSTIC_EVENTS.FindPos(sEvent);
	}

ALERROR CEventHandler::InitFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc)

//	InitFromXML
//
//	Load all handlers

	{
	//	Load all sub-elements

	for (int i = 0; i < pDesc->GetContentElementCount(); i++)
		{
		CXMLElement *pHandler = pDesc->GetContentElement(i);

		//	If this is diagnostics code and we're not loading diagnostics, then
		//	skip.

		if (!Ctx.bLoadDiagnostics 
				&& IsDiagnosticsEvent(pHandler->GetTag()))
			continue;

		//	Parse the code

		CCodeChain::SLinkOptions Options;
		Options.bNullIfEmpty = true;

		ICCItemPtr pCode = ICCItemPtr(CCodeChain::Link(pHandler->GetContentText(0), Options));

		//	If Link returns NULL, then it means that this was just whitespace
		//	or comments only.

		if (!pCode)
			continue;

		//	Report link errors.

		else if (pCode->IsError())
			{
			Ctx.sError = strPatternSubst("<%s> event: %s", pHandler->GetTag(), pCode->GetStringValue());
			return ERR_FAIL;
			}

		//	If this is an old extension, then make sure the code is not using the
		//	gStation variable, because we no longer support it

		else if (Ctx.GetAPIVersion() < 2
				&& Ctx.GetUniverse().GetCC().HasIdentifier(pCode, CONSTLIT("gStation")))
			{
			Ctx.sError = CONSTLIT("gStation variable has been deprecated--use gSource instead.");
			return ERR_FAIL;
			}

		//	Done

		m_Handlers.Insert(pHandler->GetTag(), pCode->Reference());
		}

	return NOERROR;
	}

void CEventHandler::MergeFrom (const CEventHandler &Src)

//	MergeFrom
//
//	Merges from the source

	{
	for (int i = 0; i < Src.GetCount(); i++)
		{
		ICCItem **ppCode = m_Handlers.GetAt(Src.m_Handlers.GetKey(i));
		if (ppCode)
			{
			(*ppCode)->Discard();
			(*ppCode) = Src.m_Handlers.GetValue(i)->Reference();
			}
		else
			m_Handlers.SetAt(Src.m_Handlers.GetKey(i), Src.m_Handlers.GetValue(i)->Reference());
		}
	}
