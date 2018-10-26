///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2015  Washington State Department of Transportation
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

#include "SegmentModelData.h"
#include <IFace\AnalysisResults.h>
#include <ProductLoadMap.h>

// CSegmentModelManager
//
// Manages the creation, access, and storage of analysis models of precast segments.
// This are the models of the individual segments that are used before they are erected
// into the bridge and can be modeled with the LBAM.

class CSegmentModelManager
{
public:
   CSegmentModelManager(SHARED_LOGFILE lf,IBroker* pBroker);

   void Clear();

   void DumpAnalysisModels(GirderIndexType gdrIdx);

   std::vector<EquivPretensionLoad> GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType);

   Float64 GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType);
   sysSectionValue GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType);
   Float64 GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType);
   Float64 GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType);
   Float64 GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType);
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot);
   Float64 GetReaction(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType);

   // IReactions
   void GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,ResultsType resultsType,Float64* pRleft,Float64* pRright);
   void GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,ResultsType resultsType,Float64* pRleft,Float64* pRright);
   void GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,Float64* pRleftMin,Float64* pRleftMax,Float64* pRrightMin,Float64* pRrightMax);

   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   std::vector<Float64> GetUnitLoadMoment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,const pgsPointOfInterest& unitLoadPOI);
   std::vector<sysSectionValue> GetUnitCoupleMoment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,const pgsPointOfInterest& unitMomentPOI);

   sysSectionValue GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType);
   Float64 GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType);
   Float64 GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType);
   Float64 GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType);
   Float64 GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType comboType,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType);
   void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot);

   std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   void GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,sysSectionValue* pMin,sysSectionValue* pMax);
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,Float64* pMin,Float64* pMax);
   void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,bool bIncludePrestress,Float64* pMin,Float64* pMax);
   void GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,PierIndexType pierIdx,const CGirderKey& girderKey,bool bIncludeImpact,Float64* pMin,Float64* pMax);
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,Float64* pMin,Float64* pMax);

   void GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax);
   void GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   void GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   void GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax);
   void GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax);

   // IExternalLoad
   bool CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName);
   bool AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType comboType);
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
   std::vector<Float64> GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<sysSectionValue> GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<Float64> GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<Float64> GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   std::vector<Float64> GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType);
   Float64 GetReaction(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType);
   void GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);

private:
	DECLARE_SHARED_LOGFILE;
   IBroker* m_pBroker; // must be a weak reference (this is the agent's pointer and it is a weak refernece)

   CProductLoadMap m_ProductLoadMap;

   typedef std::map<CSegmentKey,CSegmentModelData> SegmentModels;
   SegmentModels m_ReleaseModels;
   SegmentModels m_StorageModels;

   typedef std::multimap<LoadingCombinationType,std::_tstring> LoadCombinationMap;
   std::map<CGirderKey,LoadCombinationMap> m_LoadCombinationMaps;
   LoadCombinationMap& GetLoadCombinationMap(const CGirderKey& girderKey);

   void GetPrestressSectionStresses(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);
   void GetPrestressSectionResults(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz);
   void GetPrestressSectionResults(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz);


   void GetSectionResults(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz);
   void GetSectionResults(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz);
   void GetSectionResults(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz);

   void GetSectionStresses(IntervalIndexType intervalIdx,LoadCaseIDType lcid,ResultsType resultsType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot);
   void GetSectionStress(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot);

   void GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadCaseIDType lcid,ResultsType resultsType,Float64* pRleft,Float64* pRright);
   Float64 GetReaction(IntervalIndexType intervalIdx,LoadCaseIDType lcid,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType);
   Float64 GetReaction(IntervalIndexType intervalIdx,LoadCaseIDType lcid,PierIndexType pierIdx,const CGirderKey& girderKey);


   PoiIDType AddPointOfInterest(CSegmentModelData* pModelData,const pgsPointOfInterest& poi);
   void ZeroResults(const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz);


   CSegmentModelData* GetSegmentModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx);


   void BuildReleaseModel(const CSegmentKey& segmentKey);
   void BuildStorageModel(const CSegmentKey& segmentKey);

   CSegmentModelData* GetReleaseModel(const CSegmentKey& segmentKey);
   CSegmentModelData* GetStorageModel(const CSegmentKey& segmentKey);

   CSegmentModelData BuildSegmentModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 leftSupportDistance,Float64 rightSupportDistance,PoiAttributeType refAttribute);
   PoiIDType AddPointOfInterest(SegmentModels& models,const pgsPointOfInterest& poi);

   void ApplyPretensionLoad(CSegmentModelData* pModelData,const CSegmentKey& segmentKey);

   CSegmentModelData* GetModelData(SegmentModels& models,const CSegmentKey& segmentKey);

   LoadCaseIDType GetLoadCaseID(pgsTypes::ProductForceType pfType);
   LoadCaseIDType GetLoadCaseID(pgsTypes::StrandType strandType);
   LoadCaseIDType GetLoadCaseID(CSegmentModelData* pModelData,LPCTSTR strLoadingName);
   LoadCaseIDType GetFirstExternalLoadCaseID();

   void GetMemberLocation(const pgsPointOfInterest& poi,CSegmentModelData* pModelData,MemberIDType* pMbrID,Float64* pLocation);

   CSegmentKey GetSegmentKey(const CGirderKey& girderKey,PierIndexType pierIdx);

   void AddLoading(CSegmentModelData& model_data,pgsTypes::ProductForceType pfType);

   bool CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz);
   bool CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy);
   bool CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r);
};
