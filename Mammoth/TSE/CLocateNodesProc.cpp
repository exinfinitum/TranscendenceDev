//	CLocateNodesProc.cpp
//
//	CLocateNodesProce class

#include "PreComp.h"

#define CRITERIA_TAG						CONSTLIT("Criteria")
#define MAP_FUNCTION_TAG					CONSTLIT("MapFunction")

#define PERCENTILE_ATTRIB					CONSTLIT("percentile")

struct SComputedNode
	{
	CTopologyNode *pNode;
	Metric rValue;
	};

template<>
int Kernel::KeyCompare<SComputedNode> (const SComputedNode &Key1, const SComputedNode &Key2)
	{
	if (Key1.rValue > Key2.rValue)
		return 1;
	else if (Key1.rValue < Key2.rValue)
		return -1;
	else
		return 0;
	}

CLocateNodesProc::~CLocateNodesProc (void)

//	CLocateNodesProc destructor

	{
	int i;

	for (i = 0; i < m_Locations.GetCount(); i++)
		delete m_Locations[i].pProc;

	if (m_pMapFunction)
		delete m_pMapFunction;
	}

ALERROR CLocateNodesProc::OnBindDesign (SDesignLoadCtx &Ctx)

//	OnBindDesign
//
//	Bind design

	{
	ALERROR error;
	int i;

	for (i = 0; i < m_Locations.GetCount(); i++)
		if (error = m_Locations[i].pProc->BindDesign(Ctx))
			return error;

	return NOERROR;
	}

CEffectCreator *CLocateNodesProc::OnFindEffectCreator (const CString &sUNID)

//	OnFindEffectCreator
//
//	Finds the effect creator
//
//	{unid}:p{index}/{index}
//		           ^

	{
	const char *pPos = sUNID.GetASCIIZPointer();

	//	If we've got a slash, then recurse down

	if (*pPos == '/')
		{
		pPos++;

		//	Get the processor index

		int iIndex = ::strParseInt(pPos, -1, &pPos);
		if (iIndex < 0 || iIndex >= m_Locations.GetCount())
			return NULL;

		//	Let the processor handle it

		return m_Locations[iIndex].pProc->FindEffectCreator(CString(pPos));
		}

	//	Otherwise we have no clue

	else
		return NULL;
	}

ALERROR CLocateNodesProc::OnInitFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, const CString &sUNID)

//	OnInitFromXML
//
//	Initialize from XML element

	{
	ALERROR error;
	int i;

	//	Loop over all elements

	for (i = 0; i < pDesc->GetContentElementCount(); i++)
		{
		CXMLElement *pItem = pDesc->GetContentElement(i);

		//	See if this is an element handled by our base class

		if ((error = InitBaseItemXML(Ctx, pItem)) != ERR_NOTFOUND)
			{
			if (error != NOERROR)
				return error;
			}

		//	If this is a map function, read it

		else if (strEquals(pItem->GetTag(), MAP_FUNCTION_TAG))
			{
			if (pItem->GetContentElementCount() > 0)
				{
				if (m_pMapFunction)
					delete m_pMapFunction;

				if (error = I2DFunction::CreateFromXML(Ctx, pItem->GetContentElement(0), &m_pMapFunction))
					return error;
				}
			}

		//	Otherwise, treat it as a location definition and insert it in the list

		else
			{
			CString sNewUNID = strPatternSubst(CONSTLIT("%s/%d"), sUNID, m_Locations.GetCount());
			SLocation *pEntry = m_Locations.Insert();

			if (error = ITopologyProcessor::CreateFromXMLAsGroup(Ctx, pItem, sNewUNID, &pEntry->pProc))
				return error;

			//	Read the range

			if (error = ParseRange(Ctx, pItem->GetAttribute(PERCENTILE_ATTRIB), &pEntry->rMin, &pEntry->rMax))
				return error;
			}
		}

	return NOERROR;
	}

ALERROR CLocateNodesProc::OnProcess (SProcessCtx &Ctx, CTopologyNodeList &NodeList, CString *retsError)

//	OnProcess
//
//	Process on topology

	{
	ALERROR error;
	int i, j;

	//	If no locations, then we're done

	if (m_Locations.GetCount() == 0)
		return NOERROR;

	//	If we have a criteria, the filter the nodes

	CTopologyNodeList FilteredNodeList;
	CTopologyNodeList &NewNodeList = FilterNodes(Ctx.Topology, m_Criteria, NodeList, FilteredNodeList);

	//	If no nodes, then we're done

	if (NewNodeList.GetCount() == 0)
		return NOERROR;

	//	Compute a value for all nodes

	TArray<SComputedNode> Results;
	Results.InsertEmpty(NewNodeList.GetCount());
	if (m_pMapFunction)
		{
		for (i = 0; i < NewNodeList.GetCount(); i++)
			{
			Results[i].pNode = &NewNodeList[i];

			int x, y;
			Results[i].pNode->GetDisplayPos(&x, &y);

			Results[i].rValue = m_pMapFunction->Eval((Metric)x, (Metric)y);
			}

		//	Sort the list by ascending value

		Results.Sort();
		}
	else
		{
		for (i = 0; i < NewNodeList.GetCount(); i++)
			{
			Results[i].pNode = &NewNodeList[i];
			Results[i].rValue = 1.0f;
			}
		}

	//	Mark all nodes in the list; we clear the mark when we
	//	process a node so each node is only processed in a single
	//	location.

	TArray<bool> SavedMarks;
	SavedMarks.InsertEmpty(NewNodeList.GetCount());
	for (i = 0; i < NewNodeList.GetCount(); i++)
		{
		SavedMarks[i] = NewNodeList[i].IsMarked();
		NewNodeList[i].SetMarked(true);
		}

	//	Loop over all locations

	for (i = 0; i < m_Locations.GetCount(); i++)
		{
		double rMin = m_Locations[i].rMin;
		double rMax = m_Locations[i].rMax;

		//	Generate a list of nodes that match this location

		CTopologyNodeList LocationNodes;
		for (j = 0; j < Results.GetCount(); j++)
			{
			CTopologyNode *pNode = Results[j].pNode;
			double rPercentile = ((double)j + 0.5f) / (double)Results.GetCount();
			if (pNode->IsMarked() && rPercentile >= rMin && rPercentile <= rMax)
				{
				pNode->SetMarked(false);
				LocationNodes.Insert(pNode);
				}
			}
		
		//	Process

		if (LocationNodes.GetCount() > 0)
			if (error = m_Locations[i].pProc->Process(Ctx, LocationNodes, retsError))
				return error;
		}

	//	Remove from the original node list

	if (Ctx.bReduceNodeList)
		{
		for (i = 0; i < NewNodeList.GetCount(); i++)
			if (!NewNodeList[i].IsMarked())
				NodeList.Delete(&NewNodeList[i]);
		}

	//	Done

	for (i = 0; i < NewNodeList.GetCount(); i++)
		NewNodeList[i].SetMarked(SavedMarks[i]);

	return NOERROR;
	}

ALERROR CLocateNodesProc::ParseRange (SDesignLoadCtx &Ctx, const CString &sRange, double *retrMin, double *retrMax)

//	ParseRange
//
//	Parse a string of the form
//
//	{min}-{max}
//
//	Where {min} and {max} are integers from 0 to 100 and returns a floating point
//	range from 0.0 to 1.0

	{
	if (sRange.IsBlank())
		{
		*retrMin = 0.0f;
		*retrMax = 1.0f;
		}
	else
		{
		const char *pPos = sRange.GetASCIIZPointer();
		while (*pPos == ' ')
			pPos++;

		int iFirst = strParseInt(pPos, 1, &pPos);
		int iSecond = -1;
		while (*pPos == ' ')
			pPos++;

		if (*pPos == '-')
			{
			pPos++;
			while (*pPos == ' ')
				pPos++;

			iSecond = strParseInt(pPos, 100, &pPos);
			}

		*retrMin = ((double)iFirst / 100.0f) - 0.005f;
		if (iSecond == -1)
			*retrMax = *retrMin + 0.01f;
		else
			*retrMax = ((double)iSecond / 100.0f) + 0.005f;
		}

	return NOERROR;
	}
