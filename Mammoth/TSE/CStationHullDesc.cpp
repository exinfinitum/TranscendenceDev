//	CStationHullDesc.cpp
//
//	CStationHullDesc class
//	Copyright 2017 Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

#define HULL_TAG								CONSTLIT("Hull")

#define ARMOR_ID_ATTRIB							CONSTLIT("armorID")
#define ARMOR_LEVEL_ATTRIB						CONSTLIT("armorLevel")
#define CANNOT_BE_HIT_ATTRIB					CONSTLIT("cannotBeHit")
#define HIT_POINTS_ATTRIB						CONSTLIT("hitPoints")
#define HULL_TYPE_ATTRIB						CONSTLIT("hullType")
#define IMMUTABLE_ATTRIB						CONSTLIT("immutable")
#define MAX_HIT_POINTS_ATTRIB					CONSTLIT("maxHitPoints")
#define MAX_STRUCTURAL_HIT_POINTS_ATTRIB		CONSTLIT("maxStructuralHitPoints")
#define MULTI_HULL_ATTRIB						CONSTLIT("multiHull")
#define REGEN_ATTRIB							CONSTLIT("regen")
#define REPAIR_RATE_ATTRIB						CONSTLIT("repairRate")
#define STRUCTURAL_HIT_POINTS_ATTRIB			CONSTLIT("structuralHitPoints")

#define FIELD_ARMOR_CLASS						CONSTLIT("armorClass")
#define FIELD_ARMOR_LEVEL						CONSTLIT("armorLevel")
#define FIELD_HP								CONSTLIT("hp")
#define FIELD_REGEN								CONSTLIT("regen")					//	hp repaired per 180 ticks

#define HULL_TYPE_ASTEROID						CONSTLIT("asteroidHull")
#define HULL_TYPE_MULTIPLE						CONSTLIT("multiHull")
#define HULL_TYPE_SINGLE						CONSTLIT("singleHull")
#define HULL_TYPE_UNDERGROUND					CONSTLIT("undergroundHull")

void CStationHullDesc::AddTypesUsed (TSortMap<DWORD, bool> *retTypesUsed) const

//	AddTypesUsed
//
//	Adds to the list of types used by this object.

	{
	retTypesUsed->SetAt(m_pArmor.GetUNID(), true);
	}

ALERROR CStationHullDesc::Bind (SDesignLoadCtx &Ctx)

//	Bind
//
//	Bind design

	{
	ALERROR error;

	if (error = m_pArmor.Bind(Ctx, itemcatArmor))
		return error;

	if (m_pArmor && !m_pArmor->IsArmor())
		{
		Ctx.sError = strPatternSubst(CONSTLIT("Station armor not an armor item: %s"), m_pArmor->GetNounPhrase());
		return ERR_FAIL;
		}

	return NOERROR;
	}

int CStationHullDesc::CalcDamageEffectiveness (CSpaceObject *pAttacker, CInstalledDevice *pWeapon) const

//	CalcDamageEffectiveness
//
//	Returns the effectiveness of the given weapon on this station hull (0-100).

	{
	if (const CItem Item = GetArmorItem())
		{
		const CArmorItem ArmorItem = Item.AsArmorItem();
		return ArmorItem.GetDamageEffectiveness(pAttacker, pWeapon);
		}
	else
		return 100;
	}

Metric CStationHullDesc::CalcHitsToDestroy (int iLevel) const

//	CalcHitsToDestroy
//
//	Returns the average number of hits it would take to destroy the station with
//	weapons of the given level.

	{
	//	If station cannot be destroyed, then 0

	if (IsImmutable()
			|| (m_iMaxHitPoints == 0 && m_iMaxStructuralHP == 0))
		return 0.0;

	//	Compute the weapon that we want to use.

	int iDamageAdj = (m_pArmor ? m_pArmor->GetArmorClass()->GetDamageAdjForWeaponLevel(iLevel) : 100);
	Metric rWeaponDamage = (Metric)CWeaponClass::GetStdDamage(iLevel);

	//	If the station is multi-hulled, then assume WMD4 and adjust weapon damage.

	if (m_iType != hullSingle)
		{
		int iWMD = 34;
		rWeaponDamage = Max(1.0, (iWMD * rWeaponDamage / 100.0));
		}

	//	If we have hit points, then use that. Otherwise, we use structural 
	//	points.

	int iTotalHP;
	if (m_iMaxHitPoints > 0)
		{
		iTotalHP = m_iMaxHitPoints;

		//	Adjust weapon damage for station repairs. The standard fire rate is
		//	once per 8 ticks, and the repair rate is per 30 ticks.

		if (!m_Regen.IsEmpty())
			rWeaponDamage = Max(0.0, rWeaponDamage - (8.0 * m_Regen.GetHPPer180(STATION_REPAIR_FREQUENCY) / 180.0));
		}
	else
		iTotalHP = m_iMaxStructuralHP;

	//	Adjust weapon damage for armor

	rWeaponDamage = iDamageAdj * rWeaponDamage / 100.0;

	//	If weapon does no damage then we can never destroy the station

	if (rWeaponDamage <= 0.0)
		return 0.0;

	//	Otherwise, divide to figure out the number of hits to destroy.

	return Max(1.0, (Metric)iTotalHP / rWeaponDamage);
	}

bool CStationHullDesc::FindDataField (const CString &sField, CString *retsValue) const

//	FindDataField
//
//	Returns a data field.

	{
	if (strEquals(sField, FIELD_ARMOR_CLASS))
		{
		if (m_pArmor)
			*retsValue = m_pArmor->GetArmorClass()->GetShortName();
		else
			*retsValue = CONSTLIT("none");
		}
	else if (strEquals(sField, FIELD_ARMOR_LEVEL))
		{
		if (m_pArmor)
			*retsValue = strFromInt(m_pArmor->GetLevel());
		else
			*retsValue = NULL_STR;
		}
	else if (strEquals(sField, FIELD_HP))
		*retsValue = strFromInt(m_iHitPoints);
	else if (strEquals(sField, FIELD_REGEN))
		*retsValue = strFromInt(mathRound(m_Regen.GetHPPer180(STATION_REPAIR_FREQUENCY)));
	else
		return false;

	return true;
	}

CItem CStationHullDesc::GetArmorItem (void) const

//	GetArmorItem
//
//	Returns an item representing the armor type. NOTE: May be empty.

	{
	if (m_pArmor == NULL)
		return CItem();

	CItem Item(m_pArmor, 1);
	if (m_pArmor->IsScalable())
		Item.SetLevel(m_iArmorLevel);

	return Item;
	}

int CStationHullDesc::GetArmorLevel (void) const

//	GetArmorLevel
//
//	Returns the armor level

	{
	//	If we have scalable items, then use the armor level specified.

	if (m_iArmorLevel != 0)
		return m_iArmorLevel;

	//	Otherwise, use the armor class level

	else if (m_pArmor)
		return m_pArmor->GetLevel();

	//	In the rare case in which we have no armor, then level 1.

	else
		return 1;
	}

CString CStationHullDesc::GetID (EHullTypes iType)

//	GetID
//
//	Returns as an ID

	{
	switch (iType)
		{
		case hullSingle:
			return HULL_TYPE_SINGLE;

		case hullMultiple:
			return HULL_TYPE_MULTIPLE;

		case hullAsteroid:
			return HULL_TYPE_ASTEROID;

		case hullUnderground:
			return HULL_TYPE_UNDERGROUND;

		default:
			return NULL_STR;
		}
	}

ALERROR CStationHullDesc::InitFromXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc, bool bMultiHullDefault)

//	InitFromXML
//
//	Initializes from a <Hull> element.

	{
	ALERROR error;

	//	Load some basic flags.

	m_bImmutable = pDesc->GetAttributeBool(IMMUTABLE_ATTRIB);
	m_bCannotBeHit = pDesc->GetAttributeBool(CANNOT_BE_HIT_ATTRIB);

	//	Multi-hull defaults to TRUE if you use the <Hull> element.

	bool bMultiHull;
	CString sHullType;
	if (pDesc->FindAttribute(HULL_TYPE_ATTRIB, &sHullType))
		{
		m_iType = ParseHullType(sHullType);
		if (m_iType == hullUnknown)
			{
			Ctx.sError = strPatternSubst(CONSTLIT("Unknown hullType: %s"), sHullType);
			return ERR_FAIL;
			}
		}
	else if (pDesc->FindAttributeBool(MULTI_HULL_ATTRIB, &bMultiHull))
		m_iType = (bMultiHull ? hullMultiple : hullSingle);
	else
		m_iType = (bMultiHullDefault ? hullMultiple : hullSingle);

	//	Get hit points and max hit points

	if (error = m_pArmor.LoadUNID(Ctx, pDesc->GetAttribute(ARMOR_ID_ATTRIB)))
		return error;

	m_iArmorLevel = pDesc->GetAttributeIntegerBounded(ARMOR_LEVEL_ATTRIB, 1, MAX_ITEM_LEVEL, 0);
	m_iHitPoints = pDesc->GetAttributeIntegerBounded(HIT_POINTS_ATTRIB, 0, -1, -1);
	m_iMaxHitPoints = pDesc->GetAttributeIntegerBounded(MAX_HIT_POINTS_ATTRIB, 0, -1, -1);

	if (m_iHitPoints == -1 && m_iMaxHitPoints == -1)
		{
		m_iHitPoints = 0;
		m_iMaxHitPoints = 0;
		}
	else if (m_iHitPoints == -1)
		m_iHitPoints = m_iMaxHitPoints;
	else if (m_iMaxHitPoints == -1)
		m_iMaxHitPoints = m_iHitPoints;

	//	Structural hit points

	if (m_bImmutable)
		{
		m_iStructuralHP = 0;
		m_iMaxStructuralHP = 0;
		}
	else
		{
		m_iStructuralHP = pDesc->GetAttributeIntegerBounded(STRUCTURAL_HIT_POINTS_ATTRIB, 0, -1, -1);
		m_iMaxStructuralHP = pDesc->GetAttributeIntegerBounded(MAX_STRUCTURAL_HIT_POINTS_ATTRIB, 0, -1, -1);

		if (m_iStructuralHP == -1 && m_iMaxStructuralHP == -1)
			{
			m_iStructuralHP = 0;
			m_iMaxStructuralHP = 0;
			}
		else if (m_iStructuralHP == -1)
			m_iStructuralHP = m_iMaxStructuralHP;
		else if (m_iMaxStructuralHP == -1)
			m_iMaxStructuralHP = m_iStructuralHP;
		}

	//	Repair rate

	CString sRegen;
	int iRepairRate;
	if (pDesc->FindAttribute(REGEN_ATTRIB, &sRegen))
		{
		if (error = m_Regen.InitFromRegenString(Ctx, sRegen, STATION_REPAIR_FREQUENCY))
			return error;
		}
	else if (pDesc->FindAttributeInteger(REPAIR_RATE_ATTRIB, &iRepairRate) && iRepairRate > 0)
		m_Regen.InitFromRegen(6.0 * iRepairRate, STATION_REPAIR_FREQUENCY);

	//	Done

	return NOERROR;
	}

ALERROR CStationHullDesc::InitFromStationXML (SDesignLoadCtx &Ctx, CXMLElement *pDesc)

//	InitFromStationXML
//
//	Initializes from a <StationType> element (which may contain a child <Hull>
//	element.

	{
	//	If we have a <Hull> element, then load it.

	CXMLElement *pHullDesc = pDesc->GetContentElementByTag(HULL_TAG);
	if (pHullDesc)
		return InitFromXML(Ctx, pHullDesc);

	//	Otherwise, we load from the <StationType> element. For backwards 
	//	compatibility, multi-hull defaults to FALSE in this case.

	return InitFromXML(Ctx, pDesc, false);
	}

CStationHullDesc::EHullTypes CStationHullDesc::ParseHullType (const CString &sValue)

//	ParseHullType
//
//	Parses a hull type ID and returns it. If invalid, we return hullUnknown.

	{
	if (strEquals(sValue, HULL_TYPE_SINGLE))
		return hullSingle;
	else if (strEquals(sValue, HULL_TYPE_MULTIPLE))
		return hullMultiple;
	else if (strEquals(sValue, HULL_TYPE_ASTEROID))
		return hullAsteroid;
	else if (strEquals(sValue, HULL_TYPE_UNDERGROUND))
		return hullUnderground;
	else
		return hullUnknown;
	}

