//	CGroupTopologyProc.cpp
//
//	CGroupTopologyProc class

#include "PreComp.h"

#define CRITERIA_TAG						CONSTLIT("Criteria")

CGroupTopologyProc::~CGroupTopologyProc (void)

//	CGroupTopologyProc destructor

	{
	int i;

	for (i = 0; i < m_Procs.GetCount(); i++)
		delete m_Procs[i];
	}

ALERROR CGroupTopologyProc::OnBindDesign (SDesignLoadCtx &Ctx)

//	OnBindDesign
//
//	Bind design

	{
	ALERROR error;
	int i;

	for (i = 0; i < m_Procs.GetCount(); i++)
		if (error = m_Procs[i]->BindDesign(Ctx))
			return error;

	return NOERROR;
	}

CEffectCreator *CGroupTopologyProc::OnFindEffectCreator (const CString &sUNID)

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
		if (iIndex < 0 || iIndex >= m_Procs.GetCount())
			return NULL;

		//	Let the processor handle it

		return m_Procs[iIndex]->FindEffectCreator(CString(pPos));
		}

	//	Otherwise we have no clue

	else
		return NULL;
	}

ALERROR CGroupTopologyProc::OnInitFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, const CString &sUNID)

//	OnInitFromXML
//
//	Initialize from XML element

	{
	ALERROR error;
	int i;

	//	LATER: Read m_bReduceNodeList so that we can choose that option.

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

		//	Otherwise, load it as a processor

		else
			{
			ITopologyProcessor *pNewProc;
			CString sNewUNID = strPatternSubst(CONSTLIT("%s/%d"), sUNID, m_Procs.GetCount());

			if (error = ITopologyProcessor::CreateFromXML(Ctx, pItem, sNewUNID, &pNewProc))
				return error;

			m_Procs.Insert(pNewProc);
			}
		}

	return NOERROR;
	}

ALERROR CGroupTopologyProc::OnProcess (SProcessCtx &Ctx, CTopologyNodeList &NodeList, CString *retsError)

//	OnProcess
//
//	Process on topology

	{
	ALERROR error;
	int i;

	//	If no processors, then we're done

	if (m_Procs.GetCount() == 0)
		return NOERROR;

	//	If we have a criteria, the filter the nodes

	CTopologyNodeList FilteredNodeList;
	CTopologyNodeList *pNodeList = &FilterNodes(Ctx.Topology, m_Criteria, NodeList, FilteredNodeList);

	//	If we're reducing the node list, then make a copy of the node list.

	bool bSavedReduceNodeList = Ctx.bReduceNodeList;
	Ctx.bReduceNodeList = m_bReduceNodeList;

	//	If we're reducing the list, then make a copy of it (because we don't 
	//	want to change our input list).

	if (m_bReduceNodeList && pNodeList == &NodeList)
		{
		FilteredNodeList = NodeList;
		pNodeList = &FilteredNodeList;
		}

	//	Loop over all processors in order
	//	(Each call will potentially reduce the node list)

	for (i = 0; i < m_Procs.GetCount(); i++)
		{
		if (error = m_Procs[i]->Process(Ctx, *pNodeList, retsError))
			return error;
		}

	//	Restore

	Ctx.bReduceNodeList = bSavedReduceNodeList;

	return NOERROR;
	}
