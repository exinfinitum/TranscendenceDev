//	CLightningBundlePainter.cpp
//
//	CLightningBundlePainter class
//	Copyright (c) 2015 by Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"

CLightningBundlePainter::CLightningBundlePainter (int iBoltCount, CG32bitPixel rgbPrimaryColor, CG32bitPixel rgbSecondaryColor, WidthAdjArray &WidthAdjTop, WidthAdjArray &WidthAdjBottom,
	int iWidthCount, CRayEffectPainter::EWidthAdjTypes iWidthAdjType, CRayEffectPainter::EWidthAdjTypes iReshape) :
        m_iBoltCount(iBoltCount),
        m_rgbPrimaryColor(rgbPrimaryColor),
        m_rgbSecondaryColor(rgbSecondaryColor),
		m_iWidthCount(iWidthCount),
		m_iWidthAdjType(iWidthAdjType),
		m_iReshape(iReshape)

//  CLightningBundlePainter constructor

    {
    m_WidthAdjTop.TakeHandoff(WidthAdjTop);
    m_WidthAdjBottom.TakeHandoff(WidthAdjBottom);
    }

void CLightningBundlePainter::Draw (CG32bitImage &Dest, int x1, int y1, int x2, int y2, int iWidth)

//  Draw
//
//  Draw the bundle

    {
	DEBUG_TRY

	int i;

	//	Generate some intermediates

	Metric rChaos = 0.2;
	CVector vCenterLine = CVector(x2, y2) - CVector(x1, y1);
	Metric rLength;
	CVector vAxis = vCenterLine.Normal(&rLength);
	CVector vTangent = vAxis.Perpendicular();

	//	We fade at the end

	CG32bitPixel rgbTo = CG32bitPixel(m_rgbSecondaryColor, 0);

	//	Paint each bolt

	for (i = 0; i < m_iBoltCount; i++)
		{
		//	Each bolt consists of two segments. We pick a middle point along the ray 
		//	shape to divide the two segments.
		//
		//	Start by picking a value from 0.2 to 0.8

		Metric rMid = mathRandom(20, 80) / 100.0;

		//	Get the width of the shape at this point along the ray

		int iWidthMid = (int)(m_WidthAdjTop.GetCount() * rMid);
		Metric rWidthAdj = (mathRandom(1, 2) == 1 ? m_WidthAdjTop[iWidthMid] : -m_WidthAdjBottom[iWidthMid]) * iWidth * 0.5;

		//	Compute opacity at midpoint and then compute the midpoint color

		CG32bitPixel rgbMid = CG32bitPixel::Composite(m_rgbPrimaryColor, rgbTo, rMid);

		//	vMid is the midpoint, relative to the front of the ray

		CVector vMidLine = vAxis * rMid * rLength;
		CVector vMid = vMidLine + (vTangent * rWidthAdj);

		//	Convert to screen coordinates

		int xMid = x1 + (int)vMid.GetX();
		int yMid = y1 + (int)vMid.GetY();

		//	Draw the two bolts

		DrawLightning(Dest, x1, y1, xMid, yMid, m_rgbPrimaryColor, rgbMid, rChaos);
		DrawLightning(Dest, xMid, yMid, x2, y2, rgbMid, rgbTo, rChaos);
		}

	DEBUG_CATCH
    }

void CLightningBundlePainter::DrawWithOpenGL(CG32bitImage &Dest, int x1, int y1, int x2, int y2, int iRotDegrees, bool& bSuccess)
	{
	OpenGLMasterRenderQueue *pRenderQueue = Dest.GetMasterRenderQueue();
	if (!pRenderQueue)
		{
		bSuccess = false;
		return;
		}	

	int iDistX = x1 - x2;
	int iDistY = y1 - y2;
	int iCanvasHeight = Dest.GetHeight();
	int iCanvasWidth = Dest.GetWidth();

	float iDist = sqrt(float(iDistX * iDistX) + float(iDistY * iDistY));
	int iPosX = x1 - ((iDistX) / 2);
	int iPosY = y1 - ((iDistY) / 2);
	std::tuple<int, int, int> primaryColor(int(m_rgbSecondaryColor.GetRed()), int(m_rgbPrimaryColor.GetGreen()), int(m_rgbPrimaryColor.GetBlue()));
	std::tuple<int, int, int> secondaryColor(int(m_rgbSecondaryColor.GetRed()), int(m_rgbSecondaryColor.GetGreen()), int(m_rgbSecondaryColor.GetBlue()));
	float rSeed = mathRandom(20, 80) / 20.0f;
	
	for (int i = 0; i < m_iBoltCount; i++) {
		pRenderQueue->addLightningToEffectRenderQueue(iPosX, iPosY, int(iDist) * 2, m_iWidthCount * 2 * int(rSeed), iCanvasWidth, iCanvasHeight, float(iRotDegrees) * (float(PI) / 180.0f), m_iWidthAdjType, m_iReshape,
			primaryColor, secondaryColor, rSeed + float(i));
	}

	bSuccess = true;
	return;
	}
