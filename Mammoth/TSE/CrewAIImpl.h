//	CrewAIImpl.h
//
//	CCrewAI class
//	Copyright (c) 2015 by Kronosaur Productions, LLC. All Rights Reserved.

#pragma once

//	This is the list of supported archetypes. The order and number must match
//	the ARCHETYPE_DEFS array in PsycheDefs.h

enum ECrewArchetypes
	{
	archetypeNone =							0,

	archetypeBrotherhood =					1,	//	Biased toward cohesion
	archetypeOrder =						2,	//	Biased toward loyalty
	archetypeVengeance =					3,	//	Biased toward belief
	};

class CCrewPsyche
	{
	public:
		CCrewPsyche (void);

		ECrewArchetypes GetArchetype (void) const { return (ECrewArchetypes)m_dwArchetype; }
		DWORD GetBelief (void) const { return m_dwBelief; }
		DWORD GetCohesion (void) const { return m_dwCohesion; }
		DWORD GetLoyalty (void) const { return m_dwLoyalty; }
		void ReadFromStream (SLoadCtx &Ctx);
		void SetArchetype (ECrewArchetypes iArchetype);
		void SetBelief (DWORD dwBelief) { m_dwBelief = dwBelief; }
		void SetCohesion (DWORD dwCohesion) { m_dwCohesion = dwCohesion; }
		void SetLoyalty (DWORD dwLoyalty) { m_dwLoyalty = dwLoyalty; }
		void WriteToStream (IWriteStream *pStream);

	private:
		static DWORD GenerateNormalLevel (void);

		DWORD m_dwArchetype:8;
		DWORD m_dwBelief:8;
		DWORD m_dwCohesion:8;
		DWORD m_dwLoyalty:8;
	};

class CCrewAI : public CBaseShipAI
	{
	public:
		CCrewAI (void);

		//	IShipController virtuals

		virtual void AccumulateCrewMetrics (SCrewMetrics &Metrics);
		virtual CString DebugCrashInfo (void);
		virtual CString GetClass (void) { return CONSTLIT("crew"); }

	protected:

		//	CBaseShipAI overrides

		virtual bool OnGetAISettingInteger (const CString &sSetting, int *retiValue);
		virtual bool OnGetAISettingString (const CString &sSetting, CString *retsValue);
		virtual void OnObjDestroyedNotify (const SDestroyCtx &Ctx);
		virtual void OnReadFromStream (SLoadCtx &Ctx);
		virtual bool OnSetAISettingInteger (const CString &sSetting, int iValue);
		virtual bool OnSetAISettingString (const CString &sSetting, const CString &sValue);
		virtual void OnWriteToStream (IWriteStream *pStream);

	private:
		CCrewPsyche m_Psyche;				//	Controls response to orders and situations

		CString m_sCommandGroup;			//	Units in the same group can be commanded as a group
		int m_iCommandRank;					//	Highest rank in a group is responsible for orders
	};
