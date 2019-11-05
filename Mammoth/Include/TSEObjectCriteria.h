//	TSEObjectCriteria.h
//
//	Defines classes and interfaces for selecting space objects
//	Copyright (c) 2018 Kronosaur Productions, LLC. All Rights Reserved.

#pragma once

class CSpaceObject;

class CSpaceObjectCriteria
	{
	public:
		enum CriteriaPosCheckTypes
			{
			checkNone,
			checkPosIntersect,
			checkLineIntersect,
			};

		enum CriteriaSortTypes
			{
			sortNone,
			sortByDistance,
			};

		struct SCtx
			{
			SCtx (const CSpaceObjectCriteria &Crit);

			//	Computed from criteria
			Metric rMinRadius2;
			Metric rMaxRadius2;

			//	Nearest/farthest object
			CSpaceObject *pBestObj;
			Metric rBestDist2;

			//	Sorted results
			TSortMap<Metric, CSpaceObject *> DistSort;

			//	Temporaries
			bool bCalcPolar;
			bool bCalcDist2;
			};

		CSpaceObjectCriteria (void) { }
		explicit CSpaceObjectCriteria (const CString &sCriteria);
		CSpaceObjectCriteria (CSpaceObject *pSourceArg, const CString &sCriteria);

		bool ExcludesIntangible (void) const { return !m_bIncludeIntangible; }
		bool ExcludesPlayer (void) const { return m_bExcludePlayer; }
		bool ExcludesVirtual (void) const { return !m_bIncludeVirtual; }
		const CSpaceObjectCriteria &GetORExpression (void) const { return (m_pOr ? *m_pOr : m_Null); }
		CriteriaSortTypes GetSort (void) const { return m_iSort; }
		ESortOptions GetSortOrder (void) const { return m_iSortOrder; }
		CSpaceObject *GetSource (void) const { return m_pSource; }
		bool HasORExpression (void) const { return (m_pOr ? true : false); }
		void Init (const CString &sCriteria) { *this = CSpaceObjectCriteria(); Parse(NULL, sCriteria); }
		void Init (CSpaceObject *pSource, const CString &sCriteria) { *this = CSpaceObjectCriteria(); Parse(pSource, sCriteria); }
		bool IsEmpty (void) const { return (m_dwCategories == 0); }
		bool MatchesActiveOnly (void) const { return m_bActiveObjectsOnly; }
		bool MatchesAngryOnly (void) const { return m_bAngryObjectsOnly; }
		bool MatchesAttributes (const CSpaceObject &Obj) const;
		bool MatchesCategory (DWORD dwCat) const { return (m_dwCategories & dwCat) != 0; }
		const CString &MatchesData (void) const { return m_sData; }
		bool MatchesDockedWithSource (void) const { return m_bDockedWithSource; }
		bool MatchesEnemyOnly (void) const { return m_bEnemyObjectsOnly; }
		bool MatchesFartherThan (void) const { return m_bFartherThan; }
		bool MatchesFarthestOnly (void) const { return m_bFarthestOnly; }
		bool MatchesFriendlyOnly (void) const { return m_bFriendlyObjectsOnly; }
		bool MatchesHomeBaseIsSource (void) const { return m_bHomeBaseIsSource; }
		int MatchesIntersectAngle (void) const { return m_iIntersectAngle; }
		bool MatchesKilledOnly (void) const { return m_bKilledObjectsOnly; }
		bool MatchesLevel (int iLevel) const;
		bool MatchesManufacturedOnly (void) const { return m_bManufacturedObjectsOnly; }
		Metric MatchesMaxRadius (void) const { return m_rMaxRadius; }
		Metric MatchesMinRadius (void) const { return m_rMinRadius; }
		bool MatchesNearerThan (void) const { return m_bNearerThan; }
		bool MatchesNearestOnly (void) const { return m_bNearestOnly; }
		bool MatchesOrder (const CSpaceObject *pSource, const CSpaceObject &Obj) const;
		bool MatchesPerceivableOnly (void) const { return m_bPerceivableOnly; }
		int MatchesPerception (void) const { return m_iPerception; }
		bool MatchesPlayer (void) const { return m_bSelectPlayer; }
		bool MatchesPosition (const CSpaceObject &Obj) const;
		bool MatchesSovereign (CSovereign *pSovereign) const;
		bool MatchesStargate (const CString &sID) const { return (m_sStargateID.IsBlank() || strEquals(m_sStargateID, sID)); }
		bool MatchesStargatesOnly (void) const { return m_bStargatesOnly; }
		bool MatchesStructureScaleOnly (void) const { return m_bStructureScaleOnly; }
		bool MatchesTargetIsSource (void) const { return m_bTargetIsSource; }
		void SetIncludeIntangible (bool bValue = true) { m_bIncludeIntangible = bValue; }
		void SetLineIntersect (const CVector &vPos1, const CVector &vPos2) { m_iPosCheck = checkLineIntersect; m_vPos1 = vPos1; m_vPos2 = vPos2; }
		void SetPosIntersect (const CVector &vPos) { m_iPosCheck = checkPosIntersect; m_vPos1 = vPos; }
		void SetSource (CSpaceObject *pSource);

	private:
		void Parse (CSpaceObject *pSource, const CString &sCriteria);
		void ParseSubExpression (const char *pPos);

		CSpaceObject *m_pSource = NULL;				//	Source

		DWORD m_dwCategories = 0;					//	Only these object categories
		bool m_bSelectPlayer = false;				//	Select the player
		bool m_bIncludeVirtual = false;				//	Include virtual objects
		bool m_bActiveObjectsOnly = false;			//	Only active object (e.g., objects that can attack)
		bool m_bKilledObjectsOnly = false;			//	Only objects that cannot attack
		bool m_bFriendlyObjectsOnly = false;		//	Only friendly to source
		bool m_bEnemyObjectsOnly = false;			//	Only enemy to source
		bool m_bAngryObjectsOnly = false;			//	Only enemy and angry objects
		bool m_bManufacturedObjectsOnly = false;	//	Exclude planets, stars, etc.
		bool m_bStructureScaleOnly = false;			//	Only structure-scale objects
		bool m_bStargatesOnly = false;				//	Only stargates
		bool m_bNearestOnly = false;				//	The nearest object to the source
		bool m_bFarthestOnly = false;				//	The fartest object to the source
		bool m_bNearerThan = false;					//	Only objects nearer than rMinRadius
		bool m_bFartherThan = false;				//	Only objects farther than rMaxRadius
		bool m_bHomeBaseIsSource = false;			//	Only objects whose home base is the source
		bool m_bDockedWithSource = false;			//	Only objects currently docked with source
		bool m_bExcludePlayer = false;				//	Exclude the player
		bool m_bTargetIsSource = false;				//	Only objects whose target is the source
		bool m_bIncludeIntangible = false;			//	Include intangible objects

		bool m_bPerceivableOnly = false;			//	Only objects that can be perceived by the source
		int m_iPerception = 0;						//	Cached perception of pSource

		bool m_bSourceSovereignOnly = false;		//	Only objects the same sovereign as source
		DWORD m_dwSovereignUNID = 0;				//	Only objects with this sovereign UNID

		CString m_sData;							//	Only objects with non-Nil data
		CString m_sStargateID;						//	Only objects with this stargate ID (if non blank)
		Metric m_rMinRadius = 0.0;					//	Only objects at or beyond the given radius
		Metric m_rMaxRadius = g_InfiniteDistance;	//	Only objects within the given radius
		int m_iIntersectAngle = -1;					//	Only objects that intersect line from source
		IShipController::OrderTypes m_iOrder = IShipController::orderNone;	//	Only objects with this order

		TArray<CString> m_AttribsRequired;			//	Required attributes
		TArray<CString> m_AttribsNotAllowed;		//	Exclude objects with these attributes
		TArray<CString> m_SpecialRequired;			//	Special required attributes
		TArray<CString> m_SpecialNotAllowed;		//	Special excluding attributes

		int m_iEqualToLevel = -1;					//	Objects of this level
		int m_iGreaterThanLevel = -1;
		int m_iLessThanLevel = -1;

		CriteriaPosCheckTypes m_iPosCheck = checkNone;
		CVector m_vPos1;
		CVector m_vPos2;

		CriteriaSortTypes m_iSort = sortNone;
		ESortOptions m_iSortOrder = AscendingSort;

		TUniquePtr<CSpaceObjectCriteria> m_pOr;		//	OR sub-expression.

		static const CSpaceObjectCriteria m_Null;
	};

