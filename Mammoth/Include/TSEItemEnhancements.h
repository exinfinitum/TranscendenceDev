//	TSEItemEnhancements.h
//
//	Defines classes and interfaces for items
//	Copyright (c) 2016 Kronosaur Productions, LLC. All Rights Reserved.

#pragma once

enum ItemEnhancementTypes
	{
	etNone =							0x0000,
	etBinaryEnhancement =				0x0001,
	etLoseEnhancement =					0x0002,	//	Lose enhancements

	etStrengthen =						0x0100,	//	+hp, data1 = %increase (10% increments)
	etRegenerate =						0x0200,	//	data1 = rate
	etReflect =							0x0300,	//	data2 = damage type reflected
	etRepairOnHit =						0x0400,	//	repair damage on hit, data2 = damage type of hit
	etResist =							0x0500,	//	-damage, data1 = %damage adj
	etResistEnergy =					0x0600,	//	-energy damage, data1 = %damage adj (90%, 80%, etc)
	etResistMatter =					0x0700,	//	-matter damage, data1 = %damage adj (90%, 80%, etc)
	etResistByLevel =					0x0800,	//	-damage, data1 = %damage adj, data2 = damage type
	etResistByDamage =					0x0900,	//	-damage, data1 = %damage adj, data2 = damage type
	etResistByDamage2 =					0x0a00,	//	-damage, data1 = %damage adj, data2 = damage type
	etSpecialDamage =					0x0b00,	//	Immunity to damage effects:
												//		data2 = 0: immune to radiation
												//		data2 = 1: immune to blinding
												//		... see SpecialDamageTypes
	etImmunityIonEffects =				0x0c00,	//	Immunity to ion effects (blinding, EMP, etc.)
												//		(if disadvantage, interferes with shields)
	etPhotoRegenerate =					0x0d00,	//	regen near sun
	etPhotoRecharge =					0x0e00,	//	refuel near sun
	etPowerEfficiency =					0x0f00,	//	power usage decrease, 01 = 90%/110%, 02 = 80%/120%
	etSpeedOld =						0x1000,	//	decrease cycle time
	etTurret =							0x1100,	//	weapon turret, data1 is angle
	etMultiShot =						0x1200,	//	multiple shots, data2 = count, data1 = %weakening
	etSpeed =							0x1300,	//	decrease cycle time
												//		A = adj (0-100)
												//			[if disavantage] adj by 5% starting at 100%.
												//
												//		B = min delay in ticks (do not decrease below this; 0 = no limit)
												//		C = max delay in ticks (do not increase above this; 0 = no limit)
	etConferSpecialDamage =				0x1400,	//	weapon gains special damage
												//		A = SpecialDamageTypes
												//		B = damage level
	etHPBonus =							0x1500,	//	+hp%, like etStrengthen
												//		X = %increase
	etHealerRegenerate =				0x1600,	//	Same as etRegenerate, but uses healer
	etResistHPBonus =					0x1700,	//	resist damage
												//		B = damage type
												//		X = %bonus (if disadvantage, this is decrease)
	etTracking =						0x1800,	//	weapon gains tracking
												//		X = maneuver rate
	etOmnidirectional =					0x1900,	//	weapon gain omnidirectional
												//		X = 0: omni; >0 <360 : swivel

	etData1Mask =						0x000f,	//	4-bits of data (generally for damage adj)
	etData2Mask =						0x00f0,	//	4-bits of data (generally for damage type)
	etTypeMask =						0x7f00,	//	Type
	etDisadvantage =					0x8000,	//	If set, this is a disadvantage

	etDataAMask =					0x000000ff,	//	8-bits of data
	etDataAMax =						   255,
	etDataBMask =					0x00ff0000,	//	8-bits of data
	etDataBMax =						   255,
	etDataCMask =					0xff000000,	//	8-bits of data
	etDataCMax =						   255,
	etDataXMask =					0xffff0000,	//	16-bits of data
	etDataXMax =						 65535,
	};

enum EnhanceItemStatus
	{
	eisOK,										//	Enhancement OK
	eisNoEffect,								//	Nothing happens
	eisAlreadyEnhanced,							//	Already has this exact enhancement
	eisWorse,									//	A disadvantage was made worse
	eisRepaired,								//	Disadvantage was repaired
	eisEnhancementRemoved,						//	Enhancement removed
	eisEnhancementReplaced,						//	Enhancement replaced by another enhancement
	eisBetter,									//	Enhancement made better
	};

enum ERegenTypes
	{
	regenNone						= 0,

	regenStandard					= 1,	//	Standard regeneration
	regenSolar						= 2,	//	Regenerate when near a star
	regenFromShields				= 3,	//	Regenerate by draining shields
	regenFromHealer					= 4,	//	Consume healer to regenerate
	regenFromCharges				= 5,	//	Consume charges to regenerate
	};

class CItemEnhancement
	{
	public:
		CItemEnhancement (void) : m_dwID(OBJID_NULL), m_dwMods(0), m_pEnhancer(NULL), m_iExpireTime(-1) { }
		CItemEnhancement (DWORD dwMods) : m_dwID(OBJID_NULL), m_dwMods(dwMods), m_pEnhancer(NULL), m_iExpireTime(-1) { }

		void AccumulateAttributes (CItemCtx &Ctx, TArray<SDisplayAttribute> *retList) const;
		inline DWORD AsDWORD (void) const { return m_dwMods; }
		EnhanceItemStatus Combine (const CItem &Item, CItemEnhancement Enhancement);
		int GetAbsorbAdj (const DamageDesc &Damage) const;
		int GetActivateRateAdj (int *retiMinDelay = NULL, int *retiMaxDelay = NULL) const;
		int GetDamageAdj (void) const;
		int GetDamageAdj (const DamageDesc &Damage) const;
		DamageTypes GetDamageType (void) const;
		inline int GetDataA (void) const { return (int)(DWORD)(m_dwMods & etDataAMask); }
		inline int GetDataB (void) const { return (int)(DWORD)((m_dwMods & etDataBMask) >> 16); }
		inline int GetDataC (void) const { return (int)(DWORD)((m_dwMods & etDataCMask) >> 24); }
		inline int GetDataX (void) const { return (int)(DWORD)((m_dwMods & etDataXMask) >> 16); }
		int GetEnhancedRate (int iRate) const;
		inline CItemType *GetEnhancementType (void) const { return m_pEnhancer; }
		inline DWORD GetExpireTime (void) const { return (DWORD)m_iExpireTime; }
		int GetFireArc (void) const;
		int GetHPAdj (void) const;
		int GetHPBonus (void) const;
		inline DWORD GetID (void) const { return m_dwID; }
		inline int GetLevel (void) const { return (int)(DWORD)(m_dwMods & etData1Mask); }
		inline int GetLevel2 (void) const { return (int)(DWORD)((m_dwMods & etData2Mask) >> 4); }
		int GetManeuverRate (void) const;
		inline DWORD GetModCode (void) const { return m_dwMods; }
		int GetPowerAdj (void) const;
		int GetReflectChance (DamageTypes iDamage) const;
		Metric GetRegen180 (CItemCtx &Ctx, int iTicksPerUpdate) const;
		int GetResistEnergyAdj (void) const { return (GetType() == etResistEnergy ? Level2DamageAdj(GetLevel(), IsDisadvantage()) : 100); }
		int GetResistHPBonus (void) const;
		int GetResistMatterAdj (void) const { return (GetType() == etResistMatter ? Level2DamageAdj(GetLevel(), IsDisadvantage()) : 100); }
		SpecialDamageTypes GetSpecialDamage (int *retiLevel = NULL) const;
		inline ItemEnhancementTypes GetType (void) const { return (ItemEnhancementTypes)(m_dwMods & etTypeMask); }
		int GetValueAdj (const CItem &Item) const;
		bool HasCustomDamageAdj (void) const;
		ALERROR InitFromDesc (ICCItem *pItem, CString *retsError);
		ALERROR InitFromDesc (const CString &sDesc, CString *retsError);
		ALERROR InitFromDesc (SDesignLoadCtx &Ctx, const CString &sDesc);
		inline bool IsBlindingImmune (void) const { return IsIonEffectImmune() || ((GetType() == etSpecialDamage) && GetLevel2() == specialBlinding && !IsDisadvantage()); }
		inline bool IsDecaying (void) const { return ((GetType() == etRegenerate) && IsDisadvantage()); }
		inline bool IsDeviceDamageImmune (void) const { return IsIonEffectImmune() || ((GetType() == etSpecialDamage) && GetLevel2() == specialDeviceDamage && !IsDisadvantage()); }
		inline bool IsDisadvantage (void) const { return ((m_dwMods & etDisadvantage) ? true : false); }
		inline bool IsDisintegrationImmune (void) const { return ((GetType() == etSpecialDamage) && GetLevel2() == specialDisintegration && !IsDisadvantage()); }
		inline bool IsEMPImmune (void) const { return IsIonEffectImmune() || ((GetType() == etSpecialDamage) && GetLevel2() == specialEMP && !IsDisadvantage()); }
		inline bool IsEmpty (void) const { return (m_dwMods == 0 && m_pEnhancer == NULL); }
		inline bool IsEnhancement (void) const { return (m_dwMods && !IsDisadvantage()); }
		bool IsEqual (const CItemEnhancement &Comp) const;
		inline bool IsNotEmpty (void) const { return !IsEmpty(); }
		inline bool IsPhotoRecharge (void) const { return ((GetType() == etPhotoRecharge) && !IsDisadvantage()); }
		inline bool IsPhotoRegenerating (void) const { return ((GetType() == etPhotoRegenerate) && !IsDisadvantage()); }
		inline bool IsRadiationImmune (void) const { return ((GetType() == etSpecialDamage) && GetLevel2() == 0 && !IsDisadvantage()); }
		inline bool IsReflective (void) const { return ((GetType() == etReflect) && !IsDisadvantage()); }
		bool IsReflective (const DamageDesc &Damage, int *retiReflectChance = NULL) const;
		inline bool IsRepairOnDamage (void) const { return ((GetType() == etRepairOnHit) && !IsDisadvantage()); }
		inline bool IsShatterImmune (void) const { return ((GetType() == etSpecialDamage) && GetLevel2() == specialShatter && !IsDisadvantage()); }
		inline bool IsShieldInterfering (void) const { return ((GetType() == etImmunityIonEffects) && IsDisadvantage()); }
		inline bool IsStacking (void) const { return (GetType() == etStrengthen && GetLevel() == 0); }
		void ReadFromStream (DWORD dwVersion, IReadStream *pStream);
		void ReadFromStream (SLoadCtx &Ctx);
		inline void SetEnhancementType (CItemType *pType) { m_pEnhancer = pType; }
		inline void SetExpireTime (int iTime) { m_iExpireTime = iTime; }
		inline void SetID (DWORD dwID) { m_dwID = dwID; }
		void SetModBonus (int iBonus);
		inline void SetModCode (DWORD dwMods) { m_dwMods = dwMods; }
		inline void SetModImmunity (SpecialDamageTypes iSpecial) { m_dwMods = Encode12(etSpecialDamage, 0, (int)iSpecial); }
		inline void SetModEfficiency (int iAdj) { m_dwMods = (iAdj > 0 ? EncodeABC(etPowerEfficiency, iAdj) : EncodeABC(etPowerEfficiency | etDisadvantage, -iAdj)); }
		inline void SetModOmnidirectional (int iFireArc) { m_dwMods = EncodeAX(etOmnidirectional, 0, Max(0, iFireArc)); }
		inline void SetModReflect (DamageTypes iDamageType) { m_dwMods = Encode12(etReflect, 0, (int)iDamageType); }
		inline void SetModResistDamage (DamageTypes iDamageType, int iAdj) { m_dwMods = Encode12(etResistByDamage | (iAdj > 100 ? etDisadvantage : 0), DamageAdj2Level(iAdj), (int)iDamageType); }
		inline void SetModResistDamageClass (DamageTypes iDamageType, int iAdj) { m_dwMods = Encode12(etResistByDamage2 | (iAdj > 100 ? etDisadvantage : 0), DamageAdj2Level(iAdj), (int)iDamageType); }
		inline void SetModResistDamageTier (DamageTypes iDamageType, int iAdj) { m_dwMods = Encode12(etResistByLevel | (iAdj > 100 ? etDisadvantage : 0), DamageAdj2Level(iAdj), (int)iDamageType); }
		inline void SetModResistEnergy (int iAdj) { m_dwMods = Encode12(etResistEnergy | (iAdj > 100 ? etDisadvantage : 0), DamageAdj2Level(iAdj)); }
		inline void SetModResistHPBonus (DamageTypes iDamageType, int iBonus) { m_dwMods = EncodeDX(etResistHPBonus | (iBonus < 0 ? etDisadvantage : 0), iDamageType, Absolute(iBonus)); }
		inline void SetModResistMatter (int iAdj) { m_dwMods = Encode12(etResistMatter | (iAdj > 100 ? etDisadvantage : 0), DamageAdj2Level(iAdj)); }
		void SetModSpecialDamage (SpecialDamageTypes iSpecial, int iLevel = 0);
		void SetModSpeed (int iAdj, int iMinDelay = 0, int iMaxDelay = 0);
		void SetModTracking (int iManeuverRate);
		bool UpdateArmorRegen (CItemCtx &ArmorCtx, SUpdateCtx &UpdateCtx, int iTick) const;
		void WriteToStream (IWriteStream *pStream) const;

		static const CItemEnhancement &Null (void) { return m_Null; }

	private:
		bool CalcRegen (CItemCtx &ItemCtx, int iTicksPerUpdate, CRegenDesc &retRegen, ERegenTypes *retiType = NULL) const;
		bool CalcNewHPBonus (const CItem &Item, const CItemEnhancement &NewEnhancement, int *retiNewBonus) const;
		bool CanBeCombinedWith (const CItemEnhancement &NewEnhancement) const;
		EnhanceItemStatus CombineAdvantageWithAdvantage (const CItem &Item, CItemEnhancement Enhancement);
		EnhanceItemStatus CombineAdvantageWithDisadvantage (const CItem &Item, CItemEnhancement Enhancement);
		EnhanceItemStatus CombineDisadvantageWithDisadvantage (const CItem &Item, CItemEnhancement Enhancement);
		EnhanceItemStatus CombineDisadvantageWithAdvantage (const CItem &Item, CItemEnhancement Enhancement);
		inline DamageTypes GetDamageTypeField (void) const { return (DamageTypes)(DWORD)((m_dwMods & etData2Mask) >> 4); }
		inline bool IsIonEffectImmune (void) const { return ((GetType() == etImmunityIonEffects) && !IsDisadvantage()); }

		static int DamageAdj2Level (int iDamageAdj);
		static DWORD EncodeABC (DWORD dwTypeCode, int A = 0, int B = 0, int C = 0);
		static DWORD EncodeAX (DWORD dwTypeCode, int A = 0, int X = 0);
		static DWORD EncodeDX (DWORD dwTypeCode, DamageTypes iDamageType, int X = 0);
		static DWORD Encode12 (DWORD dwTypeCode, int Data1 = 0, int Data2 = 0);
		static int Level2Bonus (int iLevel, bool bDisadvantage = false);
		static int Level2DamageAdj (int iLevel, bool bDisadvantage = false);

		DWORD m_dwID;							//	Global ID
		DWORD m_dwMods;							//	Mod code
		CItemType *m_pEnhancer;					//	Item that added this mod (may be NULL)
		int m_iExpireTime;						//	Universe tick when mod expires (-1 == no expiration)

		static CItemEnhancement m_Null;
	};

class CItemEnhancementStack
	{
	public:
		CItemEnhancementStack (void) :
				m_bCacheValid(false),
				m_dwRefCount(1)
			{ }

		void AccumulateAttributes (CItemCtx &Ctx, TArray<SDisplayAttribute> *retList) const;
		inline CItemEnhancementStack *AddRef (void) { m_dwRefCount++; return this; }
		int ApplyDamageAdj (const DamageDesc &Damage, int iDamageAdj) const;
		void ApplySpecialDamage (DamageDesc *pDamage) const;
		int CalcActivateDelay (CItemCtx &DeviceCtx) const;
		Metric CalcRegen180 (CItemCtx &ItemCtx, int iTicksPerUpdate) const;
		void Delete (void);
		int GetAbsorbAdj (const DamageDesc &Damage) const;
		int GetActivateDelayAdj (void) const;
		int GetBonus (void) const;
		inline int GetCount (void) const { return m_Stack.GetCount(); }
		const DamageDesc &GetDamage (void) const;
		int GetDamageAdj (const DamageDesc &Damage) const;
		int GetFireArc (void) const;
		int GetManeuverRate (void) const;
		int GetPowerAdj (void) const;
		int GetResistDamageAdj (DamageTypes iDamage) const;
		int GetResistEnergyAdj (void) const;
		int GetResistMatterAdj (void) const;
		void Insert (const CItemEnhancement &Mods);
		void InsertActivateAdj (int iAdj, int iMin, int iMax);
		void InsertHPBonus (int iBonus);
		inline bool IsEmpty (void) const { return (m_Stack.GetCount() == 0); }
		bool IsBlindingImmune (void) const;
		bool IsDecaying (void) const;
		bool IsDeviceDamageImmune (void) const;
		bool IsDisintegrationImmune (void) const;
		bool IsEMPImmune (void) const;
		bool IsPhotoRegenerating (void) const;
		bool IsPhotoRecharging (void) const;
		bool IsRadiationImmune (void) const;
		bool IsShatterImmune (void) const;
		bool IsShieldInterfering (void) const;
		inline bool IsTracking (void) const { return (GetManeuverRate() > 0); }
		bool ReflectsDamage (DamageTypes iDamage, int *retiChance = NULL) const;
		bool RepairOnDamage (DamageTypes iDamage) const;
		bool UpdateArmorRegen (CItemCtx &ArmorCtx, SUpdateCtx &UpdateCtx, int iTick) const;

		static TSharedPtr<CItemEnhancementStack> ReadFromStream (SLoadCtx &Ctx);
		static void WriteToStream (CItemEnhancementStack *pStack, IWriteStream *pStream);

	private:
		~CItemEnhancementStack (void) { }
		void CalcCache (void) const;

		TArray<CItemEnhancement> m_Stack;

		mutable bool m_bCacheValid;				//	If TRUE, these cache values are OK.
		mutable int m_iBonus;					//	Cached bonus
		mutable DamageDesc m_Damage;			//	Cached damage descriptor

		DWORD m_dwRefCount;
	};

class CRandomEnhancementGenerator
	{
	public:
		CRandomEnhancementGenerator (void) : m_iChance(0), m_pCode(NULL) { }
		~CRandomEnhancementGenerator (void);

		CRandomEnhancementGenerator &operator= (const CRandomEnhancementGenerator &Src);

		void EnhanceItem (CItem &Item) const;
		inline int GetChance (void) const { return m_iChance; }
		ALERROR InitFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc);
		bool IsVariant (void) const;

	private:
		int m_iChance;
		CItemEnhancement m_Mods;
		ICCItem *m_pCode;
	};

