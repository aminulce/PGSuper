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

// CreepAtDeckPlacementTable.cpp : Implementation of CCreepAtDeckPlacementTable
#include "stdafx.h"
#include "CreepAtDeckPlacementTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreepAtDeckPlacementTable::CCreepAtDeckPlacementTable(ColumnIndexType NumColumns, IDisplayUnits* pDispUnit) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( offset,      pDispUnit->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDispUnit->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDispUnit->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDispUnit->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDispUnit->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDispUnit->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDispUnit->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDispUnit->GetStressUnit(),          false );
   DEFINE_UV_PROTOTYPE( time,        pDispUnit->GetLongTimeUnit(),        false );

   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);
}

CCreepAtDeckPlacementTable* CCreepAtDeckPlacementTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IDisplayUnits* pDispUnit,Uint16 level)
{
   GET_IFACE2(pBroker,IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);

   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   // Create and configure the table
   ColumnIndexType numColumns = 8;
   CCreepAtDeckPlacementTable* table = new CCreepAtDeckPlacementTable( numColumns, pDispUnit );
   pgsReportStyleHolder::ConfigureTable(table);


   table->m_GirderData = girderData;

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << "[5.9.5.4.2b] Creep of Girder Concrete : " << symbol(DELTA) << Sub2("f","pCR") << rptNewLine;

   if ( girderData.TempStrandUsage != pgsTypes::ttsPretensioned )
      *pParagraph << rptRcImage(strImagePath + "Delta_FpCR_PT.gif") << rptNewLine;
   else
      *pParagraph << rptRcImage(strImagePath + "Delta_FpCR.gif") << rptNewLine;

   // creep loss   
   *pParagraph << table << rptNewLine;
   (*table)(0,0) << COLHDR("Location from"<<rptNewLine<<"Left Support",rptLengthUnitTag,  pDispUnit->GetSpanLengthUnit() );

   if ( girderData.TempStrandUsage == pgsTypes::ttsPretensioned )
      (*table)(0,1) << COLHDR(Sub2("f","cgp"), rptStressUnitTag, pDispUnit->GetStressUnit() );
   else
      (*table)(0,1) << COLHDR(Sub2("f","cgp") << " + " << symbol(DELTA) << Sub2("f","pp"), rptStressUnitTag, pDispUnit->GetStressUnit() );

   (*table)(0,2) << Sub2("k","td");
   (*table)(0,3) << COLHDR(Sub2("t","i"),rptTimeUnitTag,pDispUnit->GetLongTimeUnit());
   (*table)(0,4) << COLHDR(Sub2("t","d"),rptTimeUnitTag,pDispUnit->GetLongTimeUnit());
   (*table)(0,5) << Sub2(symbol(psi),"b") << "(" << Sub2("t","d") << "," << Sub2("t","i") << ")";
   (*table)(0,6) << Sub2("K","id");
   (*table)(0,7) << COLHDR(symbol(DELTA) << Sub2("f","pCR"), rptStressUnitTag, pDispUnit->GetStressUnit() );

   return table;
}

void CCreepAtDeckPlacementTable::AddRow(rptChapter* pChapter,IBroker* pBroker,RowIndexType row,LOSSDETAILS& details,IDisplayUnits* pDispUnit,Uint16 level)
{
   if ( m_GirderData.TempStrandUsage == pgsTypes::ttsPretensioned )
      (*this)(row,1) << stress.SetValue(details.pLosses->ElasticShortening().PermanentStrand_Fcgp());
   else
      (*this)(row,1) << stress.SetValue(details.pLosses->ElasticShortening().PermanentStrand_Fcgp() + details.RefinedLosses2005.GetDeltaFpp());

   (*this)(row,2) << scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToDeck().GetKtd());
   (*this)(row,3) << time.SetValue(details.RefinedLosses2005.GetAdjustedInitialAge());
   (*this)(row,4) << time.SetValue(details.RefinedLosses2005.GetAgeAtDeckPlacement());
   (*this)(row,5) << scalar.SetValue(details.RefinedLosses2005.GetCreepInitialToDeck().GetCreepCoefficient());
   (*this)(row,6) << scalar.SetValue(details.RefinedLosses2005.GetKid());
   (*this)(row,7) << stress.SetValue(details.RefinedLosses2005.CreepLossBeforeDeckPlacement() );
}
