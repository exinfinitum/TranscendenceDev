//	CShipPerformanceDesc.cpp
//
//	CShipPerformanceDesc class
//	Copyright (c) 2016 by Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

CShipPerformanceDesc CShipPerformanceDesc::m_Null;

void CShipPerformanceDesc::Init (SShipPerformanceCtx &Ctx)

//  Init
//
//  Initialize the performance descriptor from the parameters accumulated in
//  the context block.

    {
	//	Adjust speed if our armor is too heavy or too light

	Ctx.rArmorSpeedBonus = Ctx.pClass->GetHullDesc().GetArmorLimits().CalcArmorSpeedBonus(Ctx.Armor) * LIGHT_SPEED * 0.01;

	//	If this is positive (a bonus) then increase the speed limit.

	if (Ctx.rArmorSpeedBonus > 0.0)
		Ctx.rMaxSpeedLimit = Min(Ctx.rMaxSpeedLimit + Ctx.rArmorSpeedBonus, LIGHT_SPEED);

	//	Equipment installed

	m_Abilities = Ctx.Abilities;

    //  Initialize our maneuvering performance

	m_RotationDesc = Ctx.RotationDesc;
    m_IntegralRotationDesc.InitFromDesc(Ctx.RotationDesc);

	//	Initialize reactor desc

	m_ReactorDesc = Ctx.ReactorDesc;

    //  Compute our drive parameters. We start with the core stats

    m_DriveDesc = Ctx.DriveDesc;

    //  Adjust max speed.

	if (Ctx.rArmorSpeedBonus != 0.0)
		m_DriveDesc.AddMaxSpeed(Ctx.rArmorSpeedBonus);

	//	Apply the speed limit

	m_DriveDesc.SetMaxSpeed(Min(m_DriveDesc.GetMaxSpeed(), Ctx.rMaxSpeedLimit));

	//  If drive damaged, cut thrust in half

    if (Ctx.bDriveDamaged)
        m_DriveDesc.AdjThrust(0.5);

    //  If we're running at half speed...

    if (Ctx.bDriveDamaged)
        m_DriveDesc.AdjMaxSpeed(Min(Ctx.rOperatingSpeedAdj, 0.5));
	else if (Ctx.rOperatingSpeedAdj != 1.0)
        m_DriveDesc.AdjMaxSpeed(Ctx.rOperatingSpeedAdj);

    //  Cargo space

	Ctx.CargoDesc.ValidateCargoSpace(Ctx.iMaxCargoSpace);
    m_CargoDesc = Ctx.CargoDesc;

	//	Other flags

	m_iCyberDefenseAdj = Ctx.iCyberDefenseAdj;
	m_iPerceptionAdj = Ctx.iPerceptionAdj;
	m_iStealth = Ctx.iStealthFromArmor;
	m_fShieldInterference = Ctx.bShieldInterference;

	//	Done

	m_fInitialized = true;
    }
