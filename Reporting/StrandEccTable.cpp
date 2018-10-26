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

#include "StdAfx.h"
#include <Reporting\StrandEccTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStrandEccTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CStrandEccTable::CStrandEccTable()
{
}

CStrandEccTable::CStrandEccTable(const CStrandEccTable& rOther)
{
   MakeCopy(rOther);
}

CStrandEccTable::~CStrandEccTable()
{
}

//======================== OPERATORS  =======================================
CStrandEccTable& CStrandEccTable::operator= (const CStrandEccTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CStrandEccTable::Build(IBroker* pBroker,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,
                                   IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyType spType = (pSectProp->GetSectionPropertiesMode() == pgsTypes::spmGross ? pgsTypes::sptGross : pgsTypes::sptTransformed );
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   bool bTempStrands = (0 < pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary) ? true : false);

   // Setup table
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   std::_tostringstream os;
   if ( pDocType->IsPGSuperDocument() )
      os << "Strand Eccentricity for Span " << LABEL_GROUP(segmentKey.groupIndex) << " Girder " << LABEL_GIRDER(segmentKey.girderIndex);
   else
      os << "Strand Eccentricity for Group " << LABEL_GROUP(segmentKey.groupIndex) << " Girder " << LABEL_GIRDER(segmentKey.girderIndex) << " Segment " << LABEL_SEGMENT(segmentKey.segmentIndex);

   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(bTempStrands ? 9 : 7,os.str().c_str());

   p_table->SetNumberOfHeaderRows(2);

   ColumnIndexType col = 0;

   // build first heading row
   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   p_table->SetRowSpan(0,col,2);
   (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

   // straight/harped/temporary
   p_table->SetColumnSpan(0,col, (bTempStrands ? 5 : 3));
   (*p_table)(0,col++) << _T("Eccentricity");

   // strand slope
   p_table->SetColumnSpan(0,col, 2);
   (*p_table)(0,col++) << _T("Strand Slope");

   ColumnIndexType i;
   for ( i = col; i < p_table->GetNumberOfColumns(); i++ )
      p_table->SetColumnSpan(0,i,SKIP_CELL);

   // build second hearing row
   col = 0;
   p_table->SetRowSpan(1,col++,SKIP_CELL);
   p_table->SetRowSpan(1,col++,SKIP_CELL);

   (*p_table)(1,col++) << COLHDR(_T("Straight"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*p_table)(1,col++) << COLHDR(LABEL_HARP_TYPE(pStrandGeom->GetAreHarpedStrandsForcedStraight(segmentKey)),   rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

   if ( bTempStrands )
   {
      (*p_table)(1,col++) << COLHDR(_T("Temporary"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(1,col++) << COLHDR(_T("All") << rptNewLine << _T("(w/ Temp)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(1,col++) << COLHDR(_T("Permanent") << rptNewLine << _T("(w/o Temp)"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }
   else
   {
      (*p_table)(1,col++) << COLHDR(_T("All") << rptNewLine << _T("Strands"), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   }

   (*p_table)(1,col++) << _T("Average") << rptNewLine << _T("(1:n)");
   (*p_table)(1,col++) << _T("Maximum") << rptNewLine << _T("(1:n)");

   INIT_UV_PROTOTYPE( rptPointOfInterest, gdrloc, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptPointOfInterest, spanloc, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptLengthSectionValue, ecc,    pDisplayUnits->GetComponentDimUnit(),  false );

   GET_IFACE2( pBroker, IPointOfInterest, pPoi );
   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);

   StrandIndexType Ns, Nh, Nt;
   Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
   Nh = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
   Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);

   std::vector<pgsPointOfInterest> pois(pPoi->GetPointsOfInterest(segmentKey));
   pPoi->RemovePointsOfInterest(pois,POI_CLOSURE);
   pPoi->RemovePointsOfInterest(pois,POI_BOUNDARY_PIER);

   pgsPointOfInterest prev_poi(segmentKey,0);
   bool bSkipToNextRow = false;

   RowIndexType firstRow = p_table->GetNumberOfHeaderRows();
   RowIndexType row = firstRow;
   std::vector<pgsPointOfInterest>::iterator iter(pois.begin());
   std::vector<pgsPointOfInterest>::iterator end(pois.end());
   for ( ; iter != end; iter++ )
   {
      bSkipToNextRow = false;

      const pgsPointOfInterest& poi = *iter;

      col = 0;

      if ( row != firstRow && IsEqual(poi.GetDistFromStart(),prev_poi.GetDistFromStart()) )
      {
         row--;
         bSkipToNextRow = true;
      }

#pragma Reminder("UPDATE: review")
      // does the eccentricity table need two typs of poi location objects?
      //if ( stage == pgsTypes::CastingYard )
      //{
         //(*p_table)(row,col++) << gdrloc.SetValue(stage, poi);
         (*p_table)(row,col++) << gdrloc.SetValue(POI_RELEASED_SEGMENT, poi);
         (*p_table)(row,col++) << _T("");
      //}
      //else
      //{
      //   (*p_table)(row,col++) << _T("");
      //   (*p_table)(row,col++) << spanloc.SetValue( stage, poi, end_size );
      //}

      if ( !bSkipToNextRow )
      {
         Float64 nEff;
         if ( 0 < Ns )
            (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetSsEccentricity( spType, intervalIdx, poi, &nEff ) );
         else
            (*p_table)(row,col++) << _T("-");

         if ( 0 < Nh )
            (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetHsEccentricity( spType, intervalIdx, poi, &nEff ) );
         else
            (*p_table)(row,col++) << _T("-");

         if ( bTempStrands )
         {
            if ( 0 < Nt )
               (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetTempEccentricity( spType, intervalIdx, poi, &nEff ) );
            else
               (*p_table)(row,col++) << _T("-");

            (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetEccentricity( spType, intervalIdx, poi, true, &nEff ) );
         }

         (*p_table)(row,col++) << ecc.SetValue( pStrandGeom->GetEccentricity( spType, intervalIdx, poi, false, &nEff ) );

         if ( 0 < Nh )
         {
            Float64 avg_slope = pStrandGeom->GetAvgStrandSlope( poi );
            avg_slope = fabs(avg_slope);

            if ( IsZero( 1./avg_slope ) )
               (*p_table)(row,col++) << symbol(INFINITY);
            else
               (*p_table)(row,col++) << avg_slope;

            Float64 max_slope = pStrandGeom->GetMaxStrandSlope( poi );
            max_slope = fabs(max_slope);
            if ( IsZero( 1./max_slope ) )
               (*p_table)(row,col++) << symbol(INFINITY);
            else
               (*p_table)(row,col++) << max_slope;
         }
         else
         {
            (*p_table)(row,col++) << _T("-");
            (*p_table)(row,col++) << _T("-");
         }
      }

      row++;
      prev_poi = poi;
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CStrandEccTable::MakeCopy(const CStrandEccTable& rOther)
{
   // Add copy code here...
}

void CStrandEccTable::MakeAssignment(const CStrandEccTable& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool CStrandEccTable::AssertValid() const
{
   return true;
}

void CStrandEccTable::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for CStrandEccTable") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool CStrandEccTable::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("CStrandEccTable");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for CStrandEccTable");

   TESTME_EPILOG("StrandEccTable");
}
#endif // _UNITTEST
