///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2014  Washington State Department of Transportation
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
#include "TimeStepLossEngineer.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\LoadFactors.h>

#include <PgsExt\ReportStyleHolder.h>
#include <Reporting\ReportNotes.h>
#include <PgsExt\GirderLabel.h>

#include <numeric>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define IGNORE_CREEP_EFFECTS
//#define IGNORE_SHRINKAGE_EFFECTS
//#define IGNORE_RELAXATION_EFFECTS



/////////////////////////////////////////////////////////////////////////////
// CTimeStepLossEngineer
HRESULT CTimeStepLossEngineer::FinalConstruct()
{
   return S_OK;
}

void CTimeStepLossEngineer::SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;
}

const LOSSDETAILS* CTimeStepLossEngineer::GetLosses(const pgsPointOfInterest& poi)
{
#pragma Reminder("UPDATE: Time Step Model")
   // THINK ABOUT HOW TIME STEP CAN BE RUN FROM INTERVAL 0 to INTERVAL i 
   // INSTEAD OF RUNNING FOR ALL INTERVALS ALL AT ONCE
   std::map<CGirderKey,LOSSES>::iterator found;
   CGirderKey girderKey(poi.GetSegmentKey());
   found = m_Losses.find( girderKey );
   if ( found == m_Losses.end() )
   {
      // losses haven't been computed for this girder yet.... compute them
      ComputeLosses(girderKey);

      found = m_Losses.find( girderKey );
      ATLASSERT(found != m_Losses.end());
   }

   std::map<pgsPointOfInterest,LOSSDETAILS>& losses(found->second.SectionLosses);
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator poiFound = losses.find(poi);

   if ( poiFound == losses.end() )
   {
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
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter1(losses.begin());
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter2(losses.begin());
      iter2++;
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(losses.end());
      for ( ; iter2 != end; iter1++, iter2++ )
      {
         const pgsPointOfInterest& poi1(iter1->first);
         const pgsPointOfInterest& poi2(iter2->first);
         if ( poi1 < poi && poi < poi2 )
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
   return &(*poiFound).second;
}

const LOSSDETAILS* CTimeStepLossEngineer::GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config)
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
   std::map<CGirderKey,LOSSES>::const_iterator found;
   found = m_Losses.find( girderKey );
   if ( found == m_Losses.end() )
   {
      // losses not found for this girder
      ComputeLosses(girderKey);

      found = m_Losses.find( girderKey );
      ATLASSERT(found != m_Losses.end());
   }

   return &(*found).second.AnchorSet[ductIdx];
}

Float64 CTimeStepLossEngineer::GetElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType)
{
#pragma Reminder("IMPLEMENT CTimeStepLossEngineer::GetElongation()")
   ATLASSERT(false);
   return 0;
}

void CTimeStepLossEngineer::ComputeLosses(const CGirderKey& girderKey)
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Computing prestress losses"));


   LOSSES losses;

   ComputeFrictionLosses(girderKey,&losses);
   ComputeAnchorSetLosses(girderKey,&losses);
   ComputeSectionLosses(girderKey,&losses);

   m_Losses.insert(std::make_pair(girderKey,losses));
}

void CTimeStepLossEngineer::ComputeFrictionLosses(const CGirderKey& girderKey,LOSSES* pLosses)
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Computing friction losses"));

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IPosttensionForce,pPTForce);
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(ILossParameters,pLossParams);
   Float64 Dset, wobble, friction;
   pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi(pIPOI->GetPointsOfInterest(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,ALL_SEGMENTS)));

   GET_IFACE(IGirder,pGdr);
   WebIndexType nWebs = pGdr->GetWebCount(girderKey);

   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi(*iter);

      LOSSDETAILS details;
      details.LossMethod = pgsTypes::TIME_STEP;

      //////////////////////////////////////////////////////////////////////////
      // Friction Losses
      //////////////////////////////////////////////////////////////////////////
      const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(poi.GetSegmentKey());
      const CPTData* pPTData = pGirder->GetPostTensioning();

      DuctIndexType nDucts = pTendonGeom->GetDuctCount(poi.GetSegmentKey());
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         FRICTIONLOSSDETAILS frDetails;

         const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);
         Float64 Pj;
         if ( pDuct->bPjCalc )
         {
            Pj = pPTForce->GetPjackMax(poi.GetSegmentKey(),pDuct->nStrands);
         }
         else
         {
            Pj = pDuct->Pj;
         }

         Float64 aps = pPTData->pStrand->GetNominalArea();
         StrandIndexType nStrands = pDuct->nStrands;
         Float64 Aps = aps*nStrands;
         Float64 fpj = (nStrands == 0 ? 0 : Pj/Aps);
         
         Float64 Xg = pIPOI->ConvertPoiToGirderCoordinate(poi); // distance along girder
         Float64 Lg = pBridge->GetGirderLength(poi.GetSegmentKey());

         // determine from which end of the girder to measure the angular change of the tendon path
         pgsTypes::MemberEndType endType;
         if ( pDuct->JackingEnd == pgsTypes::jeLeft )
         {
            endType = pgsTypes::metStart;
         }
         else if ( pDuct->JackingEnd == pgsTypes::jeRight )
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

         Float64 alpha = pTendonGeom->GetAngularChange(poi,ductIdx,endType);

         frDetails.X = Xg;
         frDetails.alpha = alpha;
         frDetails.dfpF = fpj*(1 - exp(-(friction*alpha + X*wobble)));

         details.FrictionLossDetails.push_back(frDetails);
      }

      pLosses->SectionLosses.insert(std::make_pair(poi,details));
   }
}

void CTimeStepLossEngineer::ComputeAnchorSetLosses(const CGirderKey& girderKey,LOSSES* pLosses)
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Computing anchor set losses"));

   // First, compute the seating wedge, then compute anchor set loss at each POI
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pIGirder);

   Float64 girder_length = pBridge->GetGirderLength(girderKey);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(girderKey);
   const CPTData* pPTData = pGirder->GetPostTensioning();

   GET_IFACE(ILossParameters,pLossParams);
   Float64 Dset, wobble, friction;
   pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   WebIndexType nWebs = pIGirder->GetWebCount(girderKey);


#pragma Reminder("UPDATE: this assumes that the PT starts/ends at the face of girder")
   // the input allows the PT to start and end at any arbitrary point along the girder

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);

      // find location of minimum friction loss. this is the location of no movement in the strand
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator frMinIter;
      if ( pDuct->JackingEnd == pgsTypes::jeLeft )
      {
         // jack at left end, no movement occurs at right end
         frMinIter = pLosses->SectionLosses.end();
         frMinIter--;
      }
      else if ( pDuct->JackingEnd == pgsTypes::jeRight )
      {
         // jack at right end, no movement occurs at left end
         frMinIter = pLosses->SectionLosses.begin();
      }
      else
      {
         // jack at both ends, no movement occurs somewhere in the middle... search for it
         std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
         std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());
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


      if ( pDuct->JackingEnd == pgsTypes::jeLeft || pDuct->JackingEnd == pgsTypes::jeBoth )
      {
         Float64 dfpAT, dfpS, Xset;
         ComputeAnchorSetLosses(pPTData,pDuct,ductIdx,pgsTypes::metStart,pLosses,girder_length,frMinIter,&dfpAT,&dfpS,&Xset);
         anchor_set.Lset[pgsTypes::metStart]  = Min(girder_length,Xset);
         anchor_set.dfpAT[pgsTypes::metStart] = dfpAT;
         anchor_set.dfpS[pgsTypes::metStart]  = dfpS;
      }
      else
      {
         anchor_set.Lset[pgsTypes::metStart]  = 0;
         anchor_set.dfpAT[pgsTypes::metStart] = 0;
         anchor_set.dfpS[pgsTypes::metStart]  = 0;
      }

      if ( pDuct->JackingEnd == pgsTypes::jeRight || pDuct->JackingEnd == pgsTypes::jeBoth  )
      {
         Float64 dfpAT, dfpS, Xset;
         ComputeAnchorSetLosses(pPTData,pDuct,ductIdx,pgsTypes::metEnd,pLosses,girder_length,frMinIter,&dfpAT,&dfpS,&Xset);
         anchor_set.Lset[pgsTypes::metEnd]  = Min(girder_length,girder_length - Xset);
         anchor_set.dfpAT[pgsTypes::metEnd] = dfpAT;
         anchor_set.dfpS[pgsTypes::metEnd]  = dfpS;
      }
      else
      {
         anchor_set.Lset[pgsTypes::metEnd]  = 0;
         anchor_set.dfpAT[pgsTypes::metEnd] = 0;
         anchor_set.dfpS[pgsTypes::metEnd]  = 0;
      }

      pLosses->AnchorSet.push_back(anchor_set);
   }

   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(iter->first);
      LOSSDETAILS& details(iter->second);

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         ANCHORSETDETAILS& anchorSetDetails( pLosses->AnchorSet[ductIdx] );
         FRICTIONLOSSDETAILS& frDetails(details.FrictionLossDetails[ductIdx]);

         Float64 dfpA[2] = {0,0};
         const CDuctData* pDuct = pPTData->GetDuct(ductIdx/nWebs);
         if ( frDetails.X <= anchorSetDetails.Lset[pgsTypes::metStart] )
         {
            // POI is in the left anchorage zone
            if ( IsZero(anchorSetDetails.Lset[pgsTypes::metStart]) )
               dfpA[pgsTypes::metStart] = 0;
            else
               dfpA[pgsTypes::metStart] = ::LinInterp(frDetails.X,anchorSetDetails.dfpAT[pgsTypes::metStart],anchorSetDetails.dfpS[pgsTypes::metStart],anchorSetDetails.Lset[pgsTypes::metStart]);
         }

         if ( girder_length-anchorSetDetails.Lset[pgsTypes::metEnd] <= frDetails.X )
         {
            // POI is in the right anchorage zone
            if ( IsZero(anchorSetDetails.Lset[pgsTypes::metEnd]) )
               dfpA[pgsTypes::metEnd] = 0;
            else
               dfpA[pgsTypes::metEnd] = ::LinInterp(anchorSetDetails.Lset[pgsTypes::metEnd] - (girder_length-frDetails.X),anchorSetDetails.dfpS[pgsTypes::metEnd],anchorSetDetails.dfpAT[pgsTypes::metEnd],anchorSetDetails.Lset[pgsTypes::metEnd]);
         }

         frDetails.dfpA = dfpA[pgsTypes::metStart] + dfpA[pgsTypes::metEnd];
      }

   }
}

void CTimeStepLossEngineer::ComputeSectionLosses(const CGirderKey& girderKey,LOSSES* pLosses)
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      CString strMsg;
      strMsg.Format(_T("Computing time-dependent losses: Interval %d of %d"),LABEL_INTERVAL(intervalIdx),nIntervals);
      pProgress->UpdateMessage(strMsg);

      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());

      // Initialize Time Step Analysis
      // Basically computes the restraining forces at each POI
      for ( ; iter != end; iter++ )
      {
         LOSSDETAILS& details(iter->second);
         const pgsPointOfInterest& poi(iter->first);

         InitializeTimeStepAnalysis(intervalIdx,poi,details);
      }

      if ( 0 < pIntervals->GetDuration(intervalIdx) )
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
   }
}

void CTimeStepLossEngineer::InitializeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details)
{
   // Initializes the time step analysis
   // Computes the force required to restrain the initial strains due to creep, shrinkage, and relaxation at each POI

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GET_IFACE(ILongRebarGeometry,pRebarGeom);
   GET_IFACE(ICombinedForces,pForces);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);


   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);

   IntervalIndexType stressStrandsIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   bool bIsClosure = poi.HasAttribute(POI_CLOSURE);
   IntervalIndexType compositeClosureIntervalIdx = (bIsClosure ? pIntervals->GetCompositeClosureJointInterval(segmentKey) : INVALID_INDEX);


   // Material Properties
   Float64 EGirder = (bIsClosure ? pMaterials->GetClosureJointEc(segmentKey,intervalIdx) : pMaterials->GetSegmentEc(segmentKey,intervalIdx));
   Float64 EDeck = pMaterials->GetDeckEc(intervalIdx);
   Float64 EStrand[3] = { pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetE(),
                          pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetE(),
                          pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE()};
   Float64 ETendon = pMaterials->GetTendonMaterial(girderKey)->GetE();
   
   Float64 gdrHeight = pGirder->GetHeight(poi);

   // Initialize time step details
   TIME_STEP_DETAILS tsDetails;

   // TIME PARAMETERS
   tsDetails.intervalIdx = intervalIdx;
   tsDetails.tStart      = pIntervals->GetStart(intervalIdx);
   tsDetails.tMiddle     = pIntervals->GetMiddle(intervalIdx);
   tsDetails.tEnd        = pIntervals->GetEnd(intervalIdx);

   // TRANSFORMED PROPERTIES OF COMPOSITE SECTION (all parts are transformed to equivalent girder concrete)
   tsDetails.Atr = pSectProp->GetAg(pgsTypes::sptTransformed,intervalIdx,poi);
   tsDetails.Itr = pSectProp->GetIx(pgsTypes::sptTransformed,intervalIdx,poi);
   tsDetails.Ytr = pSectProp->GetY(pgsTypes::sptTransformed,intervalIdx,poi,pgsTypes::TopGirder); // Zero elevation is at top of girder

   // SEGMENT PARAMETERS

   // net section properties of segment
   tsDetails.Girder.An  = pSectProp->GetNetAg(intervalIdx,poi);
   tsDetails.Girder.In  = pSectProp->GetNetIg(intervalIdx,poi);
   tsDetails.Girder.Ytn = pSectProp->GetNetYtg(intervalIdx,poi);
   tsDetails.Girder.Ybn = pSectProp->GetNetYbg(intervalIdx,poi);

   // DECK PARAMETERS

   // net section properties of deck
   tsDetails.Deck.An  = pSectProp->GetNetAd(intervalIdx,poi);
   tsDetails.Deck.In  = pSectProp->GetNetId(intervalIdx,poi);
   tsDetails.Deck.Ytn = pSectProp->GetNetYtd(intervalIdx,poi); // distance from CG of net section to top of deck
   tsDetails.Deck.Ybn = pSectProp->GetNetYbd(intervalIdx,poi); // distance from CG of net section to bottom of deck

   // deck rebar
   if ( compositeDeckIntervalIdx <= intervalIdx  )
   {
      // deck is composite so the rebar is in play
      tsDetails.DeckRebar[pgsTypes::drmTop   ].As = pRebarGeom->GetAsTopMat(poi,ILongRebarGeometry::All);
      tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys = pRebarGeom->GetTopMatLocation(poi,ILongRebarGeometry::All);

      tsDetails.DeckRebar[pgsTypes::drmBottom].As = pRebarGeom->GetAsBottomMat(poi,ILongRebarGeometry::All);
      tsDetails.DeckRebar[pgsTypes::drmBottom].Ys = pRebarGeom->GetBottomMatLocation(poi,ILongRebarGeometry::All);
   }

   // Girder/Closure Rebar
   CComPtr<IRebarSection> rebar_section;
   pRebarGeom->GetRebars(poi,&rebar_section);

   // POI is in a precast segment....
   CComPtr<IEnumRebarSectionItem> enum_items;
   rebar_section->get__EnumRebarSectionItem(&enum_items);
   CComPtr<IRebarSectionItem> item;
   while ( enum_items->Next(1,&item,NULL) != S_FALSE )
   {
      TIME_STEP_REBAR tsRebar;

      if ( (bIsClosure && intervalIdx < compositeClosureIntervalIdx) // POI is in a closure and the closure is not composite with the girder yet
            || // -OR-
           (intervalIdx < releaseIntervalIdx) // POI is in a segment and it is before the prestress is released
         )
      {
         // don't model the rebar
         tsRebar.As = 0;
         tsRebar.Ys = 0;
      }
      else
      {
         CComPtr<IRebar> rebar;
         item->get_Rebar(&rebar);

         Float64 Ab;
         rebar->get_NominalArea(&Ab);

         tsRebar.As = Ab;

         // location of rebar is measured from the bottom of the girder up
         // we want the location measured from the top down... subtract y from girder height
         CComPtr<IPoint2d> p;
         item->get_Location(&p);

         Float64 y;
         p->get_Y(&y);

         tsRebar.Ys = gdrHeight - y;
      }

      item.Release();

      tsDetails.GirderRebar.push_back(tsRebar);
   }

   // Compute unrestrained creep strain due to loads applied prior to this interval
   // Tadros 1977, Equations 3 and 4
   tsDetails.Girder.ec.reserve(intervalIdx);
   tsDetails.Girder.rc.reserve(intervalIdx);
   tsDetails.Deck.ec.reserve(intervalIdx);
   tsDetails.Deck.rc.reserve(intervalIdx);

   if ( 0 < intervalIdx )
   {
      for ( IntervalIndexType i = 0; i < intervalIdx-1; i++ )
      {
         // Girder
#if defined IGNORE_CREEP_EFFECTS
         Float64 Cs = 0;
         Float64 Ce = 0;
#else
         Float64 Cs = (bIsClosure ? pMaterials->GetClosureJointCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start) : pMaterials->GetSegmentCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::Start));
         Float64 Ce = (bIsClosure ? pMaterials->GetClosureJointCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End)   : pMaterials->GetSegmentCreepCoefficient(segmentKey,i,pgsTypes::Middle,intervalIdx,pgsTypes::End));
#endif

         Float64 EGirder = (bIsClosure ? pMaterials->GetClosureJointEc(segmentKey,i) : pMaterials->GetSegmentEc(segmentKey,i));
         Float64 EAGirder = EGirder*details.TimeStepDetails[i].Girder.An;
         Float64 EIGirder = EGirder*details.TimeStepDetails[i].Girder.In;

         Float64 dP = 0;
         Float64 dM = 0;
         for ( int lcIdx = 0; lcIdx < 18; lcIdx++ )
         {
            dP += details.TimeStepDetails[i].Girder.dP[lcIdx];
            dM += details.TimeStepDetails[i].Girder.dM[lcIdx];
         }

         Float64 e = IsZero(EAGirder) ? 0 : (Ce-Cs)*dP/EAGirder;
         Float64 r = IsZero(EIGirder) ? 0 : (Ce-Cs)*dM/EIGirder;

         tsDetails.Girder.e += e;
         tsDetails.Girder.r += r;

         TIME_STEP_CONCRETE::CREEP_STRAIN creepStrain;
         TIME_STEP_CONCRETE::CREEP_CURVATURE creepCurvature;

         creepStrain.A = details.TimeStepDetails[i].Girder.An;
         creepStrain.E = EGirder;
         creepStrain.P = dP;
         creepStrain.Cs = Cs;
         creepStrain.Ce = Ce;
         creepStrain.e = e;

         creepCurvature.I = details.TimeStepDetails[i].Girder.In;
         creepCurvature.E = EGirder;
         creepCurvature.M = dM;
         creepCurvature.Cs = Cs;
         creepCurvature.Ce = Ce;
         creepCurvature.r = r;

         tsDetails.Girder.ec.push_back(creepStrain);
         tsDetails.Girder.rc.push_back(creepCurvature);

         // Deck
#if defined IGNORE_CREEP_EFFECTS
         Cs = 0;
         Ce = 0;
#else
         Cs = pMaterials->GetDeckCreepCoefficient(i,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
         Ce = pMaterials->GetDeckCreepCoefficient(i,pgsTypes::Middle,intervalIdx,pgsTypes::End);
#endif

         Float64 Edeck = pMaterials->GetDeckEc(i);
         Float64 EAdeck = Edeck*details.TimeStepDetails[i].Deck.An;
         Float64 EIdeck = Edeck*details.TimeStepDetails[i].Deck.In;

         dP = 0;
         dM = 0;
         for ( int lcIdx = 0; lcIdx < 18; lcIdx++ )
         {
            dP += details.TimeStepDetails[i].Deck.dP[lcIdx];
            dM += details.TimeStepDetails[i].Deck.dM[lcIdx];
         }

         e = IsZero(EAdeck) ? 0 : (Ce-Cs)*dP/EAdeck;
         r = IsZero(EIdeck) ? 0 : (Ce-Cs)*dM/EIdeck;

         tsDetails.Deck.e += e;
         tsDetails.Deck.r += r;

         creepStrain.A = details.TimeStepDetails[i].Deck.An;
         creepStrain.E = Edeck;
         creepStrain.P = dP;
         creepStrain.Cs = Cs;
         creepStrain.Ce = Ce;
         creepStrain.e = e;

         creepCurvature.I = details.TimeStepDetails[i].Deck.In;
         creepCurvature.E = Edeck;
         creepCurvature.M = dM;
         creepCurvature.Cs = Cs;
         creepCurvature.Ce = Ce;
         creepCurvature.r = r;

         tsDetails.Deck.ec.push_back(creepStrain);
         tsDetails.Deck.rc.push_back(creepCurvature);
      }

      // Compute total unrestrained deformation due to creep and shrinkage during this interval
      // AKA Compute initial strain conditions due to creep and shrinkage
#if defined IGNORE_SHRINKAGE_EFFECTS
      Float64 esi = 0;
#else
      Float64 esi = (bIsClosure ? pMaterials->GetClosureJointFreeShrinkageStrain(segmentKey,intervalIdx) : pMaterials->GetSegmentFreeShrinkageStrain(segmentKey,intervalIdx));
#endif 
      tsDetails.Girder.esi = esi;

#if defined IGNORE_SHRINKAGE_EFFECTS
      esi = 0;
#else
      esi = pMaterials->GetDeckFreeShrinkageStrain(intervalIdx);
#endif
      tsDetails.Deck.esi = esi;

      // Compute forces to restrain creep and shrinkage deformations
      tsDetails.Girder.PrCreep     = -tsDetails.Girder.e   * tsDetails.Girder.An * EGirder; // Tadros 1977, Eqn 10
      tsDetails.Girder.MrCreep     = -tsDetails.Girder.r   * tsDetails.Girder.In * EGirder; // Tadros 1977, Eqn 11
      tsDetails.Girder.PrShrinkage = -tsDetails.Girder.esi * tsDetails.Girder.An * EGirder; // Tadros 1977, Eqn 10

      tsDetails.Deck.PrCreep     = -tsDetails.Deck.e   * tsDetails.Deck.An * EDeck; // Tadros 1977, Eqn 10
      tsDetails.Deck.MrCreep     = -tsDetails.Deck.r   * tsDetails.Deck.In * EDeck; // Tadros 1977, Eqn 11
      tsDetails.Deck.PrShrinkage = -tsDetails.Deck.esi * tsDetails.Deck.An * EDeck; // Tadros 1977, Eqn 10
   }

   // STRAND PARAMETERS
   if ( !bIsClosure && stressStrandsIntervalIdx <= intervalIdx ) // skip if at a closure joint (no strands in a closure joint) or if the strands are not yet stressed
   {
      // The strands are stressed
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = pgsTypes::StrandType(i);

         // time from strand stressing to end of this interval
         Float64 tStressing       = pIntervals->GetStart(stressStrandsIntervalIdx);
         Float64 tEndThisInterval = pIntervals->GetEnd(intervalIdx);
         tsDetails.Strands[strandType].tEnd = Max(0.0,tEndThisInterval - tStressing);

         // section properties
         tsDetails.Strands[strandType].As = pStrandGeom->GetStrandArea(segmentKey,intervalIdx,strandType);

         // location of strands from top of girder.... need to use eccentricity based on transformed section properties
         Float64 nEffectiveStrands = 0;
         tsDetails.Strands[strandType].Ys = (intervalIdx < stressStrandsIntervalIdx ? 0 : tsDetails.Ytr + pStrandGeom->GetEccentricity(pgsTypes::sptTransformed,intervalIdx,poi,strandType,&nEffectiveStrands));
         if ( nEffectiveStrands == 0 )
         {
            tsDetails.Strands[strandType].Ys = 0;
         }

         if ( intervalIdx == stressStrandsIntervalIdx )
         {
            // Strands are stress in this interval.. get Pjack and fpj
            tsDetails.Strands[strandType].Pj  = (tsDetails.Strands[strandType].As == 0 ? 0 : pStrandGeom->GetPjack(segmentKey,strandType));
            tsDetails.Strands[strandType].fpj = (tsDetails.Strands[strandType].As == 0 ? 0 : tsDetails.Strands[strandType].Pj/tsDetails.Strands[strandType].As);

            // strands relax over the duration of the interval. compute the amount of relaxation
   #if defined IGNORE_RELAXATION_EFFECTS
            tsDetails.Strands[strandType].fr = 0;
   #else
            tsDetails.Strands[strandType].fr = pMaterials->GetStrandRelaxation(segmentKey,tsDetails.Strands[strandType].tEnd,0.0,tsDetails.Strands[strandType].fpj,strandType);
   #endif
         }
         else
         {
            // strands were stressed in a previous interval
            if ( intervalIdx == releaseIntervalIdx )
            {
               // this the interval when the prestress force is release into the girders, apply the
               // prestress as an external force. The prestress force is the area of strand times
               // the effective stress in the strands at the end of the previous interval.
               // Negative sign because the force resisted by the girder section is equal and opposite
               // the force in the strand (stands put a compression force into the girder)
               Float64 P = -tsDetails.Strands[strandType].As*details.TimeStepDetails[intervalIdx-1].Strands[strandType].fpe;
               tsDetails.dP[pftPrestress] += P;
               tsDetails.dM[pftPrestress] += P*(tsDetails.Strands[strandType].Ys - tsDetails.Ytr);
            }

            // relaxation during this interval
            // by using the effective prestress at the end of the previous interval we get a very good approximation for 
            // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
   #if defined IGNORE_RELAXATION_EFFECTS
            tsDetails.Strands[strandType].fr = 0;
   #else
            tsDetails.Strands[strandType].fr = pMaterials->GetStrandRelaxation(segmentKey,tsDetails.Strands[strandType].tEnd,details.TimeStepDetails[intervalIdx-1].Strands[strandType].tEnd,details.TimeStepDetails[intervalIdx-1].Strands[strandType].fpe,strandType);
   #endif
         }

         // apparent strain due to relacation
         tsDetails.Strands[strandType].er = tsDetails.Strands[strandType].fr/EStrand[strandType]; // Tadros 1977, second term in Eqn 8

         // force required to resist apparent relaxation strain
         tsDetails.Strands[strandType].PrRelaxation = -tsDetails.Strands[strandType].fr*tsDetails.Strands[strandType].As; // Tadros 1977, Eqn 10
      }
   }

   // TENDON PARAMETERS
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType stressTendonIntervalIdx  = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

      TIME_STEP_STRAND tsTendon;

      Float64 tStressing = pIntervals->GetStart(stressTendonIntervalIdx);
      Float64 tEndThisInterval = pIntervals->GetEnd(intervalIdx);
      tsTendon.tEnd = Max(0.0,tEndThisInterval - tStressing);
      tsTendon.As = pTendonGeom->GetTendonArea(girderKey,intervalIdx,ductIdx);
      tsTendon.Ys = (intervalIdx < stressTendonIntervalIdx ? 0 : tsDetails.Ytr + pTendonGeom->GetEccentricity(intervalIdx,poi,ductIdx));

      Float64 dfpF = details.FrictionLossDetails[ductIdx].dfpF; // friction loss at this POI
      Float64 dfpA = details.FrictionLossDetails[ductIdx].dfpA; // anchor set loss at this POI
      // The jacking force that is transfered to the girder is Pjack - (Friction Loss + Anchor Set Loss)Apt
      tsTendon.Pj  = (tsTendon.As == 0 ? 0 : pTendonGeom->GetPjack(girderKey,ductIdx) - tsTendon.As*(dfpF + dfpA));
      tsTendon.fpj = (tsTendon.As == 0 ? 0 : tsTendon.Pj/tsTendon.As);

      // time from tendon stressing to end of this interval
      if ( intervalIdx < stressTendonIntervalIdx )
      {
         // tendons not installed yet, no relaxation
         tsTendon.fr = 0;
      }
      else if ( intervalIdx == stressTendonIntervalIdx )
      {
         // this the interval when the pt force is release into the girders, apply the
         // prestress as an external force. The prestress force is the area of strand times
         // the effective stress at the end of the previous interval.
         // Negative sign because the force resisted by the girder section is equal and opposite
         // the force in the strand
         Float64 P = -tsTendon.As*tsTendon.fpj;
         tsDetails.dP[pftTotalPostTensioning] += P;
         tsDetails.dM[pftTotalPostTensioning] += P*(tsTendon.Ys - tsDetails.Ytr);

         // relaxation during the stressing interval
#if defined IGNORE_RELAXATION_EFFECTS
         tsTendon.fr = 0;
#else
         tsTendon.fr = pMaterials->GetTendonRelaxation(girderKey,ductIdx,tsTendon.tEnd,0.0,tsTendon.fpj);
#endif
      }
      else
      {
         // by using the effective prestress at the end of the previous interval we get a very good approximation for 
         // the actual (reduced) relaxation. See "Time-Dependent Analysis of Composite Frames", Tadros, Ghali, Dilger, pg 876
#if defined IGNORE_RELAXATION_EFFECTS
         tsTendon.fr = 0;
#else
         tsTendon.fr = pMaterials->GetTendonRelaxation(girderKey,ductIdx,tsTendon.tEnd,details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].tEnd,details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].fpe);
#endif
      }

      // apparent strain due to relaxation
      tsTendon.er = tsTendon.fr/ETendon;

      // force required to resist apparent relaxation strain
      tsTendon.PrRelaxation = -tsTendon.fr*tsTendon.As;

      tsDetails.Tendons.push_back(tsTendon);
   }

   // Total force/moment to restrain initial strains
   // Tadros 1977, Eqn 12 and 13
   if ( releaseIntervalIdx <= intervalIdx )
   {
      // Total Restraining Force
      tsDetails.Pr[TIMESTEP_CR] = tsDetails.Girder.PrCreep     + tsDetails.Deck.PrCreep;
      
      tsDetails.Pr[TIMESTEP_SH] = tsDetails.Girder.PrShrinkage + tsDetails.Deck.PrShrinkage;
      
      tsDetails.Pr[TIMESTEP_PS] = tsDetails.Strands[pgsTypes::Straight ].PrRelaxation
                                + tsDetails.Strands[pgsTypes::Harped   ].PrRelaxation
                                + tsDetails.Strands[pgsTypes::Temporary].PrRelaxation;

      tsDetails.Mr[TIMESTEP_CR] = tsDetails.Girder.MrCreep + tsDetails.Girder.PrCreep*(tsDetails.Girder.Ytn - tsDetails.Ytr)
                                + tsDetails.Deck.MrCreep   - tsDetails.Deck.PrCreep   *(tsDetails.Deck.Ybn + tsDetails.Ytr);
      
      tsDetails.Mr[TIMESTEP_SH] = tsDetails.Girder.PrShrinkage*(tsDetails.Girder.Ytn - tsDetails.Ytr)
                                - tsDetails.Deck.PrShrinkage  *(tsDetails.Deck.Ybn + tsDetails.Ytr);
      
      tsDetails.Mr[TIMESTEP_PS] = tsDetails.Strands[pgsTypes::Straight ].PrRelaxation*(tsDetails.Strands[pgsTypes::Straight ].Ys - tsDetails.Ytr)
                                + tsDetails.Strands[pgsTypes::Harped   ].PrRelaxation*(tsDetails.Strands[pgsTypes::Harped   ].Ys - tsDetails.Ytr)
                                + tsDetails.Strands[pgsTypes::Temporary].PrRelaxation*(tsDetails.Strands[pgsTypes::Temporary].Ys - tsDetails.Ytr);

      for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         tsDetails.Pr[TIMESTEP_PS] += tsDetails.Tendons[ductIdx].PrRelaxation;
         tsDetails.Mr[TIMESTEP_PS] += tsDetails.Tendons[ductIdx].PrRelaxation*(tsDetails.Tendons[ductIdx].Ys - tsDetails.Ytr);
      }
   }

   details.TimeStepDetails.push_back(tsDetails);
}

void CTimeStepLossEngineer::AnalyzeInitialStrains(IntervalIndexType intervalIdx,const CGirderKey& girderKey,LOSSES* pLosses)
{
   // Compute the response to the inital strains (see Tadros 1977, section titled "Effects of Initial Strains")
   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(IIntervals,pIntervals);

   // Create a load group for the restraining forces in this interval
   GET_IFACE(IExternalLoading,pExtLoading);
   std::_tstring strLoadGroupName[3] = {_T("Creep"),_T("Shrinkage"),_T("Relaxation")};

#pragma Reminder("UPDATE: CreateLoadGroup doesn't work") // using hard coded InitialStrains loads groups created in the analysis agent
//   // for the time being... get this fixed
//   //pExtLoading->CreateLoadGroup(strLoadGroupName[TIMESTEP_CR].c_str());
//   //pExtLoading->CreateLoadGroup(strLoadGroupName[TIMESTEP_SH].c_str());
//   //pExtLoading->CreateLoadGroup(strLoadGroupName[TIMESTEP_PS].c_str());
//   // Using pre-defined product load types pftCreep, pftShrinkage, pftRelaxation

   // At each POI, compute the equivalent restraining loads and apply to the load group
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter1(pLosses->SectionLosses.begin());
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter2(iter1);
   iter2++;

   for ( ; iter2 != end; iter1++, iter2++ )
   {
      const pgsPointOfInterest& poi1(iter1->first);
      const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
      LOSSDETAILS& details1(iter1->second);
      bool bIsClosure1 = poi1.HasAttribute(POI_CLOSURE);
      bool bIsClosureEffective1 = (bIsClosure1 ? pIntervals->GetCompositeClosureJointInterval(segmentKey1) <= intervalIdx : false);
      
      const pgsPointOfInterest& poi2(iter2->first);
      const CSegmentKey& segmentKey2(poi2.GetSegmentKey());
      LOSSDETAILS& details2(iter2->second);
      bool bIsClosure2 = poi2.HasAttribute(POI_CLOSURE);
      bool bIsClosureEffective2 = (bIsClosure2 ? pIntervals->GetCompositeClosureJointInterval(segmentKey2) <= intervalIdx : false);

      TIME_STEP_DETAILS& tsDetails1(details1.TimeStepDetails[intervalIdx]);
      TIME_STEP_DETAILS& tsDetails2(details2.TimeStepDetails[intervalIdx]);

      // Compute initial strain at poi1
      Float64 Atr1 = tsDetails1.Atr;
      Float64 Itr1 = tsDetails1.Itr;
      Float64 E1 = (bIsClosure1 ? pMaterials->GetClosureJointEc(segmentKey1,intervalIdx) : pMaterials->GetSegmentEc(segmentKey1,intervalIdx));
      Float64 EA1 = Atr1*E1;
      Float64 EI1 = Itr1*E1;

      // Compute initial strain at poi2
      Float64 Atr2 = tsDetails2.Atr;
      Float64 Itr2 = tsDetails2.Itr;
      Float64 E2 = (bIsClosure2 ? pMaterials->GetClosureJointEc(segmentKey2,intervalIdx) : pMaterials->GetSegmentEc(segmentKey2,intervalIdx));
      Float64 EA2 = Atr2*E2;
      Float64 EI2 = Itr2*E2;

      for ( int i = 0; i < 3; i++ )
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
         Float64 e(0.5*(e1 + e2));
         Float64 r(0.5*(r1 + r2));

         // if POI is at a closure joint and the closure joint is not yet effective (cast and concrete has set)
         // there can't be any strains applied
         if ( (bIsClosure1 && !bIsClosureEffective1) || (bIsClosure2 && !bIsClosureEffective2) )
         {
            e = 0;
            r = 0;
         }

         // Negative signs because we want to impose deformations that are equal and opposite
         // to the deformations required to fully restrain the initial strains
         tsDetails1.e[i][pgsTypes::Ahead] = -e;
         tsDetails1.r[i][pgsTypes::Ahead] = -r;

         tsDetails2.e[i][pgsTypes::Back] = -e;
         tsDetails2.r[i][pgsTypes::Back] = -r;

         pExtLoading->CreateInitialStrainLoad(intervalIdx,poi1,poi2,e,r,strLoadGroupName[i].c_str());
      }
   }

   // Get the interal forces caused by the initial strains
   std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(iter->first);
      bool bIsClosure = poi.HasAttribute(POI_CLOSURE);
      IntervalIndexType compositeClosureIntervalIdx = (bIsClosure ? pIntervals->GetCompositeClosureJointInterval(poi.GetSegmentKey()) : INVALID_INDEX);

      LOSSDETAILS& details(iter->second);
      TIME_STEP_DETAILS& tsDetails(details.TimeStepDetails[intervalIdx]);

      for ( int i = 0; i < 3; i++ )
      {
         Float64 P = pExtLoading->GetAxial( intervalIdx,strLoadGroupName[i].c_str(),poi,pgsTypes::ContinuousSpan);
         Float64 M = pExtLoading->GetMoment(intervalIdx,strLoadGroupName[i].c_str(),poi,pgsTypes::ContinuousSpan);

         // If the POI is at a closure joint, and it is before the closure is composite
         // with the adjacent girder segments, load doesn't get transfered to the concrete.
         // At strongbacks, a moment is computed, but this is the moment in the strongback hardware,
         // not the moment in the closure.
         if ( bIsClosure && intervalIdx < compositeClosureIntervalIdx )
         {
            P = 0;
            M = 0;
         }

         tsDetails.Pre[i] = P;
         tsDetails.Mre[i] = M;
      }
   }
}

void CTimeStepLossEngineer::FinalizeTimeStepAnalysis(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LOSSDETAILS& details)
{
   // Determine the effect of the initial strains on the various parts of the cross section
   // Determine the change in force in each part of the cross section
   // Determine the stresses on the girder

   TIME_STEP_DETAILS& tsDetails(details.TimeStepDetails[intervalIdx]);

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GET_IFACE(ICombinedForces,pForces);
   GET_IFACE(IProductForces,pProductForces);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);

   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

   IntervalIndexType stressStrandsIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < releaseIntervalIdx )
   {
      // Prestress force not released onto segment yet...
      if ( stressStrandsIntervalIdx <= intervalIdx )
      {
         // strands are stressed, but not released
         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);

            // prestress loss is the intrinsic relaxation during this interval
            tsDetails.Strands[strandType].loss = tsDetails.Strands[strandType].fr;

            tsDetails.Strands[strandType].dP   = -tsDetails.Strands[strandType].loss * tsDetails.Strands[strandType].As;
            tsDetails.Strands[strandType].dFps = -tsDetails.Strands[strandType].loss;

            if ( intervalIdx == stressStrandsIntervalIdx )
            {
               // This is the interval when strands are stressed... 
               // Force in the strand at the end of the interval is equal to Pj - Change in Force during this interval
               // Effective prestress = fpj + dFps
               tsDetails.Strands[strandType].P   = tsDetails.Strands[strandType].Pj  + tsDetails.Strands[strandType].dP;
               tsDetails.Strands[strandType].fpe = tsDetails.Strands[strandType].fpj + tsDetails.Strands[strandType].dFps;
            }
            else
            {
               // Force in strand is force at end of previous interval plus change in force during this interval
               tsDetails.Strands[strandType].P   = details.TimeStepDetails[intervalIdx-1].Strands[strandType].P   + tsDetails.Strands[strandType].dP;

               // Effective prestress is effective prestress at end of previous interval plus change in stress during this interval
               tsDetails.Strands[strandType].fpe = details.TimeStepDetails[intervalIdx-1].Strands[strandType].fpe + tsDetails.Strands[strandType].dFps;
            }
         }
      }

      return; // nothing more to do
   }


   bool bIsClosure = poi.HasAttribute(POI_CLOSURE);
   IntervalIndexType compositeClosureIntervalIdx = (bIsClosure ? pIntervals->GetCompositeClosureJointInterval(segmentKey) : INVALID_INDEX);

   // Get Live Load Moment
   Float64 MllMin, MllMax;
   pForces->GetCombinedLiveLoadMoment(pgsTypes::lltDesign,intervalIdx,poi,pgsTypes::ContinuousSpan,&MllMin,&MllMax);

   for ( int i = 0; i < 12; i++ ) // only do this for the first 12 product force types
   {
      ProductForceType pfType = (ProductForceType)i;

      // Get section forces due to externally applied loads in this interval
      Float64 M = pProductForces->GetMoment(intervalIdx,pfType,poi,pgsTypes::ContinuousSpan);

      // If the POI is at a closure joint, and it is before the closure is composite
      // with the adjacent girder segments, the moment is zero. At strongbacks,
      // a moment is computed, but this is the moment in the strongback hardware,
      // not the moment in the closure.
      if ( bIsClosure && intervalIdx < compositeClosureIntervalIdx )
      {
         M = 0;
         MllMin = 0;
         MllMax = 0;
      }

      if ( intervalIdx == storageIntervalIdx )
      {
         // We don't want the moment while the girder segment is in storage. We want the change in moment
         // between release and storage. No new load was added, the boundary conditions changed causing a
         // change in moment.
         Float64 Mrelease = pProductForces->GetMoment(releaseIntervalIdx,pfType,poi,pgsTypes::ContinuousSpan);
         M -= Mrelease;
      }
      else if ( intervalIdx == erectionIntervalIdx )
      {
         // When the segment is erected we want the difference between the moment during storage and erection.
         // Again, the change in moment is due to a change in boundary conditions
         Float64 Mstorage = pProductForces->GetMoment(storageIntervalIdx,pfType,poi,pgsTypes::ContinuousSpan);
         M -= Mstorage;
      }

      tsDetails.dM[i] += M;
      //tsDetails.dP[i] += 0;
   }

   Float64 EGirder   = (bIsClosure ? 
                         pMaterials->GetClosureJointEc(segmentKey,intervalIdx) : 
                         pMaterials->GetSegmentEc(segmentKey,intervalIdx));
   Float64 EaGirder  = (bIsClosure ? 
                         pMaterials->GetClosureJointAgeAdjustedEc(segmentKey,intervalIdx) : 
                         pMaterials->GetSegmentAgeAdjustedEc(segmentKey,intervalIdx));
   Float64 EDeck      = pMaterials->GetDeckEc(intervalIdx);
   Float64 EaDeck     = pMaterials->GetDeckAgeAdjustedEc(intervalIdx);
   Float64 EStrand[3] = { pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetE(),
                          pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetE(),
                          pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetE()};

   Float64 ETendon = pMaterials->GetTendonMaterial(girderKey)->GetE();

   Float64 EDeckRebar, EGirderRebar, Fy, Fu;
   pMaterials->GetDeckRebarProperties(&EDeckRebar,&Fy,&Fu);

   if ( bIsClosure )
   {
      pMaterials->GetClosureJointLongitudinalRebarProperties(segmentKey,&EGirderRebar,&Fy,&Fu);
   }
   else
   {
      pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey,&EGirderRebar,&Fy,&Fu);
   }

   // EaGirder*tsDetails.Atr and EaGirder*tsDetails.Itr are used frequenlty... 
   // Use a variable here so we don't have to multiply it over and over
   Float64 EGirder_Atr = EGirder*tsDetails.Atr;
   Float64 EGirder_Itr = EGirder*tsDetails.Itr;

   Float64 EaGirder_Atr = EaGirder*tsDetails.Atr;
   Float64 EaGirder_Itr = EaGirder*tsDetails.Itr;

   // Compute total change of force on the section during this interval

   if ( !IsZero(EaGirder_Atr) )
   {
      tsDetails.dP[pftCreep]      += -(tsDetails.Pre[TIMESTEP_CR] + tsDetails.Pr[TIMESTEP_CR]);
      tsDetails.dP[pftShrinkage]  += -(tsDetails.Pre[TIMESTEP_SH] + tsDetails.Pr[TIMESTEP_SH]);
      tsDetails.dP[pftRelaxation] += -(tsDetails.Pre[TIMESTEP_PS] + tsDetails.Pr[TIMESTEP_PS]);
   }

   if ( !IsZero(EaGirder_Itr) )
   {
      tsDetails.dM[pftCreep]      += -(tsDetails.Mre[TIMESTEP_CR] + tsDetails.Mr[TIMESTEP_CR]);
      tsDetails.dM[pftShrinkage]  += -(tsDetails.Mre[TIMESTEP_SH] + tsDetails.Mr[TIMESTEP_SH]);
      tsDetails.dM[pftRelaxation] += -(tsDetails.Mre[TIMESTEP_PS] + tsDetails.Mr[TIMESTEP_PS]);
   }

   tsDetails.Girder.P = details.TimeStepDetails[intervalIdx-1].Girder.P;
   tsDetails.Girder.M = details.TimeStepDetails[intervalIdx-1].Girder.M;
   tsDetails.Deck.P = details.TimeStepDetails[intervalIdx-1].Deck.P;
   tsDetails.Deck.M = details.TimeStepDetails[intervalIdx-1].Deck.M;

   Float64 P = 0;
   Float64 M = 0;
   for ( int i = 0; i < 18; i++ )
   {
      P += tsDetails.dP[i];
      M += tsDetails.dM[i];

      // Deformation of the composite section this interval
      tsDetails.der[i] = (IsZero(EaGirder_Atr) ? 0 : tsDetails.dP[i]/EaGirder_Atr);
      tsDetails.drr[i] = (IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[i]/EaGirder_Itr);

      // Total deformation of the composite section
      tsDetails.er[i] += details.TimeStepDetails[intervalIdx-1].er[i] + tsDetails.der[i];
      tsDetails.rr[i] += details.TimeStepDetails[intervalIdx-1].rr[i] + tsDetails.drr[i];

      // Compute change in force in each part of the cross section for this interval

      // Girder
      tsDetails.Girder.dP[i]  = IsZero(EaGirder_Atr) ? 0 : tsDetails.dP[i]*EaGirder*tsDetails.Girder.An/EaGirder_Atr;
      tsDetails.Girder.dP[i] += IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[i]*EaGirder*tsDetails.Girder.An*(tsDetails.Girder.Ytn-tsDetails.Ytr)/EaGirder_Itr;
      tsDetails.Girder.dM[i]  = IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[i]*EaGirder*tsDetails.Girder.In/EaGirder_Itr;

      tsDetails.Girder.P += tsDetails.Girder.dP[i];
      tsDetails.Girder.M += tsDetails.Girder.dM[i];

      // Deck
      tsDetails.Deck.dP[i]  = IsZero(EaGirder_Atr) ? 0 : tsDetails.dP[i]*EaDeck*tsDetails.Deck.An/EaGirder_Atr;
      tsDetails.Deck.dP[i] -= IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[i]*EaDeck*tsDetails.Deck.An*(tsDetails.Deck.Ybn + tsDetails.Ytr)/EaGirder_Itr;
      tsDetails.Deck.dM[i]  = IsZero(EaGirder_Itr) ? 0 : tsDetails.dM[i]*EaDeck*tsDetails.Deck.In/EaGirder_Itr;

      tsDetails.Deck.P += tsDetails.Deck.dP[i];
      tsDetails.Deck.M += tsDetails.Deck.dM[i];
   }

   // Deck Rebar
   tsDetails.DeckRebar[pgsTypes::drmTop   ].dP  = IsZero(EaGirder_Atr) ? 0 : P*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmTop   ].As/EaGirder_Atr;
   tsDetails.DeckRebar[pgsTypes::drmTop   ].dP -= IsZero(EaGirder_Itr) ? 0 : M*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmTop   ].As*(tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys + tsDetails.Ytr)/EaGirder_Itr;
   tsDetails.DeckRebar[pgsTypes::drmBottom].dP  = IsZero(EaGirder_Atr) ? 0 : P*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmBottom].As/EaGirder_Atr;
   tsDetails.DeckRebar[pgsTypes::drmBottom].dP -= IsZero(EaGirder_Itr) ? 0 : M*EDeckRebar*tsDetails.DeckRebar[pgsTypes::drmBottom].As*(tsDetails.DeckRebar[pgsTypes::drmBottom].Ys + tsDetails.Ytr)/EaGirder_Itr;

   tsDetails.DeckRebar[pgsTypes::drmTop   ].P = details.TimeStepDetails[intervalIdx-1].DeckRebar[pgsTypes::drmTop   ].P + tsDetails.DeckRebar[pgsTypes::drmTop   ].dP;
   tsDetails.DeckRebar[pgsTypes::drmBottom].P = details.TimeStepDetails[intervalIdx-1].DeckRebar[pgsTypes::drmBottom].P + tsDetails.DeckRebar[pgsTypes::drmBottom].dP;

   // Girder Rebar
   IndexType rebarIdx = 0;
   std::vector<TIME_STEP_REBAR>::iterator rebarIter(tsDetails.GirderRebar.begin());
   std::vector<TIME_STEP_REBAR>::iterator rebarIterEnd(tsDetails.GirderRebar.end());
   for ( ; rebarIter != rebarIterEnd; rebarIter++, rebarIdx++ )
   {
      TIME_STEP_REBAR& tsRebar(*rebarIter);
      tsRebar.dP  = IsZero(EaGirder_Atr) ? 0 : P*EGirderRebar*tsRebar.As/EaGirder_Atr;
      tsRebar.dP += IsZero(EaGirder_Itr) ? 0 : M*EGirderRebar*tsRebar.As*(tsRebar.Ys - tsDetails.Ytr)/EaGirder_Itr;

      tsRebar.P = details.TimeStepDetails[intervalIdx-1].GirderRebar[rebarIdx].P + tsRebar.dP;
   }

   // Strands
   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = pgsTypes::StrandType(i);

      // change in strand force
      tsDetails.Strands[strandType].dP  = IsZero(EaGirder_Atr) ? 0 : P*EStrand[strandType]*tsDetails.Strands[strandType].As/EaGirder_Atr;
      tsDetails.Strands[strandType].dP += IsZero(EaGirder_Itr) ? 0 : M*EStrand[strandType]*tsDetails.Strands[strandType].As*(tsDetails.Strands[strandType].Ys - tsDetails.Ytr)/EaGirder_Itr;
      tsDetails.Strands[strandType].P  = details.TimeStepDetails[intervalIdx-1].Strands[strandType].P + tsDetails.Strands[strandType].dP;

      // Losses and effective prestress
      tsDetails.Strands[strandType].dFps = IsZero(tsDetails.Strands[strandType].As) ? 0 : tsDetails.Strands[strandType].dP/tsDetails.Strands[strandType].As;
      tsDetails.Strands[strandType].fpe  = details.TimeStepDetails[intervalIdx-1].Strands[strandType].fpe  + tsDetails.Strands[strandType].dFps;
      tsDetails.Strands[strandType].loss = details.TimeStepDetails[intervalIdx-1].Strands[strandType].loss - tsDetails.Strands[strandType].dFps;

      // check total loss = fpj - fpe
      ATLASSERT(IsEqual(tsDetails.Strands[strandType].loss,details.TimeStepDetails[stressStrandsIntervalIdx].Strands[strandType].fpj - tsDetails.Strands[strandType].fpe));

      // Add elastic effect due to live load
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
         // live load is on the structure... add elastic effect
         tsDetails.Strands[strandType].dFllMin = MllMin*(tsDetails.Strands[strandType].Ys - tsDetails.Ytr)/tsDetails.Itr;
         tsDetails.Strands[strandType].dFllMax = MllMax*(tsDetails.Strands[strandType].Ys - tsDetails.Ytr)/tsDetails.Itr;
      }
      tsDetails.Strands[strandType].fpeLLMin  = tsDetails.Strands[strandType].fpe  + tsDetails.Strands[strandType].dFllMin;
      tsDetails.Strands[strandType].lossLLMin = tsDetails.Strands[strandType].loss - tsDetails.Strands[strandType].dFllMin;

      tsDetails.Strands[strandType].fpeLLMax  = tsDetails.Strands[strandType].fpe  + tsDetails.Strands[strandType].dFllMax;
      tsDetails.Strands[strandType].lossLLMax = tsDetails.Strands[strandType].loss - tsDetails.Strands[strandType].dFllMax;
   }

   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType stressTendonIntervalIdx  = pIntervals->GetStressTendonInterval(girderKey,ductIdx);


      // Change in tendon force due to deformations during this interval
      if ( intervalIdx <= stressTendonIntervalIdx )
      {
         tsDetails.Tendons[ductIdx].dP = 0; // tendon not installed or grouted yet
         tsDetails.Tendons[ductIdx].P  = 0;
      }
      else
      {
         tsDetails.Tendons[ductIdx].dP  = IsZero(EaGirder_Atr) ? 0 : P*ETendon*tsDetails.Tendons[ductIdx].As/EaGirder_Atr;
         tsDetails.Tendons[ductIdx].dP += IsZero(EaGirder_Itr) ? 0 : M*ETendon*tsDetails.Tendons[ductIdx].As*(tsDetails.Tendons[ductIdx].Ys - tsDetails.Ytr)/EaGirder_Itr;
         tsDetails.Tendons[ductIdx].P  = details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].P + tsDetails.Tendons[ductIdx].dP;
      }

      // Losses and effective prestress
      tsDetails.Tendons[ductIdx].dFps = (tsDetails.Tendons[ductIdx].As == 0 ? 0 : tsDetails.Tendons[ductIdx].dP/tsDetails.Tendons[ductIdx].As);
      if ( intervalIdx < stressTendonIntervalIdx )
      {
         // interval is before tendons is stressed
         tsDetails.Tendons[ductIdx].fpe  = 0;
         tsDetails.Tendons[ductIdx].loss = 0;
      }
      else if ( intervalIdx == stressTendonIntervalIdx )
      {
         // tendons are stressed, but not released
         // effective stress at end of interval is fpj - relaxation in this interval
         tsDetails.Tendons[ductIdx].fpe  = tsDetails.Tendons[ductIdx].fpj + tsDetails.Tendons[ductIdx].dFps;
         tsDetails.Tendons[ductIdx].loss = tsDetails.Tendons[ductIdx].dFps;
      }
      else
      {
         // effective stress at end of this interval = effective stress at end of previous interval + change in stress this interval
         tsDetails.Tendons[ductIdx].fpe  = details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].fpe  + tsDetails.Tendons[ductIdx].dFps;
         tsDetails.Tendons[ductIdx].loss = details.TimeStepDetails[intervalIdx-1].Tendons[ductIdx].loss - tsDetails.Tendons[ductIdx].dFps;
      }
      ATLASSERT(IsEqual(tsDetails.Tendons[ductIdx].loss,tsDetails.Tendons[ductIdx].fpj - tsDetails.Tendons[ductIdx].fpe));

      // Add elastic effect due to live load
      if ( intervalIdx < liveLoadIntervalIdx // before live load
           || // OR
           IsZero(tsDetails.Tendons[ductIdx].As) // aren't any strands in the duct
         )
      {
         tsDetails.Tendons[ductIdx].dFllMin = 0;
         tsDetails.Tendons[ductIdx].dFllMax = 0;
      }
      else
      {
         // live load is on the structure... add elastic effect
         tsDetails.Tendons[ductIdx].dFllMin = MllMin*(tsDetails.Tendons[ductIdx].Ys - tsDetails.Ytr)/tsDetails.Itr;
         tsDetails.Tendons[ductIdx].dFllMax = MllMax*(tsDetails.Tendons[ductIdx].Ys - tsDetails.Ytr)/tsDetails.Itr;
      }
      tsDetails.Tendons[ductIdx].fpeLLMin  = tsDetails.Tendons[ductIdx].fpe  + tsDetails.Tendons[ductIdx].dFllMin;
      tsDetails.Tendons[ductIdx].lossLLMin = tsDetails.Tendons[ductIdx].loss - tsDetails.Tendons[ductIdx].dFllMin;

      tsDetails.Tendons[ductIdx].fpeLLMax  = tsDetails.Tendons[ductIdx].fpe  + tsDetails.Tendons[ductIdx].dFllMax;
      tsDetails.Tendons[ductIdx].lossLLMax = tsDetails.Tendons[ductIdx].loss - tsDetails.Tendons[ductIdx].dFllMax;
   }

   // Concrete stresses at end of interval
   // f = f end of previous interval + change in stress this interval
   if ( !IsZero(tsDetails.Girder.An) && !IsZero(tsDetails.Girder.In) )
   {
      for (int i = 0; i < 3; i++ )
      {
         tsDetails.Girder.f[pgsTypes::TopFace][i][ctIncremental] = tsDetails.Girder.dP[i]/tsDetails.Girder.An - tsDetails.Girder.dM[i]*tsDetails.Girder.Ytn/tsDetails.Girder.In;
         tsDetails.Girder.f[pgsTypes::TopFace][i][ctCummulative] = details.TimeStepDetails[intervalIdx-1].Girder.f[pgsTypes::TopFace][i][ctCummulative] + tsDetails.Girder.f[pgsTypes::TopFace][i][ctIncremental];
         
         tsDetails.Girder.f[pgsTypes::BottomFace][i][ctIncremental] = tsDetails.Girder.dP[i]/tsDetails.Girder.An + tsDetails.Girder.dM[i]*tsDetails.Girder.Ybn/tsDetails.Girder.In;
         tsDetails.Girder.f[pgsTypes::BottomFace][i][ctCummulative] = details.TimeStepDetails[intervalIdx-1].Girder.f[pgsTypes::BottomFace][i][ctCummulative] + tsDetails.Girder.f[pgsTypes::BottomFace][i][ctIncremental];
      }

      Float64 Hg = tsDetails.Girder.Ybn + tsDetails.Girder.Ytn;
      tsDetails.Girder.fLLMin[pgsTypes::TopFace]    = -MllMax*tsDetails.Ytr/tsDetails.Itr; // if MllMax > 0, causes compression in top of girder (fLLMin[pgsTypes::TopFace]) and tension in bottom of girder (fLLMax[pgsTypes::TopFace])
      tsDetails.Girder.fLLMax[pgsTypes::TopFace]    = -MllMin*tsDetails.Ytr/tsDetails.Itr; // if MllMin < 0, causes tension in top of girder (fLLMax[pgsTypes::TopFace]) and compression in botom of girder (fLLMin[pgsTypes::BottomFace])
      tsDetails.Girder.fLLMin[pgsTypes::BottomFace] =  MllMin*(Hg - tsDetails.Ytr)/tsDetails.Itr;
      tsDetails.Girder.fLLMax[pgsTypes::BottomFace] =  MllMax*(Hg - tsDetails.Ytr)/tsDetails.Itr;
   }

   if ( !IsZero(tsDetails.Deck.An) && !IsZero(tsDetails.Deck.In) )
   {
      if ( compositeDeckIntervalIdx <= intervalIdx )
      {
         for ( int i = 0; i < 3; i++ )
         {
            tsDetails.Deck.f[pgsTypes::TopFace][i][ctIncremental] = tsDetails.Deck.dP[i]/tsDetails.Deck.An - tsDetails.Deck.dM[i]*tsDetails.Deck.Ytn/tsDetails.Deck.In;
            tsDetails.Deck.f[pgsTypes::TopFace][i][ctCummulative] = details.TimeStepDetails[intervalIdx-1].Deck.f[pgsTypes::TopFace][i][ctCummulative] + tsDetails.Deck.f[pgsTypes::TopFace][i][ctIncremental];

            tsDetails.Deck.f[pgsTypes::BottomFace][i][ctIncremental] = tsDetails.Deck.dP[i]/tsDetails.Deck.An + tsDetails.Deck.dM[i]*tsDetails.Deck.Ybn/tsDetails.Deck.In;
            tsDetails.Deck.f[pgsTypes::BottomFace][i][ctCummulative] = details.TimeStepDetails[intervalIdx-1].Deck.f[pgsTypes::BottomFace][i][ctCummulative] + tsDetails.Deck.f[pgsTypes::BottomFace][i][ctIncremental];
         }

         Float64 tDeck = tsDetails.Deck.Ybn + tsDetails.Deck.Ytn;
         Float64 Ytr_TopDeck = tsDetails.Ytr + tDeck;
         Float64 Ytr_BotDeck = tsDetails.Ytr;
         tsDetails.Deck.fLLMin[pgsTypes::TopFace]    = -MllMax*(EDeck/EGirder)*Ytr_TopDeck/tsDetails.Itr;
         tsDetails.Deck.fLLMax[pgsTypes::TopFace]    = -MllMin*(EDeck/EGirder)*Ytr_TopDeck/tsDetails.Itr;
         tsDetails.Deck.fLLMin[pgsTypes::BottomFace] = -MllMax*(EDeck/EGirder)*Ytr_BotDeck/tsDetails.Itr;
         tsDetails.Deck.fLLMax[pgsTypes::BottomFace] = -MllMin*(EDeck/EGirder)*Ytr_BotDeck/tsDetails.Itr;
      }
   }

   // Equilibrium Checks
   // Check : Change in External Forces = Change in Internal Forces
   tsDetails.dPext = 0;
   tsDetails.dMext = 0;
   for ( int i = 0; i < 15; i++ )
   {
      tsDetails.dPext += tsDetails.dP[i];
      tsDetails.dMext += tsDetails.dM[i];
   }

   tsDetails.dPint = 0;
   tsDetails.dMint = 0;
   for ( int i = 0; i < 18; i++ )
   {
      tsDetails.dPint += tsDetails.Girder.dP[i] + tsDetails.Deck.dP[i];
      tsDetails.dMint += tsDetails.Girder.dM[i] + tsDetails.Girder.dP[i]*(tsDetails.Girder.Ytn - tsDetails.Ytr)
                       + tsDetails.Deck.dM[i]   - tsDetails.Deck.dP[i]  *(tsDetails.Deck.Ybn   + tsDetails.Ytr);
   }

   tsDetails.dPint += tsDetails.DeckRebar[pgsTypes::drmTop   ].dP 
                    + tsDetails.DeckRebar[pgsTypes::drmBottom].dP
                    + -tsDetails.dP[pftCreep]//(tsDetails.Pre[TIMESTEP_CR] + tsDetails.Pr[TIMESTEP_CR])
                    + -tsDetails.dP[pftShrinkage]//(tsDetails.Pre[TIMESTEP_SH] + tsDetails.Pr[TIMESTEP_SH])
                    + -tsDetails.dP[pftRelaxation];//(tsDetails.Pre[TIMESTEP_PS] + tsDetails.Pr[TIMESTEP_PS]);

   tsDetails.dMint += -tsDetails.DeckRebar[pgsTypes::drmTop   ].dP*(tsDetails.DeckRebar[pgsTypes::drmTop   ].Ys + tsDetails.Ytr)
                    + -tsDetails.DeckRebar[pgsTypes::drmBottom].dP*(tsDetails.DeckRebar[pgsTypes::drmBottom].Ys + tsDetails.Ytr)
                    + -tsDetails.dM[pftCreep]//(tsDetails.Mre[TIMESTEP_CR] + tsDetails.Mr[TIMESTEP_CR])
                    + -tsDetails.dM[pftShrinkage]//(tsDetails.Mre[TIMESTEP_SH] + tsDetails.Mr[TIMESTEP_SH])
                    + -tsDetails.dM[pftRelaxation];//(tsDetails.Mre[TIMESTEP_PS] + tsDetails.Mr[TIMESTEP_PS]);


   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
      tsDetails.dPint += tsDetails.Strands[strandType].dP;
      tsDetails.dMint += tsDetails.Strands[strandType].dP*(tsDetails.Strands[strandType].Ys - tsDetails.Ytr);
   }

   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      tsDetails.dPint += tsDetails.Tendons[ductIdx].dP;
      tsDetails.dMint += tsDetails.Tendons[ductIdx].dP*(tsDetails.Tendons[ductIdx].Ys - tsDetails.Ytr);
   }

   std::vector<TIME_STEP_REBAR>::iterator iter(tsDetails.GirderRebar.begin());
   std::vector<TIME_STEP_REBAR>::iterator end(tsDetails.GirderRebar.end());
   for ( ; iter != end; iter++ )
   {
      TIME_STEP_REBAR& tsRebar(*iter);
      tsDetails.dPint += tsRebar.dP;
      tsDetails.dMint += tsRebar.dP*(tsRebar.Ys - tsDetails.Ytr);
   }

   // Check: Final external forces = final interal forces
   tsDetails.Pext = 0;
   tsDetails.Mext = 0;
   
   tsDetails.Pint = 0;
   tsDetails.Mint = 0;

   if ( intervalIdx == 0 )
   {
      tsDetails.Pext = tsDetails.dPext;
      tsDetails.Mext = tsDetails.dMext;
      tsDetails.Pint = tsDetails.dPint;
      tsDetails.Mint = tsDetails.dMint;
   }
   else if ( intervalIdx == releaseIntervalIdx )
   {
      tsDetails.Pext = details.TimeStepDetails[intervalIdx-1].Pext + tsDetails.dPext;
      tsDetails.Mext = details.TimeStepDetails[intervalIdx-1].Mext + tsDetails.dMext;
      tsDetails.Pint = details.TimeStepDetails[intervalIdx-1].Pint + tsDetails.dPint - details.TimeStepDetails[0].dPint;
      tsDetails.Mint = details.TimeStepDetails[intervalIdx-1].Mint + tsDetails.dMint - details.TimeStepDetails[0].dMint;
   }
   else
   {
      // force at end of this intervale = force at end of previous interval plus change in force during this interval
      tsDetails.Pext = details.TimeStepDetails[intervalIdx-1].Pext + tsDetails.dPext;
      tsDetails.Mext = details.TimeStepDetails[intervalIdx-1].Mext + tsDetails.dMext;
      tsDetails.Pint = details.TimeStepDetails[intervalIdx-1].Pint + tsDetails.dPint;
      tsDetails.Mint = details.TimeStepDetails[intervalIdx-1].Mint + tsDetails.dMint;
   }
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

   GET_IFACE(IPointOfInterest,pPoi);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      std::_tostringstream os;
      if ( 1 < nSegments )
         os << _T("Segment ") << LABEL_SEGMENT(segIdx) << _T(" - Losses in Pretensioned Strands");
      else
         os << _T("Losses in Pretensioned Strands");

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

      Float64 end_size = pBridge->GetSegmentStartEndDistance(CSegmentKey(girderKey,segIdx));

      // Get the losses for this segment
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,segIdx)));
      std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
      for ( ; iter != end; iter++, row++ )
      {
         col = 0;
         const pgsPointOfInterest& poi(*iter);
         const LOSSDETAILS* pLossDetails = GetLosses(poi);

         (*pPSLossTable)(row,col++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

         for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
         {
            (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Straight].dFps );
            (*pPSLossTable)(row,col) << rptNewLine;
            (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Strands[pgsTypes::Harped].dFps );
            col++;
         }

         (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Straight].loss );
         (*pPSLossTable)(row,col) << rptNewLine;
         (*pPSLossTable)(row,col) << stress.SetValue( pLossDetails->TimeStepDetails.back().Strands[pgsTypes::Harped].loss );
      } // next POI
   } // next segment

   

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);

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

   std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)));
   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++, row++ )
   {
      const pgsPointOfInterest& poi(*iter);
      const LOSSDETAILS* pLossDetails = GetLosses(poi);

      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         ColumnIndexType ductCol = 0;

         IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

         (*pPTLossTable[ductIdx])(row,ductCol++) << location.SetValue( POI_ERECTED_SEGMENT, poi, end_size );

         (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->FrictionLossDetails[ductIdx].dfpF );
         (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->FrictionLossDetails[ductIdx].dfpA );

         for ( IntervalIndexType intervalIdx = ptIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
         {
            (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->TimeStepDetails[intervalIdx].Tendons[ductIdx].dFps );
         }

         (*pPTLossTable[ductIdx])(row,ductCol++) << stress.SetValue( pLossDetails->TimeStepDetails.back().Tendons[ductIdx].loss );
      }
   }

   // deletes the array of points, not the pointers themselves
   delete[] pPTLossTable;
}

void CTimeStepLossEngineer::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
#pragma Reminder("UPDATE: implement - time step loss reporting")
   ATLASSERT(false);
   //m_Engineer.ReportFinalLosses(m_BeamType,segmentKey,pChapter,pDisplayUnits);
}


void CTimeStepLossEngineer::ComputeAnchorSetLosses(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 Lg,std::map<pgsPointOfInterest,LOSSDETAILS>::iterator& frMinIter,Float64* pdfpAT,Float64* pdfpS,Float64* pXset)
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

   GET_IFACE(ILossParameters,pLossParams);
   Float64 Dset, wobble, friction;
   pLossParams->GetTendonPostTensionParameters(&Dset,&wobble,&friction);

   const CGirderKey& girderKey(pPTData->GetGirder()->GetGirderKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

   GET_IFACE(ITendonGeometry,pTendonGeom);
   Float64 Apt = pTendonGeom->GetTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);
   Float64 Pj  = (Apt == 0 ? 0 : pTendonGeom->GetPjack(girderKey,ductIdx));
   Float64 fpj = (Apt == 0 ? 0 : Pj/Apt);

   Float64 XsetMin, XsetMax; // position of end of anchor set zone measured from left end of girder
   Float64 DsetMin, DsetMax;
   Float64 dfpATMin, dfpATMax;
   Float64 dfpSMin, dfpSMax;
   BoundAnchorSet(pPTData,pDuctData,ductIdx,endType,Dset,pLosses,fpj,Lg,frMinIter,&XsetMin,&DsetMin,&dfpATMin,&dfpSMin,&XsetMax,&DsetMax,&dfpATMax,&dfpSMax);

   // If the solution nailed, get the heck outta here
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
         break;

      if ( (Dset1-Dset)*(DsetMax-Dset) > 0 )
      {
         XsetMax = Xset;
         DsetMax = Dset1;
         if ( side == -1 )
         {
            DsetMin = (DsetMin+Dset)/2;
         }

         side = -1;
      }
      else if ( (DsetMin-Dset)*(Dset1-Dset) > 0 )
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
      ATLASSERT(false); // did not converg
   }

   *pXset  = Xset;
   *pdfpAT = dfpAT;
   *pdfpS  = dfpS;
}

void CTimeStepLossEngineer::BoundAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,Float64 Dset,LOSSES* pLosses,Float64 fpj,Float64 Lg,std::map<pgsPointOfInterest,LOSSDETAILS>::iterator& frMinIter,Float64* pXsetMin,Float64* pDsetMin,Float64* pdfpATMin,Float64* pdfpSMin,Float64* pXsetMax,Float64* pDsetMax,Float64* pdfpATMax,Float64* pdfpSMax)
{
#pragma Reminder("UPDATE: this doesn't deal with the case of Lset > girder_length")
   const CSplicedGirderData* pGirder = pPTData->GetGirder();
   const CGirderGroupData*   pGroup  = pGirder->GetGirderGroup();
   const CPierData2*         pPier   = pGroup->GetPier(endType);

   SpanIndexType   spanIdx = (endType == pgsTypes::metStart ? pPier->GetNextSpan()->GetIndex() : pPier->GetPrevSpan()->GetIndex());
   GirderIndexType gdrIdx  = pGirder->GetIndex();

   GET_IFACE(IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(spanIdx,gdrIdx);

   Float64 XsetMin, XsetMax;

   Float64 deltaX = ::ConvertToSysUnits(1.0,unitMeasure::Meter);

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
      }
   }
}

Float64 CTimeStepLossEngineer::EvaluateAnchorSet(const CPTData* pPTData,const CDuctData* pDuctData,DuctIndexType ductIdx,pgsTypes::MemberEndType endType,LOSSES* pLosses,Float64 fpj,Float64 Lg,std::map<pgsPointOfInterest,LOSSDETAILS>::iterator& frMinIter,Float64 Xset,Float64* pdfpAT,Float64* pdfpS)
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
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator iter(pLosses->SectionLosses.begin());
      std::map<pgsPointOfInterest,LOSSDETAILS>::iterator end(pLosses->SectionLosses.end());
      if ( minFrDetails.X < Xset )
      {
         // anchor set exceeds the length of the tendon
         Float64 X1 = minFrDetails.X;
         Float64 f1 = minFrDetails.dfpF;
         frMinIter--; // back up one location
         Float64 X2 = frMinIter->second.FrictionLossDetails[ductIdx].X;
         Float64 f2 = frMinIter->second.FrictionLossDetails[ductIdx].dfpF;
         frMinIter++; // go back to where it was

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
         Float64 X2  = iter->second.FrictionLossDetails[ductIdx].X;
         Float64 fr2 = iter->second.FrictionLossDetails[ductIdx].dfpF;

         if ( iter == end )
         {
            X2  = Min(Lg,Xset);
            fr2 = (Xset < Lg ? dfpF_Xset : fr2);
            bDone = true;
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
      std::map<pgsPointOfInterest,LOSSDETAILS>::reverse_iterator rIter(pLosses->SectionLosses.rbegin());
      std::map<pgsPointOfInterest,LOSSDETAILS>::reverse_iterator rIterEnd(pLosses->SectionLosses.rend());
      if ( Xset < minFrDetails.X )
      {
         // anchor set exceeds the length of the tendon
         Float64 X1 = minFrDetails.X;
         Float64 f1 = minFrDetails.dfpF;
         frMinIter++;
         Float64 X2 = frMinIter->second.FrictionLossDetails[ductIdx].X;
         Float64 f2 = frMinIter->second.FrictionLossDetails[ductIdx].dfpF;
         frMinIter--;

         dfpF_Xset = ::LinInterp(Xset-X1,f1,f2,X2-X1);
         dfpF = f1;

         rIterEnd = std::map<pgsPointOfInterest,LOSSDETAILS>::reverse_iterator(frMinIter);
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
         Float64 X2  = rIter->second.FrictionLossDetails[ductIdx].X;
         Float64 fr2 = rIter->second.FrictionLossDetails[ductIdx].dfpF;

         if ( rIter == rIterEnd )
         {
            X2  = Max(Xset,0.0);
            fr2 = (0 < Xset ? dfpF_Xset : fr2);
            bDone = true;
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
      *pdfpS = 0;
   else
      *pdfpS  = 2*(dfpF_Xset - dfpF);

   Float64 Ept = pPTData->pStrand->GetE();
   Dset /= Ept;
   return Dset;
}
