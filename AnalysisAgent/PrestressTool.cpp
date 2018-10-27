///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2018  Washington State Department of Transportation
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
#include "PrestressTool.h"

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>

#include <iterator>
#include <algorithm>

CPrestressTool::CPrestressTool(IBroker* pBroker) :
m_pBroker(pBroker)
{
}

void CPrestressTool::GetPretensionStress(IntervalIndexType intervalIdx,ResultsType resultsType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   if ( resultsType == rtIncremental )
   {
      std::vector<Float64> fTopPrev, fBotPrev;
      std::vector<Float64> fTopThis, fBotThis;
      GetPretensionStress(intervalIdx-1,rtCumulative,vPoi,topLocation,botLocation,&fTopPrev,&fBotPrev);
      GetPretensionStress(intervalIdx  ,rtCumulative,vPoi,topLocation,botLocation,&fTopThis,&fBotThis);

      std::transform(fTopThis.begin(),fTopThis.end(),fTopPrev.begin(),std::back_inserter(*pfTop),std::minus<Float64>());
      std::transform(fBotThis.begin(),fBotThis.end(),fBotPrev.begin(),std::back_inserter(*pfBot),std::minus<Float64>());
   }

   GET_IFACE(IIntervals,         pIntervals);
   GET_IFACE_NOCHECK(IPretensionForce,   pPrestressForce);
   GET_IFACE_NOCHECK(IStrandGeometry,    pStrandGeom);
   GET_IFACE_NOCHECK(ISectionProperties, pSectProp);

   std::vector<Float64> moments;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);
      const CSegmentKey& segmentKey(poi.GetSegmentKey());
   
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      if ( intervalIdx < releaseIntervalIdx )
      {
         pfTop->push_back(0);
         pfBot->push_back(0);
      }
      else
      {
         // want section properties based on the non-composite girder in this interval. the prestress force is applied to the non-composite girder section
         // and the stress analysis needs to occur on the section properties for this interval.
         pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
         pgsTypes::SectionPropertyType spType = (spMode == pgsTypes::spmGross ? pgsTypes::sptGrossNoncomposite : pgsTypes::sptTransformedNoncomposite);
         Float64 A  = pSectProp->GetAg(spType,intervalIdx,poi);
         Float64 St = pSectProp->GetS(spType,intervalIdx,poi,topLocation);
         Float64 Sb = pSectProp->GetS(spType,intervalIdx,poi,botLocation);

         Float64 fTop = 0;
         Float64 fBot = 0;
         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = pgsTypes::StrandType(i);
            StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType);
            if ( 0 < nStrands )
            {
               pgsTypes::IntervalTimeType intervalTime = (spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);
               Float64 P = pPrestressForce->GetPrestressForce(poi,strandType,intervalIdx,intervalTime);
               
               Float64 nEffectiveStrands;
               Float64 e = pStrandGeom->GetEccentricity(spType,intervalIdx,poi,strandType,&nEffectiveStrands);

               Float64 ft = (IsZero(A) || IsZero(St) ? 0 : -P/A - P*e/St);
               ft = IsZero(ft) ? 0 : ft;

               Float64 fb = (IsZero(A) || IsZero(Sb) ? 0 : -P/A - P*e/Sb);
               fb = IsZero(fb) ? 0 : fb;

               fTop += ft;
               fBot += fb;
            }
         }
         pfTop->push_back(fTop);
         pfBot->push_back(fBot);
      }
   }
}
