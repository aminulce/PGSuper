///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2017  Washington State Department of Transportation
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

#include <PgsExt\TimelineManager.h>
#include <PgsExt\PointLoadData.h> // for UserLoads

#include <IFace\BeamFactory.h>


////////////////////////////////////////////////
// This class manages the definitions for intervals of time for 
// a time-step loss analysis
class CIntervalManager
{
public:
   CIntervalManager();

   void Init(IBroker* pBroker, StatusGroupIDType statusGroupID);

   // creates the time step intervals from the event model
   void BuildIntervals(const CTimelineManager* pTimelineMgr);

   IntervalIndexType GetIntervalCount() const;
   EventIndexType GetStartEvent(IntervalIndexType idx) const;
   EventIndexType GetEndEvent(IntervalIndexType idx) const;
   Float64 GetTime(IntervalIndexType idx,pgsTypes::IntervalTimeType timeType) const;
   Float64 GetDuration(IntervalIndexType idx) const;
   LPCTSTR GetDescription(IntervalIndexType idx) const;

   // returns the index of the first interval that starts with eventIdx
   IntervalIndexType GetInterval(EventIndexType eventIdx) const;

   IntervalIndexType GetErectPierInterval(PierIndexType pierIdx) const;

   // returns the index of the interval when the prestressing strands are stressed for
   // the first segment constructed for this girder
   IntervalIndexType GetFirstStressStrandInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the prestressing strands are stressed for
   // the last segment constructed for this girder
   IntervalIndexType GetLastStressStrandInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the prestressing strands are stressed
   IntervalIndexType GetStressStrandInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval when the prestressing strands are released for
   // the first segment constructed for this girder
   IntervalIndexType GetFirstPrestressReleaseInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the prestressing strands are released for
   // the last segment constructed for this girder
   IntervalIndexType GetLastPrestressReleaseInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the prestressing is release
   // to the girder (girder has reached release strength)
   IntervalIndexType GetPrestressReleaseInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval when the segment is lifted from the
   // casting bed and placed into storage
   IntervalIndexType GetLiftingInterval(const CSegmentKey& segmentKey) const;
   IntervalIndexType GetFirstLiftingInterval(const CGirderKey& girderKey) const;
   IntervalIndexType GetLastLiftingInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the segment is place in storage
   IntervalIndexType GetStorageInterval(const CSegmentKey& segmentKey) const;
   IntervalIndexType GetFirstStorageInterval(const CGirderKey& girderKey) const;
   IntervalIndexType GetLastStorageInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the segment is transported to the bridge site
   IntervalIndexType GetHaulingInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval for the first segment
   // to be erected
   IntervalIndexType GetFirstSegmentErectionInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval for the last segment
   // to be erected
   IntervalIndexType GetLastSegmentErectionInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the specified segment is erected
   IntervalIndexType GetErectSegmentInterval(const CSegmentKey& segmentKey) const;

   // returns true if a segment is erected in the specified interval
   bool IsSegmentErectionInterval(IntervalIndexType intervalIdx) const;

   // returns true if a segment belonging to the specified girder is erected in the specified interval
   bool IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const;

   // returns the index of the interval when temporary strands are removed
   IntervalIndexType GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey) const;

   // returns the index of the interval when the closure joint is cast
   IntervalIndexType GetCastClosureInterval(const CClosureKey& clousreKey) const;

   // returns the index of the interval when the first closure joint is cast for the specified girder
   IntervalIndexType GetFirstCastClosureJointInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when the last closure joint is cast for the specified girder
   IntervalIndexType GetLastCastClosureJointInterval(const CGirderKey& girderKey) const;

   // returns the index of the interval when intermediate diaphragms are cast
   IntervalIndexType GetCastIntermediateDiaphragmsInterval() const;

   // returns the index of the interval when intermediate diaphragms have finished curing
   // curing takes place of the duration of an interval and cannot take load.
   // this method returns the index of the first interval when the intermediate diaphragms can take load
   IntervalIndexType GetCompositeIntermediateDiaphragmsInterval() const;

   // returns the index of the interval when the deck and diaphragms are cast
   IntervalIndexType GetCastDeckInterval() const;

   // returns the index of the interval when the deck has finished curing
   // curing take place over the duration of an interval and cannot take load.
   // this method returns the index of the first interval when the deck can take load
   IntervalIndexType GetCompositeDeckInterval() const;

   // returns the index of the interval when live load is first
   // applied to the structure. it is assumed that live
   // load can be applied to the structure at this interval and all
   // intervals thereafter
   IntervalIndexType GetLiveLoadInterval() const;

   // returns the index of the interval when the overlay is added to the bridge
   IntervalIndexType GetOverlayInterval() const;

   // returns index of interval when the railing system is installed
   IntervalIndexType GetInstallRailingSystemInterval() const;

   // returns the index of the interval when a user defined load is applied
   IntervalIndexType GetUserLoadInterval(const CSpanKey& spanKey,UserLoads::LoadCase loadCase,LoadIDType userLoadID) const;

   // returns the interval index when a temporary support is erected
   IntervalIndexType GetTemporarySupportErectionInterval(SupportIndexType tsIdx) const;

   // returns the interval index when a temporary support is removed
   IntervalIndexType GetTemporarySupportRemovalInterval(SupportIndexType tsIdx) const;

   // returns the interval index when a tendon is stressed
   IntervalIndexType GetStressTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx) const;

   // returns the interval when the first tendon stressing occurs for the specified girder
   IntervalIndexType GetFirstTendonStressingInterval(const CGirderKey& girderKey) const;

   // returns the interval when the last tendon stressing occurs for the specified girder
   IntervalIndexType GetLastTendonStressingInterval(const CGirderKey& girderKey) const;

protected:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   StatusCallbackIDType m_scidTimelineError;

   bool m_bIsPGSuper; // used to customize labels

   struct CInterval
   {
      CInterval():StartEventIdx(INVALID_INDEX),EndEventIdx(INVALID_INDEX){}
      EventIndexType StartEventIdx; // Event related to the start of this interval
      EventIndexType EndEventIdx;   // Event related to the end of this interval
      Float64        Start;         // Start of interval
      Float64        Middle;        // Middle of interval
      Float64        End;           // End of interval
      Float64        Duration;      // Interval duration
      std::_tstring  Description;   // Description of activity occuring during this interval
   };
   std::vector<CInterval> m_Intervals;
   IntervalIndexType StoreInterval(CInterval& interval);
   void ProcessStep1(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent);
   void ProcessStep2(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent);
   void ProcessStep3(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent);
   void ProcessStep4(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent);
   void ProcessStep5(EventIndexType eventIdx,const CTimelineEvent* pTimelineEvent);

   // returns a list of closure joints that are casting during the timeline event
   std::vector<CClosureKey> GetClosureJoints(const CTimelineEvent* pTimelineEvent);

   std::map<CTendonKey,IntervalIndexType> m_StressTendonIntervals;

   // keeps track of when piers are erected
   std::map<PierIndexType,IntervalIndexType> m_ErectPierIntervals;

   // Keeps track of when temporary supports are erected and removed
   std::map<SupportIndexType,IntervalIndexType> m_ErectTemporarySupportIntervals;
   std::map<SupportIndexType,IntervalIndexType> m_RemoveTemporarySupportIntervals;


   // keep trakc of the interval when key activities occur for each segment
   std::map<CSegmentKey,IntervalIndexType> m_StressStrandIntervals;
   std::map<CSegmentKey,IntervalIndexType> m_ReleaseIntervals;
   std::map<CSegmentKey,IntervalIndexType> m_SegmentHaulingIntervals;
   std::map<CSegmentKey,IntervalIndexType> m_SegmentErectionIntervals;
   std::map<CSegmentKey,IntervalIndexType> m_RemoveTemporaryStrandsIntervals;
   std::map<CClosureKey,IntervalIndexType> m_CastClosureIntervals;
   
   // keeps track of the intervals for other key activities
   IntervalIndexType m_CastIntermediateDiaphragmsIntervalIdx;
   IntervalIndexType m_CastDeckIntervalIdx;
   IntervalIndexType m_CompositeDeckIntervalIdx;
   IntervalIndexType m_LiveLoadIntervalIdx;
   IntervalIndexType m_OverlayIntervalIdx;
   IntervalIndexType m_RailingSystemIntervalIdx;

   // map of when the strands are stressed for the first and last segment constructed for a girder
   std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>> m_StrandStressingSequenceIntervalLimits;

   // map of when the strands are release for the first and last segment constructed for a girder
   std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>> m_ReleaseSequenceIntervalLimits;

   // map of when the first and last segment is erected for a girder
   std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>> m_SegmentErectionSequenceIntervalLimits;

   class CUserLoadKey
   {
   public:
      CUserLoadKey(const CSpanKey& spanKey,LoadIDType loadID);
      CUserLoadKey(const CUserLoadKey& other);
      bool operator<(const CUserLoadKey& other) const;

      CSpanKey m_SpanKey;
      LoadIDType m_LoadID;
   };
   std::map<CUserLoadKey,IntervalIndexType> m_UserLoadInterval[2]; // interval when user DC/DW loads are applied
                                                                   // user LLIM are applied in the live load interval

#if defined _DEBUG
   void AssertValid() const;
#endif
};