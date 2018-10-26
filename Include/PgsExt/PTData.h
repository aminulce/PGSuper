///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2013  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////
#pragma once

#include <WBFLCore.h>
#include <Material\PsStrand.h>
#include <PgsExt\PgsExtExp.h>
#include <Units\Units.h>

#include <EAF\EAFUtilities.h>

class CPTData;
class CSplicedGirderData;
class CTimelineManager;


/*****************************************************************************
CLASS 
   CDuctSize

DESCRIPTION
   Utility class that defines the available duct sizes, number of 0.5" and 0.6"
   strands that can fit into the duct, and the eccentricity of the strands
   within the duct

COPYRIGHT
   Copyright � 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/
class PGSEXTCLASS CDuctSize
{
public:
   CDuctSize(const std::_tstring& name,Float64 dia,StrandIndexType n5,StrandIndexType n6,Float64 e5,Float64 e6) :
   Name(name),Diameter(dia),MaxStrands5(n5),MaxStrands6(n6),Ecc5(e5),Ecc6(e6){}

   std::_tstring Name;
   Float64 Diameter;
   StrandIndexType MaxStrands5; // max # of 0.5" strands
   StrandIndexType MaxStrands6; // max # of 0.6" strands
   Float64 Ecc5; // eccentricty for 0.5" strands
   Float64 Ecc6; // eccentricty for 0.6" strands

   StrandIndexType GetMaxStrands(matPsStrand::Size size) const;
   Float64 GetEccentricity(matPsStrand::Size size) const;

   static const CDuctSize DuctSizes[];
   static const Uint32 nDuctSizes;
};


/*****************************************************************************
CLASS 
   CDuctGeometry

DESCRIPTION
   Base class for the geometry of all duct types

COPYRIGHT
   Copyright � 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/
class PGSEXTCLASS CDuctGeometry
{
public:
   enum OffsetType { BottomGirder, TopGirder };
   enum GeometryType { Linear, Parabolic, Offset };

   CDuctGeometry();
   CDuctGeometry(const CDuctGeometry& rOther);
   CDuctGeometry(const CSplicedGirderData* pGirder);
   virtual ~CDuctGeometry();

   CDuctGeometry& operator=(const CDuctGeometry& rOther);

   void SetGirder(const CSplicedGirderData* pGirder);
   const CSplicedGirderData* GetGirder() const;

   void MakeCopy(const CDuctGeometry& rOther);
   virtual void MakeAssignment(const CDuctGeometry& rOther);

protected:
   const CSplicedGirderData* m_pGirder;
};

/*****************************************************************************
CLASS 
   CLinearDuctGeometry

DESCRIPTION
   Defines a duct described by a series of linear segments.

COPYRIGHT
   Copyright � 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/
class PGSEXTCLASS CLinearDuctGeometry : public CDuctGeometry
{
public:
   CLinearDuctGeometry();
   CLinearDuctGeometry(const CSplicedGirderData* pGirder);

   bool operator==(const CLinearDuctGeometry& rOther) const;
   bool operator!=(const CLinearDuctGeometry& rOther) const;

   void AddPoint(Float64 distFromPrev,Float64 offset,OffsetType offsetType);
   CollectionIndexType GetPointCount() const;
   void GetPoint(CollectionIndexType pntIdx,Float64* pDistFromPrev,Float64 *pOffset,OffsetType *pOffsetType) const;

   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);
   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);

protected:
   struct PointRecord 
   { 
      Float64 distFromPrev; 
      Float64 offset; 
      OffsetType offsetType; 

      bool operator==(const PointRecord& rOther) const
      {
         if ( !IsEqual(distFromPrev,rOther.distFromPrev) )
            return false;

         if ( !IsEqual(offset,rOther.offset) )
            return false;

         if ( offsetType != rOther.offsetType )
            return false;

         return true;
      }

      bool operator!=(const PointRecord& rOther) const
      {
         return !operator==(rOther);
      }
   };
   std::vector<PointRecord> m_Points;
};


/*****************************************************************************
CLASS 
   CParabolicDuctGeometry

DESCRIPTION
   Defines a parabolic duct

COPYRIGHT
   Copyright � 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/
class PGSEXTCLASS CParabolicDuctGeometry : public CDuctGeometry
{
public:
   CParabolicDuctGeometry();
   CParabolicDuctGeometry(const CSplicedGirderData* pGirder);
   CParabolicDuctGeometry(const CParabolicDuctGeometry& rOther);

   CParabolicDuctGeometry& operator=(const CParabolicDuctGeometry& rOther);
   bool operator==(const CParabolicDuctGeometry& rOther) const;
   bool operator!=(const CParabolicDuctGeometry& rOther) const;

   void Init();
   SpanIndexType GetSpanCount() const;
   void InsertSpan(SpanIndexType newSpanIdx);
   void RemoveSpan(SpanIndexType spanIdx,PierIndexType pierIdx);

   void SetStartPoint(Float64 dist,Float64 offset,OffsetType offsetType);
   void GetStartPoint(Float64 *pDist,Float64 *pOffset,OffsetType *pOffsetType) const;

   void SetLowPoint(SpanIndexType spanIdx,Float64   distLow,Float64   offsetLow,OffsetType lowOffsetType);
   void GetLowPoint(SpanIndexType spanIdx,Float64* pDistLow,Float64 *pOffsetLow,OffsetType *pLowOffsetType) const;

   void SetHighPoint(PierIndexType pierIdx,
                     Float64 distLeftIP,
                     Float64 highOffset,OffsetType highOffsetType,
                     Float64 distRightIP);

   void GetHighPoint(PierIndexType pierIdx,
                     Float64* distLeftIP,
                     Float64* highOffset,OffsetType* highOffsetType,
                     Float64* distRightIP) const;

   void SetEndPoint(Float64 dist,Float64 offset,OffsetType offsetType);
   void GetEndPoint(Float64 *pDist,Float64 *pOffset,OffsetType *pOffsetType) const;

   void MakeCopy(const CParabolicDuctGeometry& rOther);
   virtual void MakeAssignment(const CParabolicDuctGeometry& rOther);

   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);
   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);

private:
   struct HighPoint
   {
      Float64 distLeftIP;

      Float64 highOffset;
      OffsetType highOffsetType;

      Float64 distRightIP;

      HighPoint()
      {
         distLeftIP = -0.5;

         highOffset = ::ConvertToSysUnits(6.0,unitMeasure::Inch);
         highOffsetType = CDuctGeometry::TopGirder;

         distRightIP = -0.5;
      }

      bool operator==(const HighPoint& rOther) const
      {
         if ( !IsEqual(distLeftIP,rOther.distLeftIP) )
            return false;


         if ( !IsEqual(highOffset,rOther.highOffset) )
            return false;

         if ( highOffsetType != rOther.highOffsetType )
            return false;


         if ( !IsEqual(distRightIP,rOther.distRightIP) )
            return false;

         return true;
      }
   };

   struct Point
   {
      Float64 Distance;
      Float64 Offset;
      OffsetType OffsetType;

      bool operator==(const Point& rOther) const
      {
         if ( !IsEqual(Distance,rOther.Distance) )
            return false;

         if ( !IsEqual(Offset,rOther.Offset) )
            return false;

         if ( OffsetType != rOther.OffsetType )
            return false;

         return true;
      }

      bool operator!=(const Point& rOther) const
      {
         return !operator==(rOther);
      }
   };

   Point StartPoint;
   Point EndPoint;
   std::vector<Point> LowPoints; // index = spanIdx
   std::vector<HighPoint> HighPoints; // index = pierIdx-1 (HighPoints[0] => pierIdx=1)

#if defined _DEBUG
   void AssertValid();
#endif
};

/*****************************************************************************
CLASS 
   COffsetDuctGeometry

DESCRIPTION
   Defines a parabolic that is offset from another duct and derives its 
   geometry from the duct is references.

COPYRIGHT
   Copyright � 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/
class PGSEXTCLASS COffsetDuctGeometry : public CDuctGeometry
{
public:
   COffsetDuctGeometry()
   { 
      RefDuctIdx = 0; 
   }

   COffsetDuctGeometry(const CSplicedGirderData* pGirder) :
   CDuctGeometry(pGirder)
   { 
      RefDuctIdx = 0; 
   }

   bool operator==(const COffsetDuctGeometry& rOther) const
   {
      if ( RefDuctIdx != rOther.RefDuctIdx )
         return false;

      if ( Points != rOther.Points )
         return false;

      return true;
   }

   bool operator!=(const COffsetDuctGeometry& rOther) const
   {
      return !operator==(rOther);
   }

   struct Point
   {
      Float64 distance; // distance from previous
      Float64 offset; // offset from reference duct

      bool operator==(const COffsetDuctGeometry::Point& rOther) const
      {
         if ( !IsEqual(distance,rOther.distance) )
            return false;

         if ( !IsEqual(offset,rOther.offset) )
            return false;

         return true;
      }

      bool operator!=(const Point& rOther) const
      {
         return !operator==(rOther);
      }
   };

   Float64 GetOffset(Float64 x) const
   {
      if ( Points.size() == 0 )
         return 0;

      if ( Points.size() == 1 )
         return Points.front().offset;

      std::vector<Point>::const_iterator iter1(Points.begin());
      std::vector<Point>::const_iterator iter2(iter1+1);
      std::vector<Point>::const_iterator iterEnd(Points.end());

      Float64 x1 = iter1->distance;
      if ( x < x1 )
         return iter1->offset;

      for (; iter2 != iterEnd; iter1++, iter2++ )
      {
         Float64 x2 = x1 + iter2->distance;
         if ( InRange(x1,x,x2) )
         {
            Float64 offset1 = iter1->offset;
            Float64 offset2 = iter2->offset;

            Float64 offset = ::LinInterp(x-x1,offset1,offset2,x2-x1);
            return offset;
         }

         x1 = x2;
      }

      return Points.back().offset;
   }

   // returns true of the offset is uniform... that is, it is the same distance from the
   // reference duct at all points
   bool IsUniformOffset() const
   {
      if ( Points.size() <= 1 )
         return true;

      std::vector<Point>::const_iterator iter(Points.begin());
      std::vector<Point>::const_iterator end(Points.end());
      Float64 offset1 = (*iter++).offset;
      for( ; iter != end; iter++ )
      {
         Float64 offset2 = (*iter).offset;
         if ( !IsEqual(offset1,offset2) )
            return false;

         offset1 = offset2;
      }

      return true;
   }

   std::vector<Point> Points;

   DuctIndexType RefDuctIdx;

   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);
   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
};


/*****************************************************************************
CLASS 
   CDuctData

DESCRIPTION
   Utility class that defines a duct

COPYRIGHT
   Copyright � 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/
class PGSEXTCLASS CDuctData
{
public:
   CDuctData();
   CDuctData(const CSplicedGirderData* pGirder);
   bool operator==(const CDuctData& rOther) const;

   void Init(const CSplicedGirderData* pGirder);
   void SetGirder(const CSplicedGirderData* pGirder);
   void InsertSpan(SpanIndexType newSpanIndex);

   // removes a span from the duct data. spanIdx and pierIdx are
   // relative indicies measured from the start of the girder this
   // duct is a part of
   void RemoveSpan(SpanIndexType spanIdx,PierIndexType pierIdx);

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   CPTData* m_pPTData; // weak reference to the parent PTData

   Uint32 Size; // Index into CDuctSize::DuctSizes
   StrandIndexType nStrands;
   bool bPjCalc;
   Float64 Pj;
   Float64 LastUserPj;

   CDuctGeometry::GeometryType DuctGeometryType;
   CLinearDuctGeometry         LinearDuctGeometry;
   CParabolicDuctGeometry      ParabolicDuctGeometry;
   COffsetDuctGeometry         OffsetDuctGeometry;

   pgsTypes::JackingEndType JackingEnd;
};

/*****************************************************************************
CLASS 
   CPTData

DESCRIPTION
   Utility class for post-tensioning description data.

COPYRIGHT
   Copyright � 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/
class PGSEXTCLASS CPTData
{
public:
   CPTData();
   CPTData(const CPTData& rOther);
   ~CPTData();

   CPTData& operator = (const CPTData& rOther);
   bool operator==(const CPTData& rOther) const;
   bool operator!=(const CPTData& rOther) const;

   void SetGirder(CSplicedGirderData* pSplicedGirder);
   CSplicedGirderData* GetGirder();
   const CSplicedGirderData* GetGirder() const;

   void AddDuct(CDuctData& duct);
   DuctIndexType GetDuctCount() const;
   const CDuctData* GetDuct(DuctIndexType idx) const;
   CDuctData* GetDuct(DuctIndexType idx);
   bool CanRemoveDuct(DuctIndexType idx) const;
   void RemoveDuct(DuctIndexType idx);
   StrandIndexType GetNumStrands(DuctIndexType ductIndex) const;
   StrandIndexType GetNumStrands() const;
   Float64 GetPjack(DuctIndexType ductIndex) const;

   void InsertSpan(SpanIndexType newSpanIndex);
   void RemoveSpan(SpanIndexType spanIdx,PierIndexType pierIdx);

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // Temporary Strands for stability during handling
   StrandIndexType nTempStrands;
   Float64 PjTemp;
   bool    bPjTempCalc;   // true if Pj was calculated.
   Float64 LastUserPjTemp;   // Last Pj entered by user
   const matPsStrand* pStrand; // tendon strand type

protected:
   std::vector<CDuctData> m_Ducts;

   void MakeCopy(const CPTData& rOther);
   virtual void MakeAssignment(const CPTData& rOther);

private:
   CSplicedGirderData* m_pGirder; // weak reference

   CTimelineManager* GetTimelineManager();
   void RemoveFromTimeline();
};
