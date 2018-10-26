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

#include "StdAfx.h"
#include <Reporting\SpanDataChapterBuilder.h>

#include <IFace\Bridge.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSpanDataChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSpanDataChapterBuilder::CSpanDataChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSpanDataChapterBuilder::GetName() const
{
#pragma Reminder("UPDATE: this chapter doesn't make sense")
   // The name of this chapter doesn't make sense... neither does the data
   // though it is important (eccentricities and component dimensions)
   // Find a better way to report this
   return TEXT("Span Lengths");
}

rptChapter* CSpanDataChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CGirderReportSpecification* pGdrRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pGdrRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGdrRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   USES_CONVERSION;

   GET_IFACE2(pBroker, IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   SpanIndexType nSpans = pBridge->GetSpanCount();

   rptParagraph* pPara = new rptParagraph;
   (*pChapter) << pPara;

   if ( nSpans == nGroups )
   {
      *pPara << _T("C-C Pier = Abutment/Pier Line to Abutment/Pier Line length measured along the girder") << rptNewLine;
      *pPara << _T("C-C Bearing = Centerline bearing to centerline bearing length measured along the girder") << rptNewLine;
      *pPara << _T("Girder Length, Plan = End to end length of the girder projected into a horizontal plane") << rptNewLine;
      *pPara << _T("Girder Length, Along Grade = End to end length of girder measured along grade of the girder (slope adjusted) = ") << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("SlopeAdjustedGirderLength.png"),rptRcImage::Middle) << rptNewLine;
   }
   else
   {
      *pPara << _T("C-C Pier = Abutment/Pier/Temp Support Line to Abutment/Pier/Temp Support Line length measured along the segment") << rptNewLine;
      *pPara << _T("C-C Bearing = Centerline bearing to centerline bearing length measured along the segment") << rptNewLine;
      *pPara << _T("Segment Length, Plan = End to end length of the segment projected into a horizontal plane") << rptNewLine;
      *pPara << _T("Segment Length, Along Grade = End to end length of segment measured along grade of the segment (slope adjusted) = ") << rptRcImage(pgsReportStyleHolder::GetImagePath() + _T("SlopeAdjustedGirderLength.png"),rptRcImage::Middle) << rptNewLine;
   }
   *pPara << rptNewLine;

   INIT_UV_PROTOTYPE( rptLengthUnitValue, length,  pDisplayUnits->GetSpanLengthUnit(), false );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(7);
   scalar.SetPrecision(4);
   scalar.SetTolerance(1.0e-6);

   CComPtr<IDirectionDisplayUnitFormatter> direction_formatter;
   direction_formatter.CoCreateInstance(CLSID_DirectionDisplayUnitFormatter);
   direction_formatter->put_BearingFormat(VARIANT_TRUE);

   std::_tstring strSlopeTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag();

   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      ColumnIndexType nColumns;
      std::_tstring strSegmentLabel;
      if ( nGroups == nSpans )
      {
         strSegmentLabel = _T("Girder");
         nColumns = 7;
      }
      else
      {
         strSegmentLabel = _T("Segment");
         nColumns = 8;
      }

      rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nColumns);
      pTable->SetNumberOfHeaderRows(2);

      (*pPara) << pTable << rptNewLine;

      ColumnIndexType row0col = 0;
      ColumnIndexType row1col = 0;

      if ( nSpans != nGroups )
      {
         pTable->SetRowSpan(0,row0col,2);
         pTable->SetRowSpan(1,row1col++,SKIP_CELL);
         (*pTable)(0,row0col++) << strSegmentLabel;
      }

      pTable->SetRowSpan(0,row0col,2);
      pTable->SetRowSpan(1,row1col++,SKIP_CELL);
      (*pTable)(0,row0col++) << COLHDR(_T("C-C Pier"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetRowSpan(0,row0col,2);
      pTable->SetRowSpan(1,row1col++,SKIP_CELL);
      (*pTable)(0,row0col++) << COLHDR(_T("C-C Bearing"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetColumnSpan(0,row0col,2);
      (*pTable)(0,row0col++) << strSegmentLabel << _T(" Length");
      pTable->SetColumnSpan(0,row0col++,SKIP_CELL);
      (*pTable)(1,row1col++) << COLHDR(_T("Plan"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(1,row1col++) << COLHDR(_T("Along") << rptNewLine << _T("Grade"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetRowSpan(0,row0col,2);
      pTable->SetRowSpan(1,row1col++,SKIP_CELL);
      (*pTable)(0,row0col++) << _T("Grade") << rptNewLine << _T("(") << strSlopeTag << _T("/") << strSlopeTag << _T(")");

      pTable->SetColumnSpan(0,row0col,2);
      (*pTable)(0,row0col++) << _T("End Distance");
      pTable->SetColumnSpan(0,row0col++,SKIP_CELL);
      (*pTable)(1,row1col++) << COLHDR(_T("Start"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(1,row1col++) << COLHDR(_T("End"), rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      RowIndexType row = pTable->GetNumberOfHeaderRows();

      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = (girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex);
      GirderIndexType lastGirderIdx  = (girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx);
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);

         SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            ColumnIndexType col = 0;

            CSegmentKey segmentKey(girderKey,segIdx);

            if ( nSpans != nGroups )
            {
               (*pTable)(row,col++) << LABEL_SEGMENT(segIdx);
            }

            Float64 L = pBridge->GetSegmentLayoutLength(segmentKey);
            (*pTable)(row,col++) << length.SetValue(L);

            L = pBridge->GetSegmentSpanLength(segmentKey);
            (*pTable)(row,col++) << length.SetValue(L);

            L = pBridge->GetSegmentLength(segmentKey);
            (*pTable)(row,col++) << length.SetValue(L);

            L = pBridge->GetSegmentPlanLength(segmentKey);
            (*pTable)(row,col++) << length.SetValue(L);

            Float64 slope = pBridge->GetSegmentSlope(segmentKey);
            (*pTable)(row,col++) << scalar.SetValue(slope);

            Float64 endDist;
            endDist = pBridge->GetSegmentStartEndDistance(segmentKey);
            (*pTable)(row,col++) << length.SetValue(endDist);

            endDist = pBridge->GetSegmentEndEndDistance(segmentKey);
            (*pTable)(row,col++) << length.SetValue(endDist);

            row++;
         } // next segment
      } // next girder
   } // next group

   return pChapter;
}

CChapterBuilder* CSpanDataChapterBuilder::Clone() const
{
   return new CSpanDataChapterBuilder;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
