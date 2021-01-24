//	TSEShipCreator.h
//
//	Defines classes and interfaces for creating ships
//	Copyright (c) 2016 Kronosaur Productions, LLC. All Rights Reserved.

#pragma once

class CShipChallengeCtx
	{
	public:
		CShipChallengeCtx (void) :
				m_iTotalCount(0),
				m_iTotalScore(0),
				m_rTotalCombat(0.0)
			{ }

		void AddShip (CSpaceObject *pObj);
		void AddShips (CSpaceObjectList &List);
		Metric GetTotalCombat (void) const { return m_rTotalCombat; }
		int GetTotalCount (void) const { return m_iTotalCount; }
		int GetTotalScore (void) const { return m_iTotalScore; }

	private:
		int m_iTotalCount;
		int m_iTotalScore;
		Metric m_rTotalCombat;
	};

class CShipChallengeDesc
	{
	public:
		enum ECountTypes
			{
			countNone,

			countReinforcements,			//	Use m_Count as minimum number
			countScore,						//	Use m_Count as desired total score
			countStanding,					//	Use m_Count as minimum number

			countChallengeEasy,				//	1.5x ships of system level (by score)
			countChallengeStandard,			//	2.5x
			countChallengeHard,				//	4x ships
			countChallengeDeadly,			//	6x ships
			};

		CShipChallengeDesc (void) :
				m_iType(countNone)
			{ }

		ECountTypes GetCountType (void) const { return m_iType; }
		Metric GetChallengeStrength (int iLevel) const { return CalcChallengeStrength(m_iType, iLevel); }
		bool Init (ECountTypes iType, int iCount = 0);
		bool Init (ECountTypes iType, const CString &sCount);
		bool InitFromChallengeRating (const CString &sChallenge);
		bool IsEmpty (void) const { return m_iType == countNone; }
		bool NeedsMoreInitialShips (CSpaceObject *pBase, const CShipChallengeCtx &Ctx) const;
		bool NeedsMoreReinforcements (CSpaceObject *pBase) const;

	private:
		static Metric CalcChallengeStrength (ECountTypes iType, int iLevel);
		static ECountTypes ParseChallengeType (const CString &sValue);

		ECountTypes m_iType;				//	Method for determining ships numbers
		DiceRange m_Count;
	};

//	IShipGenerator

struct SShipCreateCtx
	{
	SShipCreateCtx (void)
		{ }

	CUniverse &GetUniverse (void) const { return *g_pUniverse; }

	CSystem *pSystem = NULL;					//	System to create ship in
	CSpaceObject *pGate = NULL;					//	Gate where ship will appear (may be NULL)
	CVector vPos;								//	Position where ship will appear (only if pGate is NULL)
	DiceRange PosSpread;						//	Distance from vPos (in light-seconds)
	CSpaceObject *pBase = NULL;					//	Base for this ship (may be NULL)
	CSovereign *pBaseSovereign = NULL;			//	Only if pBase is NULL
	IShipController::OrderTypes iDefaultOrder = IShipController::orderNone;
	CSpaceObject *pTarget = NULL;				//	Target for ship orders (may be NULL)
	CStationType *pEncounterInfo = NULL;		//	Encounter info (may be NULL)
	CDesignType *pOverride = NULL;				//	Override to apply to ships (may be NULL)

	DWORD dwFlags = 0;							//	Flags

	CSpaceObjectList Result;					//	List of ships created

	enum Flags
		{
		SHIPS_FOR_STATION =			0x00000001,	//	Create ships at station creation time
		ATTACK_NEAREST_ENEMY =		0x00000002,	//	After all other orders, attack nearest enemy
		RETURN_RESULT =				0x00000004,	//	Add created ships to Result
		RETURN_ESCORTS =			0x00000008,	//	Include escorts in result
		};
	};

class IShipGenerator
	{
	public:
		static ALERROR CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, IShipGenerator **retpGenerator);
		static ALERROR CreateFromXMLAsGroup (SDesignLoadCtx &Ctx, const CXMLElement *pDesc, IShipGenerator **retpGenerator);

		virtual ~IShipGenerator (void) { }
		virtual void AddTypesUsed (TSortMap<DWORD, bool> *retTypesUsed) { }
		virtual void CreateShips (SShipCreateCtx &Ctx) const { }
		virtual Metric GetAverageLevelStrength (int iLevel) const { return 0.0; }
		virtual ALERROR LoadFromXML (SDesignLoadCtx &Ctx, const CXMLElement *pDesc) { return NOERROR; }
		virtual ALERROR OnDesignLoadComplete (SDesignLoadCtx &Ctx) { return NOERROR; }
		virtual ALERROR ValidateForRandomEncounter (void) { return NOERROR; }
	};

struct SShipGeneratorCtx
	{
	CString sName;								//	If not blank, use as name of ship
	DWORD dwNameFlags = 0;						//	Name flags (only if sName is not blank)
	IItemGenerator *pItems = NULL;				//	Items to add to ship (may be NULL)
	CAttributeDataBlock InitialData;			//	Initial data
	ICCItem *pOnCreate = NULL;					//	Additional OnCreate code (may be NULL)
	DWORD dwCreateFlags = 0;

	COrderDesc OrderDesc;						//	Order for ship

	CSpaceObject *pBase = NULL;					//	Base for ship (may be NULL)
	CSpaceObject *pTarget = NULL;				//	Target for ship (may be NULL)
	};

//	CShipTable ----------------------------------------------------------------

class CShipTable : public CDesignType
	{
	public:
		CShipTable (void);
		virtual ~CShipTable (void);

		void CreateShips (SShipCreateCtx &Ctx) { if (m_pGenerator) m_pGenerator->CreateShips(Ctx); }
		Metric GetAverageLevelStrength (int iLevel) { return (m_pGenerator ? m_pGenerator->GetAverageLevelStrength(iLevel) : 0.0); }
		ALERROR ValidateForRandomEncounter (void) { if (m_pGenerator) return m_pGenerator->ValidateForRandomEncounter(); }

		//	CDesignType overrides
		static CShipTable *AsType (CDesignType *pType) { return ((pType && pType->GetType() == designShipTable) ? (CShipTable *)pType : NULL); }
		virtual DesignTypes GetType (void) const override { return designShipTable; }

	protected:
		//	CDesignType overrides
		virtual void OnAccumulateXMLMergeFlags (TSortMap<DWORD, DWORD> &MergeFlags) const override;
		virtual void OnAddTypesUsed (TSortMap<DWORD, bool> *retTypesUsed) override { if (m_pGenerator) m_pGenerator->AddTypesUsed(retTypesUsed); }
		virtual ALERROR OnBindDesign (SDesignLoadCtx &Ctx) override;
		virtual ALERROR OnCreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc) override;

	private:
		IShipGenerator *m_pGenerator;
	};

