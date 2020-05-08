
//	Euclid.h
//
//	Basic geometry definitions
//	Copyright (c) 2016 by Kronosaur Productions, LLC. All Rights Reserved.

#pragma once

#include <math.h>

typedef double Metric;

constexpr Metric PI = 3.14159265358979;
constexpr Metric HALF_PI = 0.5 * PI;
constexpr Metric TAU = 2.0 * PI;

const Metric SQRT_3 = sqrt(3.0);

constexpr Metric DBL_INFINITY = 1.7976931348623158e+308;	//	DBL_MAX

class CMath
	{
	public:
		static constexpr Metric AdjustChance (Metric rChance, Metric rAdj)

		//	AdjustChance
		//
		//	rChance is a probability from 0 to 1.0 and rAdj is a positive number 
		//	representing the number of tries for the chance.
		//
		//	For example, if rAdj = 2.0, then we return a probability equal to the chance
		//	that we succeed at rChance in 2 tries.

			{
			if (rChance <= 0.0 || rAdj <= 0.0)
				return 0.0;
			else if (rChance >= 1.0)
				return 1.0;
			else
				return 1.0 - pow(1.0 - rChance, rAdj);
			}

		static int Round (Metric rValue) { return mathRound(rValue); }
	};

//	Angles

bool AngleInArc (int iAngle, int iMinAngle, int iMaxAngle);
inline int AngleMod (int iAngle) { if (iAngle >= 0) return (iAngle % 360); else return 360 - (-iAngle % 360); }
inline int AngleMiddle (int iLowAngle, int iHighAngle)
//	Returns the angle in between the two given angles. If iLowAngle > iHighAngle then
//	we wrap around through 0
	{
	if (iLowAngle > iHighAngle)
		return (iLowAngle + ((iHighAngle + 360 - iLowAngle) / 2)) % 360;
	else
		return iLowAngle + ((iHighAngle - iLowAngle) / 2);
	}

inline int AngleToDegrees (Metric rAngle) { return AngleMod(mathRound(rAngle * 180.0 / PI)); }

inline Metric mathAngleMod (double rAngle) { if (rAngle >= 0.0) return fmod(rAngle, TAU); else return TAU - fmod(-rAngle, TAU); }
inline Metric mathAngleModDegrees (double rAngle) { if (rAngle >= 0.0) return fmod(rAngle, 360.0); else return 360.0 - fmod(-rAngle, 360.0); }
inline Metric mathAngleBearing (Metric rAngle, Metric rOrigin) { Metric rDiff = mathAngleMod(rAngle - rOrigin); return (rDiff > PI ? rDiff - TAU : rDiff); }
inline Metric mathAngleDiff (double rFrom, double rTo) { return mathAngleMod(rTo - rFrom); }
inline constexpr Metric mathDegreesToRadians (int iAngle) { return iAngle * PI / 180.0; }
inline constexpr Metric mathDegreesToRadians (Metric rDegrees) { return PI * rDegrees / 180.0; }
inline constexpr Metric mathInterpolate (Metric rFrom, Metric rTo, Metric rInterpolate) { return rFrom + (rInterpolate * (rTo - rFrom)); }
inline Metric mathLog (Metric rValue, Metric rBase) { return ((rValue > 0.0 && rBase > 0.0) ? (log(rValue) / log(rBase)) : 0.0); }
inline Metric mathLog2 (Metric rValue) { return (rValue > 0.0 ? (log(rValue) / log(2.0)) : 0.0); }
inline constexpr Metric mathRadiansToDegrees (Metric rRadians) { return 180.0 * rRadians / PI; }

//	2d vector class

class CVector
	{
	public:
		CVector (void) : x(0.0), y(0.0) { }
		CVector (Metric ix, Metric iy) : x(ix), y(iy) { }

		bool operator == (const CVector &vA) const { return (x == vA.x && y == vA.y); }

		bool Clip (Metric rLength);
		Metric Distance (const CVector &vA) const { return sqrt(Distance2(vA)); }
		Metric Distance2 (const CVector &vA) const { Metric xDiff = (vA.x - x); Metric yDiff = (vA.y - y); return (xDiff * xDiff + yDiff * yDiff); }
		Metric Dot (const CVector &vA) const { return x * vA.x + y * vA.y; }
		void GenerateOrthogonals (const CVector &vNormal, Metric *retvPara, Metric *retvPerp) const;
		const Metric &GetX (void) const { return x; }
		const Metric &GetY (void) const { return y; }
		bool InBox (const CVector &vUR, const CVector &vLL) const { return (x >= vLL.x && x < vUR.x	&& y >= vLL.y && y < vUR.y); }
		bool IsNull (void) const { return (x == 0.0 && y == 0.0); }
		Metric Length (void) const { return sqrt(x * x + y * y); }
		Metric Length2 (void) const { return (x * x + y * y); }
		Metric Longest (void) const
			{
			Metric ax = (x < 0.0 ? -x : x);
			Metric ay = (y < 0.0 ? -y : y);
			return (ax > ay ? ax : ay);
			}
		CVector Normal (void) const 
			{
			Metric rLength = Length();

			//	NOTE: We use to return a null vector, but some pieces of code
			//	expect a us to always be unit length. [LATER: Of course, this
			//	function should be called Unit instead of Normal.]

			if (rLength == 0.0)
				return CVector(1.0, 0.0);
			else
				return CVector(x / rLength, y / rLength);
			}
		CVector Normal (Metric *retrLength) const
			{
			*retrLength = Length();
			if (*retrLength == 0.0)
				return CVector(1.0, 0.0);
			else
				return CVector(x / *retrLength, y / *retrLength);
			}
		CVector Perpendicular (void) const { return CVector(-y, x); }
		Metric Polar (Metric *retrRadius = NULL) const;
		void ReadFromStream (IReadStream &Stream) { Stream.Read((char *)this, sizeof(CVector)); }
		CVector Reflect (void) const { return CVector(-x, -y); }
		CVector Rotate (int iAngle) const;
		CVector Rotate (Metric rRadians) const;
		void SetX (Metric NewX) { x = NewX; }
		void SetY (Metric NewY) { y = NewY; }
		void WriteToStream (IWriteStream &Stream) const { Stream.Write((char *)this, sizeof(CVector)); }
		const Metric &X (void) const { return x; }
		const Metric &Y (void) const { return y; }

		static CVector FromPolar (const CVector &vA) { return CVector(vA.y * cos(vA.x), vA.y * sin(vA.x)); }
		static CVector FromPolar (Metric rAngle, Metric rRadius) { return CVector(rRadius * cos(rAngle), rRadius * sin(rAngle)); }
		static CVector FromPolarInv (Metric rAngle, Metric rRadius) { return CVector(rRadius * cos(rAngle), -rRadius * sin(rAngle)); }
		
	private:
		Metric x;
		Metric y;
	};

extern const CVector NullVector;
extern Metric g_Cosine[360];
extern Metric g_Sine[360];

//	Vector-Vector operations
inline CVector operator+ (const CVector &op1, const CVector &op2) { return CVector(op1.GetX() + op2.GetX(), op1.GetY() + op2.GetY()); }
inline CVector operator- (const CVector &op1, const CVector &op2) { return CVector(op1.GetX() - op2.GetX(), op1.GetY() - op2.GetY()); }
inline CVector operator* (const CVector &op1, const CVector &op2) { return CVector(op1.GetX() * op2.GetX(), op1.GetY() * op2.GetY()); }
inline CVector operator- (const CVector &op) { return CVector(-op.GetX(), -op.GetY()); }

//	Vector-scalar operations
inline CVector operator* (const CVector &op1, const Metric op2) { return CVector(op1.GetX() * op2, op1.GetY() * op2); }
inline CVector operator* (const Metric op2, const CVector &op1) { return CVector(op1.GetX() * op2, op1.GetY() * op2); }
inline CVector operator/ (const CVector &op1, const Metric op2) { return CVector(op1.GetX() / op2, op1.GetY() / op2); }

//	Lines

class CLine
	{
	public:
		CLine (void)
			{ }

		CLine (const CVector &vFrom, const CVector &vTo) :
				m_vFrom(vFrom),
				m_vTo(vTo)
			{ }

		const CVector &From (void) const { return m_vFrom; }
		const CVector &To (void) const { return m_vTo; }
		void SetFrom (const CVector &vVector) { m_vFrom = vVector; }
		void SetTo (const CVector &vVector) { m_vTo = vVector; }

	private:
		CVector m_vFrom;
		CVector m_vTo;
	};

//	Geometry

class CGeometry
	{
	public:
		enum EFlags
			{
			FLAG_SCREEN_COORDS =			0x00000001,	//	Use screen coordinates (y is positive downward)
			FLAG_CLOCKWISE =				0x00000002,	//	Clockwise instead of counter-clockwise
			};

		enum EIntersectResults
			{
			intersectNone,
			intersectPoint,
			intersect2Points,
			};

		static void AccumulateBounds (const CVector &vPos, CVector &vLL, CVector &vUR);
		static void AddArcPoints (const CVector &vCenter, Metric rRadius, Metric rFromAngle, Metric rToAngle, TArray<CVector> *ioPoints, DWORD dwFlags = 0);
		static int AngleArc (int iMinAngle, int iMaxAngle);
		static void CombineArcs (int iMinAngle1, int iMaxAngle1, int iMinAngle2, int iMaxAngle2, int *retiMin, int *retiMax);
		static EIntersectResults IntersectLineCircle (const CVector &vFrom, const CVector &vTo, const CVector &vCenter, Metric rRadius, CVector *retvP1 = NULL, CVector *retvP2 = NULL);
	};

//	Transform class

enum XFormType
	{
	xformIdentity,

	xformTranslate,
	xformScale,
	xformRotate,

	xformRotateX,
	xformRotateY,
	xformRotateZ,
	};

class CXForm
	{
	public:
		CXForm (void);
		CXForm (XFormType type);
		CXForm (XFormType type, Metric rX, Metric rY);
		CXForm (XFormType type, const CVector &vVector);
		CXForm (XFormType type, Metric rDegrees);

		void Transform (Metric x, Metric y, Metric *retx, Metric *rety) const;
		CVector Transform (const CVector &vVector) const;

	private:
		Metric m_Xform[3][3];

	friend const CXForm operator* (const CXForm &op1, const CXForm &op2);
	};

const CXForm operator* (const CXForm &op1, const CXForm &op2);

//	Shapes ---------------------------------------------------------------------

void CreateArcPolygon (Metric rInnerRadius, Metric rOuterRadius, int iArc, TArray<CVector> *retPoints);

//	Graphs ---------------------------------------------------------------------

class CIntGraph
	{
	public:
		CIntGraph (void);

		void AddGraph (CIntGraph &Source);
		void AddNode (int x, int y, DWORD *retdwID = NULL);
		void Connect (DWORD dwFromID, DWORD dwToID);
		void DeleteAll (void);
		bool FindNearestNode (int x, int y, DWORD *retdwID);
		void GenerateDelaunayConnections (void);
		void GenerateRandomConnections (DWORD dwStartNode, int iMinConnections, int iMaxConnections);
		int GetNodeCount (void);
		int GetNodeConnections (DWORD dwID, TArray<DWORD> *retConnections) const;
		int GetNodeForwardConnections (DWORD dwID, TArray<DWORD> *retConnections) const;
		DWORD GetNodeID (int iIndex);
		int GetNodeIndex (DWORD dwID);
		void GetNodePos (DWORD dwID, int *retx, int *rety);
		bool IsCrossingConnection (DWORD dwFromID, DWORD dwToID);

	private:
		struct SNode
			{
			int x;							//	Coordinates
			int y;

			int iFirstForward;				//	First forward connection (-1 if no connections; -2 if this node is free)
			int iFirstBackward;				//	First backward connection (-1 if none; if free, index of next free block)
			};

		struct SConnection
			{
			int iFrom;						//	Node ID (-1 if free)
			int iTo;						//	Node ID

			int iNext;						//	Next connection
			};

		int AllocConnection (void);
		int AllocNode (void);
		void CreateNodeIndex (void);
		void FreeConnection (int iConnection);
		void FreeNode (int iNode);
		const SConnection *GetBackwardConnection (const SNode *pNode) const { return (pNode->iFirstBackward >= 0 ? GetConnection(pNode->iFirstBackward) : NULL); }
		const SConnection *GetConnection (int iConnection) const { return &m_Connections[iConnection]; }
		SConnection *GetConnection (int iConnection) { return &m_Connections[iConnection]; }
		const SConnection *GetForwardConnection (const SNode *pNode) const { return (pNode->iFirstForward >= 0 ? GetConnection(pNode->iFirstForward) : NULL); }
		const SConnection *GetNextConnection (const SConnection *pConnection) const { return (pConnection->iNext >= 0 ? GetConnection(pConnection->iNext) : NULL); }
		int GetNextFreeNode (SNode *pNode) { return (pNode->iFirstBackward); }
		const SNode *GetNode (int iNode) const { return &m_Nodes[iNode]; }
		SNode *GetNode (int iNode) { return &m_Nodes[iNode]; }
		void MakeNodeFree (SNode *pNode, int iNextFree) { pNode->iFirstForward = -2; pNode->iFirstBackward = iNextFree; }
		bool NodeIsFree (SNode *pNode) { return (pNode->iFirstForward == -2); }

		TArray<SNode> m_Nodes;
		TArray<SConnection> m_Connections;
		int m_iFirstFreeNode;				//	-1 if none
		int m_iFirstFreeConnection;			//	-1 if none

		bool m_bNodeIndexValid;				//	TRUE if node index is valid
		TArray<int> m_NodeIndex;			//	Index of valid nodes
	};

//	Numbers

class CIntegerIP
	{
	public:
		CIntegerIP (void) : m_iCount(0), m_pNumber(NULL) { }
		CIntegerIP (const CIntegerIP &Src);
		CIntegerIP (int iCount, BYTE *pNumber = NULL);
		~CIntegerIP (void);

		CIntegerIP &operator= (const CIntegerIP &Src);
		bool operator== (const CIntegerIP &Src) const;

		CString AsBase64 (void) const;
		BYTE *GetBytes (void) const { return m_pNumber; }
		int GetLength (void) const { return m_iCount; }
		bool IsEmpty (void) const { return (m_pNumber == NULL); }
		void TakeHandoff (CIntegerIP &Src);

	private:
		void CleanUp (void);
		void Copy (const CIntegerIP &Src);
	
		int m_iCount;						//	Number of bytes
		BYTE *m_pNumber;					//	Array of bytes in big-endian order
	};

//	Statistics

template <class VALUE> class TNumberSeries
	{
	public:
		struct SHistogramPoint
			{
			int iValue;
			int iCount;
			double rPercent;
			};

#ifdef DEBUG
		void DumpSeries (void) const
			{
			int i;

			for (i = 0; i < m_Series.GetCount(); i++)
				printf("%d ", (int)m_Series[i]);

			printf("\n");
			}
#endif

		void CalcHistogram (TArray<SHistogramPoint> *retHistogram) const
			{
			int i;

			retHistogram->DeleteAll();

			if (m_Series.GetCount() == 0)
				return;

			TSortMap<int, int> Counts;
			for (i = 0; i < m_Series.GetCount(); i++)
				{
				bool bNew;
				int *pCount = Counts.SetAt((int)m_Series[i], &bNew);
				if (bNew)
					*pCount = 1;
				else
					*pCount += 1;
				}

			for (i = 0; i < Counts.GetCount(); i++)
				{
				SHistogramPoint *pPoint = retHistogram->Insert();
				pPoint->iValue = Counts.GetKey(i);
				pPoint->iCount = Counts[i];
				pPoint->rPercent = pPoint->iCount / (double)m_Series.GetCount();
				}
			}

		void DeleteAll (void) { m_Series.DeleteAll(); }

		VALUE GetMean (void) const { return (m_Series.GetCount() > 0 ? (m_Total / m_Series.GetCount()) : 0); }
		VALUE GetMedian (void) const
			{
			int iCount = m_Series.GetCount();

			if (iCount == 0)
				return 0;
			else if ((iCount % 2) == 1)
				return m_Series[iCount / 2];
			else
				{
				VALUE Low = m_Series[(iCount / 2) - 1];
				VALUE Hi = m_Series[(iCount / 2)];
				return (Low + Hi) / 2;
				}
			}

		VALUE GetMax (void) const { return (m_Series.GetCount() > 0 ? m_Max : 0); }
		VALUE GetMin (void) const { return (m_Series.GetCount() > 0 ? m_Min : 0); }

		void Insert (VALUE Value)
			{
			//	Update min, max, and total

			if (m_Series.GetCount() == 0)
				{
				m_Total = Value;
				m_Min = Value;
				m_Max = Value;
				}
			else
				{
				if (Value > m_Max)
					m_Max = Value;
				else if (Value < m_Min)
					m_Min = Value;
				
				m_Total += Value;
				}

			//	Insert

			m_Series.InsertSorted(Value);
			}

	private:
		TArray<VALUE> m_Series;
		VALUE m_Min;
		VALUE m_Max;
		VALUE m_Total;
	};

//	Miscellaneous

class CStepIncrementor
	{
	public:
		enum EStyle
			{
			styleNone,

			//	Equal speed from start to end

			styleLinear,

			//	Start fast and then move slower

			styleSquareRoot,				//	x^0.5
			styleQuadRoot,					//	x^0.25
			styleOctRoot,					//	x^0.125

			//	Start slow and then move faster

			styleSquare,					//	x^2
			styleQuad,						//	x^4
			styleOct,						//	x^8

			//	Cyclical (start, peak at end, back to start)

			styleSin,
			};

		CStepIncrementor (EStyle iStyle, Metric rStart, Metric rEnd, int iSteps);
		Metric GetAt (int iStep) const;

	private:
		EStyle m_iStyle;
		Metric m_rStart;
		int m_iSteps;

		Metric m_rRange;
		Metric m_rPower;

		TArray<Metric> m_Cycle;
	};

//  Labels ---------------------------------------------------------------------

class CLabelArranger
    {
    public:
        enum EStyles
            {
            styleSideColumns,               //  Arrange in two columns at edge of bounds
            };

        struct SLabelDesc
            {
            //  Inputs

			CString sLabel;					//	Label
            int cxWidth;                    //  Label width
            int cyHeight;                   //  Label height
            int xDest;                      //  What the label points to
            int yDest;

            //  Outputs

            RECT rcRect;                    //  Output rect of the label
            };

        CLabelArranger (void);

        void AddExclusion (const RECT &rcRect) { m_Exclusions.Insert(rcRect); }
        void Arrange (TArray<SLabelDesc> &Labels) const;
        void SetBounds (const RECT &rcRect);
        void SetRadius (int iRadius) { m_iRadius = iRadius; }
        void SetStyle (EStyles iStyle) { m_iStyle = iStyle; }

    private:
        enum EConstraints
            {
            consNone,
            consOutOfBounds,
            consExclusion,
            consCollision,
            };

        void ArrangeSideColumn (TArray<SLabelDesc> &Labels, const TSortMap<Metric, int> &Sort, int cxColumn, bool bLeftSide) const;
        void ArrangeSideColumns (TArray<SLabelDesc> &Labels) const;
        bool BalanceColumns (TSortMap<Metric, int> &From, TSortMap<Metric, int> &To) const;

        //  Arragement options

        EStyles m_iStyle;                   //  Arrangement style
        RECT m_rcBounds;                    //  Arrange within this rect (if empty, no limits)
        TArray<RECT> m_Exclusions;          //  Do not place inside these rects
        int m_iRadius;                      //  Arrange along this circle size (for some styles)
        int m_cxGrid;                       //  Align labels to this grid
        int m_cyGrid;

        //  Derived values

        int m_xCenter;
        int m_yCenter;
    };

//	Functions

void EuclidInit (void);

void IntPolarToVector (int iAngle, Metric rRadius, int *retx, int *rety);
inline void IntPolarToVector (int iAngle, int iRadius, int *retx, int *rety) { IntPolarToVector(iAngle, (Metric)iRadius, retx, rety); }
bool IntSegmentsIntersect (int A1x, int A1y, int A2x, int A2y, int B1x, int B1y, int B2x, int B2y);
int IntProportionalTransition (int iFrom, int iTo, int iPercent);
int IntVectorToPolar (int x, int y, int *retiRadius = NULL);

CVector PolarToVector (int iAngle, Metric rRadius);
CVector PolarToVectorRadians (Metric rRadians, Metric rRadius);
int VectorToPolar (const CVector &vP, Metric *retrRadius = NULL);
Metric VectorToPolarRadians (const CVector &vP, Metric *retrRadius = NULL);

#include "EuclidMatrix.h"
#include "Euclid3D.h"
#include "EuclidVoronoi.h"

