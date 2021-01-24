//	Geometry.cpp
//
//	Basic geometric functions

#include "PreComp.h"

#include "math.h"

//	Functions -----------------------------------------------------------------

Metric CalcDistanceToPath (const CVector &Pos,
						   const CVector &Path1,
						   const CVector &Path2,
						   CVector *retvNearestPoint,
						   CVector *retvAway)

//	CalcDistanceToPath
//
//	Returns the distance from Pos to the path from Path1 to Path2.

	{
	//	Create a unit vector from Path1 to Path2

	CVector vPath;
	vPath = Path2 - Path1;
	vPath = vPath.Normal();

	//	Create a vector perpendicular to the path

	CVector vPerp = vPath.Perpendicular();

	//	Create a vector from the point to one of the endpoints

	CVector vPoint = Path1 - Pos;

	//	Project this vector on to the perpendicular

	CVector vToLine = vPoint.Dot(vPerp) * vPerp;

	//	Compute the point on the line that is nearest Pos

	CVector vIntersect = Pos + vToLine;

	//	See if the point is between the two path endpoints

	bool bBetween = false;
	if (Path1.GetX() < Path2.GetX())
		bBetween = vIntersect.GetX() > Path1.GetX() && vIntersect.GetX() < Path2.GetX();
	else
		bBetween = vIntersect.GetX() > Path2.GetX() && vIntersect.GetX() < Path1.GetX();

	if (bBetween)
		{
		if (Path1.GetY() < Path2.GetY())
			bBetween = vIntersect.GetY() > Path1.GetY() && vIntersect.GetY() < Path2.GetY();
		else
			bBetween = vIntersect.GetY() > Path2.GetY() && vIntersect.GetY() < Path1.GetY();
		}

	//	If we're between, then the nearest distance is the distance to the path

	if (bBetween)
		{
		if (retvNearestPoint)
			*retvNearestPoint = vIntersect;
		if (retvAway)
			*retvAway = vToLine.Normal();
		return vToLine.Length();
		}

	//	Otherwise, the nearest distance is the nearest distance to the two endpoints

	else
		{
		CVector vDist1 = Path1 - Pos;
		CVector vDist2 = Path2 - Pos;
		Metric rLen1 = vDist1.Length();
		Metric rLen2 = vDist2.Length();

		if (rLen1 < rLen2)
			{
			if (retvNearestPoint)
				*retvNearestPoint = Path1;
			if (retvAway)
				*retvAway = vDist1.Normal();
			return rLen1;
			}
		else
			{
			if (retvNearestPoint)
				*retvNearestPoint = Path2;
			if (retvAway)
				*retvAway = vDist2.Normal();
			return rLen2;
			}
		}
	}

Metric CalcInterceptTime (const CVector &vTarget, const CVector &vTargetVel, Metric rMissileSpeed, Metric *retrRange)

//	CalcInterceptTime
//
//	Returns the time that it would take to intercept a target
//	at vTarget, moving with velocity vTargetVel, with
//	a missile of speed rMissileSpeed. Returns < 0.0 if the missile cannot
//	intercept the target.
//
//	The formula for interception is:
//
//			A +- B sqrt(C)
//	t	=	--------------
//				  D
//
//	Where	A = B rVi
//			B = 2 rRange
//			C = rMissileSpeed^2 - rVj^2
//			D = 2 (C - rVi^2)

	{
	Metric rRange = vTarget.Length();
	if (rRange == 0.0)
		return 0.0;

	CVector vPosNormal = vTarget / rRange;

	if (retrRange)
		*retrRange = rRange;

	//	Compute the orthogonals of the velocity along the position vector

	Metric rVi, rVj;
	vTargetVel.GenerateOrthogonals(vPosNormal, &rVi, &rVj);

	//	Figure out the inside of the square root. If this value is negative
	//	then we don't have an interception course.

	Metric C = rMissileSpeed * rMissileSpeed - rVj * rVj;
	if (C < 0.0)
		return -1.0;

	//	Figure out the denominator. If this value is 0 then we don't
	//	have an interception course.

	Metric D = 2 * (C - rVi * rVi);
	if (D == 0.0)
		return -1.0;

	//	Compute A and B

	Metric B = 2 * rRange;
	Metric A = B * rVi;

	//	Compute both roots

	Metric Z = B * sqrt(C);
	Metric R1 = (A + Z) / D;
	Metric R2 = (A - Z) / D;

	//	If the first root is positive then return it

	if (R1 > 0.0)
		return R1;

	//	Otherwise we return the second root, which may or may not
	//	be positive

	return R2;
	}

bool IntersectRect(const CVector &vUR1, const CVector &vLL1,
				   const CVector &vUR2, const CVector &vLL2)

//	IntersectRect
//
//	Returns TRUE if the two rects intersect

	{
	return (vUR1.GetX() > vLL2.GetX()
			&& vLL1.GetX() < vUR2.GetX()
			&& vUR1.GetY() > vLL2.GetY()
			&& vLL1.GetY() < vUR2.GetY());
	}

bool IntersectRect (const CVector &vUR, const CVector &vLL, const CVector &vPoint)

//	IntersectRect
//
//	Returns TRUE if the point is inside the rect

	{
	return (vUR.GetX() > vPoint.GetX()
			&& vLL.GetX() < vPoint.GetX()
			&& vUR.GetY() > vPoint.GetY()
			&& vLL.GetY() < vPoint.GetY());
	}

bool IntersectRectAndRay (const CVector &vUR, const CVector &vLL, const CVector &vStart, const CVector &vEnd, CVector *retvIntersection, Metric *retIntersectionFraction)

//	IntersectRect
//
//	Returns TRUE if the given ray intersects the rect. We optionally return the
//	point of intersection and the fractional distance along the ray where we 
//	first intersect.

	{
	int iIntersections = 0;
	Metric rBestFraction = 1.0;

	if (IntersectLine(vStart, vEnd, vUR, CVector(vUR.GetX(), vLL.GetY()), NULL, &rBestFraction))
		iIntersections++;

	Metric rFraction;
	if (IntersectLine(vStart, vEnd, CVector(vUR.GetX(), vLL.GetY()), vLL, NULL, &rFraction))
		{
		rBestFraction = Min(rBestFraction, rFraction);
		iIntersections++;
		}

	if (IntersectLine(vStart, vEnd, vLL, CVector(vLL.GetX(), vUR.GetY()), NULL, &rFraction))
		{
		rBestFraction = Min(rBestFraction, rFraction);
		iIntersections++;
		}

	if (IntersectLine(vStart, vEnd, CVector(vLL.GetX(), vUR.GetY()), vUR, NULL, &rFraction))
		{
		rBestFraction = Min(rBestFraction, rFraction);
		iIntersections++;
		}

	//	If no intersections, then nothing.

	if (iIntersections == 0)
		return false;

	//	If we have only 1 intersection, then it means that the ray is inside the
	//	rectangle, so we count the intersection point as the beginning of the ray.

	if (iIntersections == 1)
		rBestFraction = 0.0;

	if (retvIntersection)
		*retvIntersection = CVector(
				vStart.GetX() + rBestFraction * (vEnd.GetX() - vStart.GetX()),
				vStart.GetY() + rBestFraction * (vEnd.GetY() - vStart.GetY())
				);

	if (retIntersectionFraction)
		*retIntersectionFraction = rBestFraction;

	return true;
	}

//	ViewportTransform ---------------------------------------------------------

ViewportTransform::ViewportTransform (const CVector &vCenter, Metric xScale, Metric yScale, int xCenter, int yCenter)
	{
	m_xScale = xScale;
	m_yScale = yScale;

	//	Note: we use this manual transformation so that we don't get a 
	//	round-off error when scaling for klicks-per-pixel. We scale first
	//	and then substract for position because we want to keep the relative
	//	position of objects constant.

	m_xCenterTrans = (int)(vCenter.GetX() / xScale) - xCenter;
	m_yCenterTrans = (int)(vCenter.GetY() / yScale) + yCenter;
	}

ViewportTransform::ViewportTransform (const CVector &vCenter, Metric rScale, int xCenter, int yCenter)
	{
	m_xScale = rScale;
	m_yScale = rScale;

	//	Note: we use this manual transformation so that we don't get a 
	//	round-off error when scaling for klicks-per-pixel. We scale first
	//	and then substract for position because we want to keep the relative
	//	position of objects constant.

	m_xCenterTrans = (int)(vCenter.GetX() / rScale) - xCenter;
	m_yCenterTrans = (int)(vCenter.GetY() / rScale) + yCenter;
	}

void ViewportTransform::Offset (int x, int y)
	{
	m_xCenterTrans += x;
	m_yCenterTrans += y;
	}

void ViewportTransform::Transform (const CVector &vP, int *retx, int *rety) const
	{
	*retx = (int)(vP.GetX() / m_xScale) - m_xCenterTrans;
	*rety = m_yCenterTrans - (int)(vP.GetY() / m_yScale);
	}

