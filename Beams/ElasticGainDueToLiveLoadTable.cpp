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

// ElasticGainDueToLiveLoadTable.cpp : Implementation of CElasticGainDueToLiveLoadTable
#include "stdafx.h"
#include "ElasticGainDueToLiveLoadTable.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CElasticGainDueToLiveLoadTable::CElasticGainDueToLiveLoadTable(ColumnIndexType NumColumns, IEAFDisplayUnits* pDisplayUnits) :
rptRcTable(NumColumns,0)
{
   DEFINE_UV_PROTOTYPE( spanloc,     pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( gdrloc,      pDisplayUnits->GetSpanLengthUnit(),      false );
   DEFINE_UV_PROTOTYPE( cg,          pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( mod_e,       pDisplayUnits->GetModEUnit(),            false );
   DEFINE_UV_PROTOTYPE( force,       pDisplayUnits->GetGeneralForceUnit(),    false );
   DEFINE_UV_PROTOTYPE( area,        pDisplayUnits->GetAreaUnit(),            false );
   DEFINE_UV_PROTOTYPE( mom_inertia, pDisplayUnits->GetMomentOfInertiaUnit(), false );
   DEFINE_UV_PROTOTYPE( ecc,         pDisplayUnits->GetComponentDimUnit(),    false );
   DEFINE_UV_PROTOTYPE( moment,      pDisplayUnits->GetMomentUnit(),          false );
   DEFINE_UV_PROTOTYPE( stress,      pDisplayUnits->GetStressUnit(),          false );
}

CElasticGainDueToLiveLoadTable* CElasticGainDueToLiveLoadTable::PrepareTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   // Create and configure the table
   ColumnIndexType numColumns = 8;

   CElasticGainDueToLiveLoadTable* table = new CElasticGainDueToLiveLoadTable( numColumns, pDisplayUnits);
   pgsReportStyleHolder::ConfigureTable(table);

   table->scalar.SetFormat(sysNumericFormatTool::Fixed);
   table->scalar.SetWidth(5);
   table->scalar.SetPrecision(2);

   std::_tstring strImagePath(pgsReportStyleHolder::GetImagePath());

   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   Float64 Ec = pMaterial->GetEcGdr(span,gdr);
   Float64 Ep = pMaterial->GetStrand(span,gdr,pgsTypes::Permanent)->GetE();

   rptParagraph* pParagraph = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << pParagraph;
   *pParagraph << _T("Elastic Gain Due to Live Load [5.9.5.2.3a]") << rptNewLine;

   pParagraph = new rptParagraph;
   *pChapter << pParagraph;

   *pParagraph << _T("Change in strand stress due to live load applied to the composite girder") << rptNewLine;
   *pParagraph << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("DeltaFcdLL.png")) << rptNewLine;
   *pParagraph << rptRcImage(strImagePath + _T("Delta_FpLL.png")) << rptNewLine;

   table->mod_e.ShowUnitTag(true);
   *pParagraph << Sub2(_T("E"),_T("p")) << _T(" = ") << table->mod_e.SetValue( Ep ) << rptNewLine;
   *pParagraph << Sub2(_T("E"),_T("c")) << _T(" = ") << table->mod_e.SetValue( Ec ) << rptNewLine;
   table->mod_e.ShowUnitTag(false);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   table->m_BAT = (analysisType == pgsTypes::Simple     ? SimpleSpan : 
                   analysisType == pgsTypes::Continuous ? ContinuousSpan : MaxSimpleContinuousEnvelope);


   *pParagraph << table << rptNewLine;

   ColumnIndexType col = 0;
   (*table)(0,col++) << COLHDR(_T("Location from")<<rptNewLine<<_T("Left Support"),rptLengthUnitTag,  pDisplayUnits->GetSpanLengthUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("M"),_T("llim")), rptMomentUnitTag, pDisplayUnits->GetMomentUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("e"),_T("p")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("I"),_T("c")), rptLength4UnitTag, pDisplayUnits->GetMomentOfInertiaUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bc")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(Sub2(_T("Y"),_T("bg")), rptLengthUnitTag, pDisplayUnits->GetComponentDimUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << italic(ON) << Sub2(_T("f'''"),_T("cd")) << italic(OFF), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   (*table)(0,col++) << COLHDR(symbol(DELTA) << RPT_STRESS(_T("pLL")), rptStressUnitTag, pDisplayUnits->GetStressUnit() );
   
   return table;
}

void CElasticGainDueToLiveLoadTable::AddRow(rptChapter* pChapter,IBroker* pBroker,const pgsPointOfInterest& poi,RowIndexType row,LOSSDETAILS& details,IEAFDisplayUnits* pDisplayUnits,Uint16 level)
{
   GET_IFACE2(pBroker,IProductForces,pProdForces);
   ColumnIndexType col = 1;

   (*this)(row,col++) << moment.SetValue( details.pLosses->GetLiveLoadMoment() );
   (*this)(row,col++) << ecc.SetValue( details.pLosses->GetEccPermanent() );
   (*this)(row,col++) << mom_inertia.SetValue( details.pLosses->GetIc() );
   (*this)(row,col++) << cg.SetValue( details.pLosses->GetYbc() );
   (*this)(row,col++) << cg.SetValue( details.pLosses->GetYbg() );
   (*this)(row,col++) << stress.SetValue( details.pLosses->GetDeltaFcdLL() );
   (*this)(row,col++) << stress.SetValue( details.pLosses->ElasticGainDueToLiveLoad() );
}
