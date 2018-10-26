///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2011  Washington State Department of Transportation
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
#include <Reporting\MomentCapacityParagraphBuilder.h>

#include <PgsExt\PointOfInterest.h>
#include <PgsExt\GirderArtifact.h>

#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\MomentCapacity.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CMomentCapacityParagraphBuilder::CMomentCapacityParagraphBuilder()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

/*--------------------------------------------------------------------*/
rptParagraph* CMomentCapacityParagraphBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptParagraph* p = new rptParagraph;

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(2,_T("Moment Capacity at Midspan"));
   *p << pTable << rptNewLine;

   pTable->SetColumnStyle(1, pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_RIGHT) );
   pTable->SetStripeRowColumnStyle(1, pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_RIGHT) );

   // Setup the table

   (*pTable)(0,0) << _T("");
   (*pTable)(0,1) << _T("Composite Girder");

   // Setup up some unit value prototypes
   INIT_UV_PROTOTYPE( rptMomentUnitValue, moment, pDisplayUnits->GetMomentUnit(), true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(6);
   scalar.SetPrecision(2);

   // Interfaces
   GET_IFACE2(pBroker,IPointOfInterest,pIPOI);
   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   GET_IFACE2(pBroker,IMomentCapacity,pMomentCap);

   // Get Midspan std::vector<pgsPointOfInterest>
   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(span,girder,pgsTypes::BridgeSite3,POI_MIDSPAN);
   pgsPointOfInterest poi = *vPoi.begin();

   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,girder);
//   Removed bare girder capacity calc due to request of WSDOT staff - rdp 5/99
//   const pgsFlexuralCapacityArtifact* pGirderCap = pArtifact->GetFlexuralCapacityArtifact(pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite1,pgsTypes::StrengthI,poi.GetDistFromStart()));
   const pgsFlexuralCapacityArtifact* pCompositeCap = pArtifact->GetPositiveMomentFlexuralCapacityArtifact(pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,poi.GetDistFromStart()));

   double Mu = pCompositeCap->GetDemand();
   double Mr = pCompositeCap->GetCapacity();

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   (*pTable)(row,0) << _T("Factored Moment, Strength I, ") << Sub2(_T("M"),_T("u"));
   (*pTable)(row,1) << moment.SetValue( Mu );

   // strength II if permit truck is defined
   bool str2_passed(true);

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   if (pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit))
   {
      const pgsFlexuralCapacityArtifact* pStr2CompositeCap = pArtifact->GetPositiveMomentFlexuralCapacityArtifact(pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthII,poi.GetDistFromStart()));

      (*pTable)(++row,0) << _T("Factored Moment, Strength II, ") << Sub2(_T("M"),_T("u"));
      (*pTable)(  row,1) << moment.SetValue( pStr2CompositeCap->GetDemand() );

      str2_passed = pStr2CompositeCap->Passed();
   }

   row++;
   (*pTable)(row,0) << _T("Moment Capacity, ") << symbol(phi) << Sub2(_T("M"),_T("n"));
   (*pTable)(row,1) << moment.SetValue( Mr );

   GET_IFACE2(pBroker,ISpecification, pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   MOMENTCAPACITYDETAILS mcd;
   pMomentCap->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,true,&mcd);

   if ( mcd.Method == LRFD_METHOD && pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      // over/under reinforced sections where part of AASHTO through the 2005 edition
      if ( pCompositeCap->IsOverReinforced() )
      {
         // Show limiting capacity of over reinforced section along with Mr
         (*pTable)(row,1) << _T("(") << moment.SetValue( mcd.Phi * mcd.MnMin ) << _T(")");
      }
      row++;

      (*pTable)(row,0) << _T("Under Reinforced");
      (*pTable)(row,1) << (pCompositeCap->IsUnderReinforced() ? _T("Yes") : _T("No"));
      row++;

      (*pTable)(row,0) << _T("Over Reinforced");
      (*pTable)(row,1) << (pCompositeCap->IsOverReinforced() ? _T("Yes") : _T("No"));
      row++;
   }
   else
   {
      //if method is WSDOT or 2006 LRFD and later there is no over/under reinforced
      row++; // to finish the Mr row
      (*pTable)(row,0) << symbol(phi);
      (*pTable)(row,1) << mcd.Phi;
      row++;
   }


   (*pTable)(row,0) << _T("Status") << rptNewLine << _T("(") << symbol(phi) << Sub2(_T("M"),_T("n")) << _T("/") << Sub2(_T("M"),_T("u")) << _T(")");
   if ( pCompositeCap->Passed() )
      (*pTable)(row,1) << RPT_PASS;
   else
      (*pTable)(row,1) << RPT_FAIL;

   if ( IsZero( Mu ) )
   {
      (*pTable)(row,1) << rptNewLine << _T("(") << symbol(INFINITY) << _T(")");
   }
   else
   {
      (*pTable)(row,1) << rptNewLine << _T("(") << scalar.SetValue(Mr/Mu) << _T(")");
   }


   if ( pSpecEntry->GetSpecificationType() < lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
      if ( pCompositeCap->IsOverReinforced() )
      {
         (*pTable)(row,1) << _T(" *");
         *p << _T("* Over reinforced sections may be adequate if M") << Sub(_T("u")) << _T(" does not exceed the minimum resistance specified in LRFD C5.7.3.3.1") << rptNewLine;
         *p << _T("  Limiting capacity of over reinforced sections are shown in parentheses") << rptNewLine;
         *p << _T("  See Moment Capacity Details chapter for additional information") << rptNewLine;
      }
   }

   return p;
}

