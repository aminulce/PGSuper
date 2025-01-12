///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2023  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <Reporting\SegmentTendonGeometryChapterBuilder.h>

#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSegmentTendonGeometryChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSegmentTendonGeometryChapterBuilder::CSegmentTendonGeometryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSegmentTendonGeometryChapterBuilder::GetName() const
{
   return TEXT("Segment Tendon Geometry");
}

rptChapter* CSegmentTendonGeometryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);

   CGirderKey girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,ISegmentTendonGeometry,pTendonGeom);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   GET_IFACE2(pBroker, IBridge, pBridge);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2_NOCHECK(pBroker,ILosses,pLosses); // only used if there are segment tendons
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(),  false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        dist,     pDisplayUnits->GetSpanLengthUnit(),  false);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,        ecc,      pDisplayUnits->GetComponentDimUnit(), false);
   INIT_UV_PROTOTYPE( rptStressUnitValue,        stress,   pDisplayUnits->GetStressUnit(), false);

   location.IncludeSpanAndGirder(true);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey, segIdx);

      DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
      if (nDucts == 0)
      {
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << _T(": No tendons defined") << rptNewLine;
      }

      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, &vPoi);

      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

      for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
      {
         CString strTitle;
         strTitle.Format(_T("Segment %d, Tendon %d"), LABEL_SEGMENT(segIdx), LABEL_DUCT(ductIdx));
         rptRcTable* pTable = rptStyleManager::CreateDefaultTable(10, strTitle);
         *pPara << pTable << rptNewLine;

         ColumnIndexType col = 0;
         (*pTable)(0, col++) << _T("POI");
         (*pTable)(0, col++) << COLHDR(_T("X"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit());
         (*pTable)(0, col++) << COLHDR(_T("Y"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*pTable)(0, col++) << _T("Slope X");
         (*pTable)(0, col++) << _T("Slope Y");
         (*pTable)(0, col++) << COLHDR(Sub2(_T("e"), _T("pt")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit());
         (*pTable)(0, col++) << symbol(alpha) << rptNewLine << _T("(from Start)");
         (*pTable)(0, col++) << symbol(alpha) << rptNewLine << _T("(from End)");
         (*pTable)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pF")), rptStressUnitTag, pDisplayUnits->GetStressUnit());
         (*pTable)(0, col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pA")), rptStressUnitTag, pDisplayUnits->GetStressUnit());

         RowIndexType row = 1;
         for (const pgsPointOfInterest& poi : vPoi)
         {
            col = 0;

            if (pPoi->IsOnSegment(poi))
            {
               (*pTable)(row, col++) << location.SetValue(POI_RELEASED_SEGMENT, poi);

               const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, stressTendonIntervalIdx);
               const FRICTIONLOSSDETAILS& frDetails(pDetails->SegmentFrictionLossDetails[ductIdx]);

               ATLASSERT(IsEqual(frDetails.X, poi.GetDistFromStart()));

               Float64 ductOffset(0), eccY(0), slopeX(0), slopeY(0), angle_start(0), angle_end(0);
               if (pTendonGeom->IsOnDuct(poi))
               {
                  ductOffset = pTendonGeom->GetSegmentDuctOffset(stressTendonIntervalIdx, poi, ductIdx);

                  CComPtr<IVector3d> slope;
                  pTendonGeom->GetSegmentTendonSlope(poi, ductIdx, &slope);
                  slope->get_X(&slopeX);
                  slope->get_Y(&slopeY);

                  Float64 eccX;
                  pTendonGeom->GetSegmentTendonEccentricity(stressTendonIntervalIdx, poi, ductIdx, &eccX, &eccY);

                  angle_start = pTendonGeom->GetSegmentTendonAngularChange(poi, ductIdx, pgsTypes::metStart);
                  angle_end = pTendonGeom->GetSegmentTendonAngularChange(poi, ductIdx, pgsTypes::metEnd);
               }

               (*pTable)(row, col++) << dist.SetValue(frDetails.X);
               (*pTable)(row, col++) << ecc.SetValue(ductOffset);
               (*pTable)(row, col++) << slopeX;
               (*pTable)(row, col++) << slopeY;
               (*pTable)(row, col++) << ecc.SetValue(eccY);
               (*pTable)(row, col++) << angle_start;
               (*pTable)(row, col++) << angle_end;
               (*pTable)(row, col++) << stress.SetValue(frDetails.dfpF); // friction
               (*pTable)(row, col++) << stress.SetValue(frDetails.dfpA); // anchor set
            }
            row++;
         } // next poi

         dist.ShowUnitTag(true);
         ecc.ShowUnitTag(true);
         stress.ShowUnitTag(true);

         Float64 Lduct = pTendonGeom->GetDuctLength(segmentKey, ductIdx);
         *pPara << _T("Duct Length = ") << dist.SetValue(Lduct) << rptNewLine;
         *pPara << rptNewLine;

         Float64 pfpF = pLosses->GetSegmentTendonAverageFrictionLoss(segmentKey, ductIdx);
         *pPara << _T("Avg. Friction Loss = ") << stress.SetValue(pfpF) << rptNewLine;

         Float64 pfpA = pLosses->GetSegmentTendonAverageAnchorSetLoss(segmentKey, ductIdx);
         *pPara << _T("Avg. Anchor Set Loss = ") << stress.SetValue(pfpA) << rptNewLine;
         *pPara << rptNewLine;

         Float64 Lset = pLosses->GetSegmentTendonAnchorSetZoneLength(segmentKey, ductIdx, pgsTypes::metStart);
         *pPara << _T("Left End, ") << Sub2(_T("L"), _T("set")) << _T(" = ") << dist.SetValue(Lset) << rptNewLine;

         Lset = pLosses->GetSegmentTendonAnchorSetZoneLength(segmentKey, ductIdx, pgsTypes::metEnd);
         *pPara << _T("Right End, ") << Sub2(_T("L"), _T("set")) << _T(" = ") << dist.SetValue(Lset) << rptNewLine;

         *pPara << rptNewLine;

         Float64 elongation = pLosses->GetSegmentTendonElongation(segmentKey, ductIdx, pgsTypes::metStart);
         *pPara << _T("Left End, Elongation = ") << ecc.SetValue(elongation) << rptNewLine;

         elongation = pLosses->GetSegmentTendonElongation(segmentKey, ductIdx, pgsTypes::metEnd);
         *pPara << _T("Right End, Elongation = ") << ecc.SetValue(elongation) << rptNewLine;

         *pPara << rptNewLine;


         dist.ShowUnitTag(false);
         ecc.ShowUnitTag(false);
         stress.ShowUnitTag(false);
      } // next duct
   } // next segment


   return pChapter;
}

CChapterBuilder* CSegmentTendonGeometryChapterBuilder::Clone() const
{
   return new CSegmentTendonGeometryChapterBuilder;
}
