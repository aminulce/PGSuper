///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2016  Washington State Department of Transportation
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

class CSegmentKey;
class CGirderKey;

#include <PGSuperTypes.h> // for pgsTypes::ProductForceType

/*****************************************************************************
INTERFACE
   IIntervals

   Interface to get information about time-step analysis intervals

DESCRIPTION
*****************************************************************************/
// {FC41CE74-7B33-4f9d-8DF4-2DC16FA8E68D}
DEFINE_GUID(IID_IIntervals, 
0xfc41ce74, 0x7b33, 0x4f9d, 0x8d, 0xf4, 0x2d, 0xc1, 0x6f, 0xa8, 0xe6, 0x8d);
interface IIntervals : IUnknown
{
   // returns the number of time-step intervals
   virtual IntervalIndexType GetIntervalCount() = 0;

   // returns the timeline event index at the start of the interval
   virtual EventIndexType GetStartEvent(IntervalIndexType idx) = 0; 

   // returns the timeline event index at the end of the interval
   virtual EventIndexType GetEndEvent(IntervalIndexType idx) = 0; 

   // returns the specified time for an interval
   virtual Float64 GetTime(IntervalIndexType idx,pgsTypes::IntervalTimeType timeType) = 0;

   // returns the duration of the interval
   virtual Float64 GetDuration(IntervalIndexType idx) = 0;

   // returns the interval description
   virtual LPCTSTR GetDescription(IntervalIndexType idx) = 0;

   // returns the index of the first interval that starts with the specified event index
   virtual IntervalIndexType GetInterval(EventIndexType eventIdx) = 0;

   // returns the index of the interval when a pier is erected
   virtual IntervalIndexType GetErectPierInterval(PierIndexType pierIdx) = 0;

   // returns the index of the interval when the prestressing strands are stressed for the first segment 
   // that is constructed for this girder
   virtual IntervalIndexType GetFirstStressStrandInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the interval when the prestressing strands are stressed for the last segment 
   // that is constructed for this girder
   virtual IntervalIndexType GetLastStressStrandInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the interval when the prestressing strands are stressed
   virtual IntervalIndexType GetStressStrandInterval(const CSegmentKey& segmentKey) = 0;

   // returns the index of the interval when the prestressing strands are released for the first segment 
   // that is constructed for this girder
   virtual IntervalIndexType GetFirstPrestressReleaseInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the interval when the prestressing strands are released for the last segment 
   // that is constructed for this girder
   virtual IntervalIndexType GetLastPrestressReleaseInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the interval when the prestressing is release
   // to the girder (girder has reached release strength).
   // this is the replacement for pgsTypes::CastingYard
   virtual IntervalIndexType GetPrestressReleaseInterval(const CSegmentKey& segmentKey) = 0;

   // returns the index of the interval when a segment is lifted from the casting bed
   virtual IntervalIndexType GetLiftSegmentInterval(const CSegmentKey& segmentKey) = 0;

   virtual IntervalIndexType GetFirstLiftSegmentInterval(const CGirderKey& girderKey) = 0;
   virtual IntervalIndexType GetLastLiftSegmentInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the interval when the segments are place in storage
   virtual IntervalIndexType GetStorageInterval(const CSegmentKey& segmentKey) = 0;

   virtual IntervalIndexType GetFirstStorageInterval(const CGirderKey& girderKey) = 0;
   virtual IntervalIndexType GetLastStorageInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the interval when a segment is hauled to the bridge site
   virtual IntervalIndexType GetHaulSegmentInterval(const CSegmentKey& segmentKey) = 0;

   // returns the index of the interval when the first precast segment for a specified girder is erected
   virtual IntervalIndexType GetFirstSegmentErectionInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the interval when the last precast segment for a specified girder is erected
   virtual IntervalIndexType GetLastSegmentErectionInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the interval when a specific segment is erected
   virtual IntervalIndexType GetErectSegmentInterval(const CSegmentKey& segmentKey) = 0;

   // returns true if a segment is erected in the specified interval
   virtual bool IsSegmentErectionInterval(IntervalIndexType intervalIdx) = 0;
   virtual bool IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) = 0;

   // returns the index of the interval when temporary strands are stressed for a specific segment
   // returns INVALID_INDEX if the segment does not have temporary strands
   virtual IntervalIndexType GetTemporaryStrandStressingInterval(const CSegmentKey& segmentKey) = 0;

   // returns the index of the interval when temporary strands are installed for a specific segment
   // returns INVALID_INDEX if the segment does not have temporary strands
   virtual IntervalIndexType GetTemporaryStrandInstallationInterval(const CSegmentKey& segmentKey) = 0;

   // returns the index of the interval when temporary strands are removed from a specific segment
   // returns INVALID_INDEX if the segment does not have temporary strands
   virtual IntervalIndexType GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey) = 0;

   // returns the index of the interval when a closure joint is cast
   virtual IntervalIndexType GetCastClosureJointInterval(const CClosureKey& closureKey) = 0;

   // retuns the interval when a closure joint becomes composite with the girder
   virtual IntervalIndexType GetCompositeClosureJointInterval(const CClosureKey& closureKey) = 0;

   // returns the interval when the first closure joint becomes composite for this girder
   virtual IntervalIndexType GetFirstCompositeClosureJointInterval(const CGirderKey& girderKey) = 0;

   // returns the interval when the last closure joint becomes composite for this girder
   virtual IntervalIndexType GetLastCompositeClosureJointInterval(const CGirderKey& girderKey) = 0;

   // returns the interval when continuity occurs at a pier
   virtual void GetContinuityInterval(PierIndexType pierIdx,IntervalIndexType* pBack,IntervalIndexType* pAhead) = 0;

   // returns the index of the interval when the deck and diaphragms are cast
   // this is the replacement for pgsTypes::BridgeSite1
   virtual IntervalIndexType GetCastDeckInterval() = 0;

   // returns the index of the interval when the deck becomes composite
   // this is the replacement for pgsTypes::BridgeSite2 (also see GetOverlayInterval and GetInstallRailingSystemInterval)
   virtual IntervalIndexType GetCompositeDeckInterval() = 0;

   // returns the index of the interval when live load is first
   // applied to the structure. it is assumed that live
   // load can be applied to the structure at this interval and all
   // intervals thereafter
   // this is the replacement for pgsTypes::BridgeSite3
   virtual IntervalIndexType GetLiveLoadInterval() = 0;

   // returns the index of the interval when load rating calculations are performed
   virtual IntervalIndexType GetLoadRatingInterval() = 0;

   // returns the index of the interval when the overlay is
   // installed. 
   // this is a replacement for pgsTypes::BridgeSite2 or pgsTypes::BridgeSite3,
   // depending on when the overlay is installed (normal or future)
   virtual IntervalIndexType GetOverlayInterval() = 0;

   // returns the index of the interval when the railing system is constructed
   // this is the same as pgsTypes::BridgeSite2 for pre version 3.0 PGSuper projects
   virtual IntervalIndexType GetInstallRailingSystemInterval() = 0;

   // returns the index of the first interval when tendon stressing occurs
   virtual IntervalIndexType GetFirstTendonStressingInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the last interval when tendon stressing occurs
   virtual IntervalIndexType GetLastTendonStressingInterval(const CGirderKey& girderKey) = 0;

   // returns the index of the interval when the specified tendon is stressed
   virtual IntervalIndexType GetStressTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx) = 0;

   // returns true if a tendon is stressed during the specified interval
   virtual bool IsTendonStressingInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) = 0;

   // returns true if there is a change in prestressing during the specified interval
   virtual bool IsStressingInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) = 0;

   // returns the interval index when a temporary support is erected
   virtual IntervalIndexType GetTemporarySupportErectionInterval(SupportIndexType tsIdx) = 0;

   // returns the interval index when a temporary support is removed
   virtual IntervalIndexType GetTemporarySupportRemovalInterval(SupportIndexType tsIdx) = 0;

   // returns a vector of removal intervals for all the temporary supports in the specified group
   virtual std::vector<IntervalIndexType> GetTemporarySupportRemovalIntervals(GroupIndexType groupIdx) = 0;

   // returns a vector of intervals when user defined loads are applied to this girder
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey) = 0;

   // returns a vector of intervals when user defined loads are applied to this girder
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey,pgsTypes::ProductForceType pfType) = 0;

   // returns a vector of intervals that should be spec checked
   virtual std::vector<IntervalIndexType> GetSpecCheckIntervals(const CGirderKey& girderKey) = 0;
};
