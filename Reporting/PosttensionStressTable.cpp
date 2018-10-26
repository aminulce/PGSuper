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
#include <Reporting\PosttensionStressTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\TimelineEvent.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CPosttensionStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPosttensionStressTable::CPosttensionStressTable()
{
}

CPosttensionStressTable::CPosttensionStressTable(const CPosttensionStressTable& rOther)
{
   MakeCopy(rOther);
}

CPosttensionStressTable::~CPosttensionStressTable()
{
}

//======================== OPERATORS  =======================================
CPosttensionStressTable& CPosttensionStressTable::operator= (const CPosttensionStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CPosttensionStressTable::Build(IBroker* pBroker,const CGirderKey& girderKey,
                                            bool bDesign,IEAFDisplayUnits* pDisplayUnits,bool bGirderStresses) const
{
   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   location.IncludeSpanAndGirder(true);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   std::vector<IntervalIndexType> vIntervals(pIntervals->GetSpecCheckIntervals(girderKey));
   IntervalIndexType compositeDeckIntervalIdx        = pIntervals->GetCompositeDeckInterval(girderKey);
   IntervalIndexType loadRatingIntervalIdx           = pIntervals->GetLoadRatingInterval(girderKey);
   IntervalIndexType firstTendonStressingIntervalIdx = pIntervals->GetFirstTendonStressingInterval(girderKey);

   ATLASSERT(firstTendonStressingIntervalIdx != INVALID_INDEX);

   // we only want to report stresses due to PT in spec check intervals after the first tendon
   // is stressed. determine the first interval to report and remove all intervals from
   // vIntervals that are earlier
   IntervalIndexType minIntervalIdx = firstTendonStressingIntervalIdx;
   if ( !bGirderStresses )
   {
      // if we are reporting stresses in the deck, don't report any intervals before
      // the deck is composite
      minIntervalIdx = Max(minIntervalIdx,compositeDeckIntervalIdx);
   }

   // remove the intervals
   vIntervals.erase(std::remove_if(vIntervals.begin(),vIntervals.end(),std::bind2nd(std::less<IntervalIndexType>(),minIntervalIdx)),vIntervals.end());
   ATLASSERT(0 < vIntervals.size());

   ColumnIndexType nColumns;
   if ( bDesign )
   {
      nColumns = 1 // location column
               + vIntervals.size(); // one for each interval
   }
   else
   {
      // Load Rating
      nColumns = 2; // location column and column for live load stage
   }

   std::_tstring strTitle(bGirderStresses ? _T("Girder Stresses") : _T("Deck Stresses"));
   rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,strTitle);

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

      if ( bDesign )
      {
         p_table->SetColumnStyle(1,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
         p_table->SetStripeRowColumnStyle(1,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
      }
   }

   // Set up table headings
   ColumnIndexType col = 0;
   if ( bDesign )
   {
      (*p_table)(0,col++) << COLHDR(RPT_GDR_END_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
      std::vector<IntervalIndexType>::iterator end(vIntervals.end());
      for ( ; iter != end; iter++ )
      {
         IntervalIndexType intervalIdx = *iter;
         (*p_table)(0,col++) << COLHDR(_T("Interval ") << LABEL_INTERVAL(intervalIdx) << rptNewLine << pIntervals->GetDescription(girderKey,intervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
      }
   }
   else
   {
      (*p_table)(0,col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION, rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*p_table)(0,col++) << COLHDR(_T("Interval ") << LABEL_INTERVAL(loadRatingIntervalIdx) << rptNewLine << pIntervals->GetDescription(girderKey,loadRatingIntervalIdx), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( CSegmentKey(girderKey,ALL_SEGMENTS) ) );

   GET_IFACE2(pBroker,IPosttensionStresses,pPrestress);

   // Fill up the table
   RowIndexType row = p_table->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      col = 0;

      const pgsPointOfInterest& poi = *iter;

      (*p_table)(row,col++) << location.SetValue( POI_SPAN, poi );

      Float64 fTop, fBot;
      if ( bDesign )
      {
         std::vector<IntervalIndexType>::iterator iter(vIntervals.begin());
         std::vector<IntervalIndexType>::iterator end(vIntervals.end());
         for ( ; iter != end; iter++ )
         {
            IntervalIndexType intervalIdx = *iter;

            fTop = pPrestress->GetStress(intervalIdx,poi,topLocation,ALL_DUCTS);
            fBot = pPrestress->GetStress(intervalIdx,poi,botLocation,ALL_DUCTS);
            (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
            (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
            col++;
         }
      }
      else
      {
         // Rating
         fTop = pPrestress->GetStress(loadRatingIntervalIdx,poi,topLocation,ALL_DUCTS);
         fBot = pPrestress->GetStress(loadRatingIntervalIdx,poi,botLocation,ALL_DUCTS);
         (*p_table)(row,col) << RPT_FTOP << _T(" = ") << stress.SetValue( fTop ) << rptNewLine;
         (*p_table)(row,col) << RPT_FBOT << _T(" = ") << stress.SetValue( fBot );
         col++;
      }

      row++;
   }

   return p_table;
}

void CPosttensionStressTable::MakeCopy(const CPosttensionStressTable& rOther)
{
   // Add copy code here...
}

void CPosttensionStressTable::MakeAssignment(const CPosttensionStressTable& rOther)
{
   MakeCopy( rOther );
}
