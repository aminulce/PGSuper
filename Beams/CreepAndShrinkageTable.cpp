///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2002  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// CreepAndShrinkageTable.cpp : Implementation of CCreepAndShrinkageTable
#include "stdafx.h"
#include "CreepAndShrinkageTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreepAndShrinkageTable::CCreepAndShrinkageTable(ColumnIndexType NumColumns, IDisplayUnits* pDispUnit) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( cg,          pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDispUnit->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDispUnit->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDispUnit->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDispUnit->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDispUnit->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDispUnit->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDispUnit->GetStressUnit(),          false );
}

CCreepAndShrinkageTable* CCreepAndShrinkageTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnit,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 5;

   CCreepAndShrinkageTable* table = new CCreepAndShrinkageTable( numColumns, pDispUnit );
   pgsReportStyleHolder::ConfigureTable(table);


   std::string strImagePath(pgsReportStyleHolder::GetImagePath());


   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "Losses Due to Creep and Shrinkage [5.9.5.4.2, 5.9.5.4.3]" << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << rptRcImage(strImagePath + "Delta FpCR Equation.jpg") << rptNewLine;
   if ( pDispUnit->GetUnitDisplayMode() == pgsTypes::umSI )
      *pParagraph << rptRcImage(strImagePath + "Delta FpSR Equation SI.jpg") << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + "Delta FpSR Equation US.jpg") << rptNewLine;

   GET_IFACE2(pBroker,IEnvironment,pEnv);
   *pParagraph << "H = " << pEnv->GetRelHumidity() << "%" << rptNewLine;

   *pParagraph << table << rptNewLine;

   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDispUnit->GetSpanLengthUnit() );
   (*table)(0,1) << COLHDR(symbol(DELTA) << Sub2("f","pSR"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,2) << COLHDR(Sub2("f","cgp"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,3) << COLHDR(symbol(DELTA) << Sub2("f","cdp"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   (*table)(0,4) << COLHDR(symbol(DELTA) << Sub2("f","pCR"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   
   return table;
}

void CCreepAndShrinkageTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDispUnit,Uint16 level)
{
   (*this)(row,1) << stress.SetValue( details.RefinedLosses.ShrinkageLosses() );
   (*this)(row,2) << stress.SetValue( details.pLosses->ElasticShortening().PermanentStrand_Fcgp() );
   (*this)(row,3) << stress.SetValue( -details.pLosses->GetDeltaFcd1() );
   (*this)(row,4) << stress.SetValue( details.RefinedLosses.CreepLosses() );
}
