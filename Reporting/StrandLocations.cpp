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
#include <Reporting\StrandLocations.h>

#include <IFace\Bridge.h>
#include <IFace\Project.h>

#include <PgsExt\StrandData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CStrandLocations
****************************************************************************/


CStrandLocations::CStrandLocations()
{
}

CStrandLocations::CStrandLocations(const CStrandLocations& rOther)
{
   MakeCopy(rOther);
}

CStrandLocations::~CStrandLocations()
{
}

CStrandLocations& CStrandLocations::operator= (const CStrandLocations& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }
   return *this;
}

void CStrandLocations::Build(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,
                                IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   GET_IFACE2(pBroker,IBridge,pBridge);

   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   INIT_UV_PROTOTYPE( rptLengthUnitValue, dim, pDisplayUnits->GetComponentDimUnit(),  false );
   INIT_UV_PROTOTYPE( rptLengthUnitValue, len, pDisplayUnits->GetSpanLengthUnit(),  false );

   rptParagraph* pHead;
   pHead = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pHead;
   *pHead <<_T("Prestressing Strand Locations")<<rptNewLine;

   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   if (pStrands->GetStrandDefinitionType() == CStrandData::sdtDirectSelection)
   {
      rptParagraph* p = new rptParagraph;
      *pChapter << p;
      *p << _T("Note: Strands were defined using Non-Sequential, Direct Fill.") << rptNewLine;
   }

   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;
   (*pPara) << _T("Strand locations measured from top-center of girder") << rptNewLine;

   pPara = new rptParagraph;
   *pChapter << pPara;
   rptRcTable* pStraightLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2);
   *pPara << pStraightLayoutTable << rptNewLine;

   pPara = &(*pStraightLayoutTable)(0,0);

   // Straight strands
   pgsPointOfInterest end_poi(segmentKey,0.0);
   StrandIndexType Ns = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);
   StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Straight);
   StrandIndexType nExtendedLeft  = pStrandGeometry->GetNumExtendedStrands(segmentKey,pgsTypes::metStart,pgsTypes::Straight);
   StrandIndexType nExtendedRight = pStrandGeometry->GetNumExtendedStrands(segmentKey,pgsTypes::metEnd,pgsTypes::Straight);
   if (0 < Ns)
   {
      ColumnIndexType nColumns = 3;
      if ( 0 < nDebonded )
      {
         nColumns += 2;
      }

      if ( 0 < nExtendedLeft || 0 < nExtendedRight )
      {
         nColumns += 2;
      }

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(nColumns,_T("Straight Strands"));
      *pPara << p_table;

      ColumnIndexType col = 0;
      (*p_table)(0,col++) << _T("Strand");
      (*p_table)(0,col++) << COLHDR(_T("X"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(0,col++) << COLHDR(_T("Y"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

      if ( 0 < nDebonded )
      {
         (*p_table)(0,col++) << COLHDR(_T("Debonded from") << rptNewLine << _T("Left End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,col++) << COLHDR(_T("Debonded from") << rptNewLine << _T("Right End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      if ( 0 < nExtendedLeft || 0 < nExtendedRight )
      {
         p_table->SetColumnStyle(col,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_CENTER));
         p_table->SetStripeRowColumnStyle(col,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_CENTER));
         (*p_table)(0,col++) << _T("Extended") << rptNewLine << _T("Left End");

         p_table->SetColumnStyle(col,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_CENTER));
         p_table->SetStripeRowColumnStyle(col,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_CENTER));
         (*p_table)(0,col++) << _T("Extended") << rptNewLine << _T("Right End");
      }

      CComPtr<IPoint2dCollection> spts;
      pStrandGeometry->GetStrandPositions(end_poi, pgsTypes::Straight, &spts);
      RowIndexType row = p_table->GetNumberOfHeaderRows();
      for (StrandIndexType is = 0; is < Ns; is++, row++)
      {
         col = 0;
         (*p_table)(row,col++) << row;
         CComPtr<IPoint2d> spt;
         spts->get_Item(is, &spt);
         Float64 x,y;
         spt->Location(&x,&y);
         (*p_table)(row,col++) << dim.SetValue(x);
         (*p_table)(row,col++) << dim.SetValue(y);


         if ( 0 < nDebonded ) 
         {
            Float64 start,end;
            if ( pStrandGeometry->IsStrandDebonded(segmentKey,is,pgsTypes::Straight,&start,&end) )
            {
               (*p_table)(row,col++) << len.SetValue(start);
               (*p_table)(row,col++) << len.SetValue(Lg - end);
            }
            else
            {
               (*p_table)(row,col++) << _T("-");
               (*p_table)(row,col++) << _T("-");
            }
         }

         if ( 0 < nExtendedLeft || 0 < nExtendedRight )
         {
            if ( pStrandGeometry->IsExtendedStrand(segmentKey,pgsTypes::metStart,is,pgsTypes::Straight) )
            {
               (*p_table)(row,col++) << symbol(DOT);
            }
            else
            {
               (*p_table)(row,col++) << _T("");
            }

            if ( pStrandGeometry->IsExtendedStrand(segmentKey,pgsTypes::metEnd,is,pgsTypes::Straight) )
            {
               (*p_table)(row,col++) << symbol(DOT);
            }
            else
            {
               (*p_table)(row,col++) << _T("");
            }
         }
      }
   }
   else
   {
      *pPara <<_T("No Straight Strands in Girder")<<rptNewLine;
   }


   // Temporary strands
   pPara = &(*pStraightLayoutTable)(0,1);
   if ( 0 < pStrandGeometry->GetMaxStrands(segmentKey,pgsTypes::Temporary) )
   {
      StrandIndexType Nt = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Temporary);
      nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Temporary);
      if (0 < Nt)
      {
         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3 + (0 < nDebonded ? 2 : 0),_T("Temporary Strand"));
         *pPara << p_table;

         (*p_table)(0,0) << _T("Strand");
         (*p_table)(0,1) << COLHDR(_T("X"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*p_table)(0,2) << COLHDR(_T("Y"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

         if ( 0 < nDebonded )
         {
            (*p_table)(0,3) << COLHDR(_T("Left End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
            (*p_table)(0,4) << COLHDR(_T("Right End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         }

         CComPtr<IPoint2dCollection> spts;
         pStrandGeometry->GetStrandPositions(end_poi, pgsTypes::Temporary, &spts);

         RowIndexType row = p_table->GetNumberOfHeaderRows();
         for (StrandIndexType is = 0; is < Nt; is++, row++)
         {
            (*p_table)(row,0) << row;
            CComPtr<IPoint2d> spt;
            spts->get_Item(is, &spt);
            Float64 x,y;
            spt->get_X(&x);
            spt->get_Y(&y);
            (*p_table)(row,1) << dim.SetValue(x);
            (*p_table)(row,2) << dim.SetValue(y);

            if ( 0 < nDebonded ) 
            {
               Float64 start,end;
               if ( pStrandGeometry->IsStrandDebonded(segmentKey,is,pgsTypes::Temporary,&start,&end) )
               {
                  (*p_table)(row,3) << len.SetValue(start);
                  (*p_table)(row,4) << len.SetValue(end);
               }
               else
               {
                  (*p_table)(row,3) << _T("-");
                  (*p_table)(row,4) << _T("-");
               }
            }
         }
      }
      else
      {
         *pPara <<_T("No Temporary Strands in Girder")<<rptNewLine;
      }
   }

   // Harped strands
   pPara = new rptParagraph;
   *pChapter << pPara;
   rptRcTable* pHarpedLayoutTable = pgsReportStyleHolder::CreateLayoutTable(2);
   *pPara << pHarpedLayoutTable << rptNewLine;

   pPara = &(*pHarpedLayoutTable)(0,0);

   StrandIndexType Nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);
   nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey,pgsTypes::Harped);
   if (0 < Nh)
   {
      bool areHarpedStraight = pStrandGeometry->GetAreHarpedStrandsForcedStraight(segmentKey);

      std::_tstring label( (areHarpedStraight ? _T("Adjustable Straight Strand Locations") : _T("Harped Strand Locations at Ends of Girder")));

      rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3 + (0 < nDebonded ? 2 : 0),label.c_str());
      *pPara << p_table;

      (*p_table)(0,0) << _T("Strand");
      (*p_table)(0,1) << COLHDR(_T("X"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
      (*p_table)(0,2) << COLHDR(_T("Y"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

      if ( 0 < nDebonded )
      {
         (*p_table)(0,3) << COLHDR(_T("Left End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
         (*p_table)(0,4) << COLHDR(_T("Right End"),rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      }

      CComPtr<IPoint2dCollection> spts;
      pStrandGeometry->GetStrandPositions(end_poi, pgsTypes::Harped, &spts);

      RowIndexType row = p_table->GetNumberOfHeaderRows();
      StrandIndexType is;
      for (is = 0; is < Nh; is++,row++)
      {
         (*p_table)(row,0) << (Uint16)row;
         CComPtr<IPoint2d> spt;
         spts->get_Item(is, &spt);
         Float64 x,y;
         spt->Location(&x,&y);
         (*p_table)(row,1) << dim.SetValue(x);
         (*p_table)(row,2) << dim.SetValue(y);

         if ( 0 < nDebonded ) 
         {
            Float64 start,end;
            if ( pStrandGeometry->IsStrandDebonded(segmentKey,is,pgsTypes::Harped,&start,&end) )
            {
               (*p_table)(row,3) << len.SetValue(start);
               (*p_table)(row,4) << len.SetValue(end);
            }
            else
            {
               (*p_table)(row,3) << _T("-");
               (*p_table)(row,4) << _T("-");
            }
         }
      }

      if ( !areHarpedStraight )
      {
         // harped strands at harping point
         Float64 mid = pBridge->GetSegmentLength(segmentKey)/2.;
         pgsPointOfInterest harp_poi(segmentKey,mid);

         pPara = &(*pHarpedLayoutTable)(0,1);

         p_table = pgsReportStyleHolder::CreateDefaultTable(3,_T("Harped Strand Locations at Harping Points"));
         *pPara << p_table;

         (*p_table)(0,0) << _T("Strand");
         (*p_table)(0,1) << COLHDR(_T("X"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
         (*p_table)(0,2) << COLHDR(_T("Y"),rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );

         CComPtr<IPoint2dCollection> hspts;
         pStrandGeometry->GetStrandPositions(harp_poi, pgsTypes::Harped, &hspts);

         row = p_table->GetNumberOfHeaderRows();
         for (is = 0; is < Nh; is++, row++)
         {
            (*p_table)(row,0) << row;
            CComPtr<IPoint2d> spt;
            hspts->get_Item(is, &spt);
            Float64 x,y;
            spt->Location(&x,&y);
            (*p_table)(row,1) << dim.SetValue(x);
            (*p_table)(row,2) << dim.SetValue(y);
         }
      }
   }
   else
   {
      *pPara <<_T("No Harped Strands in Girder")<<rptNewLine;
   }

}

void CStrandLocations::MakeCopy(const CStrandLocations& rOther)
{
   // Add copy code here...
}

void CStrandLocations::MakeAssignment(const CStrandLocations& rOther)
{
   MakeCopy( rOther );
}
