//	C3DObjectPos.cpp
//
//	C3DObjectPos class
//	Copyright (c) 2017 by Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

#define POS_ANGLE_ATTRIB						CONSTLIT("posAngle")
#define POS_RADIUS_ATTRIB						CONSTLIT("posRadius")
#define POS_Z_ATTRIB							CONSTLIT("posZ")
#define X_ATTRIB								CONSTLIT("x")
#define Y_ATTRIB								CONSTLIT("y")
#define Z_ATTRIB								CONSTLIT("z")

C3DObjectPos::C3DObjectPos (const CVector &vPos, int iZ)

//	C3DObjectPos constructo

	{
	Metric rRadius;
	m_iPosAngle = VectorToPolar(vPos, &rRadius);
	m_iPosRadius = mathRound(rRadius / g_KlicksPerPixel);
	m_iPosZ = iZ;
	}

void C3DObjectPos::CalcCoord (int iScale, CVector *retvPos) const

//	CalcCoord
//
//	Calculates the coordinate

	{
	C3DConversion::CalcCoord(iScale, GetAngle(), GetRadius(), GetZ(), retvPos);
	}

void C3DObjectPos::CalcCoord (int iScale, int iRotation, CVector *retvPos) const

//	CalcCoord
//
//	Calculates the coordinate

	{
	C3DConversion::CalcCoord(iScale, AngleMod(iRotation + GetAngle()), GetRadius(), GetZ(), retvPos);
	}

bool C3DObjectPos::InitFromXY (int iScale, const CVector &vPos, int iZ)

//	InitFromXY
//
//	Reverse engineer a polar position based on XY.

	{
	if (iScale <= 0)
		{
		m_iPosAngle = 0;
		m_iPosRadius = 0;
		m_iPosZ = 0;
		return false;
		}

	Metric rAngle;
	Metric rRadius;
	C3DConversion::CalcPolar(iScale, vPos, iZ, &rAngle, &rRadius);

	m_iPosAngle = AngleMod(mathRound(mathRadiansToDegrees(rAngle)));
	m_iPosRadius = mathRound(rRadius);
	m_iPosZ = iZ;

	return true;
	}

bool C3DObjectPos::InitFromXML (CXMLElement *pDesc, DWORD dwFlags)

//	InitFromXML
//
//	Initializes from an XML element. We accept the following forms:
//
//	posAngle="nnn"	posRadius="nnn"	posZ="nnn"
//
//	OR
//
//	x="nnn" y="nnn" z="nnn"		-> use the 3D transformation

	{
	//	Initialize based on which of the formats we've got. If we have posAngle
	//	then we have polar coordinates.

	int iAngle;
	if (pDesc->FindAttributeInteger(POS_ANGLE_ATTRIB, &iAngle))
		{
		m_iPosAngle = AngleMod(iAngle);
		m_iPosRadius = pDesc->GetAttributeIntegerBounded(POS_RADIUS_ATTRIB, 0, -1);
		m_iPosZ = pDesc->GetAttributeInteger(POS_Z_ATTRIB);
		}

	//	If we don't support x,y coords, then we're done

	else if (dwFlags & FLAG_NO_XY)
		{
		m_iPosAngle = 0;
		m_iPosRadius = 0;
		m_iPosZ = 0;
		return false;
		}

	//	Otherwise, we expect Cartessian coordinates

	else
		{
		//	Get the position

		int x = pDesc->GetAttributeInteger(X_ATTRIB);
		int y = -pDesc->GetAttributeInteger(Y_ATTRIB);

		//	Convert to polar coordinates

		int iRadius;
		m_iPosAngle = IntVectorToPolar(x, y, &iRadius);
		m_iPosRadius = iRadius;

		m_iPosZ = pDesc->GetAttributeInteger(Z_ATTRIB);
		}

	return true;
	}

void C3DObjectPos::ReadFromStream (SLoadCtx &Ctx)

//	ReadFromStream
//
//	DWORD		m_iPosAngle, m_iPosRadius
//	DWORD		m_iPosZ, unused

	{
	DWORD dwLoad;
	Ctx.pStream->Read(dwLoad);
	m_iPosAngle = (int)LOWORD(dwLoad);
	m_iPosRadius = (int)HIWORD(dwLoad);

	Ctx.pStream->Read(dwLoad);
	m_iPosZ = (int)LOWORD(dwLoad);
	}

void C3DObjectPos::WriteToStream (IWriteStream &Stream) const

//	WriteToStream
//
//	DWORD		m_iPosAngle, m_iPosRadius
//	DWORD		m_iPosZ, unused

	{
	DWORD dwSave = MAKELONG(m_iPosAngle, m_iPosRadius);
	Stream.Write(dwSave);

	dwSave = MAKELONG(m_iPosZ, 0);
	Stream.Write(dwSave);
	}
