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

#include "stdafx.h"
#include "resource.h"
#include "TimeStepLossEngineer.h"
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\LoadFactors.h>

#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\ReportNotes.h>
#include <PgsExt\GirderLabel.h>

#include <EAF\EAFStatusCenter.h>
#include <PgsExt\StatusItem.h>
#include "..\PGSuperException.h"

#include <Details.h>

#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimeStepLossEngineer
HRESULT CTimeStepLossEngineer::FinalConstruct()
{
   m_Bat = pgsTypes::ContinuousSpan;

   return S_OK;
}

void CTimeStepLossEngineer::FinalRelease()
{
}

void CTimeStepLossEngineer::SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidProjectCriteria = pStatusCenter->RegisterCallback( new pgsProjectCriteriaStatusCallback(pBroker) );
}

const LOSSDETAILS* CTimeStepLossEngineer::GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx)
{
   GET_IFACE(ILossParameters,pLossParameters);
   if ( pLossParameters->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      std::_tstring msg(_T("Prestress losses cannot be computed. Use Project Criteria that specifies the time-step method for prestress loss calculations."));
      
      pgsProjectCriteriaStatusItem* pStatusItem = new pgsProjectCriteriaStatusItem(m_StatusGroupID,m_scidProjectCriteria,msg.c_str());

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->Add(pStatusItem);

      msg += std::_tstring(_T("\nSee Status Center for Details"));
      THROW_UNWIND(msg.c_str(),XREASON_PROJECT_CRITERIA);
   }

   CGirderKey girderKey(poi.GetSegmentKey());

   if ( intervalIdx == INVALID_INDEX )
   {
      // INVALID_INDEX means compute losses for all intervals
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
      intervalIdx = nIntervals-1;
   }

   // at the end of the analysis, this is the number of intervals that should be analyzed
   IntervalIndexType nIntervalsToBeAnalyzed = intervalIdx+1;

   // Have losses been computed for this girder?
   std::map<CGirderKey,LOSSES>::iterator found;
   found = m_Losses.find( girderKey );
   if ( found == m_Losses.end() )
   {
      // No, compute them now for all intervals up to and including intervalIdx
      ComputeLosses(girderKey,intervalIdx);

      found = m_Losses.find( girderKey );
      ATLASSERT(found != m_Losses.end());
   }
   else
   {
      // Yes! Losses have been computed for some intervals

      // Have losses been computed up to and including the requested interval?
      // Losses at all POI are computed to the same interval, so check the first loss record
      // to see how many intervals have previously been analyzed

      LOSSES& losses = found->second;
      SectionLossContainer::iterator iter = losses.SectionLosses.begin();
      LOSSDETAILS& details = iter->second;
      IntervalIndexType nIntervalsPreviouslyAnalyzed = details.TimeStepDetails.size();
      ATLASSERT(0 < nIntervalsPreviouslyAnalyzed);
      if ( nIntervalsPreviouslyAnalyzed < nIntervalsToBeAnalyzed )
      {
         // Losses have not been computed all the way up to and including the requested interval.
         
         // Get the loss objects for this girderline
         GET_IFACE(IBridgeDescription,pIBridgeDesc);
         std::vector<LOSSES*> vpLosses;
         GroupIndexType nGroups = pIBridgeDesc->GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            GirderIndexType nGirdersThisGroup = pIBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
            GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirdersThisGroup-1);
            CGirderKey thisGirderKey(grpIdx,gdrIdx);
            
            std::map<CGirderKey,LOSSES>::iterator foundForThisGirder = m_Losses.find( thisGirderKey );
            ATLASSERT(foundForThisGirder != m_Losses.end());

            LOSSES* pLosses = &(foundForThisGirder->second);
            vpLosses.push_back(pLosses);
         }

         // some intervals have been analyzed, but not all the way up to and including
         // intervalIdx. continue the analysis up to and including intervalIdx.
         ComputeLosses(girderKey.girderIndex,intervalIdx,&vpLosses);
      }
   }

   // Get the loss record for the POI we are interested in
   SectionLossContainer& losses(found->second.SectionLosses);
   SectionLossContainer::iterator poiFound = losses.find(poi);

   if ( poiFound == losses.end() )
   {
      // couldn't find the POI we are interested in... need to approximate results

      ATLASSERT( poi.GetID() == INVALID_ID || 
                 poi.IsHarpingPoint()      || 
                 poi.HasAttribute(POI_CRITSECTSHEAR1) || 
                 poi.HasAttribute(POI_CRITSECTSHEAR2) || 
                 poi.HasAttribute(POI_LIFT_SEGMENT) ||
                 poi.HasAttribute(POI_HAUL_SEGMENT)
                );
      // losses were not computed at the POI we are looking for. This happens for a couple of reasons.
      // 1) The POI was created on the fly (its ID is INVALID_ID)
      // 2) Harp Points -> we tweak the position of the harp point so we get the results on the side
      //    with the sloped strands
      // 3) Critical Section for Shear -> We don't know where these locations are prior to computing losses. Losses
      //    are needed to compute this locations
      // 4) Lifting and Hauling POI - these change with changing locations of the pick and bunk points

      // approximate the losses at the poi by using the losses at the nearest poi
      // (could use linear interpolation, however, take a look at the LOSSDETAILS struct...
      // it would be a royal pain to interpolate all the values)
      SectionLossContainer::iterator iter1(losses.begin());
      SectionLossContainer::iterator iter2(losses.begin());
      iter2++;
      SectionLossContainer::iterator end(losses.end());
      for ( ; iter2 != end; iter1++, iter2++ )
      {
         const pgsPointOfInterest& poi1(iter1->first);
         const pgsPointOfInterest& poi2(iter2->first);
         if ( poi1 <= poi && poi < poi2 )
         {
            if ( poi.GetDistFromStart() - poi1.GetDistFromStart() < poi2.GetDistFromStart() - poi.GetDistFromStart() )
            {
               // poi is closer to poi1 than poi2
               const LOSSDETAILS& details = iter1->second;
               return &details;
            }
            else
            {
               // poi is closer to poi2 than poi1
               const LOSSDETAILS& details = iter2->second;
               return &details;
            }
         }
      }
   }

   ATLASSERT( poiFound != losses.end() );
   const LOSSDETAILS* pLossDetails = &(*poiFound).second;
   ATLASSERT(intervalIdx < pLossDetails->TimeStepDetails.size());
   return pLossDetails;
}

const LOSSDETAILS* CTimeStepLossEngineer::GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx)
{
   ATLASSERT(false); // not doing design with Time Step method... therefore this should never be called
   return NULL;
}

void CTimeStepLossEngineer::ClearDesignLosses()
{
   ATLASSERT(false); // should not get called... not doing design with time-step method
}

const ANCHORSETDETAILS* CTimeStepLossEngineer::GetAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx)
{
   ComputeLosses(girderKey,0);
   std::map<CGirderKey,LOSSES>::const_iterator found;
   found = m_Losses.find( girderKey );
   ATLASSERT(found != m_Losses.end());
   return &(*found).second.AnchorSet[ductIdx];
}

Float64 CTimeStepLossEngineer::GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType)
{
   ComputeLosses(girderKey,0);
   std::map<CGirderKey,LOSSES>::const_iterator found;
   found = m_Losses.find( girderKey );
   ATLASSERT(found != m_Losses.end());
   const LOSSES& losses = found->second;

   CTendonKey tendonKey(girderKey,ductIdx);

#if defined _DEBUG
   // Elongation calculations are kind of tricky... they are computed with friction losses
   // Re-compute them here using a more direct method and compare to what was computed before.
   GET_IFACE(IBridgeDescription,pBridgeDesc);
   GET_IFACE(IGirder,pIGirder);
   const CSplicedGirderData* pGirder = pBridgeDesc->GetGirder(girderKey);
   const CPTData* pPTData = pGirder->GetPostTensioning();
   WebIndexType nWebs = pIGirder->GetWebCount(girderKey);
   const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);

   Float64 elongation = 0;

   if ( (pDuct->JackingEnd == pgsTypes::jeStart  && endType == pgsTypes::metStart) ||
        (pDuct->JackingEnd == pgsTypes::jeEnd && endType == pgsTypes::metEnd) ||
        pDuct->JackingEnd == pgsTypes::jeBoth 
      )
   {
      Float64 Pj;
      if ( pDuct->bPjCalc )
      {
         GET_IFACE(IPosttensionForce,pPTForce);
         Pj = pPTForce->GetPjackMax(girderKey,pDuct->nStrands);
      }
      else
      {
         Pj = pDuct->Pj;
      }

      Float64 apt = pPTData->pStrand->GetNominalArea();
      StrandIndexType nStrands = pDuct->nStrands;
      Float64 Apt = apt*nStrands;

      GET_IFACE(IMaterials,pMaterials);
      Float64 Ept = pMaterials->GetTendonMaterial(girderKey)->GetE();

      // this is iterating by POI
      GET_IFACE(IPointOfInterest,pPoi);
      SectionLossContainer::const_iterator sectionLossIter(losses.SectionLosses.begin());
      while ( !pPoi->IsOnGirder(sectionLossIter->first) && sectionLossIter != losses.SectionLosses.end() )
      {
         sectionLossIter++;
      }

      const LOSSDETAILS& lossDetails = sectionLossIter->second;
      Float64 X1    = lossDetails.FrictionLossDetails[ductIdx].X;
      Float64 dfpF1 = lossDetails.FrictionLossDetails[ductIdx].dfpF;

      sectionLossIter++;

      SectionLossContainer::const_iterator sectionLossIterEnd(losses.SectionLosses.end());
      for ( ; sectionLossIter != sectionLossIterEnd; sectionLossIter++ )
      {
         if ( pPoi->IsOnGirder(sectionLossIter->first) )
         {
            const LOSSDETAILS& lossDetails = sectionLossIter->second;
            Float64 X2    = lossDetails.FrictionLossDetails[ductIdx].X;
            Float64 dfpF2 = lossDetails.FrictionLossDetails[ductIdx].dfpF;

            Float64 dX = X2 - X1;
            Float64 dP = Pj - 0.5*Apt*(dfpF2 + dfpF1);
            Float64 dElongation = dP*dX;
            elongation += dElongation;

            // get ready for next time throught hte loop
            X1 = X2;
            dfpF1 = dfpF2;
         }
      }

      elongation /= (Ept*Apt);

      if ( pDuct->JackingEnd == pgsTypes::jeBoth )
      {
         // half of total elongation at each end
         elongation /= 2;
      }
   }

   Float64 e = (endType == pgsTypes::metStart ? m_Elongation[tendonKey].first : m_Elongation[tendonKey].second);
//#if defined _BETA_VERSION
//   if ( !IsEqual(elongation,e) )
//   {
//      CString strMsg;
//      strMsg.Format(_T("Elongation 1 = %s, Elongation 2 = %s"),
//         ::FormatDimension(elongation,m_pDisplayUnits->GetComponentDimUnit()),
//         ::FormatDimension(e,m_pDisplayUnits->GetComponentDimUnit()));
//      AfxMessageBox(strMsg);
//   }
//#endif // _BETA_RELEASE

   ATLASSERT(IsEqual(elongation,e));
#endif // _DEBUG

   return (endType == pgsTypes::metStart ? m_Elongation[tendonKey].first : m_Elongation[tendonKey].second);
}

void CTimeStepLossEngineer::GetAverageFrictionAndAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx,Float64* pfpF,Float64* pfpA)
{
   CTendonKey tendonKey(girderKey,ductIdx);

   std::map<CTendonKey,std::pair<Float64,Float64>>::iterator found = m_AvgFrictionAndAnchorSetLoss.find(tendonKey);
   if ( found == m_AvgFrictionAndAnchorSetLoss.end() )
   {
      ComputeLosses(girderKey,0);
      found = m_AvgFrictionAndAnchorSetLoss.find(tendonKey);
      ATLASSERT(found != m_AvgFrictionAndAnchorSetLoss.end());
   }

   *pfpF = found->second.first;
   *pfpA = found->second.second;
}

void CTimeStepLossEngineer::ComputeLosses(const CGirderKey& girderKey,IntervalIndexType endAnalysisIntervalIdx)
{
   std::map<CGirderKey,LOSSES>::const_iterator found;
   found = m_Losses.find( girderKey );
   if ( found == m_Losses.end() )
   {
      // Time-step losses must be computed for the entire girder line at once

      // create the loss objects for this girderline
      std::vector<LOSSES*> vpLosses;
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      GroupIndexType nGroups = pIBridgeDesc->GetGirderGroupCount();
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         GirderIndexType nGirders = pIBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
         GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);

         CGirderKey thisGirderKey(grpIdx,gdrIdx);
         LOSSES losses;
         std::pair<std::map<CGirderKey,LOSSES>::iterator,bool> result;
         result = m_Losses.insert(std::make_pair(thisGirderKey,losses)); // losses is copied when inserted
         ATLASSERT(result.second == true);
         LOSSES* pLosses = &(result.first->second);
         vpLosses.push_back(pLosses);
      }

      ComputeLosses(girderKey.girderIndex,endAnalysisIntervalIdx,&vpLosses);
   }
}

void CTimeStepLossEngineer::ComputeLosses(GirderIndexType girderLineIdx,IntervalIndexType endAnalysisIntervalIdx,std::vector<LOSSES*>* pvpLosses)
{
   if ( m_pProgress )
   {
      return;
   }

   // Get frequently used interfaces
   // NOTE: We can't get this interfaces and hold them for the lifetime of this object
   // because it creates circular references. As a result there are massive memory leaks
   // Get them here, do the full timestep analysis, then release them.
   m_pBroker->GetInterface(IID_IProgress,          (IUnknown**)&m_pProgress);
   m_pBroker->GetInterface(IID_IBridgeDescription, (IUnknown**)&m_pBridgeDesc);
   m_pBroker->GetInterface(IID_IBridge,            (IUnknown**)&m_pBridge);
   m_pBroker->GetInterface(IID_IStrandGeometry,    (IUnknown**)&m_pStrandGeom);
   m_pBroker->GetInterface(IID_ITendonGeometry,    (IUnknown**)&m_pTendonGeom);
   m_pBroker->GetInterface(IID_IIntervals,         (IUnknown**)&m_pIntervals);
   m_pBroker->GetInterface(IID_ISectionProperties, (IUnknown**)&m_pSectProp);
   m_pBroker->GetInterface(IID_IGirder,            (IUnknown**)&m_pGirder);
   m_pBroker->GetInterface(IID_IMaterials,         (IUnknown**)&m_pMaterials);
   m_pBroker->GetInterface(IID_IPretensionForce,   (IUnknown**)&m_pPSForce);
   m_pBroker->GetInterface(IID_IPosttensionForce,  (IUnknown**)&m_pPTForce);
   m_pBroker->GetInterface(IID_ILossParameters,    (IUnknown**)&m_pLossParams);
   m_pBroker->GetInterface(IID_IPointOfInterest,   (IUnknown**)&m_pPoi);
   m_pBroker->GetInterface(IID_ILongRebarGeometry, (IUnknown**)&m_pRebarGeom);
   m_pBroker->GetInterface(IID_IProductLoads,      (IUnknown**)&m_pProductLoads);
   m_pBroker->GetInterface(IID_IProductForces,     (IUnknown**)&m_pProductForces);
   m_pBroker->GetInterface(IID_ICombinedForces,    (IUnknown**)&m_pCombinedForces);
   m_pBroker->GetInterface(IID_IExternalLoading,   (IUnknown**)&m_pExternalLoading);
   m_pBroker->GetInterface(IID_IEAFDisplayUnits,   (IUnknown**)&m_pDisplayUnits);

   {
      // scope the use all the interface points accessed above and released below

      CEAFAutoProgress ap(m_pProgress);
      m_pProgress->UpdateMessage(_T("Computing prestress losses"));

      m_StrandTypes.clear();

      GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();
      ATLASSERT(pvpLosses->size() == nGroups);
      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         GirderIndexType nGirdersThisGroup = m_pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
         GirderIndexType gdrIdx = Min(girderLineIdx,nGirdersThisGroup-1);
         CGirderKey girderKey(grpIdx,gdrIdx);

         LOSSES* pLosses = (*pvpLosses)[grpIdx];

         if ( pLosses->SectionLosses.size() == 0 )
         {
            // if this is the first time analyzing this losses, compute friction and anchor set losses
            ComputeFrictionLosses( girderKey,pLosses);
            ComputeAnchorSetLosses(girderKey,pLosses);
         }
      }

      ComputeSectionLosses(girderLineIdx,endAnalysisIntervalIdx,pvpLosses);
   }

   m_pProgress.Release();
   m_pBridgeDesc.Release();
   m_pBridge.Release();
   m_pStrandGeom.Release();
   m_pTendonGeom.Release();
   m_pIntervals.Release();
   m_pSectProp.Release();
   m_pGirder.Release();
   m_pMaterials.Release();
   m_pPSForce.Release();
   m_pPTForce.Release();
   m_pLossParams.Release();
   m_pPoi.Release();
   m_pRebarGeom.Release();
   m_pProductLoads.Release();
   m_pProductForces.Release();
   m_pCombinedForces.Release();
   m_pExternalLoading.Release();
   m_pDisplayUnits.Release();
}

void CTimeStepLossEngineer::ComputeFrictionLosses(const CGirderKey& girderKey,LOSSES* pLosses)
{
   CString strMsg;
   strMsg.Format(_T("Computing friction losses for tendons in Group %d"),LABEL_GROUP(girderKey.groupIndex));
   m_pProgress->UpdateMessage(strMsg);

   Float64 Dset, wobble, friction;
   m_pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   std::vector<pgsPointOfInterest> vPoi(m_pPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));

   WebIndexType nWebs = m_pGirder->GetWebCount(girderKey);

   const CSplicedGirderData* pGirder = m_pBridgeDesc->GetGirder(girderKey);
   const CPTData* pPTData = pGirder->GetPostTensioning();
   DuctIndexType nDucts = m_pTendonGeom->GetDuctCount(girderKey);
   Float64 Lg = m_pBridge->GetGirderLength(girderKey);

   std::vector<pgsPointOfInterest>::iterator begin(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi(*iter);

      LOSSDETAILS details;
      details.FrictionLossDetails.reserve(nDucts);
#if defined _DEBUG
      details.POI = poi;
#endif

      details.LossMethod = pgsTypes::TIME_STEP;

      bool bIsOnGirder = m_pPoi->IsOnGirder(poi);

      if ( bIsOnGirder )
      {
         //////////////////////////////////////////////////////////////////////////
         // Friction Losses
         //////////////////////////////////////////////////////////////////////////
         Float64 Xg = m_pPoi->ConvertPoiToGirderCoordinate(poi); // distance along girder

         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            FRICTIONLOSSDETAILS frDetails;

            const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);
            Float64 Pj;
            if ( pDuct->bPjCalc )
            {
               Pj = m_pPTForce->GetPjackMax(girderKey,pDuct->nStrands);
            }
            else
            {
               Pj = pDuct->Pj;
            }

            Float64 aps = pPTData->pStrand->GetNominalArea();
            StrandIndexType nStrands = pDuct->nStrands;
            Float64 Aps = aps*nStrands;
            Float64 fpj = (nStrands == 0 ? 0 : Pj/Aps);
            
            // determine from which end of the girder to measure the angular change of the tendon path
            pgsTypes::MemberEndType endType;
            if ( pDuct->JackingEnd == pgsTypes::jeStart )
            {
               endType = pgsTypes::metStart;
            }
            else if ( pDuct->JackingEnd == pgsTypes::jeEnd )
            {
               endType = pgsTypes::metEnd;
            }
            else
            {
               // jacked from both ends.... if Xg < Lg/2, measure from start, otherwise from the end
               endType = (Xg < Lg/2) ? pgsTypes::metStart : pgsTypes::metEnd;
            }

            Float64 X = Xg; // distance from stressing end
            if ( endType == pgsTypes::metEnd )
            {
               X = Lg - Xg;
            }

            Float64 alpha = m_pTendonGeom->GetAngularChange(poi,ductIdx,endType);

            frDetails.X = Xg;
            frDetails.alpha = alpha;
            frDetails.dfpF = fpj*(1 - exp(-(friction*alpha + X*wobble)));

            // calculation incremental elongation at this poi... it will
            // be summed with previously computed increments to get the total
            // elongation
            CTendonKey tendonKey(girderKey,ductIdx);
            if ( iter == begin || pLosses->SectionLosses[*(iter-1)].FrictionLossDetails.size() == 0 )
            {
               // first poi that is on the girder ... initialize with zero
               m_Elongation.insert(std::make_pair(tendonKey,std::make_pair(0,0)));
            }
            else
            {
               pgsPointOfInterest& prevPoi(*(iter-1));
               Float64 dX = frDetails.X - pLosses->SectionLosses[prevPoi].FrictionLossDetails[ductIdx].X;
               Float64 df = fpj - 0.5*(frDetails.dfpF + pLosses->SectionLosses[prevPoi].FrictionLossDetails[ductIdx].dfpF);
               m_Elongation[tendonKey].first += df*dX;
            }

            details.FrictionLossDetails.push_back(frDetails);
         } // next duct
      }
      else
      {
         // poi is not on girder.... just add a place holder
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            FRICTIONLOSSDETAILS frDetails;
            frDetails.alpha = 0;
            frDetails.X = 0;
            frDetails.dfpF = 0;
            frDetails.dfpA = 0;

            details.FrictionLossDetails.push_back(frDetails);
         } // next duct
      } // end if bIsOnGirder

      pLosses->SectionLosses.insert(std::make_pair(poi,details));
   } // next poi

   // For elongation calculations we have the sum of (PL/A)... we need (PL/AE)
   // Elongation is shoved into array for pgsTypes::metStart for easy of computation
   // now make sure the elongation is in the correct position of the array for
   // the actual jacking condition
   Float64 Ept = m_pMaterials->GetTendonMaterial(girderKey)->GetE();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      CTendonKey tendonKey(girderKey,ductIdx);
      m_Elongation[tendonKey].first /= Ept;
      const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);
      if ( pDuct->JackingEnd == pgsTypes::jeStart )
      {
         // jacking from left end, no elongation at end
         m_Elongation[tendonKey].second = 0;
      }
      else if ( pDuct->JackingEnd == pgsTypes::jeEnd )
      {
         // jacking from right end, no elongation at start
         m_Elongation[tendonKey].second = m_Elongation[tendonKey].first;
         m_Elongation[tendonKey].first = 0;
      }
      else
      {
         // jacking from both ends at the same time... elongation is split equally
         m_Elongation[tendonKey].first *= 0.5;
         m_Elongation[tendonKey].second = m_Elongation[tendonKey].first;
      }
   }
}

void CTimeStepLossEngineer::ComputeAnchorSetLosses(const CGirderKey& girderKey,LOSSES* pLosses)
{
   CString strMsg;
   strMsg.Format(_T("Computing anchor set losses for tendons in Group %d"),LABEL_GROUP(girderKey.groupIndex));
   m_pProgress->UpdateMessage(strMsg);

   // First, compute the seating wedge, then compute anchor set loss at each POI
   Float64 girder_length = m_pBridge->GetGirderLength(girderKey);

   const CSplicedGirderData* pGirder = m_pBridgeDesc->GetGirder(girderKey);
   const CPTData* pPTData = pGirder->GetPostTensioning();

   Float64 Dset, wobble, friction;
   m_pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   WebIndexType nWebs = m_pGirder->GetWebCount(girderKey);


#pragma Reminder("UPDATE: this assumes that the PT starts/ends at the face of girder")
   // the input allows the PT to start and end at any arbitrary point along the girder

   DuctIndexType nDucts = m_pTendonGeom->GetDuctCount(girderKey);
   pLosses->AnchorSet.reserve(nDucts);

   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);

      // find location of minimum friction loss. this is the location of no movement in the strand
      SectionLossContainer::iterator frMinIter;
      if ( pDuct->JackingEnd == pgsTypes::jeStart )
      {
         // jack at left end, no movement occurs at right end
         frMinIter = pLosses->SectionLosses.end();
         frMinIter--;
      }
      else if ( pDuct->JackingEnd == pgsTypes::jeEnd )
      {
         // jack at right end, no movement occurs at left end
         frMinIter = pLosses->SectionLosses.begin();
      }
      else
      {
         // jack at both ends, no movement occurs somewhere in the middle... search for it
         SectionLossContainer::iterator iter(pLosses->SectionLosses.begin());
         SectionLossContainer::iterator end(pLosses->SectionLosses.end());
         Float64 dfpF_Prev = iter->second.FrictionLossDetails[ductIdx].dfpF;
         iter++;
         for ( ; iter != end; iter++ )
         {
            Float64 dfpF_Curr = iter->second.FrictionLossDetails[ductIdx].dfpF;
            if ( dfpF_Curr < dfpF_Prev )
            {
               // the current friction loss is less than the previous friction loss... 
               // that means we have gone past the POI where the min value occurs.
               frMinIter = --iter; // back up one
               break;
            }

            dfpF_Prev = dfpF_Curr;
         }
      }

      ANCHORSETDETAILS anchor_set;
      anchor_set.girderKey = girderKey;
      anchor_set.ductIdx   = ductIdx;

      anchor_set.Lset[pgsTypes::metStart]  = 0;
      anchor_set.dfpAT[pgsTypes::metStart] = 0;
      anchor_set.dfpS[pgsTypes::metStart]  = 0;

      anchor_set.Lset[pgsTypes::metEnd]  = 0;
      anchor_set.dfpAT[pgsTypes::metEnd] = 0;
      anchor_set.dfpS[pgsTypes::metEnd]  = 0;

      if ( 0 < frMinIter->second.FrictionLossDetails.size() )
      {
         if ( pDuct->JackingEnd == pgsTypes::jeStart || pDuct->JackingEnd == pgsTypes::jeBoth )
         {
            Float64 dfpAT, dfpS, Xset;
            ComputeAnchorSetLosses(pPTData,pDuct,ductIdx,pgsTypes::metStart,pLosses,girder_length,frMinIter,&dfpAT,&dfpS,&Xset);
            anchor_set.Lset[pgsTypes::metStart]  = Min(girder_length,Xset);
            anchor_set.dfpAT[pgsTypes::metStart] = dfpAT;
            anchor_set.dfpS[pgsTypes::metStart]  = dfpS;
         }

         if ( pDuct->JackingEnd == pgsTypes::jeEnd || pDuct->JackingEnd == pgsTypes::jeBoth  )
         {
            Float64 dfpAT, dfpS, Xset;
            ComputeAnchorSetLosses(pPTData,pDuct,ductIdx,pgsTypes::metEnd,pLosses,girder_length,frMinIter,&dfpAT,&dfpS,&Xset);
            anchor_set.Lset[pgsTypes::metEnd]  = Min(girder_length,girder_length - Xset);
            anchor_set.dfpAT[pgsTypes::metEnd] = dfpAT;
            anchor_set.dfpS[pgsTypes::metEnd]  = dfpS;
         }
      }

      pLosses->AnchorSet.push_back(anchor_set);
   }

   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      ANCHORSETDETAILS& anchorSetDetails( pLosses->AnchorSet[ductIdx] );

      // sum of friction and anchor set losses along the tendon (for computing average value)
      Float64 Sum_dfpF = 0;
      Float64 Sum_dfpA = 0;

      IndexType nPoints = 0;
      SectionLossContainer::iterator iter(pLosses->SectionLosses.begin());
      SectionLossContainer::iterator end(pLosses->SectionLosses.end());
      for ( ; iter != end; iter++ )
      {
         const pgsPointOfInterest& poi(iter->first);

         bool bIsOnGirder = m_pPoi->IsOnGirder(poi);

         if ( bIsOnGirder )
         {
            LOSSDETAILS& details(iter->second);

            FRICTIONLOSSDETAILS& frDetails(details.FrictionLossDetails[ductIdx]);

            Float64 dfpA[2] = {0,0};
            const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);
            if ( frDetails.X <= anchorSetDetails.Lset[pgsTypes::metStart] )
            {
               // POI is in the left anchorage zone
               if ( IsZero(anchorSetDetails.Lset[pgsTypes::metStart]) )
               {
                  dfpA[pgsTypes::metStart] = 0;
               }
               else
               {
                  dfpA[pgsTypes::metStart] = ::LinInterp(frDetails.X,anchorSetDetails.dfpAT[pgsTypes::metStart],anchorSetDetails.dfpS[pgsTypes::metStart],anchorSetDetails.Lset[pgsTypes::metStart]);
               }
            }

            if ( girder_length-anchorSetDetails.Lset[pgsTypes::metEnd] <= frDetails.X )
            {
               // POI is in the right anchorage zone
               if ( IsZero(anchorSetDetails.Lset[pgsTypes::metEnd]) )
               {
                  dfpA[pgsTypes::metEnd] = 0;
               }
               else
               {
                  dfpA[pgsTypes::metEnd] = ::LinInterp(anchorSetDetails.Lset[pgsTypes::metEnd] - (girder_length-frDetails.X),anchorSetDetails.dfpS[pgsTypes::metEnd],anchorSetDetails.dfpAT[pgsTypes::metEnd],anchorSetDetails.Lset[pgsTypes::metEnd]);
               }
            }

            frDetails.dfpA = dfpA[pgsTypes::metStart] + dfpA[pgsTypes::metEnd];

            Sum_dfpF += frDetails.dfpF;
            Sum_dfpA += frDetails.dfpA;

            nPoints++;
         } // end if bIsOnGirder
      } // next poi
      
      Float64 dfpF = (nPoints == 0 ? 0 : Sum_dfpF/nPoints);
      Float64 dfpA = (nPoints == 0 ? 0 : Sum_dfpA/nPoints);

      CTendonKey tendonKey(girderKey,ductIdx);
      m_AvgFrictionAndAnchorSetLoss.insert(std::make_pair(tendonKey,std::make_pair(dfpF,dfpA)));
   } // next duct
}

void CTimeStepLossEngineer::ComputeSectionLosses(GirderIndexType girderLineIdx,IntervalIndexType endAnalysisIntervalIdx,std::vector<LOSSES*>* pvpLosses)
{
   bool bIgnoreCreepEffects      = m_pLossParams->IgnoreCreepEffects();
   bool bIgnoreShrinkageEffects  = m_pLossParams->IgnoreShrinkageEffects();
   bool bIgnoreRelaxationEffects = m_pLossParams->IgnoreRelaxationEffects();
   bool bIgnoreTimeDependentEffects = bIgnoreCreepEffects && bIgnoreShrinkageEffects && bIgnoreRelaxationEffects;

   IntervalIndexType nIntervals = m_pIntervals->GetIntervalCount();
   IntervalIndexType startAnalysisIntervalIdx = (*pvpLosses)[0]->SectionLosses.begin()->second.TimeStepDetails.size();

   GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();

   if ( startAnalysisIntervalIdx == 0 )
   {
      // only want to do this once...
      // ASSERTS are going to fire in GirderModelManager::AddLoadGroup because this loading has a name
      // that is already used... that's ok and it is exactly what we want.
      m_pExternalLoading->CreateLoading(girderLineIdx,m_pProductLoads->GetProductLoadName(pftCreep));
      m_pExternalLoading->CreateLoading(girderLineIdx,m_pProductLoads->GetProductLoadName(pftShrinkage));
      m_pExternalLoading->CreateLoading(girderLineIdx,m_pProductLoads->GetProductLoadName(pftRelaxation));
   }

   for ( IntervalIndexType intervalIdx = startAnalysisIntervalIdx; intervalIdx <= endAnalysisIntervalIdx; intervalIdx++ )
   {
      CString strMsg;
      strMsg.Format(_T("Performing time-step analysis: Interval %d of %d"),LABEL_INTERVAL(intervalIdx),nIntervals);
      m_pProgress->UpdateMessage(strMsg);

      for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
      {
         GirderIndexType nGirdersThisGroup = m_pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
         GirderIndexType gdrIdx = Min(girderLineIdx,nGirdersThisGroup-1);
         CGirderKey girderKey(grpIdx,gdrIdx);

         LOSSES* pLosses = (*pvpLosses)[grpIdx];

         IntervalIndexType releaseIntervalIdx = m_pIntervals->GetFirstPrestressReleaseInterval(girderKey);

         SectionLossContainer::iterator iter(pLosses->SectionLosses.begin());
         SectionLossContainer::iterator end(pLosses->SectionLosses.end());

         // Initialize Time Step Analysis
         // Basically computes the restraining forces at each POI
         for ( ; iter != end; iter++ )
         {
            LOSSDETAILS& details(iter->second);
            const pgsPointOfInterest& poi(iter->first);

            InitializeTimeStepAnalysis(intervalIdx,poi,details);
         }

         if ( !bIgnoreTimeDependentEffects && 0 < m_pIntervals->GetDuration(intervalIdx) && releaseIntervalIdx <= intervalIdx )
         {
            AnalyzeInitialStrains(intervalIdx,girderKey,pLosses);
         }

         // Finalize Time Step Analysis
         // Basically computes the resultant strains and rotations at each POI
         iter = pLosses->SectionLosses.begin();
         for ( ; iter != end; iter++ )
         {
            LOSSDETAILS& details(iter->second);
            const pgsPointOfInterest& poi(iter->first);

            FinalizeTimeStepAnalysis(intervalIdx,poi,details);
         }
      } // next group
   } // next interval
}

void CTimeStepLossEngineer::InitializeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details)
{
   // Initializes the time step analysis
   // Gets basic information about the cross section such as section properties.
   // Computes the force required to restrain the initial strains due to creep, shrinkage, and relaxation at each POI
   // during this interval for loads applied during previous intervals.
   //
   // Essentially, computing the second term in Eqns 2 and 3 to get the initial strains,
   // computing the restraining forces in each piece with Eqns 10 and 11, and the composite
   // section restraining forces with Eqns 12 and 13.

   bool bIgnoreCreepEffects      = m_pLossParams->IgnoreCreepEffects();
   bool bIgnoreShrinkageEffects  = m_pLossParams->IgnoreShrinkageEffects();
   bool bIgnoreRelaxationEffects = m_pLossParams->IgnoreRelaxationEffects();

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);


   DuctIndexType nDucts = m_pTendonGeom->GetDuctCount(segmentKey);

   IntervalIndexType stressStrandsIntervalIdx = m_pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = m_pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = m_pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = m_pIntervals->GetLiveLoadInterval();

   CClosureKey closureKey;
   bool bIsInClosure = m_pPoi->IsInClosureJoint(poi,&closureKey);
   IntervalIndexType compositeClosureIntervalIdx = (bIsInClosure ? m_pIntervals->GetCompositeClosureJointInterval(closureKey) : INVALID_INDEX);
   bool bIsOnSegment = m_pPoi->IsOnSegment(poi);

   bool bIsOnGirder = m_pPoi->IsOnGirder(poi);

   // Material Properties
   Float64 EDeck  = m_pMaterials->GetDeckEc(intervalIdx);
   Float64 EaDeck = m_pMaterials->GetDeckAgeAdjustedEc(intervalIdx);
   Float64 EGirder, EaGirder;
   if ( bIsOnSegment )
   {
      EGirder  = m_pMaterials->GetSegmentEc(segmentKey,intervalIdx);
      EaGirder = m_pMaterials->GetSegmentAgeAdjustedEc(segmentKey,intervalIdx);
   }
   else if ( bIsInClosure )
   {
      EGirder  = m_pMaterials->GetClosureJointEc(closureKey,intervalIdx);
      EaGirder = m_pMaterials->GetClosureJointAgeAdjustedEc(closureKey,intervalIdx);
   }
   else
   {
      EGirder  = EDeck;
      EaGirder = EaDeck;
   }
   Float64 EStrand[3] = { m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetE(),
                          m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetE(),
                          m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE()};
   Float64 ETendon = m_pMaterials->GetTendonMaterial(girderKey)->GetE();
   
   Float64 EDeckRebar, EGirderRebar, Fy, Fu;
   m_pMaterials->GetDeckRebarProperties(&EDeckRebar,&Fy,&Fu);

   if ( bIsInClosure )
   {
      m_pMaterials->GetClosureJointLongitudinalRebarProperties(closureKey,&EGirderRebar,&Fy,&Fu);
   }
   else
   {
      m_pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey,&EGirderRebar,&Fy,&Fu);
   }

   Float64 gdrHeight = m_pGirder->GetHeight(poi);

   // Initialize time step details
   TIME_STEP_DETAILS tsDetails;

   // TIME PARAMETERS
   tsDetails.intervalIdx = intervalIdx;
   tsDetails.tStart      = m_pIntervals->GetTime(intervalIdx,pgsTypes::Start);
   tsDetails.tMiddle     = m_pIntervals->GetTime(intervalIdx,pgsTypes::Middle);
   tsDetails.tEnd        = m_pIntervals->GetTime(intervalIdx,pgsTypes::End);

   // TRANSFORMED PROPERTIES OF COMPOSITE SECTION (all parts are transformed to equivalent girder concrete)
   tsDetails.Atr = m_pSectProp->GetAg(pgsTypes::sptTransformed,intervalIdx,poi);
   tsDetails.Itr = m_pSectProp->GetIx(pgsTypes::sptTransformed,intervalIdx,poi);
   tsDetails.Ytr = -m_pSectProp->GetY(pgsTypes::sptTransformed,intervalIdx,poi,pgsTypes::TopGirder); // Negative because this is measured down from Y=0 at the top of the girder
   tsDetails.E   = EaGirder;

   // SEGMENT PARAMETERS

   // net section properties of segment
   tsDetails.Girder.An  = m_pSectProp->GetNetAg(intervalIdx,poi);
   tsDetails.Girder.In  = m_pSectProp->GetNetIg(intervalIdx,poi);
   tsDetails.Girder.Yn  = -m_pSectProp->GetNetYtg(intervalIdx,poi);
   tsDetails.Girder.H   = gdrHeight;
   tsDetails.Girder.E   = EaGirder;

   // DECK PARAMETERS

   // net section properties of deck
   tsDetails.Deck.An  = m_pSectProp->GetNetAd(intervalIdx,poi);
   tsDetails.Deck.In  = m_pSectProp->GetNetId(intervalIdx,poi);
   tsDetails.Deck.Yn  = m_pSectProp->GetNetYbd(intervalIdx,poi); // distance from CG of net section to bottom of deck
   tsDetails.Deck.H   = tsDetails.Deck.Yn + m_pSectProp->GetNetYtd(intervalIdx,poi);
   tsDetails.Deck.E   = EaDeck;

   // deck rebar
   if ( compositeDeckIntervalIdx <= intervalIdx  )
   {
      // deck is composite so the rebar is in play
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::DeckRebarMatType matType = pgsTypes::DeckRebarMatType(i);
         for ( int j = 0; j < 2; j++ )
         {
            pgsTypes::DeckRebarBarType barType = pgsTypes::DeckRebarBarType(j);

            Float64 As, Ys;
            // don't adjust the bar area for development here because it isn't adjusted for the transformed
            // section properties. this needs to be consistent
            m_pRebarGeom->GetDeckReinforcing(poi,matType,barType,pgsTypes::drcAll,false/*don't adjust for dev length*/,&As,&Ys);
            tsDetails.DeckRebar[matType][barType].As = As;
            tsDetails.DeckRebar[matType][barType].Ys = Ys;
            tsDetails.DeckRebar[matType][barType].E  = EDeckRebar;
         }
      }
   }

   // Girder/Closure Rebar
   CComPtr<IRebarSection> rebar_section;
   m_pRebarGeom->GetRebars(poi,&rebar_section);

   // POI is in a precast segment....
   CComPtr<IEnumRebarSectionItem> enum_items;
   rebar_section->get__EnumRebarSectionItem(&enum_items);
   CComPtr<IRebarSectionItem> item;
   while ( enum_items->Next(1,&item,NULL) != S_FALSE )
   {
      TIME_STEP_REBAR tsRebar;

      if ( (bIsInClosure && (intervalIdx < compositeClosureIntervalIdx)) // POI is in a closure and the closure is not composite with the girder yet
            || // -OR-
           (intervalIdx < releaseIntervalIdx) // POI is in a segment and it is before the prestress is released
         )
      {
         // don't model the rebar
         tsRebar.As = 0;
         tsRebar.Ys = 0;
         tsRebar.E  = 0;
      }
      else
      {
         CComPtr<IRebar> rebar;
         item->get_Rebar(&rebar);
         Float64 Ab;
         rebar->get_NominalArea(&Ab);

         tsRebar.As = Ab;

         CComPtr<IPoint2d> p;
         item->get_Location(&p);

         Float64 y;
         p->get_Y(&y); // this is in Girder Section Coordinates.. that's exactly what we want

         tsRebar.Ys = y;

         tsRebar.E  = EGirderRebar;
      }

      item.Release();

      tsDetails.GirderRebar.push_back(tsRebar);
   }

   // STRAND PARAMETERS
   if ( bIsOnSegment && stressStrandsIntervalIdx <= intervalIdx )
   {
      // The strands are stressed
      const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
      for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
      {
         pgsTypes::StrandType strandType = *strandTypeIter;

         // time from strand stressing to end of this interval
         Float64 tStressing       = m_pIntervals->GetTime(stressStrandsIntervalIdx,pgsTypes::Start);
         Float64 tEndThisInterval = m_pIntervals->GetTime(intervalIdx,pgsTypes::End);

#if defined LUMP_STRANDS
         tsDetails.Strands[strandType].tEnd = Max(0.0,tEndThisInterval - tStressing);

         // Assumes transformed section properties are based on the strands being lumped in one location
         // Consider modeling each strand individually. This would be a more accurate analysis, however
         // it would take longer. Instead of looping over three strand types, we'll have to loop over
         // each strand for each strand type.

         // section properties
         tsDetails.Strands[strandType].As = m_pStrandGeom->GetStrandArea(poi,intervalIdx,strandType);

         // location of strands from top of girder
         tsDetails.Strands[strandType].Ys = (intervalIdx <= stressStrandsIntervalIdx ? 0 : m_pStrandGeom->GetStrandLocation(poi,strandType,intervalIdx));

         tsDetails.Strands[strandType].E = EStrand[strandType];

         if ( intervalIdx == stressStrandsIntervalIdx )
         {
            // Strands are stress in this interval.. get Pjack and fpj
            // Pjack is for all the strands (even debonded strands).... need the nominal strand area
            // (area of all the strands, even the debonded strands) to get fpj
            Float64 As = m_pStrandGeom->GetStrandArea(segmentKey,intervalIdx,strandType);
            tsDetails.Strands[strandType].Pj  = (As == 0 ? 0 : m_pStrandGeom->GetPjack(segmentKey,strandType));
            tsDetails.Strands[strandType].fpj = (As == 0 ? 0 : tsDetails.Strands[strandType].Pj/As);

            // strands relax over the duration of the interval. compute the amount of relaxation
            if ( bIgnoreRelaxationEffects )
            {
               tsDetails.Strands[strandType].fr = 0;
            }
            else
            {
               tsDetails.Strands[strandType].fr = m_pMaterials->GetStrandRelaxation(segmentKey,intervalIdx,tsDetails.Strands[strandType].fpj,strandType);
            }
         }
         else
         {
            TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

            // strands were stressed in a previous interval
            if ( intervalIdx == releaseIntervalIdx )
            {
               // accounts for lack of development and location of debonding
               Float64 xfer_factor = m_pPSForce->GetXferLengthAdjustment(poi,strandType);

               // xfer_factor reduces the nominal strand force (force based on all strands)
               // do the actual force by making adjustments for lack of full development
               // and debonding. Since the strands data structure contains the actual
               // effective area of strands (area reduced for debonding) we need to
               // get the nominal area here and use it in the calculation below
               Float64 As = m_pStrandGeom->GetStrandArea(segmentKey,intervalIdx,strandType);

               // this the interval when the prestress force is release into the girders, apply the
               // prestress as an external force. The prestress force is the area of strand times
               // the effective stress in the strands at the end of the previous interval.
               // Negative sign because the force resisted by the girder section is equal and opposite
               // the force in the strand (strands put a compression force into the girder)
               Float64 P = -xfer_factor*As*prevTimeStepDetails.Strands[strandType].fpe;
               tsDetails.dPi[pftPretension] += P;
               tsDetails.dMi[pftPretension] += P*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys);

               tsDetails.Pi[pftPretension] += prevTimeStepDetails.Pi[pftPretension] + tsDetails.dPi[pftPretension];
               tsDetails.Mi[pftPretension] += prevTimeStepDetails.Mi[pftPretension] + tsDetails.dMi[pftPretension];
            }

            // relaxation during this interval
            // by using the effective prestress at the end of the previous interval we get a very good approximation for 
            // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
            if ( bIgnoreRelaxationEffects )
            {
               tsDetails.Strands[strandType].fr = 0;
            }
            else
            {
               tsDetails.Strands[strandType].fr = m_pMaterials->GetStrandRelaxation(segmentKey,intervalIdx,prevTimeStepDetails.Strands[strandType].fpe,strandType);
            }
         }

         // apparent strain due to relacation
         tsDetails.Strands[strandType].er = tsDetails.Strands[strandType].fr/EStrand[strandType]; // Tadros 1977, second term in Eqn 8

         // force required to resist apparent relaxation strain
         tsDetails.Strands[strandType].PrRelaxation = -tsDetails.Strands[strandType].fr*tsDetails.Strands[strandType].As; // Tadros 1977, Eqn 10
#else
         Float64 Pjack = m_pStrandGeom->GetPjack(segmentKey,strandType);
         StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
         if ( 0 < nStrands )
         {
            Pjack /= nStrands; // Pjack per strand
         }

         for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
         {
            TIME_STEP_STRAND strand;
            strand.tEnd = Max(0.0,tEndThisInterval - tStressing);

            if ( !bIsInClosure )
            {
               // only fill up with data if NOT at closure joint...

               // section properties
               strand.As = m_pMaterials->GetStrandMaterial(segmentKey,strandType)->GetNominalArea();

               // If poi is at a section where this strand is debonded, don't count it
               if ( !m_pStrandGeom->IsStrandDebonded(poi,strandIdx, strandType) )
               {
                  strand.As = 0;
               }
   
               // location of strand from top of girder
               CComPtr<IPoint2d> point;
               m_pStrandGeom->GetStrandPosition(poi,strandIdx,strandType,&point);
               point->get_Y(&strand.Ys);

               strand.E = EStrand[strandType];

               if ( intervalIdx == stressStrandsIntervalIdx )
               {
                  // Strands are stress in this interval.. get Pjack and fpj
                  strand.Pj  = (IsZero(strand.As) ? 0 : Pjack);
                  strand.fpj = (IsZero(strand.As) ? 0 : strand.Pj/strand.As);

                  // strands relax over the duration of the interval. compute the amount of relaxation
                  if ( bIgnoreTimeDependentEffects )
                  {
                     strand.fr = 0;
                  }
                  else
                  {
                     strand.fr = m_pMaterials->GetStrandRelaxation(segmentKey,intervalIdx,strand.fpj,strandType);
                  }
               }
               else
               {
                  // strands were stressed in a previous interval
                  if ( intervalIdx == releaseIntervalIdx )
                  {
                     // this is the interval when the prestress force is release into the girders, apply the
                     // prestress as an external force. The prestress force is the area of strand times
                     // the effective stress in the strand at the end of the previous interval.
                     // Negative sign because the force resisted by the girder section is equal and opposite
                     // the force in the strand (stand put a compression force into the girder)

                     // accounts for lack of development and location of debonding
#pragma Reminder("UPDATE: this should be the adjustment for the individual strands, not all the strands taken together")
                     Float64 xfer_factor = m_pPSForce->GetXferLengthAdjustment(poi,strandType);

                     Float64 P = -xfer_factor*strand.As*prevTimeStepDetails.Strands[strandType][strandIdx].fpe;
                     tsDetails.dPi[pftPretension] += P;
                     tsDetails.dMi[pftPretension] += P*(tsDetails.Ytr - strand.Ys);

                     tsDetails.Pi[pftPretension] += prevTimeStepDetails.Pi[pftPretension] + tsDetails.dPi[pftPretension];
                     tsDetails.Mi[pftPretension] += prevTimeStepDetails.Mi[pftPretension] + tsDetails.dMi[pftPretension];
                  }

                  // relaxation during this interval
                  // by using the effective prestress at the end of the previous interval we get a very good approximation for 
                  // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
                  if ( bIgnoreTimeDependentEffects )
                  {
                     strand.fr = 0;
                  }
                  else
                  {
                     strand.fr = m_pMaterials->GetStrandRelaxation(segmentKey,intervalIdx,prevTimeStepDetails.Strands[strandType][strandIdx].fpe,strandType);
                  }
               }

               // apparent strain due to relacation
               strand.er = strand.fr/EStrand[strandType]; // Tadros 1977, second term in Eqn 8

               // force required to resist apparent relaxation strain
               strand.PrRelaxation = -strand.fr*strand.As; // Tadros 1977, Eqn 10
            } // if not closure

            tsDetails.Strands[strandType].push_back(strand);
         } // next strand
#endif // LUMP_STRANDS
      } // next strand type
   }

   // TENDON PARAMETERS
   if ( bIsOnGirder )
   {
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         IntervalIndexType stressTendonIntervalIdx  = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

         TIME_STEP_STRAND tsTendon;

         Float64 tStressing = m_pIntervals->GetTime(stressTendonIntervalIdx,pgsTypes::Start);
         Float64 tEndThisInterval = m_pIntervals->GetTime(intervalIdx,pgsTypes::End);
         tsTendon.tEnd = Max(0.0,tEndThisInterval - tStressing);
         tsTendon.As = m_pTendonGeom->GetTendonArea(girderKey,intervalIdx,ductIdx);
         tsTendon.Ys = (intervalIdx < stressTendonIntervalIdx ? 0 : m_pTendonGeom->GetDuctOffset(intervalIdx,poi,ductIdx));
         tsTendon.E = ETendon;

         if ( intervalIdx == stressTendonIntervalIdx )
         {
            // The jacking force that is transfered to the girder is Pjack - Apt*(Friction Loss + Anchor Set Loss)
#if defined USE_AVERAGE_TENDON_FORCE
            Float64 dfpF, dfpA; // equiv. PT loads are based on average loss so use the same thing here

             look up directly, don't call this method... calling this method here will
             mess up the m_Losses data structure
            GetAverageFrictionAndAnchorSetLoss(girderKey,ductIdx,&dfpF,&dfpA);

            CTendonKey tendonKey(girderKey,ductIdx);
            std::map<CTendonKey,std::pair<Float64,Float64>>::iterator found = m_AvgFrictionAndAnchorSetLoss.find(tendonKey);
            ATLASSERT( found != m_AvgFrictionAndAnchorSetLoss.end() ); // this should have already been done!

            dfpF = found->second.first;
            dfpA = found->second.second;
#else
            Float64 dfpF = details.FrictionLossDetails[ductIdx].dfpF; // friction loss at this POI
            Float64 dfpA = details.FrictionLossDetails[ductIdx].dfpA; // anchor set loss at this POI
#endif // USE_AVERAGE_TENDON_FORCE

            tsTendon.Pj  = m_pTendonGeom->GetPjack(girderKey,ductIdx) - tsTendon.As*(dfpF + dfpA);
         }
         else
         {
            tsTendon.Pj = 0;
         }
         tsTendon.fpj = IsZero(tsTendon.As) ? 0 : tsTendon.Pj/tsTendon.As;
         tsTendon.Pi[pftPostTensioning]  = tsTendon.Pj;

         // time from tendon stressing to end of this interval
         if ( intervalIdx < stressTendonIntervalIdx )
         {
            // tendons not installed yet, no relaxation
            tsTendon.fr = 0;
         }
         else if ( intervalIdx == stressTendonIntervalIdx )
         {
            TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

            // this the interval when the pt force is release into the girders, apply the
            // prestress as an external force. The prestress force is the area of strand times
            // the effective stress at the end of the previous interval.
            // Negative sign because the force resisted by the girder section is equal and opposite
            // the force in the strand
            tsDetails.dPi[pftPostTensioning] -= tsTendon.Pj;
            tsDetails.dMi[pftPostTensioning] -= tsTendon.Pj*(tsDetails.Ytr - tsTendon.Ys);

            tsDetails.Pi[pftPostTensioning] += prevTimeStepDetails.Pi[pftPostTensioning] + tsDetails.dPi[pftPostTensioning];
            tsDetails.Mi[pftPostTensioning] += prevTimeStepDetails.Mi[pftPostTensioning] + tsDetails.dMi[pftPostTensioning];

            // Secondary effects
            Float64 dM = m_pProductForces->GetMoment(intervalIdx,pftSecondaryEffects,poi,m_Bat, rtIncremental);
            tsDetails.dMi[pftSecondaryEffects] += dM;
            tsDetails.Mi[pftSecondaryEffects] += prevTimeStepDetails.Mi[pftSecondaryEffects] + tsDetails.dMi[pftSecondaryEffects];

            //Float64 dP = m_pProductForces->GetAxial(intervalIdx,pftSecondaryEffects,poi,m_Bat, rtIncremental);
            Float64 dP = 0;
            tsDetails.dPi[pftSecondaryEffects] += dP;
            tsDetails.Pi[pftSecondaryEffects] += prevTimeStepDetails.Pi[pftSecondaryEffects] + tsDetails.dPi[pftSecondaryEffects];

            // relaxation during the stressing interval
            if ( bIgnoreRelaxationEffects )
            {
               tsTendon.fr = 0;
            }
            else
            {
               tsTendon.fr = m_pMaterials->GetTendonRelaxation(girderKey,ductIdx,intervalIdx,tsTendon.fpj);
            }
         }
         else
         {
            TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

            // by using the effective prestress at the end of the previous interval we get a very good approximation for 
            // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
            if ( bIgnoreRelaxationEffects )
            {
               tsTendon.fr = 0;
            }
            else
            {
               tsTendon.fr = m_pMaterials->GetTendonRelaxation(girderKey,ductIdx,intervalIdx,prevTimeStepDetails.Tendons[ductIdx].fpe);
            }
         }

         // apparent strain due to relaxation
         tsTendon.er = tsTendon.fr/ETendon;

         // force required to resist apparent relaxation strain
         tsTendon.PrRelaxation = -tsTendon.fr*tsTendon.As;

         tsDetails.Tendons.push_back(tsTendon);
      } // next duct
   }

   // Compute unrestrained creep strain due to loads applied prior to this interval
   // Tadros 1977, Second term in Equations 3 and 4
   tsDetails.Girder.ec.reserve(intervalIdx);
   tsDetails.Girder.rc.reserve(intervalIdx);
   tsDetails.Deck.ec.reserve(intervalIdx);
   tsDetails.Deck.rc.reserve(intervalIdx);

   if ( 0 < intervalIdx )
   {
      for ( IntervalIndexType i = 0; i < intervalIdx; i++ )
      {
         TIME_STEP_CONCRETE::CREEP_STRAIN    girderCreepStrain,    deckCreepStrain;
         TIME_STEP_CONCRETE::CREEP_CURVATURE girderCreepCurvature, deckCreepCurvature;

         TIME_STEP_DETAILS& iTimeStepDetails(details.TimeStepDetails[i]); // time step details for interval i

         // Girder
         Float64 Cs, Ce;
         if ( bIgnoreCreepEffects )
         {
            Cs = 0;
            Ce = 0;
         }
         else
         {
            Cs = (bIsInClosure ? m_pMaterials->GetClosureJointCreepCoefficient(closureKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start) : m_pMaterials->GetSegmentCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start));
            Ce = (bIsInClosure ? m_pMaterials->GetClosureJointCreepCoefficient(closureKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End)   : m_pMaterials->GetSegmentCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End));
         }

         Float64 dP_Girder = 0;
         Float64 dM_Girder = 0;
         Float64 dP_Deck = 0;
         Float64 dM_Deck = 0;
         std::vector<ProductForceType> vProductForces = GetApplicableProductLoads(i,poi);
         std::vector<ProductForceType>::iterator pfIter(vProductForces.begin());
         std::vector<ProductForceType>::iterator pfIterEnd(vProductForces.end());
         for ( ; pfIter != pfIterEnd; pfIter++ )
         {
            ProductForceType pfType = *pfIter;
            dP_Girder += iTimeStepDetails.Girder.dPi[pfType];
            dM_Girder += iTimeStepDetails.Girder.dMi[pfType];

            dP_Deck += iTimeStepDetails.Deck.dPi[pfType];
            dM_Deck += iTimeStepDetails.Deck.dMi[pfType];
         }

         // Modulus in interval i (not age adjusted because we apply the creep coefficients Ce and Cs)
         Float64 EiGirder = (bIsInClosure ? m_pMaterials->GetClosureJointEc(closureKey,i) : m_pMaterials->GetSegmentEc(segmentKey,i));
         Float64 EiAGirder = EiGirder*iTimeStepDetails.Girder.An;
         Float64 EiIGirder = EiGirder*iTimeStepDetails.Girder.In;
         Float64 e = IsZero(EiAGirder) ? 0 : (Ce-Cs)*dP_Girder/EiAGirder;
         Float64 r = IsZero(EiIGirder) ? 0 : (Ce-Cs)*dM_Girder/EiIGirder;

         tsDetails.Girder.eci += e;
         tsDetails.Girder.rci += r;

         girderCreepStrain.A = iTimeStepDetails.Girder.An;
         girderCreepStrain.E = EiGirder;
         girderCreepStrain.P = dP_Girder;
         girderCreepStrain.Cs = Cs;
         girderCreepStrain.Ce = Ce;
         girderCreepStrain.e = e;

         girderCreepCurvature.I = iTimeStepDetails.Girder.In;
         girderCreepCurvature.E = EiGirder;
         girderCreepCurvature.M = dM_Girder;
         girderCreepCurvature.Cs = Cs;
         girderCreepCurvature.Ce = Ce;
         girderCreepCurvature.r = r;

         tsDetails.Girder.ec.push_back(girderCreepStrain);
         tsDetails.Girder.rc.push_back(girderCreepCurvature);

         // Deck
         if ( bIgnoreCreepEffects )
         {
            Cs = 0;
            Ce = 0;
         }
         else
         {
            Cs = m_pMaterials->GetDeckCreepCoefficient(i,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
            Ce = m_pMaterials->GetDeckCreepCoefficient(i,pgsTypes::Middle,intervalIdx,pgsTypes::End);
         }

         // Modulus in interval i (not age adjusteded because we apply the creep coefficients Ce and Cs)
         Float64 EiDeck = m_pMaterials->GetDeckEc(i);
         Float64 EiADeck = EiDeck*iTimeStepDetails.Deck.An;
         Float64 EiIDeck = EiDeck*iTimeStepDetails.Deck.In;

         e = IsZero(EiADeck) ? 0 : (Ce-Cs)*dP_Deck/EiADeck;
         r = IsZero(EiIDeck) ? 0 : (Ce-Cs)*dM_Deck/EiIDeck;

         tsDetails.Deck.eci += e;
         tsDetails.Deck.rci += r;

         deckCreepStrain.A = iTimeStepDetails.Deck.An;
         deckCreepStrain.E = EiDeck;
         deckCreepStrain.P = dP_Deck;
         deckCreepStrain.Cs = Cs;
         deckCreepStrain.Ce = Ce;
         deckCreepStrain.e = e;

         deckCreepCurvature.I = iTimeStepDetails.Deck.In;
         deckCreepCurvature.E = EiDeck;
         deckCreepCurvature.M = dM_Deck;
         deckCreepCurvature.Cs = Cs;
         deckCreepCurvature.Ce = Ce;
         deckCreepCurvature.r = r;

         tsDetails.Deck.ec.push_back(deckCreepStrain);
         tsDetails.Deck.rc.push_back(deckCreepCurvature);
      }

      // Compute total unrestrained deformation due to creep and shrinkage during this interval
      // AKA Compute initial strain conditions due to creep and shrinkage
      Float64 esi;
      if ( bIgnoreShrinkageEffects )
      {
         esi = 0;
      }
      else
      {
         esi = (bIsInClosure ? m_pMaterials->GetClosureJointFreeShrinkageStrain(closureKey,intervalIdx) : m_pMaterials->GetSegmentFreeShrinkageStrain(segmentKey,intervalIdx));
      }

      tsDetails.Girder.esi = esi;

      if ( bIgnoreShrinkageEffects )
      {
         esi = 0;
      }
      else
      {
         esi = m_pMaterials->GetDeckFreeShrinkageStrain(intervalIdx);
      }

      tsDetails.Deck.esi = esi;

      // Compute forces to restrain creep and shrinkage deformations in each individual
      // piece of the cross section. (Restraining force for relaxation deformations
      // was done above)

      // The concrete shrinkage strain is negative (concrete parts want to get shorter). So, strain < 0.
      // It requires a tension force to restrain this deformation. The force, F = -1*(strain)(E)(A).
      // The -1*strain reverses the deformation and results in a tension force.
      tsDetails.Girder.PrCreep     = -tsDetails.Girder.eci * tsDetails.Girder.An * EGirder; // Tadros 1977, Eqn 10
      tsDetails.Girder.MrCreep     = -tsDetails.Girder.rci * tsDetails.Girder.In * EGirder; // Tadros 1977, Eqn 11
      tsDetails.Girder.PrShrinkage = -tsDetails.Girder.esi * tsDetails.Girder.An * EGirder; // Tadros 1977, Eqn 10

      tsDetails.Deck.PrCreep     = -tsDetails.Deck.eci * tsDetails.Deck.An * EDeck; // Tadros 1977, Eqn 10
      tsDetails.Deck.MrCreep     = -tsDetails.Deck.rci * tsDetails.Deck.In * EDeck; // Tadros 1977, Eqn 11
      tsDetails.Deck.PrShrinkage = -tsDetails.Deck.esi * tsDetails.Deck.An * EDeck; // Tadros 1977, Eqn 10
   }

   // Total restraining force
   // Tadros 1977, Eqn 12 and 13.
   if ( releaseIntervalIdx <= intervalIdx )
   {
      tsDetails.Pr[TIMESTEP_CR] = tsDetails.Girder.PrCreep     + tsDetails.Deck.PrCreep;
      tsDetails.Pr[TIMESTEP_SH] = tsDetails.Girder.PrShrinkage + tsDetails.Deck.PrShrinkage;
      
#if defined LUMP_STRANDS
      tsDetails.Pr[TIMESTEP_RE] = tsDetails.Strands[pgsTypes::Straight ].PrRelaxation
                                + tsDetails.Strands[pgsTypes::Harped   ].PrRelaxation
                                + tsDetails.Strands[pgsTypes::Temporary].PrRelaxation;
#endif // LUMP_STRANDS

      tsDetails.Mr[TIMESTEP_CR] = tsDetails.Girder.MrCreep + tsDetails.Girder.PrCreep*(tsDetails.Ytr - tsDetails.Girder.Yn)
                                + tsDetails.Deck.MrCreep   + tsDetails.Deck.PrCreep  *(tsDetails.Ytr - tsDetails.Deck.Yn);
      
      tsDetails.Mr[TIMESTEP_SH] = tsDetails.Girder.PrShrinkage*(tsDetails.Ytr - tsDetails.Girder.Yn)
                                + tsDetails.Deck.PrShrinkage  *(tsDetails.Ytr - tsDetails.Deck.Yn);
      
#if defined LUMP_STRANDS
      tsDetails.Mr[TIMESTEP_RE] = tsDetails.Strands[pgsTypes::Straight ].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Straight ].Ys)
                                + tsDetails.Strands[pgsTypes::Harped   ].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Harped   ].Ys)
                                + tsDetails.Strands[pgsTypes::Temporary].PrRelaxation*(tsDetails.Ytr - tsDetails.Strands[pgsTypes::Temporary].Ys);
#endif // LUMP_STRANDS

#if !defined LUMP_STRANDS
      tsDetails.Pr[TIMESTEP_RE] = 0; // start with 0 and sum contribrution of each strand
      tsDetails.Mr[TIMESTEP_RE] = 0;
      const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
      for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
      {
         pgsTypes::StrandType strandType = *strandTypeIter;
         StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
         for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
         {
            TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
            tsDetails.Pr[TIMESTEP_RE] += strand.PrRelaxation;
            tsDetails.Mr[TIMESTEP_RE] += strand.PrRelaxation*(tsDetails.Ytr - strand.Ys);
         } // next strand
      } // next strand type
#endif // !LUMP_STRANDS

      if ( bIsOnGirder )
      {
         for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

            tsDetails.Pr[TIMESTEP_RE] += tendon.PrRelaxation;
            tsDetails.Mr[TIMESTEP_RE] += tendon.PrRelaxation*(tsDetails.Ytr - tendon.Ys);
         }
      }
   }

   details.TimeStepDetails.push_back(tsDetails);
}

void CTimeStepLossEngineer::AnalyzeInitialStrains(IntervalIndexType intervalIdx,const CGirderKey& girderKey,LOSSES* pLosses)
{
   // Compute the response to the inital strains (see Tadros 1977, section titled "Effects of Initial Strains")
   
   // Instead of applying forces that are equal and opposite to the sum of the restraining forces of the individual
   // parts as described in the Tadros 1977 paper, we will apply member deformation/strain loads that are equal
   // and opposite in magnitude of the initial strains. This will directly yield the restraining forces at each
   // section caused by creep, shrinkage, and relaxation. These direct restraining forces will then be distributed
   // to the individual elements of the cross section. This method yields the same results at the method described
   // in the Tadros 1977 paper, however it also yields the resultant shear forces which are not readily available
   // from the Tadros method.

   SegmentIndexType nSegments = m_pBridge->GetSegmentCount(girderKey);

   SectionLossContainer::iterator iter1(pLosses->SectionLosses.begin());
   SectionLossContainer::iterator end(pLosses->SectionLosses.end());
   SectionLossContainer::iterator iter2(iter1);
   iter2++;

   for ( ; iter2 != end; iter1++, iter2++ )
   {
      const pgsPointOfInterest& poi1(iter1->first);
      const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
      LOSSDETAILS& details1(iter1->second);

      CClosureKey closureKey1;
      bool bIsInClosure1 = m_pPoi->IsInClosureJoint(poi1,&closureKey1);
      bool bIsClosure1Effective = (bIsInClosure1 ? m_pIntervals->GetCompositeClosureJointInterval(closureKey1) <= intervalIdx : false);
      bool bIsOnSegment1 = m_pPoi->IsOnSegment(poi1);
      
      const pgsPointOfInterest& poi2(iter2->first);
      const CSegmentKey& segmentKey2(poi2.GetSegmentKey());
      LOSSDETAILS& details2(iter2->second);
      CClosureKey closureKey2;
      bool bIsInClosure2 = m_pPoi->IsInClosureJoint(poi2,&closureKey2);
      bool bIsClosure2Effective = (bIsInClosure2 ? m_pIntervals->GetCompositeClosureJointInterval(closureKey2) <= intervalIdx : false);
      bool bIsOnSegment2 = m_pPoi->IsOnSegment(poi2);

      TIME_STEP_DETAILS& tsDetails1(details1.TimeStepDetails[intervalIdx]);
      TIME_STEP_DETAILS& tsDetails2(details2.TimeStepDetails[intervalIdx]);

      Float64 Atr1 = tsDetails1.Atr;
      Float64 Itr1 = tsDetails1.Itr;
      Float64 E1 = (bIsInClosure1 ? m_pMaterials->GetClosureJointAgeAdjustedEc(closureKey1,intervalIdx) : m_pMaterials->GetSegmentAgeAdjustedEc(segmentKey1,intervalIdx));
      Float64 EA1 = Atr1*E1;
      Float64 EI1 = Itr1*E1;

      Float64 Atr2 = tsDetails2.Atr;
      Float64 Itr2 = tsDetails2.Itr;
      Float64 E2 = (bIsInClosure2 ? m_pMaterials->GetClosureJointAgeAdjustedEc(closureKey2,intervalIdx) : m_pMaterials->GetSegmentAgeAdjustedEc(segmentKey2,intervalIdx));
      Float64 EA2 = Atr2*E2;
      Float64 EI2 = Itr2*E2;

      // Compute the initial strain at poi1 and poi2
      for ( int i = 0; i < 3; i++ ) // i is one of the TIMESTEP_XXX constants
      {
         Float64 Pr1 = tsDetails1.Pr[i];
         Float64 Mr1 = tsDetails1.Mr[i];
         Float64 e1  = (EA1 == 0 ? 0 : Pr1/EA1);
         Float64 r1  = (EI1 == 0 ? 0 : Mr1/EI1);

         Float64 Pr2 = tsDetails2.Pr[i];
         Float64 Mr2 = tsDetails2.Mr[i];
         Float64 e2  = (EA2 == 0 ? 0 : Pr2/EA2);
         Float64 r2  = (EI2 == 0 ? 0 : Mr2/EI2);

         // The formulation in "Time-Dependent Analysis of Composite Frames (Tadros 1977)" assumes that the
         // initial strains vary lineraly along the bridge. The underlying LBAM and FEM models
         // only supports constant imposed deformations so we are going to use the average
         // values between POI. In general, the POI are tightly spaced so this is a good approximation
         // NOTE: The negative sign is because we want equal and opposite strains
         Float64 e(-0.5*(e1 + e2));
         Float64 r(-0.5*(r1 + r2));

         // if a poi is at a closure joint and the closure joint is not yet effective there can't be any loads applied
         if ( (bIsInClosure1 && !bIsClosure1Effective) || 
              (bIsInClosure2 && !bIsClosure2Effective) ||
              !bIsOnSegment1 ||
              !bIsOnSegment2)
         {
            e = 0;
            r = 0;
         }

         tsDetails1.e[i][pgsTypes::Ahead] = e;
         tsDetails1.r[i][pgsTypes::Ahead] = r;

         tsDetails2.e[i][pgsTypes::Back] = e;
         tsDetails2.r[i][pgsTypes::Back] = r;

         // create the loading as a product force load. This will directly yield the resultant
         // restraining force for the product force type
         if ( !IsZero(e) && !IsZero(r) )
         {
            ProductForceType pfType = (ProductForceType)(pftCreep + i);
            m_pExternalLoading->CreateInitialStrainLoad(intervalIdx,pfType,poi1,poi2,e,r);
         }
      }
   }
}

void CTimeStepLossEngineer::FinalizeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details)
{
   // Determine the effect of the externally applied loads and the initial strains on the various parts 
   // of the cross section using a transformed section analysis.
   // Determine the change in force/moment in each part of the cross section.
   // Determine the change in stress at the top and bottom of the concrete parts (girder and deck)

   TIME_STEP_DETAILS& tsDetails(details.TimeStepDetails[intervalIdx]);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);

   CClosureKey closureKey;
   bool bIsInClosure = m_pPoi->IsInClosureJoint(poi,&closureKey);
   bool bIsOnSegment = m_pPoi->IsOnSegment(poi);

   bool bIsOnGirder = m_pPoi->IsOnGirder(poi);

   IntervalIndexType stressStrandsIntervalIdx = m_pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = m_pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = m_pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx      = m_pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = m_pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = m_pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < releaseIntervalIdx )
   {
      // Prestress force not released onto segment yet...
      if ( bIsOnSegment && stressStrandsIntervalIdx <= intervalIdx )
      {
         // strands are stressed, but not released
         const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
         for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
         {
            pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
            // prestress loss is the intrinsic relaxation during this interval
            tsDetails.Strands[strandType].loss = tsDetails.Strands[strandType].fr;

            tsDetails.Strands[strandType].dfpei[pftPretension] = -tsDetails.Strands[strandType].loss;
            tsDetails.Strands[strandType].dfpe += tsDetails.Strands[strandType].dfpei[pftPretension];

            if ( intervalIdx == stressStrandsIntervalIdx )
            {
               // This is the interval when strands are stressed... 
               // Force in the strand at the end of the interval is equal to Pj - Change in Force during this interval
               // Effective prestress = fpj + dfpe
               tsDetails.Strands[strandType].Pi[pftPretension] = tsDetails.Strands[strandType].Pj  + tsDetails.Strands[strandType].dPi[pftPretension];
               tsDetails.Strands[strandType].fpe = tsDetails.Strands[strandType].fpj + tsDetails.Strands[strandType].dfpei[pftPretension];
            }
            else
            {
               TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

               // Force in strand is force at end of previous interval plus change in force during this interval
               tsDetails.Strands[strandType].Pi[pftPretension] = prevTimeStepDetails.Strands[strandType].Pi[pftPretension] + tsDetails.Strands[strandType].dPi[pftPretension];

               // Effective prestress is effective prestress at end of previous interval plus change in stress during this interval
               tsDetails.Strands[strandType].fpe = prevTimeStepDetails.Strands[strandType].fpe + tsDetails.Strands[strandType].dfpei[pftPretension];
            }
#else
            StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];

               // prestress loss is the intrinsic relaxation during this interval
               strand.loss = strand.fr;

               strand.dfpe = -strand.loss;

               if ( intervalIdx == stressStrandsIntervalIdx )
               {
                  // This is the interval when strands are stressed... 
                  // Force in the strand at the end of the interval is equal to Pj - Change in Force during this interval
                  // Effective prestress = fpj + dfpe
                  strand.Pi[pftPretension] = strand.Pj  + strand.dPi[pftPretension];
                  strand.fpe = strand.fpj + strand.dfpei[pftPretension];
               }
               else
               {
                  TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

                  // Force in strand is force at end of previous interval plus change in force during this interval
                  strand.Pi[pftPretension] = prevTimeStepDetails.Strands[strandType][strandIdx].Pi[pftPretension] + strand.dPi[pftPretension];

                  // Effective prestress is effective prestress at end of previous interval plus change in stress during this interval
                  strand.fpe = prevTimeStepDetails.Strands[strandType][strandIdx].fpe + strand.dfpei[pftPretension];
               }
            } // next strand idx
#endif // LUMP_STRANDS
         } // next strand type
      }
   }
   else
   {
      // Release interval and later

      TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);

      // get some material properties that we are going to need for the analysis
      Float64 EaDeck  = m_pMaterials->GetDeckAgeAdjustedEc(intervalIdx);

      Float64 EaGirder;
      if ( bIsOnSegment )
      {
         EaGirder = m_pMaterials->GetSegmentAgeAdjustedEc(segmentKey,intervalIdx);
      }
      else if ( bIsInClosure )
      {
         EaGirder = m_pMaterials->GetClosureJointAgeAdjustedEc(closureKey,intervalIdx);
      }
      else
      {
         // poi is in the cast-in-place diaphragm between girder groups
         // this is assumed to be the same material as the deck
         EaGirder = EaDeck;
      }

      Float64 EStrand[3] = { m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetE(),
                             m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetE(),
                             m_pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE()};

      Float64 ETendon = m_pMaterials->GetTendonMaterial(girderKey)->GetE();

      Float64 EDeckRebar, EGirderRebar, Fy, Fu;
      m_pMaterials->GetDeckRebarProperties(&EDeckRebar,&Fy,&Fu);

      if ( bIsInClosure )
      {
         m_pMaterials->GetClosureJointLongitudinalRebarProperties(closureKey,&EGirderRebar,&Fy,&Fu);
      }
      else
      {
         m_pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey,&EGirderRebar,&Fy,&Fu);
      }

      // EaGirder*tsDetails.Atr and EaGirder*tsDetails.Itr are used frequently... 
      // Use a variable here so we don't have to multiply it over and over
      Float64 EaGirder_Atr = EaGirder*tsDetails.Atr;
      Float64 EaGirder_Itr = EaGirder*tsDetails.Itr;

      Float64 EaGirder_An = EaGirder*tsDetails.Girder.An;
      Float64 EaGirder_In = EaGirder*tsDetails.Girder.In;

      Float64 EaDeck_An = EaDeck*tsDetails.Deck.An;
      Float64 EaDeck_In = EaDeck*tsDetails.Deck.In;

#if defined _BETA_VERSION
      // Verify transformed age-adjusted section properties
      Float64 EA  = 0;
      Float64 EAy = 0;
      EA  = EaGirder*tsDetails.Girder.An + EaDeck*tsDetails.Deck.An;
      EAy = EaGirder*tsDetails.Girder.An*tsDetails.Girder.Yn + EaDeck*tsDetails.Deck.An*tsDetails.Deck.Yn;

      EA  += EDeckRebar*(tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbIndividual].As + 
                         tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbLumpSum   ].As + 
                         tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].As + 
                         tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum   ].As);

      EAy += EDeckRebar*(tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbIndividual].As*tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbIndividual].Ys +
                         tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbLumpSum   ].As*tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbLumpSum   ].Ys +
                         tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].As*tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].Ys +
                         tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum   ].As*tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum   ].Ys);

      std::vector<TIME_STEP_REBAR>::iterator rIter(tsDetails.GirderRebar.begin());
      std::vector<TIME_STEP_REBAR>::iterator rIterEnd(tsDetails.GirderRebar.end());
      for ( ; rIter != rIterEnd; rIter++ )
      {
         TIME_STEP_REBAR& tsRebar(*rIter);
         EA  += EGirderRebar*tsRebar.As;
         EAy += EGirderRebar*tsRebar.As*tsRebar.Ys;
      }

      if ( bIsOnSegment )
      {
         const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
         for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
         {
            pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
            EA  += EStrand[strandType]*tsDetails.Strands[strandType].As;
            EAy += EStrand[strandType]*tsDetails.Strands[strandType].As*tsDetails.Strands[strandType].Ys;
#else
            StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
               EA  += EStrand[strandType]*strand.As;
               EAy += EStrand[strandType]*strand.As*strand.Ys;
            }
#endif // LUMP_STRANDS
         }
      }

      DuctIndexType nDucts = 0; // outside of if block because we need it in several places
      if ( bIsOnGirder )
      {
         nDucts = m_pTendonGeom->GetDuctCount(girderKey); // outside of if block because we need it in several places
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            IntervalIndexType stressTendonIntervalIdx  = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

            TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

            if ( stressTendonIntervalIdx < intervalIdx )
            {
               EA  += ETendon*tendon.As;
               EAy += ETendon*tendon.As*tendon.Ys;
            }
         }
      }

      Float64 Ytr = IsZero(EA) ? 0 : EAy/EA;
      Float64 Atr = IsZero(EaGirder) ? 0 : EA/EaGirder;

      Float64 EI  = 0;

      EI  = EaGirder*(tsDetails.Girder.In + tsDetails.Girder.An*pow((Ytr - tsDetails.Girder.Yn),2));
      EI += EaDeck*(tsDetails.Deck.In + tsDetails.Deck.An*pow((Ytr - tsDetails.Deck.Yn),2));

      EI += EDeckRebar*(tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbIndividual].As*pow((Ytr - tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbIndividual].Ys),2));
      EI += EDeckRebar*(tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbLumpSum   ].As*pow((Ytr - tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbLumpSum   ].Ys),2));
      EI += EDeckRebar*(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].As*pow((Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].Ys),2));
      EI += EDeckRebar*(tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum   ].As*pow((Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum   ].Ys),2));

      rIter = tsDetails.GirderRebar.begin();
      for ( ; rIter != rIterEnd; rIter++ )
      {
         TIME_STEP_REBAR& tsRebar(*rIter);
         EI += EGirderRebar*(tsRebar.As*pow((Ytr - tsRebar.Ys),2));
      }

      if ( bIsOnSegment )
      {
         const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
         for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
         {
            pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
            TIME_STEP_STRAND& strand = tsDetails.Strands[strandType];
            EI += EStrand[strandType]*strand.As*pow((Ytr - strand.Ys),2);
#else
            StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
               EI += EStrand[strandType]*strand.As*pow((Ytr - strand.Ys),2);
            }
#endif // LUMP_STRANDS
         }
      }

      if ( bIsOnGirder )
      {
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            IntervalIndexType stressTendonIntervalIdx  = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

            TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

            if ( stressTendonIntervalIdx < intervalIdx )
            {
               EI += ETendon*(tendon.As*pow((Ytr - tendon.Ys),2));
            }
         }
      }

      Float64 Itr = IsZero(EaGirder) ? 0 : EI/EaGirder;

      ATLASSERT(IsEqual(tsDetails.Ytr,Ytr));
      ATLASSERT(IsEqual(tsDetails.Atr,Atr));
      ATLASSERT(IsEqual(tsDetails.Itr,Itr));
#if !defined _DEBUG
      if ( !IsEqual(tsDetails.Ytr,Ytr) || !IsEqual(tsDetails.Atr,Atr) || !IsEqual(tsDetails.Itr,Itr) )
      {
         CString strMsg;
         strMsg.Format(_T("Ytr (%s,%s)\nAtr (%s,%s)\nItr (%s,%s)\n at POI %d"),
            ::FormatDimension(tsDetails.Ytr,m_pDisplayUnits->GetComponentDimUnit()),
            ::FormatDimension(Ytr,m_pDisplayUnits->GetComponentDimUnit()),
            ::FormatDimension(tsDetails.Atr,m_pDisplayUnits->GetAreaUnit()),
            ::FormatDimension(Atr,m_pDisplayUnits->GetAreaUnit()),
            ::FormatDimension(tsDetails.Itr,m_pDisplayUnits->GetMomentOfInertiaUnit()),
            ::FormatDimension(Itr,m_pDisplayUnits->GetMomentOfInertiaUnit()),
            poi.GetID());

         AfxMessageBox(strMsg);
      }
#endif // !_DEBUG
#endif // _BETA_VERSION

      IntervalIndexType compositeClosureIntervalIdx = (bIsInClosure ? m_pIntervals->GetCompositeClosureJointInterval(closureKey) : INVALID_INDEX);

      //
      // Compute total change of force on the section during this interval
      //

      int n = GetProductForceCount();
      for ( int i = 0; i < n; i++ ) 
      {
         ProductForceType pfType = (ProductForceType)(i);

         // Get change in force in this interval (this includes girder, slab, etc AND creep, shrinkage, and relaxation... live load is excluded and handled elsewhere)
         Float64 dP = 0;
         Float64 dM = 0;
         if ( bIsInClosure && intervalIdx < compositeClosureIntervalIdx )
         {
            // If the POI is at a closure joint, and it is before the closure is composite
            // with the adjacent girder segments, the moment is zero. At strongbacks,
            // a moment is computed, but this is the moment in the strongback hardware,
            // not the moment in the closure.
            dP = 0;
            dM = 0;
         }
         else if ( pfType == pftPretension || pfType == pftPostTensioning || pfType == pftSecondaryEffects )
         {
            // Don't get change in force for pre or post tensioning... these are taken care of in InitializeTimeStepAnalysis
            // and if you try to get them, it will cause recursion
            dP = tsDetails.dPi[pfType];
            dM = tsDetails.dMi[pfType];
         }
         else if ( pfType == pftCreep || pfType == pftShrinkage || pfType == pftRelaxation )
         {
            std::_tstring strLoading(m_pProductLoads->GetProductLoadName(pfType));

            dP = m_pExternalLoading->GetAxial(intervalIdx,strLoading.c_str(),poi,m_Bat, rtIncremental);
            dM = m_pExternalLoading->GetMoment(intervalIdx,strLoading.c_str(),poi,m_Bat, rtIncremental);

            // change in force this interval
            tsDetails.dMi[pfType] += dM;
            tsDetails.dPi[pfType] += dP;

            // total change in force at the end of this interval
            tsDetails.Pi[pfType] += prevTimeStepDetails.Pi[pfType] + tsDetails.dPi[pfType];
            tsDetails.Mi[pfType] += prevTimeStepDetails.Mi[pfType] + tsDetails.dMi[pfType];
         }
         else
         {
            // for the product force types that get us here, none of them cause
            // axial responses
            dP = 0;//m_pProductForces->GetAxial(intervalIdx,pfType,poi,m_Bat, rtIncremental);
            dM = m_pProductForces->GetMoment(intervalIdx,pfType,poi,m_Bat, rtIncremental);

            // change in force this interval
            tsDetails.dMi[pfType] += dM;
            tsDetails.dPi[pfType] += dP;

            // total change in force at the end of this interval
            tsDetails.Pi[pfType] += prevTimeStepDetails.Pi[pfType] + tsDetails.dPi[pfType];
            tsDetails.Mi[pfType] += prevTimeStepDetails.Mi[pfType] + tsDetails.dMi[pfType];
         }

         // Deformation of the composite section this interval
         tsDetails.der[pfType] = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dPi[pfType]/EaGirder_Atr);
         tsDetails.drr[pfType] = (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]/EaGirder_Itr);

         // Total deformation of the composite section
         tsDetails.er[pfType] = prevTimeStepDetails.er[pfType] + tsDetails.der[pfType];
         tsDetails.rr[pfType] = prevTimeStepDetails.rr[pfType] + tsDetails.drr[pfType];

         //
         // Distribute the changes in section forces to the individual elements of the cross section
         //

         // Compute change in force in girder for this interval
         Float64 dei = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dPi[pfType]/EaGirder_Atr)
                     + (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]*(tsDetails.Ytr - tsDetails.Girder.Yn)/EaGirder_Itr);
         Float64 dri = (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]/EaGirder_Itr);
         Float64 dPi = dei*EaGirder_An;
         Float64 dMi = dri*EaGirder_In;

         tsDetails.Girder.dei[pfType] += dei;
         tsDetails.Girder.dri[pfType] += dri;
         tsDetails.Girder.dPi[pfType] += dPi;
         tsDetails.Girder.dMi[pfType] += dMi;

         if ( pfType == pftCreep )
         {
            dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_CR]/EaGirder_Atr;
            dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_CR]*(tsDetails.Ytr - tsDetails.Girder.Yn)/EaGirder_Itr;
            dri  = IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_CR]/EaGirder_Itr;

            dPi = dei*EaGirder_An + tsDetails.Girder.PrCreep;
            dMi = dri*EaGirder_In + tsDetails.Girder.MrCreep;

            tsDetails.Girder.dei[pfType] += dei;
            tsDetails.Girder.dri[pfType] += dri;
            tsDetails.Girder.dPi[pfType] += dPi;
            tsDetails.Girder.dMi[pfType] += dMi;
         }
         else if ( pfType == pftShrinkage )
         {
            dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_SH]/EaGirder_Atr;
            dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_SH]*(tsDetails.Ytr - tsDetails.Girder.Yn)/EaGirder_Itr;
            dri  = IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_SH]/EaGirder_Itr;

            dPi = dei*EaGirder_An + tsDetails.Girder.PrShrinkage;
            dMi = dri*EaGirder_In;

            tsDetails.Girder.dei[pfType] += dei;
            tsDetails.Girder.dri[pfType] += dri;
            tsDetails.Girder.dPi[pfType] += dPi;
            tsDetails.Girder.dMi[pfType] += dMi;
         }
         else if ( pfType == pftRelaxation )
         {
            dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_RE]/EaGirder_Atr;
            dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_RE]*(tsDetails.Ytr - tsDetails.Girder.Yn)/EaGirder_Itr;
            dri  = IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_RE]/EaGirder_Itr;

            dPi = dei*EaGirder_An;
            dMi = dri*EaGirder_In;

            tsDetails.Girder.dei[pfType] += dei;
            tsDetails.Girder.dri[pfType] += dri;
            tsDetails.Girder.dPi[pfType] += dPi;
            tsDetails.Girder.dMi[pfType] += dMi;
         }

         tsDetails.Girder.Pi[pfType]  += prevTimeStepDetails.Girder.Pi[pfType] + tsDetails.Girder.dPi[pfType];
         tsDetails.Girder.Mi[pfType]  += prevTimeStepDetails.Girder.Mi[pfType] + tsDetails.Girder.dMi[pfType];
         tsDetails.Girder.dP          += tsDetails.Girder.dPi[pfType];
         tsDetails.Girder.dM          += tsDetails.Girder.dMi[pfType];
         tsDetails.Girder.P           += tsDetails.Girder.Pi[pfType];
         tsDetails.Girder.M           += tsDetails.Girder.Mi[pfType];

         tsDetails.Girder.ei[pfType]  += prevTimeStepDetails.Girder.ei[pfType] + tsDetails.Girder.dei[pfType];
         tsDetails.Girder.ri[pfType]  += prevTimeStepDetails.Girder.ri[pfType] + tsDetails.Girder.dri[pfType];
         tsDetails.Girder.de          += tsDetails.Girder.dei[pfType];
         tsDetails.Girder.dr          += tsDetails.Girder.dri[pfType];
         tsDetails.Girder.e           += tsDetails.Girder.ei[pfType];
         tsDetails.Girder.r           += tsDetails.Girder.ri[pfType];

         // Compute girder stresses at end of interval
         // f = f end of previous interval + change in stress this interval
         if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
         {
            tsDetails.Girder.f[pgsTypes::TopFace][pfType][rtIncremental] = tsDetails.Girder.dPi[pfType]/tsDetails.Girder.An + tsDetails.Girder.dMi[pfType]*tsDetails.Girder.Yn/tsDetails.Girder.In;
            tsDetails.Girder.f[pgsTypes::TopFace][pfType][rtCumulative]  = prevTimeStepDetails.Girder.f[pgsTypes::TopFace][pfType][rtCumulative] + tsDetails.Girder.f[pgsTypes::TopFace][pfType][rtIncremental];
            
            tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtIncremental] = tsDetails.Girder.dPi[pfType]/tsDetails.Girder.An + tsDetails.Girder.dMi[pfType]*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
            tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtCumulative]  = prevTimeStepDetails.Girder.f[pgsTypes::BottomFace][pfType][rtCumulative] + tsDetails.Girder.f[pgsTypes::BottomFace][pfType][rtIncremental];
         }

         if ( compositeDeckIntervalIdx <= intervalIdx )
         {
            // Compute change in force in deck for this interval
            dei = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dPi[pfType]/EaGirder_Atr)
                + (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]*(tsDetails.Ytr - tsDetails.Deck.Yn)/EaGirder_Itr);
            dri = (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]/EaGirder_Itr);
            dPi = dei*EaDeck_An;
            dMi = dri*EaDeck_In;

            tsDetails.Deck.dei[pfType] += dei;
            tsDetails.Deck.dri[pfType] += dri;
            tsDetails.Deck.dPi[pfType] += dPi;
            tsDetails.Deck.dMi[pfType] += dMi;

            if ( pfType == pftCreep )
            {
               dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_CR]/EaGirder_Atr;
               dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_CR]*(tsDetails.Ytr - tsDetails.Deck.Yn)/EaGirder_Itr;
               dri  = IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_CR]/EaGirder_Itr;

               dPi = dei*EaDeck_An + tsDetails.Deck.PrCreep;
               dMi = dri*EaDeck_In + tsDetails.Deck.MrCreep;

               tsDetails.Deck.dei[pfType] += dei;
               tsDetails.Deck.dri[pfType] += dri;
               tsDetails.Deck.dPi[pfType] += dPi;
               tsDetails.Deck.dMi[pfType] += dMi;
            }
            else if ( pfType == pftShrinkage )
            {
               dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_SH]/EaGirder_Atr;
               dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_SH]*(tsDetails.Ytr - tsDetails.Deck.Yn)/EaGirder_Itr;
               dri  = IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_SH]/EaGirder_Itr;

               dPi = dei*EaDeck_An + tsDetails.Deck.PrShrinkage;
               dMi = dri*EaDeck_In;

               tsDetails.Deck.dei[pfType] += dei;
               tsDetails.Deck.dri[pfType] += dri;
               tsDetails.Deck.dPi[pfType] += dPi;
               tsDetails.Deck.dMi[pfType] += dMi;
            }
            else if ( pfType == pftRelaxation )
            {
               dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_RE]/EaGirder_Atr;
               dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_RE]*(tsDetails.Ytr - tsDetails.Deck.Yn)/EaGirder_Itr;
               dri  = IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_RE]/EaGirder_Itr;

               dPi = dei*EaDeck_An;
               dMi = dri*EaDeck_In;

               tsDetails.Deck.dei[pfType] += dei;
               tsDetails.Deck.dri[pfType] += dri;
               tsDetails.Deck.dPi[pfType] += dPi;
               tsDetails.Deck.dMi[pfType] += dMi;
            }

            tsDetails.Deck.Pi[pfType]  += prevTimeStepDetails.Deck.Pi[pfType] + tsDetails.Deck.dPi[pfType];
            tsDetails.Deck.Mi[pfType]  += prevTimeStepDetails.Deck.Mi[pfType] + tsDetails.Deck.dMi[pfType];
            tsDetails.Deck.dP          += tsDetails.Deck.dPi[pfType];
            tsDetails.Deck.dM          += tsDetails.Deck.dMi[pfType];
            tsDetails.Deck.P           += tsDetails.Deck.Pi[pfType];
            tsDetails.Deck.M           += tsDetails.Deck.Mi[pfType];

            tsDetails.Deck.ei[pfType]  += prevTimeStepDetails.Deck.ei[pfType] + tsDetails.Deck.dei[pfType];
            tsDetails.Deck.ri[pfType]  += prevTimeStepDetails.Deck.ri[pfType] + tsDetails.Deck.dri[pfType];
            tsDetails.Deck.de          += tsDetails.Deck.dei[pfType];
            tsDetails.Deck.dr          += tsDetails.Deck.dri[pfType];
            tsDetails.Deck.e           += tsDetails.Deck.ei[pfType];
            tsDetails.Deck.r           += tsDetails.Deck.ri[pfType];


            // Compute deck stresses at end of interval
            // f = f end of previous interval + change in stress this interval
            if ( !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
            {
               tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtIncremental] = tsDetails.Deck.dPi[pfType]/tsDetails.Deck.An + tsDetails.Deck.dMi[pfType]*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
               tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::TopFace][pfType][rtCumulative] + tsDetails.Deck.f[pgsTypes::TopFace][pfType][rtIncremental];

               tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtIncremental] = tsDetails.Deck.dPi[pfType]/tsDetails.Deck.An + tsDetails.Deck.dMi[pfType]*tsDetails.Deck.Yn/tsDetails.Deck.In;
               tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtCumulative] = prevTimeStepDetails.Deck.f[pgsTypes::BottomFace][pfType][rtCumulative] + tsDetails.Deck.f[pgsTypes::BottomFace][pfType][rtIncremental];
            }

            // Compute change in force in deck rebar
            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;

               for ( int j = 0; j < 2; j++ )
               {
                  pgsTypes::DeckRebarBarType barType = (pgsTypes::DeckRebarBarType)j;

                  dei = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dPi[pfType]/EaGirder_Atr)
                      + (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]*(tsDetails.Ytr - tsDetails.DeckRebar[matType][barType].Ys)/EaGirder_Itr);
                  dPi = dei*EDeckRebar*tsDetails.DeckRebar[matType][barType].As;

                  tsDetails.DeckRebar[matType][barType].dei[pfType] += dei;
                  tsDetails.DeckRebar[matType][barType].dPi[pfType] += dPi;

                  if ( pfType == pftCreep )
                  {
                     dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_CR]/EaGirder_Atr;
                     dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_CR]*(tsDetails.Ytr - tsDetails.DeckRebar[matType][barType].Ys)/EaGirder_Itr;
                     dPi  = dei*EDeckRebar*tsDetails.DeckRebar[matType][barType].As;

                     tsDetails.DeckRebar[matType][barType].dei[pfType] += dei;
                     tsDetails.DeckRebar[matType][barType].dPi[pfType] += dPi;
                  }
                  else if ( pfType == pftShrinkage )
                  {
                     dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_SH]/EaGirder_Atr;
                     dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_SH]*(tsDetails.Ytr - tsDetails.DeckRebar[matType][barType].Ys)/EaGirder_Itr;
                     dPi  = dei*EDeckRebar*tsDetails.DeckRebar[matType][barType].As;

                     tsDetails.DeckRebar[matType][barType].dei[pfType] += dei;
                     tsDetails.DeckRebar[matType][barType].dPi[pfType] += dPi;
                  }
                  else if ( pfType == pftRelaxation )
                  {
                     dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_RE]/EaGirder_Atr;
                     dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_RE]*(tsDetails.Ytr - tsDetails.DeckRebar[matType][barType].Ys)/EaGirder_Itr;
                     dPi  = dei*EDeckRebar*tsDetails.DeckRebar[matType][barType].As;

                     tsDetails.DeckRebar[matType][barType].dei[pfType] += dei;
                     tsDetails.DeckRebar[matType][barType].dPi[pfType] += dPi;
                  }

                  tsDetails.DeckRebar[matType][barType].Pi[pfType]  += prevTimeStepDetails.DeckRebar[matType][barType].Pi[pfType] + tsDetails.DeckRebar[matType][barType].dPi[pfType];
                  tsDetails.DeckRebar[matType][barType].dP          += tsDetails.DeckRebar[matType][barType].dPi[pfType];
                  tsDetails.DeckRebar[matType][barType].P           += tsDetails.DeckRebar[matType][barType].Pi[pfType];

                  tsDetails.DeckRebar[matType][barType].ei[pfType]  += prevTimeStepDetails.DeckRebar[matType][barType].ei[pfType] + tsDetails.DeckRebar[matType][barType].dei[pfType];
                  tsDetails.DeckRebar[matType][barType].de          += tsDetails.DeckRebar[matType][barType].dei[pfType];
                  tsDetails.DeckRebar[matType][barType].e           += tsDetails.DeckRebar[matType][barType].ei[pfType];
               } // neck deck bar type
            } // next deck bat type
         } // end if deck is composite

         // Compute change in force in girder rebar
         IndexType rebarIdx = 0;
         std::vector<TIME_STEP_REBAR>::iterator rebarIter(tsDetails.GirderRebar.begin());
         std::vector<TIME_STEP_REBAR>::iterator rebarIterEnd(tsDetails.GirderRebar.end());
         for ( ; rebarIter != rebarIterEnd; rebarIter++, rebarIdx++ )
         {
            TIME_STEP_REBAR& tsRebar(*rebarIter);
            dei = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dPi[pfType]/EaGirder_Atr)
                + (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]*(tsDetails.Ytr - tsRebar.Ys)/EaGirder_Itr);
            dPi = dei*EGirderRebar*tsRebar.As;

            tsRebar.dei[pfType] += dei;
            tsRebar.dPi[pfType] += dPi;

            if ( pfType == pftCreep )
            {
               dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_CR]/EaGirder_Atr;
               dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_CR]*(tsDetails.Ytr - tsRebar.Ys)/EaGirder_Itr;
               dPi  = dei*EGirderRebar*tsRebar.As;

               tsRebar.dei[pfType] += dei;
               tsRebar.dPi[pfType] += dPi;
            }
            else if ( pfType == pftShrinkage )
            {
               dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_SH]/EaGirder_Atr;
               dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_SH]*(tsDetails.Ytr - tsRebar.Ys)/EaGirder_Itr;
               dPi  = dei*EGirderRebar*tsRebar.As;

               tsRebar.dei[pfType] += dei;
               tsRebar.dPi[pfType] += dPi;
            }
            else if ( pfType == pftRelaxation )
            {
               dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_RE]/EaGirder_Atr;
               dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_RE]*(tsDetails.Ytr - tsRebar.Ys)/EaGirder_Itr;
               dPi  = dei*EGirderRebar*tsRebar.As;

               tsRebar.dei[pfType] += dei;
               tsRebar.dPi[pfType] += dPi;
            }

            tsRebar.Pi[pfType]  += prevTimeStepDetails.GirderRebar[rebarIdx].Pi[pfType] + tsRebar.dPi[pfType];
            tsRebar.dP          += tsRebar.dPi[pfType];
            tsRebar.P           += tsRebar.Pi[pfType];

            tsRebar.ei[pfType]  += prevTimeStepDetails.GirderRebar[rebarIdx].ei[pfType] + tsRebar.dei[pfType];
            tsRebar.de          += tsRebar.dei[pfType];
            tsRebar.e           += tsRebar.ei[pfType];
         }

         // Compute change in force in strands
         if ( bIsOnSegment )
         {
            const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
            std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
            std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
            for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
            {
               pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
               // change in strand force
               dei = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dPi[pfType]/EaGirder_Atr)
                   + (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys)/EaGirder_Itr);
               dPi = dei*EStrand[strandType]*tsDetails.Strands[strandType].As;

               tsDetails.Strands[strandType].dei[pfType] += dei;
               tsDetails.Strands[strandType].dPi[pfType] += dPi;

               if ( pfType == pftCreep )
               {
                  dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_CR]/EaGirder_Atr;
                  dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_CR]*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys)/EaGirder_Itr;
                  dPi  = dei*EStrand[strandType]*tsDetails.Strands[strandType].As;

                  tsDetails.Strands[strandType].dei[pfType] += dei;
                  tsDetails.Strands[strandType].dPi[pfType] += dPi;
               }
               else if ( pfType == pftShrinkage )
               {
                  dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_SH]/EaGirder_Atr;
                  dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_SH]*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys)/EaGirder_Itr;
                  dPi  = dei*EStrand[strandType]*tsDetails.Strands[strandType].As;

                  tsDetails.Strands[strandType].dei[pfType] += dei;
                  tsDetails.Strands[strandType].dPi[pfType] += dPi;
               }
               else if ( pfType == pftRelaxation )
               {
                  dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_RE]/EaGirder_Atr;
                  dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_RE]*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys)/EaGirder_Itr;
                  dPi  = dei*EStrand[strandType]*tsDetails.Strands[strandType].As + tsDetails.Strands[strandType].PrRelaxation;

                  tsDetails.Strands[strandType].dei[pfType] += dei;
                  tsDetails.Strands[strandType].dPi[pfType] += dPi;
               }

               tsDetails.Strands[strandType].Pi[pfType]  += prevTimeStepDetails.Strands[strandType].Pi[pfType] + tsDetails.Strands[strandType].dPi[pfType];
               tsDetails.Strands[strandType].dP          += tsDetails.Strands[strandType].dPi[pfType];
               tsDetails.Strands[strandType].P           += tsDetails.Strands[strandType].Pi[pfType];
 
               tsDetails.Strands[strandType].ei[pfType]  += prevTimeStepDetails.Strands[strandType].ei[pfType] + tsDetails.Strands[strandType].dei[pfType];
               tsDetails.Strands[strandType].de          += tsDetails.Strands[strandType].dei[pfType];
               tsDetails.Strands[strandType].e           += tsDetails.Strands[strandType].ei[pfType];

               // Losses and effective prestress
               tsDetails.Strands[strandType].dfpei[pfType] += IsZero(tsDetails.Strands[strandType].As) ? 0 : tsDetails.Strands[strandType].dPi[pfType]/tsDetails.Strands[strandType].As;
               tsDetails.Strands[strandType].dfpe += tsDetails.Strands[strandType].dfpei[pfType];
               if ( i == 0 )
               {
                  tsDetails.Strands[strandType].fpe  = prevTimeStepDetails.Strands[strandType].fpe;
                  tsDetails.Strands[strandType].loss = prevTimeStepDetails.Strands[strandType].loss;
               }
               tsDetails.Strands[strandType].fpe  += tsDetails.Strands[strandType].dfpei[pfType];
               tsDetails.Strands[strandType].loss -= tsDetails.Strands[strandType].dfpei[pfType];

               // check total loss = fpj - fpe
               ATLASSERT(IsEqual(tsDetails.Strands[strandType].loss,details.TimeStepDetails[stressStrandsIntervalIdx].Strands[strandType].fpj - tsDetails.Strands[strandType].fpe));
#else
               StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
               for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
               {
                  TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];

                  // change in strand force
                  dei = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dPi[pfType]/EaGirder_Atr)
                      + (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]*(tsDetails.Ytr - strand.Ys)/EaGirder_Itr);
                  dPi = dei*EStrand[strandType]*strand.As;

                  strand.dei[pfType] += dei;
                  strand.dPi[pfType] += dPi;

                  if ( pfType == pftCreep )
                  {
                     dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_CR]/EaGirder_Atr;
                     dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_CR]*(tsDetails.Ytr - strand.Ys)/EaGirder_Itr;
                     dPi  = dei*EStrand[strandType]*tsDetails.Strands[strandType].As;

                     strand.dei[pfType] += dei;
                     strand.dPi[pfType] += dPi;
                  }
                  else if ( pfType == pftShrinkage )
                  {
                     dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_SH]/EaGirder_Atr;
                     dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_SH]*(tsDetails.Ytr - strand.Ys)/EaGirder_Itr;
                     dPi  = dei*EStrand[strandType]*tsDetails.Strands[strandType].As;

                     strand.dei[pfType] += dei;
                     strand.dPi[pfType] += dPi;
                  }
                  else if ( pfType == pftRelaxation )
                  {
                     dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_RE]/EaGirder_Atr;
                     dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_RE]*(tsDetails.Ytr - strand.Ys)/EaGirder_Itr;
                     dPi  = dei*EStrand[strandType]*tsDetails.Strands[strandType].As + strand.PrRelaxation;

                     strand.dei[pfType] += dei;
                     strand.dPi[pfType] += dPi;
                  }

                  strand.Pi[pfType]  += prevTimeStepDetails.Strands[strandType][strandIdx].Pi[pfType] + strand.dPi[pfType];
                  strand.dP          += strand.dPi[pfType];
                  strand.P           += strand.Pi[pfType];

                  strand.ei[pfType]  += prevTimeStepDetails.Strands[strandType][strandIdx].ei[pfType] + strands.dei[pfType];
                  strand.de          += strands.dei[pfType];
                  strand.e           += strands.ei[pfType];

                  // Losses and effective prestress
                  strand.dfpei[pfType] += IsZero(strand.As) ? 0 : strand.dPi[pfType]/strand.As;
                  strand.dfpe += strand.dfpei[pfType];
                  if ( i == 0 )
                  {
                    strand.fpe  = prevTimeStepDetails.Strands[strandType][strandIdx].fpe;
                    strand.loss = prevTimeStepDetails.Strands[strandType][strandIdx].loss;
                  }
                  strand.fpe  += strand.dfpei[pfType];
                  strand.loss -= strand.dfpei[pfType];

                  // check total loss = fpj - fpe
                  ATLASSERT(IsEqual(strand.loss,details.TimeStepDetails[stressStrandsIntervalIdx].Strands[strandType][strandIdx].fpj - strand.fpe));
               } // next strand
#endif // LUMP_STRANDS
            } // next strand type

            // force due to pretensioning at the end of this interval
            tsDetails.Pi[pftPretension] = prevTimeStepDetails.Pi[pftPretension] + tsDetails.dPi[pftPretension];
            tsDetails.Mi[pftPretension] = prevTimeStepDetails.Mi[pftPretension] + tsDetails.dMi[pftPretension];
         } // if not closure joint

         // Compute change in force in tendons
         if ( bIsOnGirder )
         {
            for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
            {
               TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

               IntervalIndexType stressTendonIntervalIdx  = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

               // Change in tendon force due to deformations during this interval
               // tendon forces are already set in the Initialize method at and before the stressing interval
               if ( stressTendonIntervalIdx < intervalIdx )
               {
                  dei = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dPi[pfType]/EaGirder_Atr)
                      + (IsZero(EaGirder_Itr) ? 0 : tsDetails.dMi[pfType]*(tsDetails.Ytr - tendon.Ys)/EaGirder_Itr);
                  dPi = dei*ETendon*tendon.As;

                  tendon.dei[pfType] += dei;
                  tendon.dPi[pfType] += dPi;

                  if ( pfType == pftCreep )
                  {
                     dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_CR]/EaGirder_Atr;
                     dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_CR]*(tsDetails.Ytr - tendon.Ys)/EaGirder_Itr;
                     dPi  = dei*ETendon*tendon.As;

                     tendon.dei[pfType] += dei;
                     tendon.dPi[pfType] += dPi;
                  }
                  else if ( pfType == pftShrinkage )
                  {
                     dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_SH]/EaGirder_Atr;
                     dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_SH]*(tsDetails.Ytr - tendon.Ys)/EaGirder_Itr;
                     dPi  = dei*ETendon*tendon.As;

                     tendon.dei[pfType] += dei;
                     tendon.dPi[pfType] += dPi;
                  }
                  else if ( pfType == pftRelaxation )
                  {
                     dei  = IsZero(EaGirder_Atr) ? 0 : -tsDetails.Pr[TIMESTEP_RE]/EaGirder_Atr;
                     dei += IsZero(EaGirder_Itr) ? 0 : -tsDetails.Mr[TIMESTEP_RE]*(tsDetails.Ytr - tendon.Ys)/EaGirder_Itr;
                     dPi  = dei*ETendon*tendon.As + tendon.PrRelaxation;

                     tendon.dei[pfType] += dei;
                     tendon.dPi[pfType] += dPi;
                  }

                  tendon.Pi[pfType]  += prevTimeStepDetails.Tendons[ductIdx].Pi[pfType] + tendon.dPi[pfType];
                  tendon.dP          += tendon.dPi[pfType];
                  tendon.P           += tendon.Pi[pfType];

                  tendon.ei[pfType]  += prevTimeStepDetails.Tendons[ductIdx].ei[pfType] + tendon.dei[pfType];
                  tendon.de          += tendon.dei[pfType];
                  tendon.e           += tendon.ei[pfType];
               }

               // Losses prestress
               tendon.dfpei[pfType] += (IsZero(tendon.As) ? 0 : tendon.dPi[pfType]/tendon.As);
               tendon.dfpe += tendon.dfpei[pfType];

               if ( intervalIdx == stressTendonIntervalIdx )
               {
                  // tendons are stressed, but not released
                  // effective stress at end of interval is fpj
                  if ( i == 0 )
                  {
                     tendon.fpe = tendon.fpj;
                  }
                  tendon.fpe  += tendon.dfpei[pfType];
                  tendon.loss += tendon.dfpei[pfType];
               }
               else if ( stressTendonIntervalIdx < intervalIdx )
               {
                  // effective stress at end of this interval = effective stress at end of previous interval + change in stress this interval
                  if ( i == 0 )
                  {
                     tendon.fpe  = prevTimeStepDetails.Tendons[ductIdx].fpe;
                     tendon.loss = prevTimeStepDetails.Tendons[ductIdx].loss;
                  }

                  tendon.fpe  += tendon.dfpei[pfType];
                  tendon.loss -= tendon.dfpei[pfType];

                  ATLASSERT(IsEqual(tendon.loss,details.TimeStepDetails[stressTendonIntervalIdx].Tendons[ductIdx].fpj - tendon.fpe));
               }
            } // next tendon
         }
      } // next product force

      //
      // Compute stress in the parts of the cross section due to live load
      //

      // Get Live Load Moment this interval
      Float64 MllMin, MllMax;
      m_pCombinedForces->GetCombinedLiveLoadMoment(intervalIdx,pgsTypes::lltDesign,poi,m_Bat,&MllMin,&MllMax);
      if ( bIsInClosure && intervalIdx < compositeClosureIntervalIdx )
      {
         MllMin = 0;
         MllMax = 0;
      }

      // Curvature due to live load moment
      Float64 rLLMin = IsZero(EaGirder_Itr) ? 0 : MllMin/EaGirder_Itr;
      Float64 rLLMax = IsZero(EaGirder_Itr) ? 0 : MllMax/EaGirder_Itr;

      // Axial force and moment in girder
      Float64 Pmin = EaGirder_An*rLLMin*(tsDetails.Ytr - tsDetails.Girder.Yn);
      Float64 Pmax = EaGirder_An*rLLMax*(tsDetails.Ytr - tsDetails.Girder.Yn);
      Float64 Mmin = EaGirder_In*rLLMin;
      Float64 Mmax = EaGirder_In*rLLMax;

      if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
      {
         tsDetails.Girder.fLLMin[pgsTypes::TopFace] = Pmin/tsDetails.Girder.An + Mmin*tsDetails.Girder.Yn/tsDetails.Girder.In;
         tsDetails.Girder.fLLMax[pgsTypes::TopFace] = Pmax/tsDetails.Girder.An + Mmax*tsDetails.Girder.Yn/tsDetails.Girder.In;
         if (tsDetails.Girder.fLLMax[pgsTypes::TopFace] < tsDetails.Girder.fLLMin[pgsTypes::TopFace])
         {
            std::swap(tsDetails.Girder.fLLMin[pgsTypes::TopFace],tsDetails.Girder.fLLMax[pgsTypes::TopFace]);
         }

         tsDetails.Girder.fLLMin[pgsTypes::BottomFace] = Pmin/tsDetails.Girder.An + Mmin*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
         tsDetails.Girder.fLLMax[pgsTypes::BottomFace] = Pmax/tsDetails.Girder.An + Mmax*(tsDetails.Girder.H + tsDetails.Girder.Yn)/tsDetails.Girder.In;
         if (tsDetails.Girder.fLLMax[pgsTypes::BottomFace] < tsDetails.Girder.fLLMin[pgsTypes::BottomFace])
         {
            std::swap(tsDetails.Girder.fLLMin[pgsTypes::BottomFace],tsDetails.Girder.fLLMax[pgsTypes::BottomFace]);
         }
      }

      // Compute stresses in deck due to live load
      // Axial force and moment in deck
      Pmin = EaDeck_An*rLLMin*(tsDetails.Ytr - tsDetails.Deck.Yn);
      Pmax = EaDeck_An*rLLMax*(tsDetails.Ytr - tsDetails.Deck.Yn);
      Mmin = EaDeck_In*rLLMin;
      Mmax = EaDeck_In*rLLMax;

      if ( compositeDeckIntervalIdx <= intervalIdx && !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
      {
         tsDetails.Deck.fLLMin[pgsTypes::TopFace] = Pmin/tsDetails.Deck.An + Mmin*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
         tsDetails.Deck.fLLMax[pgsTypes::TopFace] = Pmax/tsDetails.Deck.An + Mmax*(tsDetails.Deck.Yn - tsDetails.Deck.H)/tsDetails.Deck.In;
         if (tsDetails.Deck.fLLMax[pgsTypes::TopFace] < tsDetails.Deck.fLLMin[pgsTypes::TopFace])
         {
            std::swap(tsDetails.Deck.fLLMin[pgsTypes::TopFace],tsDetails.Deck.fLLMax[pgsTypes::TopFace]);
         }

         tsDetails.Deck.fLLMin[pgsTypes::BottomFace] = Pmin/tsDetails.Deck.An + Mmin*tsDetails.Deck.Yn/tsDetails.Deck.In;
         tsDetails.Deck.fLLMax[pgsTypes::BottomFace] = Pmax/tsDetails.Deck.An + Mmax*tsDetails.Deck.Yn/tsDetails.Deck.In;
         if (tsDetails.Deck.fLLMax[pgsTypes::BottomFace] < tsDetails.Deck.fLLMin[pgsTypes::BottomFace])
         {
            std::swap(tsDetails.Deck.fLLMin[pgsTypes::BottomFace],tsDetails.Deck.fLLMax[pgsTypes::BottomFace]);
         }
      }

      // Deck Rebar (not captured, isn't of any value)

      // Girder Rebar (not captured, isn't of any value)

      // Strands
      if ( bIsOnSegment )
      {
         const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
         std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
         for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
         {
            pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
            if ( intervalIdx < liveLoadIntervalIdx // before live load is applied
               || // OR
               IsZero(tsDetails.Strands[strandType].As) // there aren't any strands of this type
               )
            {
               tsDetails.Strands[strandType].dFllMin = 0;
               tsDetails.Strands[strandType].dFllMax = 0;
            }
            else
            {
               tsDetails.Strands[strandType].dFllMin = rLLMin*EStrand[strandType]*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys);
               tsDetails.Strands[strandType].dFllMax = rLLMax*EStrand[strandType]*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys);
            }
            tsDetails.Strands[strandType].fpeLLMin  = tsDetails.Strands[strandType].fpe  + tsDetails.Strands[strandType].dFllMin;
            tsDetails.Strands[strandType].lossLLMin = tsDetails.Strands[strandType].loss - tsDetails.Strands[strandType].dFllMin;

            tsDetails.Strands[strandType].fpeLLMax  = tsDetails.Strands[strandType].fpe  + tsDetails.Strands[strandType].dFllMax;
            tsDetails.Strands[strandType].lossLLMax = tsDetails.Strands[strandType].loss - tsDetails.Strands[strandType].dFllMax;
#else
            StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
            for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
               if ( intervalIdx < liveLoadIntervalIdx // before live load is applied
                  || // OR
                  IsZero(strand.As) // there aren't any strands of this type
                  )
               {
                  strand.dFllMin = 0;
                  strand.dFllMax = 0;
               }
               else
               {
                  // live load is on the structure... add elastic effect
                  strand.dFllMin = rLLMin*EStrand[strandType]*(tsDetails.Ytr - strand.Ys);
                  strand.dFllMax = rLLMax*EStrand[strandType]*(tsDetails.Ytr - strand.Ys);
               }
               strand.fpeLLMin  = strand.fpe  + strand.dFllMin;  // increasing stress in strand...
               strand.lossLLMin = strand.loss - strand.dFllMin;  // ...is the same as decreasing its loss

               strand.fpeLLMax  = strand.fpe  + strand.dFllMax;
               strand.lossLLMax = strand.loss - strand.dFllMax;
            } // next strand
#endif // LUMP_STRANDS
         } // next strand type
      }


      // Tendons
      if ( bIsOnGirder )
      {
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            TIME_STEP_STRAND& tendon = tsDetails.Tendons[ductIdx];

            if ( intervalIdx < liveLoadIntervalIdx // before live load
                 || // OR
                 IsZero(tendon.As) // aren't any strands in the duct
               )
            {
               tendon.dFllMin = 0;
               tendon.dFllMax = 0;
            }
            else
            {
               tendon.dFllMin = rLLMin*ETendon*(tsDetails.Ytr - tendon.Ys);
               tendon.dFllMax = rLLMax*ETendon*(tsDetails.Ytr - tendon.Ys);
            }
            tendon.fpeLLMin  = tendon.fpe  + tendon.dFllMin;
            tendon.lossLLMin = tendon.loss - tendon.dFllMin;

            tendon.fpeLLMax  = tendon.fpe  + tendon.dFllMax; // increasing stress in tendon due to external load...
            tendon.lossLLMax = tendon.loss - tendon.dFllMax; // ... is the same as reducing the loss
         } // next tendon
      }


      // total due to primary pt at the end of this interval
      tsDetails.Pi[pftPostTensioning] = prevTimeStepDetails.Pi[pftPostTensioning] + tsDetails.dPi[pftPostTensioning];
      tsDetails.Mi[pftPostTensioning] = prevTimeStepDetails.Mi[pftPostTensioning] + tsDetails.dMi[pftPostTensioning];

      // total due to secondary pt at the end of this interval
      tsDetails.Pi[pftSecondaryEffects] = prevTimeStepDetails.Pi[pftSecondaryEffects] + tsDetails.dPi[pftSecondaryEffects];
      tsDetails.Mi[pftSecondaryEffects] = prevTimeStepDetails.Mi[pftSecondaryEffects] + tsDetails.dMi[pftSecondaryEffects];
   } // end if interval < releaseIntervalidx

   //
   // Equilibrium Checks
   //
   Float64 incrementalAxialTolerance  = ::ConvertToSysUnits(0.1,unitMeasure::Newton);
   Float64 incrementalMomentTolerance = ::ConvertToSysUnits(0.1,unitMeasure::KilonewtonMeter);

   Float64 axialTolerance  = ::ConvertToSysUnits(0.1*intervalIdx,unitMeasure::Newton);
   Float64 momentTolerance = ::ConvertToSysUnits(0.1*intervalIdx,unitMeasure::KilonewtonMeter);

#pragma Reminder("UPDATE: make the equilibrium checks for debug builds only")
   // this will require updating the Time Step Parameters Chapter Builder

   // Check : Change in External Forces = Change in Internal Forces

   tsDetails.dPext = 0;
   tsDetails.dMext = 0;
   tsDetails.dPint = 0;
   tsDetails.dMint = 0;
   int N = GetProductForceCount();
   for ( int i = 0; i < N; i++ )
   {
      // Sum the change in externally applied loads
      ProductForceType pfType = (ProductForceType)i;
      tsDetails.dPext += tsDetails.dPi[pfType];
      tsDetails.dMext += tsDetails.dMi[pfType];

      // Sum the change in internal forces in each component of the cross section
      tsDetails.dPint += tsDetails.Girder.dPi[pfType] 
                       + tsDetails.Deck.dPi[pfType]
                       + tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbIndividual].dPi[pfType] 
                       + tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbLumpSum   ].dPi[pfType] 
                       + tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].dPi[pfType]
                       + tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum   ].dPi[pfType];

      tsDetails.dMint += tsDetails.Girder.dMi[pfType] + tsDetails.Girder.dPi[pfType]*(tsDetails.Ytr - tsDetails.Girder.Yn)
                       + tsDetails.Deck.dMi[pfType]   + tsDetails.Deck.dPi[pfType]  *(tsDetails.Ytr - tsDetails.Deck.Yn)
                       + tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbIndividual].dPi[pfType]*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbIndividual].Ys)
                       + tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbLumpSum   ].dPi[pfType]*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmTop   ][pgsTypes::drbLumpSum   ].Ys)
                       + tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].dPi[pfType]*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbIndividual].Ys)
                       + tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum   ].dPi[pfType]*(tsDetails.Ytr - tsDetails.DeckRebar[pgsTypes::drmBottom][pgsTypes::drbLumpSum   ].Ys);


      std::vector<TIME_STEP_REBAR>::iterator iter(tsDetails.GirderRebar.begin());
      std::vector<TIME_STEP_REBAR>::iterator end(tsDetails.GirderRebar.end());
      for ( ; iter != end; iter++ )
      {
         TIME_STEP_REBAR& tsRebar(*iter);
         tsDetails.dPint += tsRebar.dPi[pfType];
         tsDetails.dMint += tsRebar.dPi[pfType]*(tsDetails.Ytr - tsRebar.Ys);
      }

      const std::vector<pgsTypes::StrandType>& strandTypes = GetStrandTypes(segmentKey);
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIter(strandTypes.begin());
      std::vector<pgsTypes::StrandType>::const_iterator strandTypeIterEnd(strandTypes.end());
      for ( ; strandTypeIter != strandTypeIterEnd; strandTypeIter++ )
      {
         pgsTypes::StrandType strandType = *strandTypeIter;
#if defined LUMP_STRANDS
         tsDetails.dPint += tsDetails.Strands[strandType].dPi[pfType];
         tsDetails.dMint += tsDetails.Strands[strandType].dPi[pfType]*(tsDetails.Ytr - tsDetails.Strands[strandType].Ys);
#else
         StrandIndexType nStrands = m_pStrandGeom->GetStrandCount(segmentKey,strandType);
         for ( StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
         {
            TIME_STEP_STRAND& strand = tsDetails.Strands[strandType][strandIdx];
            tsDetails.dPint += strand.dPi[pfType];
            tsDetails.dMint += strand.dPi[pfType]*(tsDetails.Ytr - strand.Ys);
         } // next strand
#endif // LUMP_STRANDS
      } // next strand type

      if ( bIsOnGirder )
      {
         DuctIndexType nDucts = m_pTendonGeom->GetDuctCount(girderKey);
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            tsDetails.dPint += tsDetails.Tendons[ductIdx].dPi[pfType];
            tsDetails.dMint += tsDetails.Tendons[ductIdx].dPi[pfType]*(tsDetails.Ytr - tsDetails.Tendons[ductIdx].Ys);
         }
      }
   } // next product load type


#if defined _BETA_VERSION
   // change in internal force and moment must be the same as the change external force and moment
   // duing this interval
   ATLASSERT(IsEqual(tsDetails.dPext,tsDetails.dPint,incrementalAxialTolerance));
   ATLASSERT(IsEqual(tsDetails.dMext,tsDetails.dMint,incrementalMomentTolerance));

#if !defined _DEBUG
   if ( !IsEqual(tsDetails.dPext,tsDetails.dPint,incrementalAxialTolerance) )
   {
      CString strMsg;
      strMsg.Format(_T("Interval %d, POI %d : dPext != dPint (%s != %s)"),
         LABEL_INTERVAL(intervalIdx),
         poi.GetID(),
         ::FormatDimension(tsDetails.dPext,m_pDisplayUnits->GetGeneralForceUnit()),
         ::FormatDimension(tsDetails.dPint,m_pDisplayUnits->GetGeneralForceUnit()));

      AfxMessageBox(strMsg);
   }

   if ( !IsEqual(tsDetails.dMext,tsDetails.dMint,incrementalMomentTolerance) )
   {
      CString strMsg;
      strMsg.Format(_T("Interval %d, POI %d: dMext != dMint (%s != %s)"),
         LABEL_INTERVAL(intervalIdx),
         poi.GetID(),
         ::FormatDimension(tsDetails.dMext,m_pDisplayUnits->GetMomentUnit()),
         ::FormatDimension(tsDetails.dMint,m_pDisplayUnits->GetMomentUnit()));

      AfxMessageBox(strMsg);
   }
#endif // !_DEBUG
#endif // _BETA_VERSION

   // Check: Final external forces = final interal forces
   tsDetails.Pext = 0;
   tsDetails.Mext = 0;
   
   tsDetails.Pint = 0;
   tsDetails.Mint = 0;

   if ( intervalIdx < stressStrandsIntervalIdx )
   {
      // strands aren't stressed yet so there isn't any force
      // in the section
      tsDetails.Pext = 0;
      tsDetails.Mext = 0;
      tsDetails.Pint = 0;
      tsDetails.Mint = 0;
   }
   else if ( intervalIdx == stressStrandsIntervalIdx )
   {
      // strands are stressed this interval so the force at the end of the interval
      // is the change this interval
      tsDetails.Pext = tsDetails.dPext;
      tsDetails.Mext = tsDetails.dMext;
      tsDetails.Pint = tsDetails.dPint;
      tsDetails.Mint = tsDetails.dMint;
   }
   else if ( intervalIdx == releaseIntervalIdx )
   {
      // prestressed is transfered to the concrete section in this interval
      // internal forces need to be adjusted for stress in strand. generally internal
      // forces due to the strands are incremental changes but this interval, we need to
      // include the actual force transfered
      TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);
      tsDetails.Pext = prevTimeStepDetails.Pext + tsDetails.dPext;
      tsDetails.Mext = prevTimeStepDetails.Mext + tsDetails.dMext;
      tsDetails.Pint = prevTimeStepDetails.Pint + tsDetails.dPint - details.TimeStepDetails[stressStrandsIntervalIdx].dPint;
      tsDetails.Mint = prevTimeStepDetails.Mint + tsDetails.dMint - details.TimeStepDetails[stressStrandsIntervalIdx].dMint;
   }
   else
   {
      // force at end of this interval = force at end of previous interval + change in force during this interval
      TIME_STEP_DETAILS& prevTimeStepDetails(details.TimeStepDetails[intervalIdx-1]);
      tsDetails.Pext = prevTimeStepDetails.Pext + tsDetails.dPext;
      tsDetails.Mext = prevTimeStepDetails.Mext + tsDetails.dMext;
      tsDetails.Pint = prevTimeStepDetails.Pint + tsDetails.dPint;
      tsDetails.Mint = prevTimeStepDetails.Mint + tsDetails.dMint;
   }

   // Total internal force and moment at the end of this interval must equal
   // the total external force and moment
#if defined _BETA_VERSION
   ATLASSERT(IsEqual(tsDetails.Pext,tsDetails.Pint,axialTolerance));
   ATLASSERT(IsEqual(tsDetails.Mext,tsDetails.Mint,momentTolerance));

#if !defined _DEBUG
   // only want to put up message box if incremental values are the same but summation is different
   // if incrementals are different, summation will also be different and we would get
   // twice as many message boxes
   if ( !IsEqual(tsDetails.Pext,tsDetails.Pint,axialTolerance) && IsEqual(tsDetails.dPext,tsDetails.dPint,incrementalAxialTolerance))
   {
      CString strMsg;
      strMsg.Format(_T("Interval %d, POI %d: Pext != Pint (%s != %s)"),
         LABEL_INTERVAL(intervalIdx),
         poi.GetID(),
         ::FormatDimension(tsDetails.Pext,m_pDisplayUnits->GetGeneralForceUnit()),
         ::FormatDimension(tsDetails.Pint,m_pDisplayUnits->GetGeneralForceUnit()));

      AfxMessageBox(strMsg);
   }

   if ( !IsEqual(tsDetails.Mext,tsDetails.Mint,momentTolerance) && IsEqual(tsDetails.dMext,tsDetails.dMint,incrementalMomentTolerance) )
   {
      CString strMsg;
      strMsg.Format(_T("Interval %d, POI %d: Mext != Mint (%s != %s)"),
         LABEL_INTERVAL(intervalIdx),
         poi.GetID(),
         ::FormatDimension(tsDetails.Mext,m_pDisplayUnits->GetMomentUnit()),
         ::FormatDimension(tsDetails.Mint,m_pDisplayUnits->GetMomentUnit()));

      AfxMessageBox(strMsg);
   }
#endif // !_DEBUG
#endif // _BETA_VERSION
}

void CTimeStepLossEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
#pragma Reminder("UPDATE: this is a dummy implementation")
   // Need to do a better job of report time-step loss details for the details report

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress,   pDisplayUnits->GetStressUnit(),     false );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      std::_tostringstream os;
      if ( 1 < nSegments )
      {
         os << _T("Segment ") << LABEL_SEGMENT(segIdx) << _T(" - Losses in Pretensioned Strands");
      }
      else
      {
         os << _T("Losses in Pretensioned Strands");
      }

      rptRcTable* pPSLossTable = pgsReportStyleHolder::CreateDefaultTable(2+nIntervals,os.str().c_str());
      *pPara << pPSLossTable << rptNewLine;

      RowIndexType row = pPSLossTable->GetNumberOfHeaderRows();


      ColumnIndexType col = 0;
      (*pPSLossTable)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
      {
         (*pPSLossTable)(0,col) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("ps")) << rptNewLine << _T("Interval ") << LABEL_INTERVAL(intervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*pPSLossTable)(0,col) << rptNewLine << _T("Straight") << rptNewLine << _T("Harped");
         col++;
      }
      (*pPSLossTable)(0,col) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      (*pPSLossTable)(0,col) << rptNewLine << _T("Straight") << rptNewLine << _T("Harped");

      // Get the losses for this segment
      GET_IFACE(IPointOfInterest,pPoi);
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,segIdx),POI_ERECTED_SEGMENT | POI_TENTH_POINTS));
      std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
      for ( ; iter != end; iter++, row++ )
      {
         col = 0;
         const pgsPointOfInterest& poi(*iter);
         const LOSSDETAILS* pLossDetails = GetLosses(poi,INVALID_INDEX);

         (*pPSLossTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

         for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
         {
#if defined LUMP_STRANDS
            // dfpe is the change in force... values < 0 are losses (a reduction in force)
            // however, in the context of this table, we want losses to be positive
            // so that's why there is the - sign in stress.SetValue(-dfpe)
            (*pPSLossTable)(row,col) << stress.SetValue( -pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Straight].dfpe );
            (*pPSLossTable)(row,col) << rptNewLine;
            (*pPSLossTable)(row,col) << stress.SetValue( -pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Harped].dfpe );
#else
            (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Straight].dfpe );
            (*pPSLossTable)(row,col) << rptNewLine;
            (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Harped].dfpe );
#endif // LUMP_STRANDS
            col++;
         }

#if defined LUMP_STRANDS
         (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Straight].loss );
         (*pPSLossTable)(row,col) << rptNewLine;
         (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Harped].loss );
#else
#pragma Reminder("UPDATE: reporting of loss in strands")
         (*pPSLossTable)(row,col) << rptNewLine; //stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Straight].loss );
         (*pPSLossTable)(row,col) << rptNewLine;
         (*pPSLossTable)(row,col) << rptNewLine; //stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Harped].loss );
#endif // LLUMP_STRANDS
      } // next POI
   } // next segment

  
   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   if ( 0 < nDucts )
   {
      rptRcTable** pPTLossTable = new rptRcTable*[nDucts];
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         CString strTitle;

         IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

         strTitle.Format(_T("Duct %d - Losses in Post-Tension Tendon"),LABEL_DUCT(ductIdx));
         pPTLossTable[ductIdx] = pgsReportStyleHolder::CreateDefaultTable(4+(nIntervals-ptIntervalIdx),strTitle);
         *pPara << pPTLossTable[ductIdx] << rptNewLine;


         ColumnIndexType ductCol = 0;
         (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pF")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pA")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         for ( IntervalIndexType intervalIdx = ptIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
         {
            (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pt")) << rptNewLine << _T("Interval ") << LABEL_INTERVAL(intervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
         }
         (*pPTLossTable[ductIdx])(0,ductCol++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pT")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      } // next duct

      Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,0));
      RowIndexType row = pPTLossTable[0]->GetNumberOfHeaderRows();

      GET_IFACE(IPointOfInterest,pPoi);
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));
      std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
      for ( ; iter != end; iter++, row++ )
      {
         const pgsPointOfInterest& poi(*iter);
         if ( pPoi->IsOnGirder(poi) )
         {
            const LOSSDETAILS* pLossDetails = GetLosses(poi,INVALID_INDEX);

            for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
            {
               ColumnIndexType ductCol = 0;

               IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

               (*pPTLossTable[ductIdx])(row,ductCol++) << location.SetValue( POI_ERECTED_SEGMENT, poi );

               (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->FrictionLossDetails[ductIdx].dfpF );
               (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->FrictionLossDetails[ductIdx].dfpA );

               for ( IntervalIndexType intervalIdx = ptIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
               {
                  // dfpe is the change in force... values < 0 are losses (a reduction in force)
                  // however, in the context of this table, we want losses to be positive
                  // so that's why there is the - sign in stress.SetValue(-dfpe)
                  (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( -pLossDetails->TimeStepDetails[intervalIdx].Tendons[ductIdx].dfpe );
               }

               (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->TimeStepDetails.back().Tendons[ductIdx].loss );
            }
         }
      }

      // deletes the array of points, not the pointers themselves
      delete[] pPTLossTable;
   }
}

void CTimeStepLossEngineer::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
#pragma Reminder("UPDATE: implement - time step loss reporting")
   ATLASSERT(false);
   //m_Engineer.ReportFinalLosses(m_BeamType,segmentKey,pChapter,pDisplayUnits);
}


void CTimeStepLossEngineer::ComputeAnchorSetLosses(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64* pdfpAT,Float64* pdfpS,Float64* pXset)
{
   if ( pDuctData->nStrands == 0 )
   {
      // If there aren't any strands in the tendon, there aren't any losses.
      *pXset  = 0;
      *pdfpAT = 0;
      *pdfpS  = 0;
      return;
   }

   // solve with method of false position (aka regula falsi method)
   // http://en.wikipedia.org/wiki/False_position_method
   // http://mathworld.wolfram.com/MethodofFalsePosition.html

   Float64 Dset, wobble, friction;
   m_pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   const CGirderKey& girderKey(pPTData->GetGirder()->GetGirderKey());

   IntervalIndexType stressTendonIntervalIdx = m_pIntervals->GetStressTendonInterval(girderKey,ductIdx);

   Float64 Apt = m_pTendonGeom->GetTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);
   Float64 Pj  = (Apt == 0 ? 0 : m_pTendonGeom->GetPjack(girderKey,ductIdx));
   Float64 fpj = (Apt == 0 ? 0 : Pj/Apt);

   Float64 XsetMin, XsetMax; // position of end of anchor set zone measured from left end of girder
   Float64 DsetMin, DsetMax;
   Float64 dfpATMin, dfpATMax;
   Float64 dfpSMin, dfpSMax;
   BoundAnchorSet(pPTData,pDuctData,ductIdx,endType,Dset,pLosses,fpj,Lg,frMinIter,&XsetMin,&DsetMin,&dfpATMin,&dfpSMin,&XsetMax,&DsetMax,&dfpATMax,&dfpSMax);

   // If the solution is nailed, get the heck outta here
   if ( IsEqual(DsetMin,Dset) )
   {
      *pXset  = XsetMin;
      *pdfpAT = dfpATMin;
      *pdfpS  = dfpSMin;
      return;
   }

   if ( IsEqual(DsetMax,Dset) )
   {
      *pXset  = XsetMax;
      *pdfpAT = dfpATMax;
      *pdfpS  = dfpSMax;
      return;
   }

   // ok, we've got some work to do...
   int side = 0;
   long maxIter = 100;
   Float64 Xset;
   Float64 dfpAT;
   Float64 dfpS;
   long iter = 0;
   for ( iter = 0; iter < maxIter; iter++ )
   {
      Xset = ((DsetMin-Dset)*XsetMax - (DsetMax-Dset)*XsetMin)/((DsetMin-Dset) - (DsetMax-Dset));
      Float64 Dset1 = EvaluateAnchorSet(pPTData,pDuctData,ductIdx,endType,pLosses,fpj,Lg,frMinIter,Xset,&dfpAT,&dfpS);

      if ( IsEqual(Dset,Dset1) )
      {
         break;
      }

      if ( 0 < (Dset1-Dset)*(DsetMax-Dset) )
      {
         XsetMax = Xset;
         DsetMax = Dset1;
         if ( side == -1 )
         {
            DsetMin = (DsetMin+Dset)/2;
         }

         side = -1;
      }
      else if ( 0 < (Dset1-Dset)*(DsetMin-Dset) )
      {
         XsetMin = Xset;
         DsetMin = Dset1;
         if ( side == 1 )
         {
            DsetMax = (DsetMax+Dset)/2;
         }

         side = 1;
      }
      else
      {
         break;
      }
   }

   if ( maxIter <= iter )
   {
      ATLASSERT(false); // did not converge
   }

   *pXset  = Xset;
   *pdfpAT = dfpAT;
   *pdfpS  = dfpS;
}

void CTimeStepLossEngineer::BoundAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,Float64 Dset,LOSSES* pLosses,Float64 fpj,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64* pXsetMin,Float64* pDsetMin,Float64* pdfpATMin,Float64* pdfpSMin,Float64* pXsetMax,Float64* pDsetMax,Float64* pdfpATMax,Float64* pdfpSMax)
{
   const CSplicedGirderData* pGirder = pPTData->GetGirder();
   const CGirderGroupData*   pGroup  = pGirder->GetGirderGroup();
   const CPierData2*         pPier   = pGroup->GetPier(endType);

   SpanIndexType   spanIdx = (endType == pgsTypes::metStart ? pPier->GetNextSpan()->GetIndex() : pPier->GetPrevSpan()->GetIndex());
   GirderIndexType gdrIdx  = pGirder->GetIndex();

   Float64 span_length = m_pBridge->GetSpanLength(spanIdx,gdrIdx);

   const CGirderKey& girderKey(pGirder->GetGirderKey());
   Float64 girder_length = m_pBridge->GetGirderLength(girderKey);

   Float64 XsetMin, XsetMax;

   Float64 deltaX = ::ConvertToSysUnits(1.0,unitMeasure::Meter);
   Float64 K = 1.5; // increase deltaX by 50% each time it is used... 
   // exponentially grow the values we are trying to bound the solution.

   XsetMin = 0.5*span_length; // Take a guess at the location of the end of the anchor set zone
   if ( endType == pgsTypes::metEnd )
   {
      // need XsetMin measured from left end of girder
      XsetMin = Lg - XsetMin;
      deltaX *= -1;
   }

   Float64 dfpAT, dfpS;
   Float64 Dset1 = EvaluateAnchorSet(pPTData,pDuctData,ductIdx,endType,pLosses,fpj,Lg,frMinIter,XsetMin,&dfpAT,&dfpS);
   if ( IsEqual(Dset1,Dset) )
   {
      // Nailed it first guess!!!
      *pXsetMin  = XsetMin;
      *pDsetMin  = Dset;
      *pdfpATMin = dfpAT;
      *pdfpSMin  = dfpS;

      *pXsetMax  = *pXsetMin;
      *pDsetMax  = *pDsetMin;
      *pdfpATMax = *pdfpATMin;
      *pdfpSMax  = *pdfpSMin;

      return;
   }

   Float64 Dset1Last = 0;
   if ( Dset1 < Dset )
   {
      // XsetMin is too small... we have a lower bound
      *pXsetMin  = XsetMin;
      *pDsetMin  = Dset1;
      *pdfpATMin = dfpAT;
      *pdfpSMin  = dfpS;

      // find the upper bound
      XsetMax = XsetMin;
      while ( true )
      {
         XsetMax += deltaX;
         deltaX *= K;

         Dset1 = EvaluateAnchorSet(pPTData,pDuctData,ductIdx,endType,pLosses,fpj,Lg,frMinIter,XsetMax,&dfpAT,&dfpS);
         if ( Dset < Dset1 )
         {
            // LsetMax is too big... we have an upper bound
            *pXsetMax  = XsetMax;
            *pDsetMax  = Dset1;
            *pdfpATMax = dfpAT;
            *pdfpSMax  = dfpS;

            return;
         }

         if ( girder_length < XsetMax && IsEqual(Dset1,Dset1Last) )
         {
            // we've got too far and
            *pXsetMax = girder_length;
            *pDsetMax = Dset;
            *pdfpATMax = dfpAT;
            *pdfpSMax  = dfpS;
            return;
         }
         Dset1Last = Dset1;
      }
   }
   else if ( Dset < Dset1 )
   {
      // LsetMin is too big... we have the upper bound
      *pXsetMax  = XsetMin;
      *pDsetMax  = Dset1;
      *pdfpATMax = dfpAT;
      *pdfpSMax  = dfpS;

      // find the lower bound
      while ( true )
      {
         XsetMin -= deltaX;
         deltaX *= K;

         Dset1 = EvaluateAnchorSet(pPTData,pDuctData,ductIdx,endType,pLosses,fpj,Lg,frMinIter,XsetMin,&dfpAT,&dfpS);
         if ( Dset1 < Dset )
         {
            // XsetMin is too small.. we have a lower bound
            *pXsetMin  = XsetMin;
            *pDsetMin  = Dset1;
            *pdfpATMin = dfpAT;
            *pdfpSMin  = dfpS;

            return;
         }

         if ( XsetMin < 0 && IsEqual(Dset1,Dset1Last))
         {
            // we've got too far
            *pXsetMin = 0;
            *pDsetMin = Dset;
            *pdfpATMin = dfpAT;
            *pdfpSMin  = dfpS;
            return;
         }
         Dset1Last = Dset1;
      }
   }
}

Float64 CTimeStepLossEngineer::EvaluateAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 fpj,Float64 Lg,SectionLossContainer::iterator& frMinIter,Float64 Xset,Float64* pdfpAT,Float64* pdfpS)
{
   //
   // Computes Dset given an assumed length of the anchor set zone, Lset
   //
   FRICTIONLOSSDETAILS& minFrDetails = frMinIter->second.FrictionLossDetails[ductIdx];

   // Find friction loss at Xset
   Float64 dfpF_Xset;
   Float64 dfpF;
   Float64 Dset = 0;
   if ( endType == pgsTypes::metStart )
   {
      SectionLossContainer::iterator iter(pLosses->SectionLosses.begin());
      SectionLossContainer::iterator end(pLosses->SectionLosses.end());
      if ( minFrDetails.X < Xset )
      {
         // anchor set exceeds the length of the tendon
         Float64 X1 = minFrDetails.X;
         Float64 f1 = minFrDetails.dfpF;

         SectionLossContainer::iterator frIter = frMinIter;
         Float64 X2,f2;
         do
         {
            frIter--; // back up one location
            X2 = frIter->second.FrictionLossDetails[ductIdx].X;
            f2 = frIter->second.FrictionLossDetails[ductIdx].dfpF;
         } while (IsEqual(X1,X2));

         dfpF_Xset = ::LinInterp(Xset-X2,f2,f1,X1-X2);
         dfpF = f1;

         end = frMinIter;
      }
      else
      {
         // do a linear search for the loss details at locations that bound Lset
         Float64 X1  = iter->second.FrictionLossDetails[ductIdx].X;
         Float64 fr1 = iter->second.FrictionLossDetails[ductIdx].dfpF;
         iter++;
         for ( ; iter != end; iter++ )
         {
            Float64 X2  = iter->second.FrictionLossDetails[ductIdx].X;
            Float64 fr2 = iter->second.FrictionLossDetails[ductIdx].dfpF;

            if ( InRange(X1,Xset,X2) )
            {
               // use linear interpolation to get friction loss at Lset
               dfpF_Xset = ::LinInterp(Xset-X1,fr1,fr2,X2-X1);
               dfpF = 0;
               end = iter;
               break;
            }

            X1  = X2;
            fr1 = fr2;
         }
      }

      // Calculate incremental contribution to seating loss along the strand
      // This is numerical integration using the trapezoidal rule
      iter = pLosses->SectionLosses.begin();
      Float64 X1  = iter->second.FrictionLossDetails[ductIdx].X;
      Float64 fr1 = iter->second.FrictionLossDetails[ductIdx].dfpF;
      iter++;
      bool bDone = false;
      for ( ; !bDone; iter++ )
      {
         Float64 X2, fr2;

         if ( iter == end )
         {
            iter--;
            fr2 = iter->second.FrictionLossDetails[ductIdx].dfpF;

            X2  = Min(Lg,Xset);
            fr2 = (Xset < Lg ? dfpF_Xset : fr2);
            bDone = true;
         }
         else
         {
            X2  = iter->second.FrictionLossDetails[ductIdx].X;
            fr2 = iter->second.FrictionLossDetails[ductIdx].dfpF;
         }

         Float64 fpt1 = fpj - fr1;
         Float64 fpt2 = fpj - fr2;
         Float64 fps  = fpj - dfpF_Xset;
         Float64 dDset = (0.5*(fpt1+fpt2) - fps)*(X2-X1);
         Dset += dDset;

         X1  = X2;
         fr1 = fr2;
      }
   }
   else
   {
      SectionLossContainer::reverse_iterator rIter(pLosses->SectionLosses.rbegin());
      SectionLossContainer::reverse_iterator rIterEnd(pLosses->SectionLosses.rend());
      if ( Xset < minFrDetails.X )
      {
         // anchor set exceeds the length of the tendon
         Float64 X1 = minFrDetails.X;
         Float64 f1 = minFrDetails.dfpF;


         SectionLossContainer::iterator frIter = frMinIter;
         Float64 X2,f2;
         do
         {
            frIter++; // advance one location
            X2 = frIter->second.FrictionLossDetails[ductIdx].X;
            f2 = frIter->second.FrictionLossDetails[ductIdx].dfpF;
         } while (IsEqual(X1,X2));

         dfpF_Xset = ::LinInterp(Xset-X1,f1,f2,X2-X1);
         dfpF = f1;

         rIterEnd = SectionLossContainer::reverse_iterator(frMinIter);
      }
      else
      {
         Float64 X1  = rIter->second.FrictionLossDetails[ductIdx].X;
         Float64 fr1 = rIter->second.FrictionLossDetails[ductIdx].dfpF;
         rIter++;
         for ( ; rIter != rIterEnd; rIter++ )
         {
            Float64 X2  = rIter->second.FrictionLossDetails[ductIdx].X;
            Float64 fr2 = rIter->second.FrictionLossDetails[ductIdx].dfpF;

            if ( InRange(X2,Xset,X1) )
            {
               dfpF_Xset = ::LinInterp(Xset-X2,fr2,fr1,X1-X2);
               dfpF = 0;
               rIterEnd = rIter;
               break;
            }

            X1  = X2;
            fr1 = fr2;
         }
      }

      // Calculate incremental contribution to seating loss along the strand
      // This is numerical integration using the trapezoidal rule
      rIter = pLosses->SectionLosses.rbegin();
      Float64 X1  = rIter->second.FrictionLossDetails[ductIdx].X;
      Float64 fr1 = rIter->second.FrictionLossDetails[ductIdx].dfpF;
      rIter++;
      bool bDone = false;
      for ( ; !bDone; rIter++ )
      {
         Float64 X2, fr2;
         if ( rIter == rIterEnd )
         {
            rIter--;
            fr2 = rIter->second.FrictionLossDetails[ductIdx].dfpF;

            X2  = Max(Xset,0.0);
            fr2 = (0 < Xset ? dfpF_Xset : fr2);
            bDone = true;
         }
         else
         {
            X2  = rIter->second.FrictionLossDetails[ductIdx].X;
            fr2 = rIter->second.FrictionLossDetails[ductIdx].dfpF;
         }

         Float64 fpt1 = fpj - fr1;
         Float64 fpt2 = fpj - fr2;
         Float64 fps  = fpj - dfpF_Xset;
         Float64 dDset = (0.5*(fpt1+fpt2) - fps)*(X1-X2);
         Dset += dDset;

         X1  = X2;
         fr1 = fr2;
      }
   }

   *pdfpAT = 2*dfpF_Xset;

   if ( InRange(0.0,Xset,Lg) )
   {
      *pdfpS = 0;
   }
   else
   {
      *pdfpS  = 2*(dfpF_Xset - dfpF);
   }

   Float64 Ept = pPTData->pStrand->GetE();
   Dset /= Ept;
   return Dset;
}

LOSSDETAILS* CTimeStepLossEngineer::GetLossDetails(LOSSES* pLosses,const pgsPointOfInterest& poi)
{
   SectionLossContainer::iterator found(pLosses->SectionLosses.find(poi));
   ATLASSERT( found != pLosses->SectionLosses.end() );
   ATLASSERT(poi.GetID() == found->first.GetID());
   ATLASSERT(poi.GetID() == found->second.POI.GetID());
   return &(found->second);
}

std::vector<ProductForceType> CTimeStepLossEngineer::GetApplicableProductLoads(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bExternalForcesOnly)
{
   // creates a vector of the product force type for only those loads that are applied during this interval
   // or cause a force effect during this interval. An example of a load causing a force effect during
   // and interval after the load is applied is girder self-weight causes a secondary force effect when
   // temporary supports are removed
   //
   // if bExternalLoadsOnly is true, product force types are only for externally applied loads
   // Prestress, Post-tensioning, creep, shrinkage, and relaxation are excluded

   std::vector<ProductForceType> vProductForces;
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   std::vector<IntervalIndexType> vTSRemovalIntervals = m_pIntervals->GetTemporarySupportRemovalIntervals(segmentKey.groupIndex);
   bool bIsTempSupportRemovalInterval = false;
   if ( 0 < vTSRemovalIntervals.size() )
   {
      std::vector<IntervalIndexType>::iterator found = std::find(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),intervalIdx);
      bIsTempSupportRemovalInterval = (found == vTSRemovalIntervals.end() ? false : true);
   }

   CClosureKey closureKey;
   bool bIsInClosure = m_pPoi->IsInClosureJoint(poi,&closureKey);
   IntervalIndexType compositeClosureIntervalIdx = m_pIntervals->GetCompositeClosureJointInterval(segmentKey);

   // Force effects due to girder self weight occur at prestress release, storage, and erection of this interval,
   // erection of any other segment that is erected after this interval, and
   // any interval when a temporary support is removed after the segment is erected

   bool bGirderLoad = false;
   if ( bIsInClosure )
   {
      // poi is at a closure
      if ( compositeClosureIntervalIdx <= intervalIdx )
      {
         // this is after the closure is composite so it is considered to be part of the girder

         // if segments are erected after this poi is composite, the dead load of those segments effect this poi
         IntervalIndexType lastSegmentErectionIntervalIdx = m_pIntervals->GetLastSegmentErectionInterval(segmentKey);
         bool bDoesDeadLoadOfSegmentEffectThisClosure = (compositeClosureIntervalIdx < lastSegmentErectionIntervalIdx && intervalIdx <= lastSegmentErectionIntervalIdx ? true : false);

         // if temporary supports are removed after this closure becomes composite then deflections happen at this poi
         // count the number of intervals in the vTSRemovalIntervals vector that are greater than compositeClosureIntervalIdx... if there is 1 or more
         // temporary support removal effects this poi
         bool bTSRemovedAfterCompositeClosure = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater_equal<IntervalIndexType>(),compositeClosureIntervalIdx)));
         
         if ( bDoesDeadLoadOfSegmentEffectThisClosure || (bIsTempSupportRemovalInterval && bTSRemovedAfterCompositeClosure) )
         {
            bGirderLoad = true;
         }
      }
   }
   else
   {
      // poi is in a girder segment

      // girder load comes into play at release (obviously) and when support locations change
      // for storage and erection
      IntervalIndexType releasePrestressIntervalIdx = m_pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType storageIntervalIdx = m_pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType erectSegmentIntervalIdx = m_pIntervals->GetErectSegmentInterval(segmentKey);

      // girder load comes into play if there are segments erected after this segment is erected
      // consider the case of this segment being a cantilevered pier segment and during this interval
      // a drop in segment is erected and that new segment is supported by the cantilever end of this
      // segment. The dead load reaction of the drop-in is a point load on the cantilver.
      IntervalIndexType lastSegmentErectionIntervalIdx = m_pIntervals->GetLastSegmentErectionInterval(segmentKey);
      bool bDoesDeadLoadOfAnotherSegmentEffectThisSegment = (erectSegmentIntervalIdx < lastSegmentErectionIntervalIdx && intervalIdx <= lastSegmentErectionIntervalIdx ? true : false);

      // if temporary supports are removed after this segment is erected then deflections happen at this poi
      // count the number of intervals in the vTSRemovalIntervals vector that are greater or equal than erectSegmentIntervalIdx... if there is 1 or more
      // temporary support removal effects this poi
      bool bTSRemovedAfterSegmentErection = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater_equal<IntervalIndexType>(),erectSegmentIntervalIdx)));

      if ( intervalIdx == releasePrestressIntervalIdx || // load first introduced
           intervalIdx == storageIntervalIdx || // supports move
           intervalIdx == erectSegmentIntervalIdx || // supports move
           bDoesDeadLoadOfAnotherSegmentEffectThisSegment || // this segment gets loaded from a construction event
           (bIsTempSupportRemovalInterval && bTSRemovedAfterSegmentErection) ) // this segments gets loadede from a construction event
      {
         bGirderLoad = true;
      }
   }

   if ( bGirderLoad )
   {
      vProductForces.push_back(pftGirder);

      if ( !bExternalForcesOnly )
      {
         vProductForces.push_back(pftPretension);
      }
   }

   // Force effects due to dead loads that are applied along with the slab self-weight occur
   // at the deck casting interval and any interval when a temporary support is removed after
   // the slab (and related) dead load is applied
   IntervalIndexType castDeckIntervalIdx = m_pIntervals->GetCastDeckInterval();
   bool bTSRemovedAfterDeckCasting = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),castDeckIntervalIdx)));
   if ( intervalIdx == castDeckIntervalIdx || (bIsTempSupportRemovalInterval && bTSRemovedAfterDeckCasting) )
   {
      vProductForces.push_back(pftDiaphragm); // verify this... are there cases when we don't apply a diaphragm load?

      pgsTypes::SupportedDeckType deckType = m_pBridge->GetDeckType();
      if ( deckType != pgsTypes::sdtNone )
      {
         vProductForces.push_back(pftSlab);
         vProductForces.push_back(pftSlabPad);
      }

      if ( deckType == pgsTypes::sdtCompositeSIP )
      {
         vProductForces.push_back(pftSlabPanel);
      }

      // only model construction loads if the magnitude is non-zero
      std::vector<ConstructionLoad> vConstructionLoads;
      m_pProductLoads->GetConstructionLoad(segmentKey,&vConstructionLoads);
      std::vector<ConstructionLoad>::iterator constrLoadIter(vConstructionLoads.begin());
      std::vector<ConstructionLoad>::iterator constrLoadIterEnd(vConstructionLoads.end());
      for ( ; constrLoadIter != constrLoadIterEnd; constrLoadIter++ )
      {
         ConstructionLoad& load = *constrLoadIter;
         if ( !IsZero(load.wStart) || !IsZero(load.wEnd) )
         {
            vProductForces.push_back(pftConstruction);
            break;
         }
      }

      if ( m_pProductLoads->HasShearKeyLoad(segmentKey) )
      {
         vProductForces.push_back(pftShearKey);
      }
   }

   IntervalIndexType installOverlayIntervalIdx = m_pIntervals->GetOverlayInterval();
   bool bTSRemovedAfterOverlayInstallation = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),installOverlayIntervalIdx)));
   if ( intervalIdx == installOverlayIntervalIdx || 
       (bIsTempSupportRemovalInterval && bTSRemovedAfterOverlayInstallation)
      )
   {
      vProductForces.push_back(pftOverlay);
   }

   IntervalIndexType installRailingSystemIntervalIdx = m_pIntervals->GetInstallRailingSystemInterval();
   bool bTSRemovedAfterRailingSystemInstalled = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),installRailingSystemIntervalIdx)));
   if ( intervalIdx == installRailingSystemIntervalIdx || 
        (bIsTempSupportRemovalInterval && bTSRemovedAfterRailingSystemInstalled)
      )
   {
      Float64 wTB = m_pProductLoads->GetTrafficBarrierLoad(segmentKey);
      Float64 wSW = m_pProductLoads->GetSidewalkLoad(segmentKey);

      if ( !IsZero(wTB) )
      {
         vProductForces.push_back(pftTrafficBarrier);
      }

      if ( !IsZero(wSW) )
      {
         vProductForces.push_back(pftSidewalk);
      }
   }


   // User defined loads

   // if user defined DC or DW is applied in this interval, or if they were applied
   // in a previous interval (before temporary support removal) and 
   // temporary supports are removed in this interval, include the product load type
   for ( int i = 0; i < 2; i++ )
   {
      ProductForceType pfType = (i == 0 ? pftUserDC : pftUserDW);

      // get all intervals when the user load is applied
      CSpanKey spanKey;
      Float64 Xspan;
      m_pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

      std::vector<IntervalIndexType> vUserLoadIntervals = m_pIntervals->GetUserDefinedLoadIntervals(spanKey,pfType);
      // remove all intervals that occur after this interval
      vUserLoadIntervals.erase(std::remove_if(vUserLoadIntervals.begin(),vUserLoadIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),intervalIdx)),vUserLoadIntervals.end());

      bool bIsUserDefinedLoadInterval = false;
      bool bIsTSRemovedAfterUserDefinedLoad = false;
      std::vector<IntervalIndexType>::iterator intervalIter(vUserLoadIntervals.begin());
      std::vector<IntervalIndexType>::iterator intervalIterEnd(vUserLoadIntervals.end());
      for ( ; intervalIter != intervalIterEnd; intervalIter++ )
      {
         IntervalIndexType loadingIntervalIdx = *intervalIter;
         if ( loadingIntervalIdx == intervalIdx )
         {
            // the user defined load applied is applied in this interval
            bIsUserDefinedLoadInterval = true;
         }

         // count the number of temporary support removal intervals that occur after the loading interval
         // if it is greater than zero this loading is applied when a temporary support is removed
         bool bTSRemovedAfterLoadingApplied = (0 < std::count_if(vTSRemovalIntervals.begin(),vTSRemovalIntervals.end(),std::bind2nd(std::greater<IntervalIndexType>(),loadingIntervalIdx)));
         if ( bTSRemovedAfterLoadingApplied )
         {
            bIsTSRemovedAfterUserDefinedLoad = true;
         }
      }

      if ( bIsUserDefinedLoadInterval || (bIsTempSupportRemovalInterval && bIsTSRemovedAfterUserDefinedLoad) )
      {
         vProductForces.push_back(pfType);
      }
   }

   if ( !bIsInClosure || (bIsInClosure && compositeClosureIntervalIdx <= intervalIdx) )
   {
      if ( m_pIntervals->IsTendonStressingInterval(segmentKey,intervalIdx) )
      {
         if ( !bExternalForcesOnly )
         {
            vProductForces.push_back(pftPostTensioning);
            vProductForces.push_back(pftSecondaryEffects);
         }
      }

      // creep, shrinkage, and relaxation only occur in intervals with non-zero duration
      if ( !bExternalForcesOnly && !IsZero(m_pIntervals->GetDuration(intervalIdx)) )
      {
         if ( !m_pLossParams->IgnoreCreepEffects() )
         {
            vProductForces.push_back(pftCreep);
         }

         if ( !m_pLossParams->IgnoreShrinkageEffects() )
         {
            vProductForces.push_back(pftShrinkage);
         }

         if ( !m_pLossParams->IgnoreRelaxationEffects() )
         {
            vProductForces.push_back(pftRelaxation);
         }
      }
   }

   ATLASSERT(vProductForces.size() <= (bExternalForcesOnly ? 14 : GetProductForceCount()));

   return vProductForces;
}

void CTimeStepLossEngineer::InitializeStrandTypes(const CSegmentKey& segmentKey)
{
   std::vector<pgsTypes::StrandType> strandTypes;
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
      if ( 0 < m_pStrandGeom->GetStrandCount(segmentKey,strandType) )
      {
         strandTypes.push_back(strandType);
      }
   }

   m_StrandTypes.insert(std::make_pair(segmentKey,strandTypes));
}

const std::vector<pgsTypes::StrandType>& CTimeStepLossEngineer::GetStrandTypes(const CSegmentKey& segmentKey)
{
   std::map<CSegmentKey,std::vector<pgsTypes::StrandType>>::iterator found(m_StrandTypes.find(segmentKey));
   if ( found == m_StrandTypes.end() )
   {
      InitializeStrandTypes(segmentKey);
      found = m_StrandTypes.find(segmentKey);
   }

   return found->second;
}

int CTimeStepLossEngineer::GetProductForceCount()
{
   // returns the number of product forces that we need to consider from the ProductForceType enum.
   // all off the product forces count except for the two special cases at the end of the enum.
   int n = (int)pftProductForceTypeCount;
   n -= 1; // remove pftOverlayRating from the count as they are special cases and don't apply here
   return n;
}
