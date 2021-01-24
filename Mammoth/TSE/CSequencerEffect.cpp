//	CSequencerEffect.cpp
//
//	CSequencerEffect object

#include "PreComp.h"

ALERROR CSequencerEffect::Create (CSystem &System,
								  CEffectSequencerCreator *pType,
								  CSpaceObject *pAnchor,
								  const CVector &vPos,
								  const CVector &vVel,
								  CSequencerEffect **retpEffect)

//	Create
//
//	Creates a new object

	{
	ALERROR error;
	CSequencerEffect *pEffect;

	pEffect = new CSequencerEffect(System.GetUniverse());
	if (pEffect == NULL)
		return ERR_MEMORY;

	pEffect->Place(vPos, (pAnchor == NULL ? vVel : CVector()));
	pEffect->SetObjectDestructionHook();
	pEffect->SetCannotBeHit();

	//	Settings

	pEffect->m_pType = pType;
	pEffect->m_iStartTime = System.GetUniverse().GetTicks();
	pEffect->m_iTimeCursor = pEffect->m_iStartTime - 1;
	if (pAnchor)
		{
		pEffect->m_pAnchor = pAnchor;
		pEffect->m_vAnchorOffset = vPos - pAnchor->GetPos();
		}

	//	Add to system

	if (error = pEffect->AddToSystem(System))
		{
		delete pEffect;
		return error;
		}

	//	Done

	if (retpEffect)
		*retpEffect = pEffect;

	return NOERROR;
	}

void CSequencerEffect::ObjectDestroyedHook (const SDestroyCtx &Ctx)

//	ObjectDestroyedHook
//
//	Handle it when another object is destroyed

	{
	if (m_pAnchor == Ctx.Obj)
		m_pAnchor = NULL;
	}

void CSequencerEffect::OnReadFromStream (SLoadCtx &Ctx)

//	OnReadFromStream
//
//	Read from a stream
//
//	CString			m_pType UNID
//	CSpaceObject	m_pAnchor
//	CVector			m_vAnchorOffset
//	DWORD			m_iStartTime
//	DWORD			m_iTimeCursor

	{
	CString sUNID;
	sUNID.ReadFromStream(Ctx.pStream);
	CEffectCreator *pEffect = CEffectCreator::FindEffectCreator(sUNID);
	if (pEffect && strEquals(pEffect->GetTag(), CONSTLIT("Sequencer")))
		m_pType = (CEffectSequencerCreator *)pEffect;
	else
		m_pType = NULL;

	CSystem::ReadObjRefFromStream(Ctx, &m_pAnchor);
	Ctx.pStream->Read((char *)&m_vAnchorOffset, sizeof(CVector));
	Ctx.pStream->Read((char *)&m_iStartTime, sizeof(DWORD));
	Ctx.pStream->Read((char *)&m_iTimeCursor, sizeof(DWORD));
	}

void CSequencerEffect::OnWriteToStream (IWriteStream *pStream)

//	OnWriteToStream
//
//	Write to a stream
//
//	CString			m_pType UNID
//	CSpaceObject	m_pAnchor
//	CVector			m_vAnchorOffset
//	DWORD			m_iStartTime
//	DWORD			m_iTimeCursor

	{
	CString sUNID = (m_pType ? m_pType->GetUNIDString() : NULL_STR);
	sUNID.WriteToStream(pStream);

	WriteObjRefToStream(m_pAnchor, pStream);

	pStream->Write((char *)&m_vAnchorOffset, sizeof(CVector));
	pStream->Write((char *)&m_iStartTime, sizeof(DWORD));
	pStream->Write((char *)&m_iTimeCursor, sizeof(DWORD));
	}

void CSequencerEffect::OnUpdate (SUpdateCtx &Ctx, Metric rSecondsPerTick)

//	OnUpdate
//
//	Update

	{
	DEBUG_TRY

	int i;

	bool bFutureEvents = false;
	if (m_pType)
		{
		//	Convert start and end time relative to 0.

		int iStartTime = m_iTimeCursor - m_iStartTime;
		int iEndTime = GetUniverse().GetTicks() - m_iStartTime;
		CVector vPos = (m_pAnchor ? m_pAnchor->GetPos() + m_vAnchorOffset : GetPos());
		CVector vVel = (m_pAnchor ? m_pAnchor->GetVel() : GetVel());

		//	Loop over all creators and create the ones in the time window

		for (i = 0; i < m_pType->GetCount(); i++)
			{
			CEffectSequencerCreator::TimeTypes iEventTime = m_pType->GetTime(i, iStartTime, iEndTime);

			//	If this event happened in our time window, then create the effect

			if (iEventTime == CEffectSequencerCreator::timeNow)
				{
				m_pType->GetCreator(i)->CreateEffect(GetSystem(),
						m_pAnchor,
						vPos,
						vVel,
						0);
				}

			//	Otherwise, if it happens in the future, mark it

			else if (iEventTime == CEffectSequencerCreator::timeFuture)
				bFutureEvents = true;
			}

		//	Remember the last time we got called. We need this in case
		//	the system is not updated for a while.

		m_iTimeCursor = GetUniverse().GetTicks();
		}

	//	If we have no more events, then we can destroy this object

	if (!bFutureEvents)
		Destroy(removedFromSystem, CDamageSource());

	DEBUG_CATCH
	}
