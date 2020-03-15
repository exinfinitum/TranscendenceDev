//	IElementGenerator.cpp
//
//	IElementGenerator class
//	Copyright 2017 Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

#define GROUP_TAG					CONSTLIT("Group")
#define NODE_DISTANCE_TABLE_TAG		CONSTLIT("NodeDistanceTable")
#define NULL_TAG					CONSTLIT("Null")
#define TABLE_TAG					CONSTLIT("Table")
#define RANDOM_ITEM_TAG				CONSTLIT("RandomItem")

#define CHANCE_ATTRIB				CONSTLIT("chance")
#define COUNT_ATTRIB				CONSTLIT("count")
#define DISTANCE_ATTRIB				CONSTLIT("distance")
#define DISTANCE_TO_ATTRIB			CONSTLIT("distanceTo")
#define MAX_COUNT_ATTRIB			CONSTLIT("maxCount")

class CElementGroup : public IElementGenerator
	{
	public:
		static ALERROR CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator);

		virtual void Generate (SCtx &Ctx, TArray<SResult> &retResults) const override;

	private:
		TArray<TUniquePtr<IElementGenerator>> m_Group;
	};

class CElementNodeDistanceTable : public IElementGenerator
	{
	public:
		static ALERROR CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator);

		virtual void Generate (SCtx &Ctx, TArray<SResult> &retResults) const override;

	private:
		struct SEntry
			{
			CLargeSet DistanceMatch;
			TUniquePtr<IElementGenerator> Item;
			};

		int GetDistanceToOrigin (const CTopology &Topology, const CTopologyNode *pNode) const;
		void InitOriginList (SCtx &Ctx) const;

		CTopologyAttributeCriteria m_OriginCriteria;
		TArray<SEntry> m_Table;
		TUniquePtr<IElementGenerator> m_DefaultItem;

		mutable TArray<const CTopologyNode *> m_OriginList;
		mutable const CTopology *m_pTopology = NULL;
		mutable DWORD m_dwTopologyVersion = 0;
	};

class CElementNull : public IElementGenerator
	{
	public:
		static ALERROR CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator);

		virtual void Generate (SCtx &Ctx, TArray<SResult> &retResults) const override { }
	};

class CElementSingle : public IElementGenerator
	{
	public:
		static ALERROR CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator);

		virtual void Generate (SCtx &Ctx, TArray<SResult> &retResults) const override;

	private:
		CXMLElement *m_pElement;			//	NOTE: Callers own this pointer
	};

class CElementTable : public IElementGenerator
	{
	public:
		static ALERROR CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator);

		virtual void Generate (SCtx &Ctx, TArray<SResult> &retResults) const override;

	private:
		struct SEntry
			{
			int iChance = 0;
			int iMaxCount = -1;
			TUniquePtr<IElementGenerator> Item;
			};

		int Roll (void) const;
		int RollAndCheckLimits (TArray<int> &Counts) const;

		TArray<SEntry> m_Table;
		int m_iTotalChance;
		bool m_bHasLimits = false;
	};

//	IElementGenerator ----------------------------------------------------------

ALERROR IElementGenerator::InitFromXMLInternal (SDesignLoadCtx &Ctx, CXMLElement *pDesc)

//	InitFromXMLInternal
//
//	Initializes our member variables

	{
	//	Load the count

	if (m_Count.LoadFromXML(pDesc->GetAttribute(COUNT_ATTRIB), 1) != NOERROR)
		{
		Ctx.sError = strPatternSubst(CONSTLIT("Invalid count: %s"), pDesc->GetAttribute(COUNT_ATTRIB));
		return ERR_FAIL;
		}

	//	Success

	return NOERROR;
	}

ALERROR IElementGenerator::CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator)

//	CreateFromXML
//
//	Creates from an XML element.

	{
	ALERROR error;

	//	Create the generator of the proper class

	switch (GetGeneratorType(pDesc->GetTag()))
		{
		case typeGroup:
			error = CElementGroup::CreateFromXML(Ctx, pDesc, retGenerator);
			break;

		case typeNodeDistanceTable:
			error = CElementNodeDistanceTable::CreateFromXML(Ctx, pDesc, retGenerator);
			break;

		case typeNull:
			error = CElementNull::CreateFromXML(Ctx, pDesc, retGenerator);
			break;

		case typeTable:
			error = CElementTable::CreateFromXML(Ctx, pDesc, retGenerator);
			break;

		default:
			error = CElementSingle::CreateFromXML(Ctx, pDesc, retGenerator);
			break;
		}

	//	If error, then we didn't create anything.

	if (error != NOERROR)
		return error;

	//	Initialize the rest

	return retGenerator->InitFromXMLInternal(Ctx, pDesc);
	}

ALERROR IElementGenerator::CreateFromXMLAsGroup (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator)

//	CreateFromXMLAsGroup
//
//	Create from an XML element, assuming that the root element is a gorup of 
//	generators.

	{
	ALERROR error;

	if (error = CElementGroup::CreateFromXML(Ctx, pDesc, retGenerator))
		return error;

	//	Initialize the rest

	return retGenerator->InitFromXMLInternal(Ctx, pDesc);
	}

ALERROR IElementGenerator::CreateFromXMLAsTable (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator)

//	CreateFromXMLAsTable
//
//	Create from an XML element, assuming that the root element is a gorup of 
//	generators.

	{
	ALERROR error;

	if (error = CElementTable::CreateFromXML(Ctx, pDesc, retGenerator))
		return error;

	//	Initialize the rest

	return retGenerator->InitFromXMLInternal(Ctx, pDesc);
	}

void IElementGenerator::Generate (SCtx &Ctx, TArray<CXMLElement *> &retResults) const

//	Generate
//
//	Helper to generate into a simple array.

	{
	int i;

	TArray<SResult> Results;
	Generate(Ctx, Results);

	retResults.DeleteAll();
	retResults.InsertEmpty(Results.GetCount());
	for (i = 0; i < Results.GetCount(); i++)
		retResults[i] = Results[i].pElement;
	}

bool IElementGenerator::Generate (SCtx &Ctx, CXMLElement *pDesc, TArray<SResult> &retResults, CString *retsError)

//	Generate
//
//	Generate a list of single elements.

	{
	retResults.DeleteAll();

	//	Load the table

	SDesignLoadCtx LoadCtx;
	TUniquePtr<IElementGenerator> pGenerator;
	if (CreateFromXML(LoadCtx, pDesc, pGenerator) != NOERROR)
		{
		if (retsError) *retsError = LoadCtx.sError;
		return false;
		}

	//	Generate

	pGenerator->Generate(Ctx, retResults);

	//	Success

	return true;
	}

bool IElementGenerator::Generate (SCtx &Ctx, CXMLElement *pDesc, TArray<CXMLElement *> &retResults, CString *retsError)

//	GenerateAsGroup
//
//	Generate a list of single elements.

	{
	int i;

	TArray<SResult> Results;
	if (!Generate(Ctx, pDesc, Results, retsError))
		return false;

	//	Convert to an array of elements.

	retResults.DeleteAll();
	retResults.InsertEmpty(Results.GetCount());
	for (i = 0; i < Results.GetCount(); i++)
		retResults[i] = Results[i].pElement;

	//	Success

	return true;
	}

bool IElementGenerator::GenerateAsGroup (SCtx &Ctx, CXMLElement *pDesc, TArray<SResult> &retResults, CString *retsError)

//	GenerateAsGroup
//
//	Generate a list of single elements.

	{
	retResults.DeleteAll();

	//	Load the table

	SDesignLoadCtx LoadCtx;
	TUniquePtr<IElementGenerator> pGenerator;
	if (CreateFromXMLAsGroup(LoadCtx, pDesc, pGenerator) != NOERROR)
		{
		if (retsError) *retsError = LoadCtx.sError;
		return false;
		}

	//	Generate

	pGenerator->Generate(Ctx, retResults);

	//	Success

	return true;
	}

bool IElementGenerator::GenerateAsGroup (SCtx &Ctx, CXMLElement *pDesc, TArray<CXMLElement *> &retResults, CString *retsError)

//	GenerateAsGroup
//
//	Generate a list of single elements.

	{
	int i;

	TArray<SResult> Results;
	if (!GenerateAsGroup(Ctx, pDesc, Results, retsError))
		return false;

	//	Convert to an array of elements.

	retResults.DeleteAll();
	retResults.InsertEmpty(Results.GetCount());
	for (i = 0; i < Results.GetCount(); i++)
		retResults[i] = Results[i].pElement;

	//	Success

	return true;
	}

bool IElementGenerator::GenerateAsTable (SCtx &Ctx, CXMLElement *pDesc, TArray<SResult> &retResults, CString *retsError)

//	GenerateAsTable
//
//	Generate a list of single elements.

	{
	retResults.DeleteAll();

	//	Load the table

	SDesignLoadCtx LoadCtx;
	TUniquePtr<IElementGenerator> pGenerator;
	if (CreateFromXMLAsTable(LoadCtx, pDesc, pGenerator) != NOERROR)
		{
		if (retsError) *retsError = LoadCtx.sError;
		return false;
		}

	//	Generate

	pGenerator->Generate(Ctx, retResults);

	//	Success

	return true;
	}

bool IElementGenerator::GenerateAsTable (SCtx &Ctx, CXMLElement *pDesc, TArray<CXMLElement *> &retResults, CString *retsError)

//	GenerateAsTable
//
//	Generate a list of single elements.

	{
	int i;

	TArray<SResult> Results;
	if (!GenerateAsTable(Ctx, pDesc, Results, retsError))
		return false;

	//	Convert to an array of elements.

	retResults.DeleteAll();
	retResults.InsertEmpty(Results.GetCount());
	for (i = 0; i < Results.GetCount(); i++)
		retResults[i] = Results[i].pElement;

	//	Success

	return true;
	}

IElementGenerator::EGeneratorTypes IElementGenerator::GetGeneratorType (const CString &sTag)

//	GetGeneratorType
//
//	Converts from a tag to a type.

	{
	if (strEquals(sTag, GROUP_TAG))
		return typeGroup;
	else if (strEquals(sTag, TABLE_TAG))
		return typeTable;
	else if (strEquals(sTag, NODE_DISTANCE_TABLE_TAG))
		return typeNodeDistanceTable;
	else if (strEquals(sTag, NULL_TAG))
		return typeNull;
	else
		return typeElement;
	}

//	CElementGroup --------------------------------------------------------------

ALERROR CElementGroup::CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator)

//	CreateFromXML
//
//	Creates a group generator

	{
	int i;
	ALERROR error;

	CElementGroup *pEntry = new CElementGroup;

	//	Load each of the entries in the group

	pEntry->m_Group.InsertEmpty(pDesc->GetContentElementCount());
	for (i = 0; i < pEntry->m_Group.GetCount(); i++)
		{
		if (error = IElementGenerator::CreateFromXML(Ctx, pDesc->GetContentElement(i), pEntry->m_Group[i]))
			{
			delete pEntry;
			return error;
			}
		}

	//	Done

	retGenerator.Set(pEntry);
	return NOERROR;
	}

void CElementGroup::Generate (SCtx &Ctx, TArray<SResult> &retResults) const

//	Generate
//
//	Generate the entries

	{
	int i, j;

	int iCount = m_Count.Roll();
	for (i = 0; i < iCount; i++)
		{
		for (j = 0; j < m_Group.GetCount(); j++)
			m_Group[j]->Generate(Ctx, retResults);
		}
	}

//	CElementNodeDistanceTable --------------------------------------------------

ALERROR CElementNodeDistanceTable::CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator)

//	CreateFromXML
//
//	Creates a node distance table generator

	{
	int i;
	ALERROR error;
	const DWORD MAX_DISTANCE_VALUE = 100;

	CElementNodeDistanceTable *pGenerator = new CElementNodeDistanceTable;

	//	Parse the node criteria

	if (error = pGenerator->m_OriginCriteria.Init(pDesc->GetAttribute(DISTANCE_TO_ATTRIB)))
		{
		delete pGenerator;
		return error;
		}

	//	Now loop over all entries

	for (i = 0; i < pDesc->GetContentElementCount(); i++)
		{
		CXMLElement *pEntryXML = pDesc->GetContentElement(i);

		//	Load the generator for this entry.

		TUniquePtr<IElementGenerator> pNewEntry;
		if (error = IElementGenerator::CreateFromXML(Ctx, pEntryXML, pNewEntry))
			{
			delete pGenerator;
			return error;
			}
		
		//	If we have a distance attribute, then use that as a distance match

		CString sValue;
		if (pEntryXML->FindAttribute(DISTANCE_ATTRIB, &sValue))
			{
			//	Add a new entry to the table

			SEntry *pTableEntry = pGenerator->m_Table.Insert();
			if (!pTableEntry->DistanceMatch.InitFromString(sValue, MAX_DISTANCE_VALUE, &Ctx.sError))
				{
				delete pGenerator;
				return ERR_FAIL;
				}

			pTableEntry->Item.TakeHandoff(pNewEntry);
			}

		//	If this entry has no distance attribute, then it is the default entry

		else
			{
			pGenerator->m_DefaultItem.TakeHandoff(pNewEntry);
			}
		}

	//	Done

	retGenerator.Set(pGenerator);
	return NOERROR;
	}

int CElementNodeDistanceTable::GetDistanceToOrigin (const CTopology &Topology, const CTopologyNode *pNode) const

//	GetDistanceToOrigin
//
//	Returns the closes distance between the given node and one of the nodes in
//	the origin list.

	{
	int i;

	int iBestDist = -1;
	for (i = 0; i < m_OriginList.GetCount(); i++)
		{
		int iDist = Topology.GetDistance(pNode, m_OriginList[i]);
		if (iDist == -1)
			continue;

		if (iBestDist == -1 || iDist < iBestDist)
			{
			iBestDist = iDist;
			}
		}

	return iBestDist;
	}

void CElementNodeDistanceTable::InitOriginList (SCtx &Ctx) const

//	InitOriginList
//
//	Initializes the list of origins

	{
	int i;

	if (Ctx.pTopology == NULL)
		return;

	//	See if we've already initialize the list.

	if (m_OriginList.GetCount() > 0
			&& m_pTopology == Ctx.pTopology
			&& m_pTopology->GetVersion() == m_dwTopologyVersion)
		return;

	//	Initialize

	m_OriginList.DeleteAll();
	for (i = 0; i < Ctx.pTopology->GetTopologyNodeCount(); i++)
		{
		const CTopologyNode *pNode = Ctx.pTopology->GetTopologyNode(i);
		if (pNode->IsEndGame())
			continue;

		if (m_OriginCriteria.Matches(*pNode))
			m_OriginList.Insert(pNode);
		}

	//	Remember that we've initialized

	m_pTopology = Ctx.pTopology;
	m_dwTopologyVersion = Ctx.pTopology->GetVersion();
	}

void CElementNodeDistanceTable::Generate (SCtx &Ctx, TArray<SResult> &retResults) const

//	Generate
//
//	Generates the entries

	{
	int i;

	//	Must have a topology and a node in the context

	if (Ctx.pTopology == NULL || Ctx.pNode == NULL)
		return;

	//	If we have not yet got our list of origin nodes, then do it now.

	InitOriginList(Ctx);

	//	If the origin list is empty, then we default

	if (m_OriginList.GetCount() == 0)
		{
		if (m_DefaultItem)
			m_DefaultItem->Generate(Ctx, retResults);
		return;
		}

	//	Get the distance of this node to the origin node. If we can't get there
	//	from here, then we default.

	int iDistance = GetDistanceToOrigin(*Ctx.pTopology, Ctx.pNode);
	if (iDistance == -1)
		{
		if (m_DefaultItem)
			m_DefaultItem->Generate(Ctx, retResults);
		return;
		}

	//	Loop over all entries and make a list of the ones that match

	TArray<const SEntry *> Matches;
	for (i = 0; i < m_Table.GetCount(); i++)
		{
		const SEntry &Entry = m_Table[i];

		if (Entry.DistanceMatch.IsSet((DWORD)iDistance))
			Matches.Insert(&Entry);
		}

	//	If we have no matches, then use the default.

	if (Matches.GetCount() == 0)
		{
		if (m_DefaultItem)
			m_DefaultItem->Generate(Ctx, retResults);
		return;
		}

	//	Pick a random entry and generate that.

	int iIndex = mathRandom(0, Matches.GetCount() - 1);
	Matches[iIndex]->Item->Generate(Ctx, retResults);
	}

//	CElementNull ---------------------------------------------------------------

ALERROR CElementNull::CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator)

//	CreateFromXML
//
//	Creates a null generator

	{
	retGenerator.Set(new CElementNull);
	return NOERROR;
	}

//	CElementSingle -------------------------------------------------------------

ALERROR CElementSingle::CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator)

//	CreateFromXML
//
//	Creates a single element generator

	{
	CElementSingle *pEntry = new CElementSingle;
	pEntry->m_pElement = pDesc;
	retGenerator.Set(pEntry);
	return NOERROR;
	}

void CElementSingle::Generate (SCtx &Ctx, TArray<SResult> &retResults) const

//	Generate
//
//	Generate the entries

	{
	SResult *pResult = retResults.Insert();
	pResult->iCount = m_Count.Roll();
	pResult->pElement = m_pElement;
	}

//	CElementTable --------------------------------------------------------------

ALERROR CElementTable::CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, TUniquePtr<IElementGenerator> &retGenerator)

//	CreateFromXML
//
//	Creates a table generator

	{
	ALERROR error;

	CElementTable *pEntry = new CElementTable;

	//	Load each of the entries in the group

	pEntry->m_bHasLimits = false;
	pEntry->m_iTotalChance = 0;
	int iDefaultMaxCount = pDesc->GetAttributeIntegerBounded(MAX_COUNT_ATTRIB, 0, -1, -1);
	pEntry->m_Table.InsertEmpty(pDesc->GetContentElementCount());
	for (int i = 0; i < pEntry->m_Table.GetCount(); i++)
		{
		CXMLElement *pTableDesc = pDesc->GetContentElement(i);
		if (error = IElementGenerator::CreateFromXML(Ctx, pTableDesc, pEntry->m_Table[i].Item))
			{
			delete pEntry;
			return error;
			}

		pEntry->m_Table[i].iChance = pTableDesc->GetAttributeIntegerBounded(CHANCE_ATTRIB, 0, -1, 1);
		pEntry->m_Table[i].iMaxCount = pTableDesc->GetAttributeIntegerBounded(MAX_COUNT_ATTRIB, 0, -1, iDefaultMaxCount);

		pEntry->m_iTotalChance += pEntry->m_Table[i].iChance;

		if (pEntry->m_Table[i].iMaxCount != -1)
			pEntry->m_bHasLimits = true;
		}

	//	Done

	retGenerator.Set(pEntry);
	return NOERROR;
	}

void CElementTable::Generate (SCtx &Ctx, TArray<SResult> &retResults) const

//	Generate
//
//	Generate the entries

	{
	if (m_iTotalChance <= 0)
		return;

	int iCount = m_Count.Roll();
	for (int i = 0; i < iCount; i++)
		{
		//	NOTE: If we don't have the appropriate structure to track limits, 
		//	then we assume no limits.

		if (m_bHasLimits && Ctx.pTableCounts)
			{
			int iEntry = RollAndCheckLimits(Ctx.pTableCounts->Counts);
			if (iEntry == -1)
				return;

			m_Table[iEntry].Item->Generate(Ctx, retResults);
			}
		else
			m_Table[Roll()].Item->Generate(Ctx, retResults);
		}
	}

int CElementTable::Roll (void) const

//	Roll
//
//	Roll randomly and returns a table entry.

	{
	int iRoll = mathRandom(1, m_iTotalChance);
	for (int i = 0; i < m_Table.GetCount(); i++)
		{
		const SEntry &Entry = m_Table[i];
		if (iRoll <= Entry.iChance)
			return i;
		else
			iRoll -= Entry.iChance;
		}

	//	Should never get here, but if we do, it means that you're dreaming.
	//	This kick will wake you up...

	throw CException(ERR_FAIL);
	}

int CElementTable::RollAndCheckLimits (TArray<int> &Counts) const

//	RollAndCheckLimits
//
//	Roll on table until we get an entry that has not exceeded limits. If all 
//	entries have hit their limit, then we return -1.

	{
	//	Make sure the limits table is initialized.

	if (Counts.GetCount() < m_Table.GetCount())
		{
		Counts.DeleteAll();
		Counts.InsertEmpty(m_Table.GetCount());
		for (int i = 0; i < Counts.GetCount(); i++)
			Counts[i] = 0;
		}

	//	Generate a new table taking the limits into account.

	TArray<int> NewTable;
	NewTable.InsertEmpty(m_Table.GetCount());
	int iNewTotal = 0;
	for (int i = 0; i < m_Table.GetCount(); i++)
		{
		if (m_Table[i].iMaxCount == -1 ||
				Counts[i] < m_Table[i].iMaxCount)
			{
			NewTable[i] = m_Table[i].iChance;
			iNewTotal += m_Table[i].iChance;
			}
		else
			NewTable[i] = 0;
		}

	if (iNewTotal == 0)
		return -1;

	//	Roll

	int iRoll = mathRandom(1, iNewTotal);
	for (int i = 0; i < NewTable.GetCount(); i++)
		{
		if (NewTable[i] == 0)
			continue;

		if (iRoll <= NewTable[i])
			{
			Counts[i]++;
			return i;
			}
		else
			iRoll -= NewTable[i];
		}

	//	Should never get here.

	throw CException(ERR_FAIL);
	}
