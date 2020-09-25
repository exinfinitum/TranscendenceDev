//	PaintUtilities.cpp
//
//	Paint Utility classes

#include "PreComp.h"

const int DEFAULT_ICON_HEIGHT =				96;
const int DEFAULT_ICON_WIDTH =				96;

SPoint g_BlasterShape[8] = 
	{
		{    0,    0	},
		{   30,    6	},
		{   50,   20	},
		{   35,   50	},
		{    0,  100	},
		{  -35,   50	},
		{  -50,   20	},
		{  -30,    6	},
	};

void CreateBlasterShape (int iAngle, int iLength, int iWidth, SPoint *Poly)

//	CreateBlasterShape
//
//	Creates a blaster-shaped polygon

	{
	//	Define a transformation for this shape

	CXForm Trans(xformScale, ((Metric)iWidth)/100.0, ((Metric)iLength)/100.0);
	Trans = Trans * CXForm(xformRotate, iAngle + 270);

	//	Generate the points

	for (int i = 0; i < 8; i++)
		{
		Metric x, y;
		Trans.Transform(g_BlasterShape[i].x, g_BlasterShape[i].y, &x, &y);
		Poly[i].x = (int)x;
		Poly[i].y = (int)y;
		}
	}

void ComputeLightningPoints (int iCount, Metric *pxPoint, Metric *pyPoint, Metric rChaos)

//	ComputeLightningPoints
//
//	Computes points for lightning using a simple fractal algorithm. We assume
//	that pxPoint[0],pyPoint[0] is the starting point and pxPoint[iCount-1],pyPoint[iCount-1]
//	is the ending point.

	{
	ASSERT(iCount > 2);

	//	Half the delta

	Metric dx2 = (pxPoint[iCount-1] - pxPoint[0]) / 2;
	Metric dy2 = (pyPoint[iCount-1] - pyPoint[0]) / 2;

	//	Center point

	Metric xCenter = pxPoint[0] + dx2;
	Metric yCenter = pyPoint[0] + dy2;

	//	Fractal offset

	Metric rOffset = (mathRandom(-100, 100) / 100.0) * rChaos;

	//	Index of middle of array

	int iMiddle = iCount / 2;

	pxPoint[iMiddle] = xCenter + dy2 * rOffset;
	pyPoint[iMiddle] = yCenter - dx2 * rOffset;

	//	Recurse

	if (iMiddle > 1)
		ComputeLightningPoints(iMiddle+1, pxPoint, pyPoint, rChaos);

	if (iCount - iMiddle > 2)
		ComputeLightningPoints(iCount - iMiddle, pxPoint + iMiddle, pyPoint + iMiddle, rChaos);
	}

void ComputeLightningPoints (int iCount, CVector *pPoints, Metric rChaos)

//	ComputeLightningPoints
//
//	Computes points for lightning using a simple fractal algorithm. We assume
//	that pxPoint[0],pyPoint[0] is the starting point and pxPoint[iCount-1],pyPoint[iCount-1]
//	is the ending point.

	{
	if (iCount <= 2)
		return;

	//	Line

	CVector vLine = (pPoints[iCount - 1] - pPoints[0]);

	//	Half the line

	CVector vLineHalf = vLine / 2.0;

	//	Perpendicular half-line

	CVector vDelta = vLineHalf.Perpendicular();

	//	Center point

	CVector vCenter = pPoints[0] + vLineHalf;

	//	Perturb the line

	Metric rOffset = (mathRandom(-100, 100) / 100.0) * rChaos;

	int iMiddle = iCount / 2;
	pPoints[iMiddle] = vCenter + vDelta * rOffset;

	//	Recurse

	if (iMiddle > 1)
		ComputeLightningPoints(iMiddle+1, pPoints, rChaos);

	if (iCount - iMiddle > 2)
		ComputeLightningPoints(iCount - iMiddle, pPoints + iMiddle, rChaos);
	}

void DrawItemTypeIcon (CG32bitImage &Dest, int x, int y, const CItemType *pType, int cxSize, int cySize, bool bGray, bool bDisplayAsKnown)

//	DrawItemTypeIcon
//
//	Draws the item type icon at the given position

	{
	const CObjectImageArray &Image = pType->GetImage(bDisplayAsKnown);
	if (Image.IsLoaded())
		{
		RECT rcImage = Image.GetImageRect();

		if (cxSize <= 0 || cySize <= 0 || (cxSize == DEFAULT_ICON_WIDTH && cySize == DEFAULT_ICON_HEIGHT))
			{
			if (bGray)
				CGDraw::BltGray(Dest,
						x,
						y,
						Image.GetImage(NULL_STR),
						rcImage.left,
						rcImage.top,
						RectWidth(rcImage),
						RectHeight(rcImage),
						128);
			else
				Dest.Blt(rcImage.left,
						rcImage.top,
						RectWidth(rcImage),
						RectHeight(rcImage),
						255,
						Image.GetImage(NULL_STR),
						x,
						y);
			}
		else
			{
			if (bGray)
				CGDraw::BltTransformedGray(Dest,
						x + (cxSize / 2),
						y + (cySize / 2),
						(Metric)cxSize / (Metric)RectWidth(rcImage),
						(Metric)cySize / (Metric)RectHeight(rcImage),
						0.0,
						Image.GetImage(NULL_STR),
						rcImage.left,
						rcImage.top,
						RectWidth(rcImage),
						RectHeight(rcImage),
						128);
			else
				CGDraw::BltTransformed(Dest,
						x + (cxSize / 2),
						y + (cySize / 2),
						(Metric)cxSize / (Metric)RectWidth(rcImage),
						(Metric)cySize / (Metric)RectHeight(rcImage),
						0.0,
						Image.GetImage(NULL_STR),
						rcImage.left,
						rcImage.top,
						RectWidth(rcImage),
						RectHeight(rcImage));
			}
		}
	}

void DrawLightning (CG32bitImage &Dest,
					int xFrom, int yFrom,
					int xTo, int yTo,
					CG32bitPixel rgbFrom,
					CG32bitPixel rgbTo,
					Metric rChaos)

//	DrawLighting
//
//	Draw a lightning line, fading color from the start to the end.

	{

	OpenGLMasterRenderQueue* pRenderQueue = Dest.GetMasterRenderQueue();
	if ((pRenderQueue && (&(Dest) == pRenderQueue->getPointerToCanvas())))
	{
		int x2 = xTo;
		int y2 = yTo;
		int x1 = xFrom;
		int y1 = yFrom;
		int iDistX = x1 - x2;
		int iDistY = y1 - y2;
		float fRotRadians = -float(atan2(iDistY, iDistX));
		int iCanvasHeight = Dest.GetHeight();
		int iCanvasWidth = Dest.GetWidth();
		int iShape = rand() % 6;

		float iDist = sqrt(float(iDistX * iDistX) + float(iDistY * iDistY));
		int iPosX = x1 - ((iDistX) / 2);
		int iPosY = y1 - ((iDistY) / 2);
		std::tuple<int, int, int> primaryColor(int(rgbFrom.GetRed()), int(rgbFrom.GetGreen()), int(rgbFrom.GetBlue()));
		std::tuple<int, int, int> secondaryColor(int(rgbTo.GetRed()), int(rgbTo.GetGreen()), int(rgbTo.GetBlue()));
		float rSeed = mathRandom(20, 80) / 20.0f;
		float fWidth = (mathRandom(-100, 100) / 100.0f) * float(rChaos) * 4.0f;

		pRenderQueue->addLightningToEffectRenderQueue(iPosX, iPosY, int(iDist) * 2, int(iDist * fWidth), iCanvasWidth, iCanvasHeight, fRotRadians, iShape, iShape,
			primaryColor, secondaryColor, rSeed, OpenGLRenderLayer::blendMode::blendNormal);
		return;
	}

	const int MAX_LINE_SEGMENT = 3;
	const int MAX_POINTS = 100;

	int i;

	//	Figure out how many points. We do a quick heuristic to make sure we get
	//	enough detail.

	int iPoints = Min(Max(2, Max(Absolute(xFrom - xTo), Absolute(yFrom - yTo)) / MAX_LINE_SEGMENT), MAX_POINTS);

	//	Compute the lightning points

	CVector Points[MAX_POINTS];
	Points[0] = CVector(xFrom, yFrom);
	Points[iPoints - 1] = CVector(xTo, yTo);

	ComputeLightningPoints(iPoints, Points, rChaos);

	//	Draw the line segments

	for (i = 0; i < iPoints - 1; i++)
		{
		CG32bitPixel rgbColor = CG32bitPixel::Composite(rgbFrom, rgbTo, (Metric)i / iPoints);
		Dest.DrawLine((int)Points[i].GetX(), (int)Points[i].GetY(),
				(int)Points[i + 1].GetX(), (int)Points[i + 1].GetY(),
				1,
				rgbColor);
		}
	}

void DrawLightning (CG32bitImage &Dest,
					int xFrom, int yFrom,
					int xTo, int yTo,
					CG32bitPixel rgbColor,
					int iPoints,
					Metric rChaos)

//	DrawLightning
//
//	Draw a lightning line

	{

	OpenGLMasterRenderQueue* pRenderQueue = Dest.GetMasterRenderQueue();
	if ((pRenderQueue && (&(Dest) == pRenderQueue->getPointerToCanvas())))
		{
		int x2 = xTo;
		int y2 = yTo;
		int x1 = xFrom;
		int y1 = yFrom;
		int iDistX = x1 - x2;
		int iDistY = y1 - y2;
		float fRotRadians = -float(atan2(iDistY, iDistX));
		int iCanvasHeight = Dest.GetHeight();
		int iCanvasWidth = Dest.GetWidth();
		int iShape = rand() % 6;

		float iDist = sqrt(float(iDistX * iDistX) + float(iDistY * iDistY));
		int iPosX = x1 - ((iDistX) / 2);
		int iPosY = y1 - ((iDistY) / 2);
		std::tuple<int, int, int> primaryColor(int(rgbColor.GetRed()), int(rgbColor.GetGreen()), int(rgbColor.GetBlue()));
		std::tuple<int, int, int> secondaryColor(int(rgbColor.GetRed()), int(rgbColor.GetGreen()), int(rgbColor.GetBlue()));
		float rSeed = mathRandom(20, 80) / 20.0f;
		float fWidth = (mathRandom(-100, 100) / 100.0f) * float(rChaos) * 16.0f;

		pRenderQueue->addLightningToEffectRenderQueue(iPosX, iPosY, int(iDist) * 2, int(iDist * fWidth), iCanvasWidth, iCanvasHeight, fRotRadians, iShape, iShape,
			primaryColor, secondaryColor, rSeed, OpenGLRenderLayer::blendMode::blendScreen);
		return;
		}

	ASSERT(iPoints >= 0);

	Metric *pxPos = new Metric [iPoints];
	Metric *pyPos = new Metric [iPoints];

	pxPos[0] = xFrom;
	pyPos[0] = yFrom;
	pxPos[iPoints-1] = xTo;
	pyPos[iPoints-1] = yTo;

	ComputeLightningPoints(iPoints, pxPos, pyPos, rChaos);

	//	Draw lightning

	for (int i = 0; i < iPoints-1; i++)
		{
		Dest.DrawLine((int)pxPos[i], (int)pyPos[i],
				(int)pxPos[i+1], (int)pyPos[i+1],
				1,
				rgbColor);
		}

	//	Done

	delete [] pxPos;
	delete [] pyPos;
	}

void DrawParticle (CG32bitImage &Dest,
				   int x, int y,
				   CG32bitPixel rgbColor,
				   int iSize,
				   DWORD byOpacity)

//	DrawParticle
//
//	Draws a single particle

	{
	DWORD byOpacity2 = byOpacity / 2;

	switch (iSize)
		{
		case 0:
			Dest.SetPixelTrans(x, y, rgbColor, (BYTE)byOpacity);
			break;

		case 1:
			Dest.SetPixelTrans(x, y, rgbColor, (BYTE)byOpacity);
			Dest.SetPixelTrans(x + 1, y, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x, y + 1, rgbColor, (BYTE)byOpacity2);
			break;

		case 2:
			Dest.SetPixelTrans(x, y, rgbColor, (BYTE)byOpacity);
			Dest.SetPixelTrans(x + 1, y, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x, y + 1, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x - 1, y, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x, y - 1, rgbColor, (BYTE)byOpacity2);
			break;

		case 3:
			Dest.SetPixelTrans(x, y, rgbColor, (BYTE)byOpacity);
			Dest.SetPixelTrans(x + 1, y, rgbColor, (BYTE)byOpacity);
			Dest.SetPixelTrans(x, y + 1, rgbColor, (BYTE)byOpacity);
			Dest.SetPixelTrans(x - 1, y, rgbColor, (BYTE)byOpacity);
			Dest.SetPixelTrans(x, y - 1, rgbColor, (BYTE)byOpacity);
			Dest.SetPixelTrans(x + 1, y + 1, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x + 1, y - 1, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x - 1, y + 1, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x - 1, y - 1, rgbColor, (BYTE)byOpacity2);
			break;

		case 4:
		default:
			{
			Dest.Fill(x - 1,
					y - 1,
					3,
					3,
					CG32bitPixel(rgbColor, (BYTE)byOpacity));

			Dest.SetPixelTrans(x + 2, y, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x, y + 2, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x - 2, y, rgbColor, (BYTE)byOpacity2);
			Dest.SetPixelTrans(x, y - 2, rgbColor, (BYTE)byOpacity2);
			}
		}
	}
