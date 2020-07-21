//	CDynamicDesignTable.cpp
//
//	CDynamicDesignTable class
//	Copyright (c) 2012 by Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

#define UNID_ATTRIB								CONSTLIT("UNID")

void CDynamicDesignTable::CleanUp (void)

//	CleanUp
//
//	Frees all memory used

	{
	m_Table.DeleteAll();
	}

ALERROR CDynamicDesignTable::Compile (SEntry *pEntry, CString *retsError)

//	Compile
//
//	Compiles the type

	{
	ALERROR error;

	//	Create an entity resolver

	CEntityResolverList Resolver;
	if (pEntry->pExtension)
		pEntry->pExtension->GetUniverse().InitEntityResolver(pEntry->pExtension, &Resolver);

	//	Parse the XML

	CBufferReadBlock Source(pEntry->sSource);

	//	Parse

	CXMLElement::SParseOptions Options;
	Options.pController = &Resolver;
	Options.bNoTagCharCheck = !(pEntry->pExtension && pEntry->pExtension->GetUniverse().InDebugMode());

	CXMLElement *pDesc;
	if (error = CXMLElement::ParseXML(Source, Options, &pDesc, retsError))
		return error;

	//	Create the type

	error = CreateType(pEntry, pDesc, false, &pEntry->pType, retsError);

	//	We always save the XML.

	pEntry->pSource = pDesc;

	//	Done

	if (error)
		return error;

	return NOERROR;
	}

ALERROR CDynamicDesignTable::CreateType (SEntry *pEntry, CXMLElement *pDesc, bool bLoadDiagnostics, CDesignType **retpType, CString *retsError)

//	CreateType
//
//	Creates a new type based on the XML

	{
	ALERROR error;

	//	Set the UNID properly

	pDesc->SetAttribute(UNID_ATTRIB, strPatternSubst(CONSTLIT("0x%x"), pEntry->dwUNID));

	//	Set up the design load context

	SDesignLoadCtx Ctx;
	Ctx.pExtension = pEntry->pExtension;
	Ctx.bLoadDiagnostics = bLoadDiagnostics;

	//	Load the type

	CDesignType *pNewType;
	if (error = CDesignType::CreateFromXML(Ctx, pDesc, &pNewType))
		{
		if (retsError)
			*retsError = Ctx.sError;
		return error;
		}

	//	Done

	*retpType = pNewType;

	return NOERROR;
	}

ALERROR CDynamicDesignTable::DefineType (SDesignLoadCtx &Ctx, CExtension *pExtension, DWORD dwUNID, CXMLElement *pSource, CDesignType **retpType, CString *retsError)

//	DefineType
//
//	Defines a new dynamic type. We take ownership of pSource.

	{
	ALERROR error;

	SEntry *pEntry = m_Table.Insert(dwUNID);
	pEntry->dwUNID = dwUNID;
	pEntry->pExtension = pExtension;
	pEntry->pSource = pSource;

	//	Create the type

	if (error = CreateType(pEntry, pEntry->pSource, Ctx.bLoadDiagnostics, &pEntry->pType, retsError))
		{
		m_Table.DeleteAt(dwUNID);
		return error;
		}

	//	Done

	if (retpType)
		*retpType = pEntry->pType;

	return NOERROR;
	}

ALERROR CDynamicDesignTable::DefineType (CExtension *pExtension, DWORD dwUNID, ICCItem *pSource, CDesignType **retpType, CString *retsError)

//	DefineType
//
//	Defines a new dynamic type

	{
	ALERROR error;

	SEntry *pEntry = m_Table.Insert(dwUNID);
	pEntry->dwUNID = dwUNID;
	pEntry->pExtension = pExtension;

	//	If this is an XML element, then we can use it directly.

	if (strEquals(pSource->GetTypeOf(), CONSTLIT("xmlElement")))
		{
		CCXMLWrapper *pWrapper = dynamic_cast<CCXMLWrapper *>(pSource);
		CXMLElement *pDesc = pWrapper->GetXMLElement();

		//	Stream the element

		pEntry->sSource = pDesc->ConvertToString();

		//	If necessary, we save the source. CreateType will keep a pointer to the
		//	source (but we own it and will free it when we're done).

		pEntry->pSource = pDesc->OrphanCopy();

		//	Create the type

		if (error = CreateType(pEntry, pEntry->pSource, false, &pEntry->pType, retsError))
			{
			m_Table.DeleteAt(dwUNID);
			return error;
			}
		}

	//	Otherwise, we assume this is a string

	else
		{
		//	We need to trim leading whitespace because the XML parser doesn't like any.

		pEntry->sSource = strTrimWhitespace(pSource->GetStringValue());

		if (error = Compile(pEntry, retsError))
			{
			m_Table.DeleteAt(dwUNID);
			return error;
			}
		}

	//	Done

	if (retpType)
		*retpType = pEntry->pType;

	return NOERROR;
	}

void CDynamicDesignTable::Delete (DWORD dwUNID)

//	Delete
//
//	Delete the given entry

	{
	int iIndex;
	if (m_Table.FindPos(dwUNID, &iIndex))
		m_Table.Delete(iIndex);
	}

CDesignType *CDynamicDesignTable::FindType (DWORD dwUNID)

//	FindType
//
//	Finds the type by UNID

	{
	SEntry *pEntry = m_Table.GetAt(dwUNID);
	if (pEntry == NULL)
		return NULL;

	return pEntry->pType;
	}

int CDynamicDesignTable::GetXMLMemoryUsage (void) const

//	GetXMLMemoryUsage
//
//	Returns the amount of memory used by our XML source.

	{
	int i;

	int iTotal = 0;
	for (i = 0; i < m_Table.GetCount(); i++)
		{
		const SEntry *pEntry = &m_Table[i];
		iTotal += pEntry->pSource->GetMemoryUsage();
		}

	return iTotal;
	}

void CDynamicDesignTable::ReadFromStream (SUniverseLoadCtx &Ctx)

//	ReadFromStream
//
//	Loads table from a saved game

	{
	int i;

	CleanUp();

	DWORD dwCount;
	Ctx.pStream->Read((char *)&dwCount, sizeof(DWORD));
	for (i = 0; i < (int)dwCount; i++)
		{
		DWORD dwUNID;
		Ctx.pStream->Read((char *)&dwUNID, sizeof(DWORD));

		SEntry *pEntry = m_Table.Insert(dwUNID);
		pEntry->dwUNID = dwUNID;

		DWORD dwExtensionUNID;
		Ctx.pStream->Read((char *)&dwExtensionUNID, sizeof(DWORD));

		DWORD dwRelease;
		if (Ctx.dwVersion >= 15)
			Ctx.pStream->Read((char *)&dwRelease, sizeof(DWORD));
		else
			dwRelease = 0;

		//	Load the extension

		if (!Ctx.GetUniverse().FindExtension(dwExtensionUNID, dwRelease, &pEntry->pExtension))
			//	LATER: Need to return error
			pEntry->pExtension = NULL;

		pEntry->sSource.ReadFromStream(Ctx.pStream);

		//	Compile the type. At this point we ignore errors. There should be no
		//	parsing errors, since we already successfully parsed this at the
		//	beginning of the game.

		Compile(pEntry);
		}
	}

void CDynamicDesignTable::WriteToStream (IWriteStream *pStream)

//	WriteToStream
//
//	Writes to a saved game
//
//	DWORD		Count of types
//
//	for each type
//	DWORD		Type UNID
//	DWORD		Extension UNID
//	DWORD		Extension release
//	CString		XML Source

	{
	int i;
	DWORD dwSave = GetCount();
	pStream->Write((char *)&dwSave, sizeof(DWORD));

	for (i = 0; i < GetCount(); i++)
		{
		const SEntry *pEntry = GetEntry(i);

		pStream->Write((char *)&pEntry->dwUNID, sizeof(DWORD));

		dwSave = (pEntry->pExtension ? pEntry->pExtension->GetUNID() : 0);
		pStream->Write((char *)&dwSave, sizeof(DWORD));

		dwSave = (pEntry->pExtension ? pEntry->pExtension->GetRelease() : 0);
		pStream->Write((char *)&dwSave, sizeof(DWORD));

		if (!pEntry->sSource.IsBlank())
			pEntry->sSource.WriteToStream(pStream);
		else if (pEntry->pSource)
			{
			CString sSource = pEntry->pSource->ConvertToString();
			sSource.WriteToStream(pStream);
			}
		else
			NULL_STR.WriteToStream(pStream);
		}
	}
