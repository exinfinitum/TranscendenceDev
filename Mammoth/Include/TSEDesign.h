//	TSEDesign.h
//
//	Transcendence design classes
//	Copyright 2012 by Kronosaur Productions, LLC. All Rights Reserved.

#pragma once

class CCommunicationsHandler;
class CCompositeImageDesc;
class CCreatePainterCtx;
class CCurrencyAndValue;
class CDesignCollection;
class CDockScreen;
class CDockingPorts;
class CDynamicDesignTable;
class CEffect;
class CGameStats;
class CGenericType;
class CItem;
class CItemCtx;
class CItemEnhancementStack;
class CObjectImageArray;
class COrbit;
class COrderList;
class CMultiverseCollection;
class CMultiverseCatalogEntry;
class CShipClass;
class CSystem;
class CSystemMap;
class CTopology;
class CTopologyDescTable;
class CTradingDesc;
class IDeviceGenerator;
class IPlayerController;
struct SDestroyCtx;
struct SSystemCreateCtx;
struct STradeServiceCtx;

//	Constants & Enums

const int MAX_OBJECT_LEVEL =			25;	//	Max level for space objects
const int MAX_ITEM_LEVEL =				25;	//	Max level for items

//	Base Design Type ----------------------------------------------------------
//
//	To add a new DesignType:
//
//	[ ]	Add it to DesignTypes enum
//	[ ] Increment designCount
//	[ ] Add a char??? entry
//	[ ] Add entry to DESIGN_CHAR in CDesignType (make sure it matches the char??? entry)
//	[ ] Add entry to DESIGN_CLASS_NAME in CDesignType
//	[ ] Add case to CDesignTypeCriteria::ParseCriteria (if type should be enumerable)
//	[ ] Add constructor call to CDesignType::CreateFromXML

enum DesignTypes
	{
	designItemType =					0,
	designItemTable =					1,
	designShipClass =					2,
	designOverlayType =				    3,
	designSystemType =					4,
	designStationType =					5,
	designSovereign =					6,
	designDockScreen =					7,
	designEffectType =					8,
	designPower =						9,

	designSpaceEnvironmentType =		10,
	designShipTable =					11,
	designAdventureDesc =				12,
	designGlobals =						13,
	designImage =						14,
	designMusic =						15,
	designMissionType =					16,
	designSystemTable =					17,
	designSystemMap =					18,
	designUnused =						19,

	designEconomyType =					20,
	designTemplateType =				21,
	designGenericType =					22,
	designImageComposite =				23,
	designSound =						24,

	designCount	=						25, 

	designSetAll =						0xffffffff,
	charEconomyType =					'$',
	charAdventureDesc =					'a',
	charItemTable =						'b',
	charEffectType =					'c',
	charDockScreen =					'd',
	charSpaceEnvironmentType =			'e',
	charOverlayType =				    'f',
	charGlobals =						'g',
	charShipTable =						'h',
	charItemType =						'i',
	charSound =							'j',
	//	k
	//	l
	charImage =							'm',
	charMissionType =					'n',
	charImageComposite =				'o',
	charPower =							'p',
	charSystemTable =					'q',
	//	r
	charShipClass =						's',
	charStationType =					't',
	charMusic =							'u',
	charSovereign =						'v',
	//	w
	charGenericType =					'x',

	charSystemType =					'y',
	charSystemMap =						'z',
	charTemplateType =					'_',
	};

class CDesignTypeCriteria
	{
	public:
		CDesignTypeCriteria (void);

		CString AsString (void) const;
		inline bool ChecksLevel (void) const { return (m_iGreaterThanLevel != INVALID_COMPARE || m_iLessThanLevel != INVALID_COMPARE); }
		inline const CString &GetExcludedAttrib (int iIndex) const { return m_sExclude[iIndex]; }
		inline int GetExcludedAttribCount (void) const { return m_sExclude.GetCount(); }
		inline const CString &GetExcludedSpecialAttrib (int iIndex) const { return m_sExcludeSpecial[iIndex]; }
		inline int GetExcludedSpecialAttribCount (void) const { return m_sExcludeSpecial.GetCount(); }
		inline const CString &GetRequiredAttrib (int iIndex) const { return m_sRequire[iIndex]; }
		inline int GetRequiredAttribCount (void) const { return m_sRequire.GetCount(); }
		inline const CString &GetRequiredSpecialAttrib (int iIndex) const { return m_sRequireSpecial[iIndex]; }
		inline int GetRequiredSpecialAttribCount (void) const { return m_sRequireSpecial.GetCount(); }
        inline void IncludeType (DesignTypes iType) { m_dwTypeSet |= (1 << iType); }
		inline bool IncludesVirtual (void) const { return m_bIncludeVirtual; }
        inline bool IsEmpty (void) const { return (m_dwTypeSet == 0); }
		inline bool IsEqual (const CDesignTypeCriteria &Src) const { return strEquals(Src.AsString(), AsString()); }
		inline bool MatchesDesignType (DesignTypes iType) const
			{ return ((m_dwTypeSet & (1 << iType)) ? true : false); }
		bool MatchesLevel (int iMinLevel, int iMaxLevel) const;
		void ReadFromStream (SLoadCtx &Ctx);
		inline bool StructuresOnly (void) const { return m_bStructuresOnly; }
		void WriteToStream (IWriteStream *pStream);

		static ALERROR ParseCriteria (const CString &sCriteria, CDesignTypeCriteria *retCriteria);

	private:
		enum Flags
			{
			INVALID_COMPARE = -1000,
			};

		DWORD m_dwTypeSet;
		TArray<CString> m_sRequire;
		TArray<CString> m_sExclude;
		TArray<CString> m_sRequireSpecial;
		TArray<CString> m_sExcludeSpecial;

		int m_iGreaterThanLevel;
		int m_iLessThanLevel;

		bool m_bIncludeVirtual;
        bool m_bStructuresOnly;
	};

//	CDesignType

class CDesignType
	{
	public:
		enum ECachedHandlers
			{
			evtCanInstallItem			= 0,
			evtCanRemoveItem			= 1,
			evtOnDestroyCheck			= 2,
			evtOnGlobalTypesInit		= 3,
			evtOnObjDestroyed			= 4,
			evtOnSystemObjAttacked		= 5,
			evtOnSystemStarted			= 6,
			evtOnSystemStopped			= 7,
			evtOnSystemWeaponFire		= 8,
			evtOnUpdate					= 9,

			evtCount					= 10,
			};

        struct SMapDescriptionCtx
            {
            SMapDescriptionCtx (void) :
                    bShowDestroyed(false),
                    bEnemy(false),
                    bFriend(false)
                { }

            bool bShowDestroyed;
            bool bEnemy;
            bool bFriend;
            };

		struct SStats
			{
			size_t dwGraphicsMemory = 0;
			size_t dwWreckGraphicsMemory = 0;
			};

		static ALERROR CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, CDesignType **retpType);

		ALERROR BindDesign (SDesignLoadCtx &Ctx);
		ALERROR ComposeLoadError (SDesignLoadCtx &Ctx, const CString &sError) const;
		inline ALERROR FinishBindDesign (SDesignLoadCtx &Ctx) { return OnFinishBindDesign(Ctx); }
		inline CUniverse &GetUniverse (void) const { return *g_pUniverse; }
		ALERROR InitFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, bool bIsOverride = false);
		bool IsIncluded (DWORD dwAPIVersion, const TArray<DWORD> &ExtensionsIncluded) const;
		bool MatchesCriteria (const CDesignTypeCriteria &Criteria);
		ALERROR PrepareBindDesign (SDesignLoadCtx &Ctx);
		inline void PrepareReinit (void) { OnPrepareReinit(); }
		void ReadFromStream (SUniverseLoadCtx &Ctx);
		void Reinit (void);
		inline void UnbindDesign (void) { m_pInheritFrom = NULL; OnUnbindDesign(); }
		void WriteToStream (IWriteStream *pStream);

		inline void AddExternals (TArray<CString> *retExternals) { OnAddExternals(retExternals); }
		void AddTypesUsed (TSortMap<DWORD, bool> *retTypesUsed);
		static CDesignType *AsType (CDesignType *pType) { return pType; }
		inline void ClearMark (void) { OnClearMark(); }
		bool FindCustomProperty (const CString &sProperty, ICCItemPtr &pResult, EPropertyType *retiType = NULL, bool bNoInheritance = false) const;
		inline CEffectCreator *FindEffectCreatorInType (const CString &sUNID) { return OnFindEffectCreator(sUNID); }
		bool FindEventHandler (const CString &sEvent, SEventHandlerDesc *retEvent = NULL) const;
		inline bool FindEventHandler (ECachedHandlers iEvent, SEventHandlerDesc *retEvent = NULL) const 
			{
			if (!m_pExtra || !m_pExtra->EventsCache[iEvent].pCode) 
				return false;
				
			if (retEvent) *retEvent = m_pExtra->EventsCache[iEvent]; 
			return true;
			}

		bool FindStaticData (const CString &sAttrib, ICCItemPtr &pData) const;
		void FireCustomEvent (const CString &sEvent, ECodeChainEvents iEvent = eventNone, ICCItem *pData = NULL, ICCItem **retpResult = NULL);
		bool FireGetCreatePos (CSpaceObject *pBase, CSpaceObject *pTarget, CSpaceObject **retpGate, CVector *retvPos);
		void FireGetGlobalAchievements (CGameStats &Stats);
		bool FireGetGlobalDockScreen (const SEventHandlerDesc &Event, const CSpaceObject *pObj, CDockScreenSys::SSelector &Selector) const;
		bool FireGetGlobalPlayerPriceAdj (const SEventHandlerDesc &Event, STradeServiceCtx &ServiceCtx, ICCItem *pData, int *retiPriceAdj);
		int FireGetGlobalResurrectPotential (void);
		void FireObjCustomEvent (const CString &sEvent, CSpaceObject *pObj, ICCItem *pData = NULL, ICCItem **retpResult = NULL);
		ALERROR FireOnGlobalDockPaneInit (const SEventHandlerDesc &Event, void *pScreen, DWORD dwScreenUNID, const CString &sScreen, const CString &sScreenName, const CString &sPane, ICCItem *pData, CString *retsError);
		void FireOnGlobalEndDiagnostics (const SEventHandlerDesc &Event);
		void FireOnGlobalIntroCommand (const SEventHandlerDesc &Event, const CString &sCommand);
		void FireOnGlobalIntroStarted (const SEventHandlerDesc &Event);
		void FireOnGlobalMarkImages (const SEventHandlerDesc &Event);
		void FireOnGlobalObjDestroyed (const SEventHandlerDesc &Event, SDestroyCtx &Ctx);
		bool FireOnGlobalObjGateCheck (const SEventHandlerDesc &Event, CSpaceObject *pObj, CTopologyNode *pDestNode, const CString &sDestEntryPoint, CSpaceObject *pGateObj);
		void FireOnGlobalPlayerBoughtItem (const SEventHandlerDesc &Event, CSpaceObject *pSellerObj, const CItem &Item, const CCurrencyAndValue &Price);
		ALERROR FireOnGlobalPlayerChangedShips (CSpaceObject *pOldShip, CString *retsError = NULL);
		ALERROR FireOnGlobalPlayerEnteredSystem (CString *retsError = NULL);
		ALERROR FireOnGlobalPlayerLeftSystem (CString *retsError = NULL);
		void FireOnGlobalPlayerSoldItem (const SEventHandlerDesc &Event, CSpaceObject *pBuyerObj, const CItem &Item, const CCurrencyAndValue &Price);
		ALERROR FireOnGlobalResurrect (CString *retsError = NULL);
		void FireOnGlobalStartDiagnostics (const SEventHandlerDesc &Event);
		void FireOnGlobalSystemDiagnostics (const SEventHandlerDesc &Event);
		ALERROR FireOnGlobalSystemCreated (SSystemCreateCtx &SysCreateCtx, CString *retsError = NULL);
		void FireOnGlobalSystemStarted (const SEventHandlerDesc &Event, DWORD dwElapsedTime);
		void FireOnGlobalSystemStopped (const SEventHandlerDesc &Event);
		ALERROR FireOnGlobalTopologyCreated (CString *retsError = NULL);
		ALERROR FireOnGlobalTypesInit (SDesignLoadCtx &Ctx);
		ALERROR FireOnGlobalUniverseCreated (const SEventHandlerDesc &Event);
		ALERROR FireOnGlobalUniverseLoad (const SEventHandlerDesc &Event);
		ALERROR FireOnGlobalUniverseSave (const SEventHandlerDesc &Event);
		void FireOnGlobalUpdate (const SEventHandlerDesc &Event);
		void FireOnRandomEncounter (CSpaceObject *pObj = NULL);
		size_t GetAllocMemoryUsage (void) const;
		inline DWORD GetAPIVersion (void) const { return m_dwVersion; }
		inline const CArmorMassDefinitions &GetArmorMassDefinitions (void) const { return (m_pExtra ? m_pExtra->ArmorDefinitions : CArmorMassDefinitions::Null); }
		inline const CString &GetAttributes (void) const { return m_sAttributes; }
		inline CString GetDataField (const CString &sField) const { CString sValue; FindDataField(sField, &sValue); return sValue; }
		inline int GetDataFieldInteger (const CString &sField) { CString sValue; if (FindDataField(sField, &sValue)) return strToInt(sValue, 0, NULL); else return 0; }
		inline const CDisplayAttributeDefinitions &GetDisplayAttributes (void) const { return (m_pExtra ? m_pExtra->DisplayAttribs : CDisplayAttributeDefinitions::Null); }
		CString GetEntityName (void) const;
		ICCItem *GetEventHandler (const CString &sEvent) const;
		void GetEventHandlers (const CEventHandler **retHandlers, TSortMap<CString, SEventHandlerDesc> *retInheritedHandlers);
		CExtension *GetExtension (void) const { return m_pExtension; }
		ICCItemPtr GetGlobalData (const CString &sAttrib) const;
		inline CDesignType *GetInheritFrom (void) const { return m_pInheritFrom; }
		inline DWORD GetInheritFromUNID (void) const { return m_dwInheritFrom; }
		CXMLElement *GetLocalScreens (void) const;
        CString GetMapDescription (SMapDescriptionCtx &Ctx) const;
		CLanguageDataBlock GetMergedLanguageBlock (void) const;
		CString GetNounPhrase (DWORD dwFlags = 0) const;
		ICCItemPtr GetProperty (CCodeChainCtx &Ctx, const CString &sProperty, EPropertyType *retiType = NULL) const;
		int GetPropertyInteger (const CString &sProperty);
		CString GetPropertyString (const CString &sProperty);
		CXMLElement *GetScreen (const CString &sUNID);
		ICCItemPtr GetStaticData (const CString &sAttrib) const;
		void GetStats (SStats &Stats) const;
		CString GetTypeClassName (void) const;
		inline DWORD GetUNID (void) const { return m_dwUNID; }
		inline CXMLElement *GetXMLElement (void) const { return m_pXML; }
		TSortMap<DWORD, DWORD> GetXMLMergeFlags (void) const;
		bool HasAttribute (const CString &sAttrib) const;
		inline bool HasCustomMapDescLanguage (void) const { return (m_fHasCustomMapDescLang ? true : false); }
		inline bool HasEvents (void) const { return !m_Events.IsEmpty() || (m_pInheritFrom && m_pInheritFrom->HasEvents()); }
		bool HasLanguageBlock (void) const;
		bool HasLanguageEntry (const CString &sID) const;
		inline bool HasLiteralAttribute (const CString &sAttrib) const { return ::HasModifier(m_sAttributes, sAttrib); }
		bool HasSpecialAttribute (const CString &sAttrib) const;
        inline ICCItemPtr IncGlobalData (const CString &sAttrib, ICCItem *pValue = NULL) { return SetExtra()->GlobalData.IncData(sAttrib, pValue); }
		ICCItemPtr IncTypeProperty (const CString &sProperty, ICCItem *pValue);
		bool InheritsFrom (DWORD dwUNID) const;
		void InitCachedEvents (int iCount, char **pszEvents, SEventHandlerDesc *retEvents);
		void InitItemData (CItem &Item) const;
		void InitObjectData (CSpaceObject &Obj, CAttributeDataBlock &Data) const;
		inline bool IsMerged (void) const { return m_bIsMerged; }
		inline bool IsModification (void) const { return m_bIsModification; }
		inline bool IsOptional (void) const { return (m_dwObsoleteVersion > 0) || (m_dwMinVersion > 0) || (m_pExtra && (m_pExtra->Excludes.GetCount() > 0 || m_pExtra->Extends.GetCount() > 0)); }
		inline void MarkImages (void) { OnMarkImages(); }
		inline void SetGlobalData (const CString &sAttrib, ICCItem *pData) { SetExtra()->GlobalData.SetData(sAttrib, pData); }
		inline void SetInheritFrom (CDesignType *pType) { m_pInheritFrom = pType; }
		inline void SetMerged (bool bValue = true) { m_bIsMerged = true; }
		inline void SetModification (bool bValue = true) { m_bIsModification = true; }
		bool SetTypeProperty (const CString &sProperty, ICCItem *pValue);
		inline void SetUNID (DWORD dwUNID) { m_dwUNID = dwUNID; }
		inline void SetXMLElement (CXMLElement *pDesc) { m_pXML = pDesc; }
		inline void Sweep (void) { OnSweep(); }
		inline void TopologyInitialized (void) { OnTopologyInitialized(); }
		bool Translate (const CString &sID, ICCItem *pData, ICCItemPtr &retResult) const;
		bool Translate (CSpaceObject *pObj, const CString &sID, ICCItem *pData, ICCItemPtr &retResult) const;
		bool TranslateText (const CString &sID, ICCItem *pData, CString *retsText) const;
		bool TranslateText (CSpaceObject *pObj, const CString &sID, ICCItem *pData, CString *retsText) const;
		bool TranslateText (const CItem &Item, const CString &sID, ICCItem *pData, CString *retsText) const;

		static CString GetTypeChar (DesignTypes iType);

		//	CDesignType overrides

		virtual void Delete (void) { delete this; }
		virtual bool FindDataField (const CString &sField, CString *retsValue) const;
		virtual CCommunicationsHandler *GetCommsHandler (void) { return NULL; }
		virtual const CEconomyType *GetEconomyType (void) const;
		virtual CCurrencyAndValue GetTradePrice (CSpaceObject *pObj = NULL, bool bActual = false) const;
		virtual CTradingDesc *GetTradingDesc (void) const { return NULL; }
        virtual const CCompositeImageDesc &GetTypeImage (void) const;
		virtual CString GetNamePattern (DWORD dwNounFormFlags = 0, DWORD *retdwFlags = NULL) const { if (retdwFlags) *retdwFlags = 0; return GetDataField(CONSTLIT("name")); }
		virtual int GetLevel (int *retiMinLevel = NULL, int *retiMaxLevel = NULL) const { if (retiMinLevel) *retiMinLevel = -1; if (retiMaxLevel) *retiMaxLevel = -1; return -1; }
		virtual DesignTypes GetType (void) const = 0;
		virtual bool IsVirtual (void) const { return false; }

	protected:
		ALERROR AddEventHandler (const CString &sEvent, const CString &sCode, CString *retsError = NULL) { return m_Events.AddEvent(sEvent, sCode, retsError); }
		ICCItem *FindBaseProperty (CCodeChainCtx &Ctx, const CString &sProperty) const;
		bool IsValidLoadXML (const CString &sTag);
		void ReadGlobalData (SUniverseLoadCtx &Ctx);
		void ReportEventError (const CString &sEvent, ICCItem *pError) const;

		//	CDesignType overrides
		virtual ~CDesignType (void);

		virtual void OnAccumulateStats (SStats &Stats) const { }
		virtual void OnAccumulateXMLMergeFlags (TSortMap<DWORD, DWORD> &MergeFlags) const { }
		virtual void OnAddExternals (TArray<CString> *retExternals) { }
		virtual void OnAddTypesUsed (TSortMap<DWORD, bool> *retTypesUsed) { }
		virtual ALERROR OnBindDesign (SDesignLoadCtx &Ctx) { return NOERROR; }
		virtual void OnClearMark (void) { }
		virtual ALERROR OnCreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc) { return NOERROR; }
		virtual CEffectCreator *OnFindEffectCreator (const CString &sUNID) { return NULL; }
		virtual bool OnFindEventHandler (const CString &sEvent, SEventHandlerDesc *retEvent = NULL) const { return false; }
		virtual ALERROR OnFinishBindDesign (SDesignLoadCtx &Ctx) { return NOERROR; }
		virtual CString OnGetMapDescriptionMain (SMapDescriptionCtx &Ctx) const { return NULL_STR; }
		virtual ICCItemPtr OnGetProperty (CCodeChainCtx &Ctx, const CString &sProperty) const { return NULL; }
		virtual bool OnHasSpecialAttribute (const CString &sAttrib) const { return sAttrib.IsBlank(); }
		virtual void OnInitObjectData (CSpaceObject &Obj, CAttributeDataBlock &Data) const { }
		virtual void OnMarkImages (void) { }
		virtual ALERROR OnPrepareBindDesign (SDesignLoadCtx &Ctx) { return NOERROR; }
		virtual void OnPrepareReinit (void) { }
		virtual void OnReadFromStream (SUniverseLoadCtx &Ctx) { }
		virtual void OnReinit (void) { }
		virtual void OnSweep (void) { }
		virtual void OnTopologyInitialized (void) { }
		virtual void OnUnbindDesign (void) { }
		virtual void OnWriteToStream (IWriteStream *pStream) { }

		static bool HasAllUNIDs (const TArray<DWORD> &DesiredUNIDs, const TArray<DWORD> &AvailableUNIDs);
		static bool HasAnyUNIDs (const TArray<DWORD> &DesiredUNIDs, const TArray<DWORD> &AvailableUNIDs);

	private:
		struct SExtra
			{
			TArray<DWORD> Extends;						//	Exclude this type from bind unless ALL of these extensions are present
			TArray<DWORD> Excludes;						//	Exclude this type from bind if ANY of these extensions are present
			CDesignPropertyDefinitions PropertyDefs;	//	Custom property definitions
			CAttributeDataBlock StaticData;				//	Static data
			CAttributeDataBlock GlobalData;				//	Global (variable) data
			CAttributeDataBlock InitGlobalData;			//	Initial global data
			CLanguageDataBlock Language;				//	Language data
			CXMLElement *pLocalScreens = NULL;			//	Local dock screen
			CArmorMassDefinitions ArmorDefinitions;		//	Armor mass definitions
			CDisplayAttributeDefinitions DisplayAttribs;	//	Display attribute definitions

			SEventHandlerDesc EventsCache[evtCount];	//	Cached events
			};

		void AddUniqueHandlers (TSortMap<CString, SEventHandlerDesc> *retInheritedHandlers);
		SEventHandlerDesc *GetInheritedCachedEvent (ECachedHandlers iEvent) const;
		inline bool HasCachedEvent (ECachedHandlers iEvent) const { return (m_pExtra && m_pExtra->EventsCache[iEvent].pCode != NULL); }
		void InitCachedEvents (void);
		bool InSelfReference (CDesignType *pType);
		bool TranslateVersion2 (CSpaceObject *pObj, const CString &sID, ICCItemPtr &retResult) const;
		SExtra *SetExtra (void) { if (!m_pExtra) m_pExtra.Set(new SExtra); return m_pExtra; }

		DWORD m_dwUNID = 0;
		CExtension *m_pExtension = NULL;				//	Extension
		DWORD m_dwVersion = 0;							//	Extension API version
		DWORD m_dwObsoleteVersion = 0;					//	API version on which this type is obsolete (0 = not obsolete)
		DWORD m_dwMinVersion = 0;						//	Exclude if using an API version lower than this (0 = always include)
		CXMLElement *m_pXML = NULL;						//	Optional XML for this type

		DWORD m_dwInheritFrom = 0;						//	Inherit from this type
		CDesignType *m_pInheritFrom = NULL;				//	Inherit from this type

		CString m_sAttributes;							//	Type attributes
		CEventHandler m_Events;							//	Event handlers

		TUniquePtr<SExtra> m_pExtra;					//	Extra type stuff (not all types need this, so we only
														//		allocate when necessary).

		bool m_bIsModification = false;					//	TRUE if this modifies the type it overrides
		bool m_bIsMerged = false;						//	TRUE if we created this type by merging (inheritance)

		DWORD m_fHasCustomMapDescLang:1;				//	Cached for efficiency
	};

template <class CLASS> class CDesignTypeRef
	{
	public:
		CDesignTypeRef (void) : m_pType(NULL), m_dwUNID(0) { }

		inline operator CLASS *() const { return m_pType; }
		inline CLASS * operator->() const { return m_pType; }

		ALERROR Bind (SDesignLoadCtx &Ctx)
			{
			if (m_dwUNID)
				{
				CDesignType *pBaseType = Ctx.GetUniverse().FindDesignType(m_dwUNID);
				if (pBaseType == NULL)
					{
					Ctx.sError = strPatternSubst(CONSTLIT("Unknown design type: %x"), m_dwUNID);
					return ERR_FAIL;
					}

				m_pType = CLASS::AsType(pBaseType);
				if (m_pType == NULL)
					{
					Ctx.sError = strPatternSubst(CONSTLIT("Specified type is invalid: %x"), m_dwUNID);
					return ERR_FAIL;
					}
				}

			return NOERROR;
			}

		inline DWORD GetUNID (void) const { return m_dwUNID; }
		ALERROR LoadUNID (SDesignLoadCtx &Ctx, const CString &sUNID, DWORD dwDefault = 0) { if (!sUNID.IsBlank()) return ::LoadUNID(Ctx, sUNID, &m_dwUNID); else { m_dwUNID = dwDefault; return NOERROR; } }

		void Set (CLASS *pType)
			{
			m_pType = pType;
			if (pType)
				m_dwUNID = pType->GetUNID();
			else
				m_dwUNID = 0;
			}

		void SetUNID (DWORD dwUNID)
			{
			if (dwUNID != m_dwUNID)
				{
				m_dwUNID = dwUNID;
				m_pType = NULL;
				}
			}

	protected:
		CLASS *m_pType;
		DWORD m_dwUNID;
	};

//	Design Type References ----------------------------------------------------

class CItemTypeRef : public CDesignTypeRef<CItemType>
	{
	public:
		CItemTypeRef (void) { }
		CItemTypeRef (CItemType *pType) { Set(pType); }
		inline ALERROR Bind (SDesignLoadCtx &Ctx) { return CDesignTypeRef<CItemType>::Bind(Ctx); }
		ALERROR Bind (SDesignLoadCtx &Ctx, ItemCategories iCategory);
	};

class CArmorClassRef : public CDesignTypeRef<CArmorClass>
	{
	public:
		ALERROR Bind (SDesignLoadCtx &Ctx);
	};

class CDeviceClassRef : public CDesignTypeRef<CDeviceClass>
	{
	public:
		ALERROR Bind (SDesignLoadCtx &Ctx);
		void Set (CDeviceClass *pDevice);
	};

class CWeaponFireDescRef : public CDesignTypeRef<CWeaponFireDesc>
	{
	public:
		ALERROR Bind (SDesignLoadCtx &Ctx);
	};

class CDockScreenTypeRef
	{
	public:
		CDockScreenTypeRef (void) : m_pType(NULL), m_pLocal(NULL) { }
		CDockScreenTypeRef (const CString &sUNID) : m_sUNID(sUNID), m_pType(NULL), m_pLocal(NULL) { }

		inline operator CDockScreenType *() const { return m_pType; }
		inline CDockScreenType * operator->() const { return m_pType; }

		ALERROR Bind (SDesignLoadCtx &Ctx, CXMLElement *pLocalScreens = NULL);
		CXMLElement *GetDesc (void) const;
		CDesignType *GetDockScreen (CDesignType *pRoot, CString *retsName) const;
		CString GetStringUNID (CDesignType *pRoot) const;
		inline const CString &GetUNID (void) const { return m_sUNID; }
		inline bool IsEmpty (void) const { return m_sUNID.IsBlank(); }
		void LoadUNID (SDesignLoadCtx &Ctx, const CString &sUNID);

		ALERROR Bind (CXMLElement *pLocalScreens = NULL);
		inline void LoadUNID (const CString &sUNID) { m_sUNID = sUNID; }

	private:
		CString m_sUNID;
		CDockScreenType *m_pType;
		CXMLElement *m_pLocal;
	};

class CEconomyTypeRef
	{
	public:
		CEconomyTypeRef (void) : m_pType(NULL) { }

		inline operator const CEconomyType *() const { return m_pType; }
		inline const CEconomyType * operator->() const { return m_pType; }

		ALERROR Bind (SDesignLoadCtx &Ctx);
		inline bool IsEmpty (void) const { return (m_sUNID.IsBlank() && m_pType == NULL); }
		void LoadUNID (const CString &sUNID) { m_sUNID = sUNID; }
		void Set (CUniverse &Universe, DWORD dwUNID);
		inline void Set (const CEconomyType *pType) { m_pType = pType; }

	private:
		CString m_sUNID;
		const CEconomyType *m_pType;
	};

class CEffectCreatorRef : public CDesignTypeRef<CEffectCreator>
	{
	public:
		CEffectCreatorRef (void) : m_pSingleton(NULL), m_bDelete(false) { }
		CEffectCreatorRef (const CEffectCreatorRef &Source);
		~CEffectCreatorRef (void);

		CEffectCreatorRef &operator= (const CEffectCreatorRef &Source);

		ALERROR Bind (SDesignLoadCtx &Ctx);
		ALERROR CreateBeamEffect (SDesignLoadCtx &Ctx, CXMLElement *pDesc, const CString &sUNID);
		ALERROR CreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, const CString &sUNID);
		IEffectPainter *CreatePainter (CCreatePainterCtx &Ctx, CEffectCreator *pDefaultCreator = NULL) const;
		inline bool IsEmpty (void) const { return (m_dwUNID == 0 && m_pType == NULL); }
		ALERROR LoadEffect (SDesignLoadCtx &Ctx, const CString &sUNID, CXMLElement *pDesc, const CString &sAttrib);
		ALERROR LoadSimpleEffect (SDesignLoadCtx &Ctx, const CString &sUNID, CXMLElement *pDesc);
		void Set (CEffectCreator *pEffect);

	private:
        CAttributeDataBlock m_Data;
		mutable IEffectPainter *m_pSingleton;
		bool m_bDelete;
	};

class CItemTableRef : public CDesignTypeRef<CItemTable>
	{
	};

class CGenericTypeRef : public CDesignTypeRef<CGenericType>
	{
	};

class COverlayTypeRef : public CDesignTypeRef<COverlayType>
	{
	};

class CShipClassRef : public CDesignTypeRef<CShipClass>
	{
	};

class CShipTableRef : public CDesignTypeRef<CShipTable>
	{
	};

class CSovereignRef : public CDesignTypeRef<CSovereign>
	{
	public:
		ALERROR Bind (SDesignLoadCtx &Ctx);
	};

class CStationTypeRef : public CDesignTypeRef<CStationType>
	{
	};

class CSystemMapRef : public CDesignTypeRef<CSystemMap>
	{
	};

//  Subsidiary Classes and Structures ------------------------------------------

#include "TSEPaint.h"
#include "TSECurrency.h"
#include "TSEImages.h"
#include "TSESounds.h"
#include "TSEComms.h"

//	Items ----------------------------------------------------------------------

#include "TSEWeaponFireDesc.h"
#include "TSEItemEnhancements.h"
#include "TSEItems.h"

//	Effect Support Structures --------------------------------------------------

#include "TSEParticleSystem.h"
#include "TSEEffects.h"

//	Generic Type ---------------------------------------------------------------

class CGenericType : public CDesignType
	{
	public:
		//	CDesignType overrides

		static CGenericType *AsType (CDesignType *pType) { return ((pType && pType->GetType() == designGenericType) ? (CGenericType *)pType : NULL); }
		virtual CCommunicationsHandler *GetCommsHandler (void) override { return m_Comms.GetCommsHandler(GetInheritFrom()); }
		virtual DesignTypes GetType (void) const override { return designGenericType; }

	protected:

		virtual void OnAccumulateXMLMergeFlags (TSortMap<DWORD, DWORD> &MergeFlags) const override;
		virtual ALERROR OnCreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc) override;
		virtual void OnUnbindDesign (void) override;

	private:
		CCommunicationsStack m_Comms;
	};

//	Topology Descriptors -------------------------------------------------------
//
//	Defines CTopologyNode::SCriteria, which is needed by station encounter
//	definitions.

#include "TSETrade.h"
#include "TSETopology.h"

//	Ship Systems ---------------------------------------------------------------

#include "TSEShipEquipment.h"
#include "TSEArmor.h"
#include "TSEDevices.h"
#include "TSEShipSystems.h"

//	CItemType ------------------------------------------------------------------

#include "TSEItemType.h"

//	Ship Definitions

#include "TSEShipAI.h"
#include "TSEPlayerSettings.h"
#include "TSEShipCreator.h"
#include "TSEDocking.h"
#include "TSEOverlayType.h"

//	CShipClass -----------------------------------------------------------------

#include "TSEShipClass.h"

//	Station Definitions --------------------------------------------------------

#include "TSEObjectCriteria.h"
#include "TSEDockScreenType.h"
#include "TSEStationType.h"

//	Other Types ----------------------------------------------------------------

#include "TSESovereign.h"
#include "TSEEconomyType.h"
#include "TSEPower.h"
#include "TSESpaceEnvironment.h"
#include "TSEMissionType.h"

//	Adventures -----------------------------------------------------------------

#include "TSEGameRecords.h"
#include "TSEAdventureDesc.h"

//	Systems and System Maps ----------------------------------------------------

#include "TSESystemType.h"
#include "TSESystemMap.h"

//	Template Types -------------------------------------------------------------

class CTemplateType : public CDesignType
	{
	public:
		CTemplateType (void) { }
		virtual ~CTemplateType (void) { }

		//	CDesignType overrides
		static CTemplateType *AsType (CDesignType *pType) { return ((pType && pType->GetType() == designTemplateType) ? (CTemplateType *)pType : NULL); }
		virtual DesignTypes GetType (void) const override { return designTemplateType; }

	protected:
		//	CDesignType overrides
		virtual ALERROR OnCreateFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc) override { return NOERROR; }

	private:
	};

class CDesignList
	{
	public:
		CDesignList (void) : m_List(128) { }
		~CDesignList (void) { }

		inline void AddEntry (CDesignType *pType) { m_List.Insert(pType); }
		void Delete (DWORD dwUNID);
		void DeleteAll (bool bFree = false);
		inline int GetCount (void) const { return m_List.GetCount(); }
		inline CDesignType *GetEntry (int iIndex) const { return m_List[iIndex]; }

	private:
		TArray<CDesignType *> m_List;
	};

class CDesignTable
	{
	public:
		CDesignTable (bool bFreeTypes = false) : m_bFreeTypes(bFreeTypes) { }
		~CDesignTable (void) { DeleteAll(); }

		ALERROR AddEntry (CDesignType *pEntry);
		ALERROR AddOrReplaceEntry (CDesignType *pEntry, CDesignType **retpOldEntry = NULL);
		void Delete (DWORD dwUNID);
		void DeleteAll (void);
		CDesignType *FindByUNID (DWORD dwUNID) const;
		inline int GetCount (void) const { return m_Table.GetCount(); }
		inline CDesignType *GetEntry (int iIndex) const { return m_Table.GetValue(iIndex); }
		ALERROR Merge (const CDesignTable &Source, CDesignList &Override, const TArray<DWORD> &ExtensionsIncluded, const TSortMap<DWORD, bool> &TypesUsed, DWORD dwAPIVersion);
		ALERROR Merge (const CDynamicDesignTable &Source, CDesignList *ioOverride = NULL);

	private:
		TSortMap<DWORD, CDesignType *> m_Table;
		bool m_bFreeTypes;
	};

enum EExtensionTypes
	{
	extUnknown,

	extBase,
	extAdventure,
	extLibrary,
	extExtension,
	};

enum EGameTypes
	{
	gameUnknown,

	gameAmerica,							//	CSC America
	gameTranscendence,						//	Transcendence
	};

class CExtension
	{
	public:
		enum ELoadStates
			{
			loadNone,

			loadEntities,					//	We've loaded the entities, but nothing else.
			loadAdventureDesc,				//	We've loaded adventure descriptors, but no other types
			loadComplete,					//	We've loaded all design types.
			};

		enum EFolderTypes
			{
			folderUnknown,

			folderBase,						//	Base folder (only for base XML)
			folderCollection,				//	Collection folder
			folderExtensions,				//	Extensions folder
			};

		struct SLibraryDesc
			{
			DWORD dwUNID = 0;				//	UNID of library that we use
			DWORD dwRelease = 0;			//	Release of library that we use
			bool bOptional = false;			//	Library is optional
			};

		struct SLoadOptions
			{
			SLoadOptions (void) :
					bNoResources(false),
					bNoDigestCheck(false)
				{ }

			bool bNoResources;
			bool bNoDigestCheck;
			};

		struct SStats
			{
			size_t dwTotalTypeMemory = 0;		//	Total memory used for all bound design types
			size_t dwBaseTypeMemory = 0;		//	Total memory used for base class of bound design types (CDesignType only)
			size_t dwTotalXMLMemory = 0;		//	Total memory used for XML structures
			size_t dwWreckGraphicsMemory = 0;	//	Memory used by cached wreck images
			size_t dwGraphicsMemory = 0;		//	Total memory used by graphics
			};

		CExtension (void);
		~CExtension (void);

		static ALERROR CreateBaseFile (SDesignLoadCtx &Ctx, EGameTypes iGame, CXMLElement *pDesc, CExternalEntityTable *pEntities, CExtension **retpBase, TArray<CXMLElement *> *retEmbedded);
		static ALERROR CreateExtension (SDesignLoadCtx &Ctx, CXMLElement *pDesc, EFolderTypes iFolder, CExternalEntityTable *pEntities, CExtension **retpExtension);
		static ALERROR CreateExtensionStub (const CString &sFilespec, EFolderTypes iFolder, DWORD dwFlags, CExtension **retpExtension, CString *retsError);

		void AccumulateStats (SStats &Stats) const;
		bool CanExtend (CExtension *pAdventure) const;
		bool CanHaveAdventureDesc (void) const;
		void CleanUp (void);
		void CreateIcon (int cxWidth, int cyHeight, CG32bitImage **retpIcon) const;
		ALERROR ExecuteGlobals (SDesignLoadCtx &Ctx);
		bool IsLibraryInUse (DWORD dwUNID) const;
		inline CAdventureDesc *GetAdventureDesc (void) const { return m_pAdventureDesc; }
		inline DWORD GetAPIVersion (void) const { return m_dwAPIVersion; }
		inline DWORD GetAutoIncludeAPIVersion (void) const { return m_dwAutoIncludeAPIVersion; }
		CG32bitImage *GetCoverImage (void) const;
		inline const TArray<CString> &GetCredits (void) const { return m_Credits; }
		inline const CString &GetDisabledReason (void) const { return m_sDisabledReason; }
		inline CString GetDesc (void) { return (m_pAdventureDesc ? m_pAdventureDesc->GetDesc() : NULL_STR); }
		inline const CDesignTable &GetDesignTypes (void) { return m_DesignTypes; }
		inline const CIntegerIP &GetDigest (void) const { return m_Digest; }
		inline CExternalEntityTable *GetEntities (void) { return m_pEntities; }
		CString GetEntityName (DWORD dwUNID) const;
		inline const TArray<CString> &GetExternalResources (void) const { return m_Externals; }
		inline const CString &GetFilespec (void) const { return m_sFilespec; }
		inline EFolderTypes GetFolderType (void) const { return m_iFolderType; }
		inline const SLibraryDesc &GetLibrary (int iIndex) const { return m_Libraries[iIndex]; }
		inline int GetLibraryCount (void) const { return m_Libraries.GetCount(); }
		inline ELoadStates GetLoadState (void) const { return m_iLoadState; }
		inline DWORD GetMinExtensionAPIVersion (void) const { return m_dwMinExtensionAPIVersion; }
		inline const CTimeDate &GetModifiedTime (void) const { return m_ModifiedTime; }
		inline const CString &GetName (void) const { return m_sName; }
		inline CTopologyDescTable &GetTopology (void) { return m_Topology; }
		inline DWORD GetRelease (void) const { return m_dwRelease; }
		inline EExtensionTypes GetType (void) const { return m_iType; }
		inline CString GetTypeName (void) const { return GetTypeName(GetType()); }
		inline DWORD GetUNID (void) const { return m_dwUNID; }
		inline CUniverse &GetUniverse (void) const { return *g_pUniverse; }
		inline const CString &GetVersion (void) const { return m_sVersion; }
		size_t GetXMLMemoryUsage (void) const;
		inline bool HasIcon (void) const { CG32bitImage *pIcon = GetCoverImage(); return (pIcon && pIcon->GetWidth() > 0 && pIcon->GetHeight() > 0); }
		inline bool IsAutoInclude (void) const { return m_bAutoInclude; }
		inline bool IsDebugOnly (void) const { return m_bDebugOnly; }
		inline bool IsDisabled (void) const { return m_bDisabled; }
		inline bool IsHidden (void) const { return m_bHidden; }
		inline bool IsMarked (void) const { return m_bMarked; }
		inline bool IsOfficial (void) const { return ((m_dwUNID & 0xFF000000) < 0xA0000000); }
		inline bool IsPrivate (void) const { return m_bPrivate; }
		inline bool IsRegistered (void) const { return m_bRegistered; }
		inline bool IsRegistrationVerified (void) { return (m_bRegistered && m_bVerified); }
		ALERROR Load (ELoadStates iDesiredState, IXMLParserController *pResolver, const SLoadOptions &Options, CString *retsError);
		inline void SetDeleted (void) { m_bDeleted = true; }
		inline void SetDisabled (const CString &sReason) { if (!m_bDisabled) { m_sDisabledReason = sReason; m_bDisabled = true; } }
		inline void SetDigest (const CIntegerIP &Digest) { m_Digest = Digest; }
		inline void SetMarked (bool bMarked = true) { m_bMarked = bMarked; }
		inline void SetModifiedTime (const CTimeDate &Time) { m_ModifiedTime = Time; }
		inline void SetName (const CString &sName) { m_sName = sName; }
		inline void SetVerified (bool bVerified = true) { m_bVerified = bVerified; }
		void SweepImages (void);
		inline bool UsesCompatibilityLibrary (void) const { return m_bUsesCompatibilityLibrary; }
		inline bool UsesXML (void) const { return m_bUsesXML; }

		static ALERROR ComposeLoadError (SDesignLoadCtx &Ctx, CString *retsError);
		static void DebugDump (CExtension *pExtension, bool bFull = false);
		static CString GetTypeName (EExtensionTypes iType);

	private:
		struct SGlobalsEntry
			{
			CString sFilespec;
			ICCItem *pCode;
			};

		static ALERROR CreateExtensionFromRoot (const CString &sFilespec, CXMLElement *pDesc, EFolderTypes iFolder, CExternalEntityTable *pEntities, DWORD dwInheritAPIVersion, CExtension **retpExtension, CString *retsError);

		void AddEntityNames (CExternalEntityTable *pEntities, TSortMap<DWORD, CString> *retMap) const;
		void AddLibraryReference (SDesignLoadCtx &Ctx, DWORD dwUNID = 0, DWORD dwRelease = 0, bool bOptional = false);
		void AddDefaultLibraryReferences (SDesignLoadCtx &Ctx);
		void CleanUpXML (void);
		ALERROR LoadDesignElement (SDesignLoadCtx &Ctx, CXMLElement *pDesc);
		ALERROR LoadDesignType (SDesignLoadCtx &Ctx, CXMLElement *pDesc, CDesignType **retpType = NULL);
		ALERROR LoadGlobalsElement (SDesignLoadCtx &Ctx, CXMLElement *pDesc);
		ALERROR LoadLibraryElement (SDesignLoadCtx &Ctx, CXMLElement *pDesc);
		ALERROR LoadModuleContent (SDesignLoadCtx &Ctx, CXMLElement *pDesc);
		ALERROR LoadModuleElement (SDesignLoadCtx &Ctx, CXMLElement *pDesc);
		ALERROR LoadModulesElement (SDesignLoadCtx &Ctx, CXMLElement *pDesc);
		ALERROR LoadResourcesElement (SDesignLoadCtx &Ctx, CXMLElement *pDesc);
		ALERROR LoadSystemTypesElement (SDesignLoadCtx &Ctx, CXMLElement *pDesc);

		CString m_sFilespec;				//	Extension file
		DWORD m_dwUNID;						//	UNID of extension
		EGameTypes m_iGame;					//	Game that this extension belongs to
		EExtensionTypes m_iType;			//	Either adventure, extension, or base

		ELoadStates m_iLoadState;			//	Current load state
		EFolderTypes m_iFolderType;			//	Folder that extension came from
		CTimeDate m_ModifiedTime;			//	Timedate of extension file
		CIntegerIP m_Digest;				//	Digest (for registered files)
		DWORD m_dwAPIVersion;				//	Version of API that we're using
		CExternalEntityTable *m_pEntities;	//	Entities defined by this extension
		CString m_sDisabledReason;			//	Reason why extension is disabled

		CString m_sName;					//	Extension name
		DWORD m_dwRelease;					//	Release number
		CString m_sVersion;					//	User-visible version number
		DWORD m_dwCoverUNID;				//	UNID of cover image
		CDesignTable m_DesignTypes;			//	Design types defined by extension
		CTopologyDescTable m_Topology;		//	Topology defined by extension 
											//		(for backwards compatibility)
		TArray<SGlobalsEntry> m_Globals;	//	Globals
		TArray<CString> m_Credits;			//	List of names for credits

		TArray<SLibraryDesc> m_Libraries;	//	Extensions that we use.
		TArray<DWORD> m_Extends;			//	UNIDs that this extension extends
		DWORD m_dwMinExtensionAPIVersion;	//	Do not allow extensions older than this
		DWORD m_dwAutoIncludeAPIVersion;	//	Library adds compatibility to any
											//		extension at or below this
											//		API version.
		TArray<CString> m_Externals;		//	External resources

		CXMLElement *m_pRootXML;			//	Root XML representation (may be NULL)
		TSortMap<CString, CXMLElement *> m_ModuleXML;	//	XML for modules

		mutable CG32bitImage *m_pCoverImage;	//	Large cover image

		CAdventureDesc *m_pAdventureDesc;	//	If extAdventure, this is the descriptor

		mutable TSortMap<DWORD, CString> m_UNID2EntityName;

		bool m_bMarked;						//	Used by CExtensionCollection for various things
		bool m_bDebugOnly;					//	Only load in debug mode
		bool m_bRegistered;					//	UNID indicates this is a registered extension
		bool m_bVerified;					//	Signature and license verified
		bool m_bPrivate;					//	Do not show in stats
		bool m_bDisabled;					//	Disabled (for some reason)
		bool m_bDeleted;
		bool m_bAutoInclude;				//	Extension should always be included (if appropriate)
		bool m_bUsesXML;					//	Extension uses XML from other extensions
		bool m_bUsesCompatibilityLibrary;	//	Extension needs the compatibility library
		bool m_bHidden;						//	Available only for backwards compatibility
	};

class CExtensionCollection
	{
	public:
		enum Flags
			{
			//	Load

			FLAG_NO_RESOURCES =			0x00000001,	//	Do not load resources.
			FLAG_DEBUG_MODE =			0x00000002,	//	Game run with /debug
			FLAG_DESC_ONLY =			0x00000004,	//	Load adventure descs only
			FLAG_ERROR_ON_DISABLE =		0x00000008,	//	Return an error if an extension was loaded disabled
												//		(due to missing dependencies, etc.)
            FLAG_NO_COLLECTION =		0x00000010, //  Do not load collection
			FLAG_NO_COLLECTION_CHECK =	0x00000020,	//	Do not check signatures on collection
			FLAG_DIAGNOSTICS =			0x00000040,	//	Allow diagnostics extension to be loaded

			//	FindExtension

			FLAG_ADVENTURE_ONLY =		0x00000080,	//	Must be an adventure (not found otherwise)

			//	ComputeAvailableExtension

			FLAG_INCLUDE_AUTO =			0x00000100,	//	Include extensions that are automatic
			FLAG_AUTO_ONLY =			0x00000200,	//	Only include extensions that are automatic
			FLAG_ACCUMULATE =			0x00000400,	//	Add to result list
			FLAG_REGISTERED_ONLY =		0x00000800,	//	Only registered extensions

			//	ComputeBindOrder

			FLAG_FORCE_COMPATIBILITY_LIBRARY = 0x00001000,
			};

		struct SCollectionStatusOptions
			{
			int cxIconSize = 300;
			int cyIconSize = 150;
			};

		CExtensionCollection (void);
		~CExtensionCollection (void);

		inline void AddExtensionFolder (const CString &sFilespec) { m_ExtensionFolders.Insert(sFilespec); }
		void CleanUp (void);
		ALERROR ComputeAvailableAdventures (DWORD dwFlags, TArray<CExtension *> *retList, CString *retsError);
		ALERROR ComputeAvailableExtensions (CExtension *pAdventure, DWORD dwFlags, const TArray<DWORD> &Extensions, TArray<CExtension *> *retList, CString *retsError);
		ALERROR ComputeBindOrder (CExtension *pAdventure, const TArray<CExtension *> &DesiredExtensions, DWORD dwFlags, TArray<CExtension *> *retList, CString *retsError);
		void ComputeCoreLibraries (CExtension *pExtension, TArray<CExtension *> *retList);
		bool ComputeDownloads (const TArray<CMultiverseCatalogEntry> &Collection, TArray<CMultiverseCatalogEntry> &retNotFound);
		void DebugDump (void);
		bool FindAdventureFromDesc (DWORD dwUNID, DWORD dwFlags = 0, CExtension **retpExtension = NULL);
		bool FindBestExtension (DWORD dwUNID, DWORD dwRelease = 0, DWORD dwFlags = 0, CExtension **retpExtension = NULL);
		bool FindExtension (DWORD dwUNID, DWORD dwRelease, CExtension::EFolderTypes iFolder, CExtension **retpExtension = NULL);
		void FreeDeleted (void);
		CExtension *GetBase (void) const { return m_pBase; }
		CString GetEntityName (DWORD dwUNID);
		DWORD GetEntityValue (const CString &sName);
		CString GetExternalResourceFilespec (CExtension *pExtension, const CString &sFilename) const;
		EGameTypes GetGame (void) const { return m_iGame; }
		bool GetRequiredResources (TArray<CString> *retFilespecs);
		void InitEntityResolver (CExtension *pExtension, DWORD dwFlags, CEntityResolverList *retResolver);
		bool IsExtensionDisabledManually (DWORD dwUNID) const { return m_DisabledExtensions.Find(dwUNID); }
		bool IsRegisteredGame (CExtension *pAdventure, const TArray<CExtension *> &DesiredExtensions, DWORD dwFlags);
		ALERROR Load (const CString &sFilespec, const TSortMap<DWORD, bool> &DisabledExtensions, DWORD dwFlags, CString *retsError);
		inline bool LoadedInDebugMode (void) { return m_bLoadedInDebugMode; }
		ALERROR LoadNewExtension (const CString &sFilespec, const CIntegerIP &FileDigest, CString *retsError);
		inline void SetCollectionFolder (const CString &sFilespec) { m_sCollectionFolder = sFilespec; }
		void SetExtensionEnabled (DWORD dwUNID, bool bEnabled);
		void SweepImages (void);
		void UpdateCollectionStatus (TArray<CMultiverseCatalogEntry> &Collection, const SCollectionStatusOptions &Options);
		void UpdateRegistrationStatus (const TArray<CMultiverseCatalogEntry *> &Collection);

		static int Compare (CExtension *pExt1, CExtension *pExt2, bool bDebugMode);

	private:
		ALERROR AddCompatibilityLibrary (CExtension *pAdventure, const TArray<CExtension *> &Extensions, DWORD dwFlags, const TArray<CExtension *> &Compatibility, TArray<CExtension *> *retList, CString *retsError);
		void AddOrReplace (CExtension *pExtension);
		ALERROR AddToBindList (CExtension *pExtension, DWORD dwFlags, const TArray<CExtension *> &Compatibility, TArray<CExtension *> *retList, CString *retsError);
		void ClearAllMarks (void);
		void ComputeCompatibilityLibraries (CExtension *pAdventure, DWORD dwFlags, TArray<CExtension *> *retList);
		ALERROR ComputeFilesToLoad (const CString &sFilespec, CExtension::EFolderTypes iFolder, TSortMap<CString, int> &List, CString *retsError);
		inline CUniverse &GetUniverse (void) const { return *g_pUniverse; }
		ALERROR LoadBaseFile (const CString &sFilespec, DWORD dwFlags, CString *retsError);
		ALERROR LoadEmbeddedExtension (SDesignLoadCtx &Ctx, CXMLElement *pDesc, CExtension **retpExtension);
		ALERROR LoadFile (const CString &sFilespec, CExtension::EFolderTypes iFolder, DWORD dwFlags, const CIntegerIP &CheckDigest, bool *retbReload, CString *retsError);
		ALERROR LoadFolderStubsOnly (const CString &sFilespec, CExtension::EFolderTypes iFolder, DWORD dwFlags, CString *retsError);
		bool ReloadDisabledExtensions (DWORD dwFlags);

		EGameTypes m_iGame;					//	Game
		CString m_sCollectionFolder;		//	Path to collection folder
		TArray<CString> m_ExtensionFolders;	//	Paths to extension folders
		TSortMap<DWORD, bool> m_DisabledExtensions;

		mutable CCriticalSection m_cs;		//	Protects modifications
		TArray<CExtension *> m_Extensions;	//	All loaded extensions
		bool m_bReloadNeeded;				//	If TRUE we need to reload our folders
		bool m_bLoadedInDebugMode;			//	If TRUE we loaded in debug mode

		TArray<CExtension *> m_Deleted;		//	Keep around until next bind

		//	Indices for easy access

		CExtension *m_pBase;				//	Base extension
		TSortMap<DWORD, TArray<CExtension *> > m_ByUNID;
		TSortMap<CString, CExtension *> m_ByFilespec;
	};

class CDynamicDesignTable
	{
	public:
		CDynamicDesignTable (void) { }
		~CDynamicDesignTable (void) { CleanUp(); }

		ALERROR DefineType (CExtension *pExtension, DWORD dwUNID, ICCItem *pSource, CDesignType **retpType = NULL, CString *retsError = NULL);
		ALERROR DefineType (CExtension *pExtension, DWORD dwUNID, CXMLElement *pSource, CDesignType **retpType = NULL, CString *retsError = NULL);
		void Delete (DWORD dwUNID);
		inline void DeleteAll (void) { CleanUp(); }
		CDesignType *FindType (DWORD dwUNID);
		inline int GetCount (void) const { return m_Table.GetCount(); }
		inline CDesignType *GetType (int iIndex) const { return m_Table[iIndex].pType; }
		int GetXMLMemoryUsage (void) const;
		void ReadFromStream (SUniverseLoadCtx &Ctx);
		void WriteToStream (IWriteStream *pStream);

	private:
		struct SEntry
			{
			SEntry (void) : pExtension(NULL),
					dwUNID(0),
					pSource(NULL),
					pType(NULL)
				{ }

			~SEntry (void)
				{
				if (pType)
					pType->Delete();

				if (pSource)
					delete pSource;
				}

			CExtension *pExtension;
			DWORD dwUNID;
			CString sSource;
			CXMLElement *pSource;
			CDesignType *pType;
			};

		void CleanUp (void);
		ALERROR Compile (SEntry *pEntry, CString *retsError = NULL);
		ALERROR CreateType (SEntry *pEntry, CXMLElement *pDesc, CDesignType **retpType, CString *retsError = NULL);
		inline const SEntry *GetEntry (int iIndex) const { return &m_Table[iIndex]; }

		TSortMap<DWORD, SEntry> m_Table;
	};

struct SDesignLoadCtx
	{
	inline DWORD GetAPIVersion (void) const { return (pExtension ? pExtension->GetAPIVersion() : API_VERSION); }
	inline CUniverse &GetUniverse (void) const { return *g_pUniverse; }

	//	Context
	CDesignCollection *pDesign = NULL;		//	Design collection
	CString sResDb;							//	ResourceDb filespec
	CResourceDb *pResDb = NULL;				//	Open ResourceDb object
	CString sFolder;						//	Folder context (used when loading images)
	CExtension *pExtension = NULL;			//	Extension
	CDesignType *pType = NULL;				//	Current type being loaded
	bool bLoadAdventureDesc = false;		//	If TRUE, we are loading an adventure desc only
	bool bLoadModule = false;				//	If TRUE, we are loading elements in a module
	DWORD dwInheritAPIVersion = 0;			//	APIVersion of parent (if base file)

	//	Options
	bool bBindAsNewGame = false;			//	If TRUE, then we are binding a new game
	bool bNoResources = false;
    bool bLoopImages = false;				//  If TRUE, image effects loop by default

	//	Bind Temporaries (valid only inside BindDesign)
	TSortMap<CString, CMissionType *> MissionArcRoots;

	//	Output
	CString sError;
	CString sErrorFilespec;					//	File in which error occurred.
	};

class CDesignCollection
	{
	public:
		enum ECachedHandlers
			{
			evtGetGlobalAchievements		= 0,
			evtGetGlobalDockScreen			= 1,
			evtGetGlobalPlayerPriceAdj		= 2,
			evtOnGlobalDockPaneInit			= 3,
			evtOnGlobalEndDiagnostics		= 4,

			evtOnGlobalIntroCommand			= 5,
			evtOnGlobalIntroStarted			= 6,

			evtOnGlobalMarkImages			= 7,
			
			evtOnGlobalObjDestroyed			= 8,
			evtOnGlobalObjGateCheck			= 9,

			evtOnGlobalPlayerBoughtItem		= 10,
			evtOnGlobalPlayerSoldItem		= 11,
			evtOnGlobalStartDiagnostics		= 12,

			evtOnGlobalSystemDiagnostics	= 13,
			evtOnGlobalSystemStarted		= 14,
			evtOnGlobalSystemStopped		= 15,

			evtOnGlobalUniverseCreated		= 16,
			evtOnGlobalUniverseLoad			= 17,
			evtOnGlobalUniverseSave			= 18,
			
			evtOnGlobalUpdate				= 19,

			evtCount						= 20
			};

		enum EFlags
			{
			//	GetImage flags
			FLAG_IMAGE_COPY =			0x00000001,
			FLAG_IMAGE_LOCK =			0x00000002,
			};

		struct SStats
			{
			TArray<CExtension *> Extensions;

			int iAllTypes = 0;					//	Number of bound types (including dynamic)
			int iDynamicTypes = 0;				//	Count of dynamtic types
			int iMergedTypes = 0;				//	Count of merged types
			int iItemTypes = 0;					//	Count of item types
			int iShipClasses = 0;				//	Count of ship classes
			int iStationTypes = 0;				//	Count of station types
			int iResourceTypes = 0;				//	Count of images, sounds, music
			int iDockScreens = 0;				//	Count of dock screen types
			int iMissionTypes = 0;				//	Count of mission types
			int iSovereigns = 0;				//	Count of sovereigns
			int iOverlayTypes = 0;				//	Count of overlays
			int iSystemTypes = 0;				//	Count of system types
			int iEffectTypes = 0;				//	Count of effects
			int iSupportTypes = 0;				//	Count of tables, generic types, etc.

			size_t dwTotalTypeMemory = 0;		//	Total memory used for all bound design types
			size_t dwBaseTypeMemory = 0;		//	Total memory used for base class of bound design types (CDesignType only)
			size_t dwTotalXMLMemory = 0;		//	Total memory used for XML structures (excluding dynamic)
			size_t dwWreckGraphicsMemory = 0;	//	Memory used by cached wreck images
			size_t dwGraphicsMemory = 0;		//	Total memory used by graphics
			};

		CDesignCollection (void);
		~CDesignCollection (void);

		ALERROR AddDynamicType (CExtension *pExtension, DWORD dwUNID, ICCItem *pSource, bool bNewGame, CString *retsError);
		ALERROR BindDesign (const TArray<CExtension *> &BindOrder, const TSortMap<DWORD, bool> &TypesUsed, DWORD dwAPIVersion, bool bNewGame, bool bNoResources, CString *retsError);
		void CleanUp (void);
		void ClearImageMarks (void);
		void DebugOutputExtensions (void) const;
		inline const CEconomyType *FindEconomyType (const CString &sID) { const CEconomyType **ppType = m_EconomyIndex.GetAt(sID); return (ppType ? *ppType : NULL); }
		inline const CDesignType *FindEntry (DWORD dwUNID) const { return m_AllTypes.FindByUNID(dwUNID); }
		inline CDesignType *FindEntry (DWORD dwUNID) { return m_AllTypes.FindByUNID(dwUNID); }
		CExtension *FindExtension (DWORD dwUNID) const;
		CXMLElement *FindSystemFragment (const CString &sName, CSystemTable **retpTable = NULL) const;
		void FireGetGlobalAchievements (CGameStats &Stats);

		static constexpr DWORD FLAG_NO_OVERRIDE = 0x00000001;
		bool FireGetGlobalDockScreen (const CSpaceObject *pObj, DWORD dwFlags, CDockScreenSys::SSelector *retSelector = NULL) const;

		bool FireGetGlobalPlayerPriceAdj (STradeServiceCtx &ServiceCtx, ICCItem *pData, int *retiPriceAdj);
		void FireOnGlobalEndDiagnostics (void);
		void FireOnGlobalIntroCommand (const CString &sCommand);
		void FireOnGlobalIntroStarted (void);
		void FireOnGlobalMarkImages (void);
		void FireOnGlobalObjDestroyed (SDestroyCtx &Ctx);
		bool FireOnGlobalObjGateCheck (CSpaceObject *pObj, CTopologyNode *pDestNode, const CString &sDestEntryPoint, CSpaceObject *pGateObj);
		void FireOnGlobalPaneInit (void *pScreen, CDesignType *pRoot, const CString &sScreen, const CString &sPane, ICCItem *pData);
		void FireOnGlobalPlayerBoughtItem (CSpaceObject *pSellerObj, const CItem &Item, const CCurrencyAndValue &Price);
		void FireOnGlobalPlayerChangedShips (CSpaceObject *pOldShip);
		void FireOnGlobalPlayerEnteredSystem (void);
		void FireOnGlobalPlayerLeftSystem (void);
		void FireOnGlobalPlayerSoldItem (CSpaceObject *pBuyerObj, const CItem &Item, const CCurrencyAndValue &Price);
		void FireOnGlobalStartDiagnostics (void);
		void FireOnGlobalSystemCreated (SSystemCreateCtx &SysCreateCtx);
		void FireOnGlobalSystemDiagnostics (void);
		void FireOnGlobalSystemStarted (DWORD dwElapsedTime);
		void FireOnGlobalSystemStopped (void);
		ALERROR FireOnGlobalTypesInit (SDesignLoadCtx &Ctx);
		void FireOnGlobalUniverseCreated (void);
		void FireOnGlobalUniverseLoad (void);
		void FireOnGlobalUniverseSave (void);
		void FireOnGlobalUpdate (int iTick);
		inline DWORD GetAdventureUNID (void) const { return (m_pAdventureExtension ? m_pAdventureExtension->GetUNID() : 0); }
		inline DWORD GetAPIVersion (void) const { return m_dwMinAPIVersion; }
		inline CArmorMassDefinitions &GetArmorMassDefinitions (void) { return m_ArmorDefinitions; }
		inline const CArmorMassDefinitions &GetArmorMassDefinitions (void) const { return m_ArmorDefinitions; }
		inline int GetCount (void) const { return m_AllTypes.GetCount(); }
		inline int GetCount (DesignTypes iType) const { return m_ByType[iType].GetCount(); }
		inline const CDisplayAttributeDefinitions &GetDisplayAttributes (void) const { return m_DisplayAttribs; }
		DWORD GetDynamicUNID (const CString &sName);
		void GetEnabledExtensions (TArray<CExtension *> *retExtensionList);
		inline CDesignType *GetEntry (int iIndex) const { return m_AllTypes.GetEntry(iIndex); }
		inline CDesignType *GetEntry (DesignTypes iType, int iIndex) const { return m_ByType[iType].GetEntry(iIndex); }
		inline CExtension *GetExtension (int iIndex) const { return m_BoundExtensions[iIndex]; }
		inline int GetExtensionCount (void) const { return m_BoundExtensions.GetCount(); }
		CG32bitImage *GetImage (DWORD dwUNID, DWORD dwFlags = 0);
		CString GetStartingNodeID (void);
		void GetStats (SStats &Result) const;
		CTopologyDescTable *GetTopologyDesc (void) const { return m_pTopology; }
		inline CUniverse &GetUniverse (void) const { return *g_pUniverse; }
		inline bool HasDynamicTypes (void) { return (m_DynamicTypes.GetCount() > 0); }
		bool IsAdventureExtensionBound (DWORD dwUNID);
		inline bool IsBindComplete (void) const { return !m_bInBindDesign; }
		bool IsRegisteredGame (void);
		void MarkGlobalImages (void);
		void NotifyTopologyInit (void);
		void ReadDynamicTypes (SUniverseLoadCtx &Ctx);
		void Reinit (void);
		void SweepImages (void);
		void WriteDynamicTypes (IWriteStream *pStream);

		//	Dock Screens

		inline CDockScreenType *FindDockScreen (DWORD dwUNID) { return CDockScreenType::AsType(FindEntry(dwUNID)); }
		CDesignType *ResolveDockScreen (CDesignType *pLocalScreen, const CString &sScreen, CString *retsScreenActual = NULL, bool *retbIsLocal = NULL);

	private:
		void CacheGlobalEvents (CDesignType *pType);
		ALERROR CreateTemplateTypes (SDesignLoadCtx &Ctx);
		ALERROR ResolveInheritingType (SDesignLoadCtx &Ctx, CDesignType *pType);
		ALERROR ResolveOverrides (SDesignLoadCtx &Ctx, const TSortMap<DWORD, bool> &TypesUsed);
		ALERROR ResolveTypeHierarchy (SDesignLoadCtx &Ctx);

		//	Loaded types. These are initialized at load-time and never change.

		CDesignTable m_Base;
		CTopologyDescTable m_BaseTopology;
		CExternalEntityTable m_BaseEntities;

		//	Cached data initialized at bind-time

		DWORD m_dwMinAPIVersion;
		TArray<CExtension *> m_BoundExtensions;
		CDesignTable m_AllTypes;
		CDesignList m_ByType[designCount];
		CDesignList m_OverrideTypes;
		CTopologyDescTable *m_pTopology;
		CExtension *m_pAdventureExtension;
		CAdventureDesc *m_pAdventureDesc;
		TSortMap<CString, const CEconomyType *> m_EconomyIndex;
		CArmorMassDefinitions m_ArmorDefinitions;
		CDisplayAttributeDefinitions m_DisplayAttribs;
		CGlobalEventCache *m_EventsCache[evtCount];

		//	Dynamic design types

		CDynamicDesignTable m_DynamicTypes;
		CDynamicDesignTable m_HierarchyTypes;
		CDynamicDesignTable m_CreatedTypes;
		TSortMap<CString, CDesignType *> m_DynamicUNIDs;

		//	State

		bool m_bInBindDesign;
	};

//	Utility functions

DWORD ExtensionVersionToInteger (DWORD dwVersion);
CString GenerateLevelFrequency (const CString &sLevelFrequency, int iCenterLevel);
CString GenerateRandomName (const CString &sList, const CString &sSubst);
CString GenerateRandomNameFromTemplate (const CString &sName, const CString &sSubst = NULL_STR);
CString GetRGBColor (CG32bitPixel rgbColor);
CString GetDamageName (DamageTypes iType);
CString GetDamageShortName (DamageTypes iType);
CString GetDamageType (DamageTypes iType);
int GetDiceCountFromAttribute(const CString &sValue);
int GetFrequency (const CString &sValue);
CString GetFrequencyName (FrequencyTypes iFrequency);
int GetFrequencyByLevel (const CString &sLevelFrequency, int iLevel);
CString GetItemCategoryID (ItemCategories iCategory);
CString GetItemCategoryName (ItemCategories iCategory);
bool IsConstantName (const CString &sList);
bool IsEnergyDamage (DamageTypes iType);
bool IsMatterDamage (DamageTypes iType);
ALERROR LoadDamageAdj (CXMLElement *pItem, const CString &sAttrib, int *retiAdj, int *retiCount = NULL);
DamageTypes LoadDamageTypeFromXML (const CString &sAttrib);
DWORD LoadExtensionVersion (const CString &sVersion);
DWORD LoadNameFlags (CXMLElement *pDesc);
CG32bitPixel LoadRGBColor (const CString &sString, CG32bitPixel rgbDefault = CG32bitPixel::Null());
ALERROR LoadUNID (SDesignLoadCtx &Ctx, const CString &sString, DWORD *retdwUNID);
bool SetFrequencyByLevel (CString &sLevelFrequency, int iLevel, int iFreq);

//	Inline implementations

inline bool DamageDesc::IsEnergyDamage (void) const { return ::IsEnergyDamage(m_iType); }
inline bool DamageDesc::IsMatterDamage (void) const { return ::IsMatterDamage(m_iType); }

inline void IEffectPainter::PlaySound (CSpaceObject *pSource) { if (!m_bNoSound) GetCreator()->PlaySound(pSource); }

inline CSystemMap *CTopologyNode::GetDisplayPos (int *retxPos, int *retyPos) const { if (retxPos) *retxPos = m_xPos; if (retyPos) *retyPos = m_yPos; return (m_pMap ? m_pMap->GetDisplayMap() : NULL); }
