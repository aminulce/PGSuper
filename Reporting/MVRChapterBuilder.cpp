///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2010  Washington State Department of Transportation
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

#include <Reporting\MVRChapterBuilder.h>
#include <Reporting\ReportNotes.h>
#include <Reporting\CastingYardMomentsTable.h>
#include <Reporting\ProductMomentsTable.h>
#include <Reporting\ProductShearTable.h>
#include <Reporting\ProductReactionTable.h>
#include <Reporting\ProductDisplacementsTable.h>
#include <Reporting\ProductRotationTable.h>
#include <Reporting\CombinedMomentsTable.h>
#include <Reporting\CombinedShearTable.h>
#include <Reporting\CombinedReactionTable.h>
#include <Reporting\ConcurrentShearTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\UserShearTable.h>
#include <Reporting\UserReactionTable.h>
#include <Reporting\UserDisplacementsTable.h>
#include <Reporting\UserRotationTable.h>
#include <Reporting\LiveLoadDistributionFactorTable.h>
#include <Reporting\VehicularLoadResultsTable.h>
#include <Reporting\VehicularLoadReactionTable.h>
#include <Reporting\LiveLoadReactionTable.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#include <psgLib\SpecLibraryEntry.h>
#include <psgLib\RatingLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CMVRChapterBuilder
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CMVRChapterBuilder::CMVRChapterBuilder(bool bDesign,bool bRating)
{
   m_bDesign = bDesign;
   m_bRating = bRating;
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CMVRChapterBuilder::GetName() const
{
   return TEXT("Moments, Shears, and Reactions");
}

rptChapter* CMVRChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CGirderReportSpecification* pGdrRptSpec    = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   SpanIndexType span;
   GirderIndexType girder;

   if ( pSGRptSpec )
   {
      pSGRptSpec->GetBroker(&pBroker);
      span = pSGRptSpec->GetSpan();
      girder = pSGRptSpec->GetGirder();
   }
   else if ( pGdrRptSpec )
   {
      pGdrRptSpec->GetBroker(&pBroker);
      span = ALL_SPANS;
      girder = pGdrRptSpec->GetGirder();
   }
   else
   {
      span = ALL_SPANS;
      girder  = ALL_GIRDERS;
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptParagraph* p = 0;

   GET_IFACE2(pBroker,ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE2(pBroker,ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   bool bDesign = m_bDesign;
   bool bRating;
   
   if ( m_bRating )
   {
      bRating = true;
   }
   else
   {
      // include load rating results if we are always load rating
      GET_IFACE2(pBroker,IRatingSpecification,pRatingSpec);
      bRating = pRatingSpec->AlwaysLoadRate();
   }

   bool bIndicateControllingLoad = true;

   // Product Moments
   if ( bDesign )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *p << "Load Responses - Casting Yard"<<rptNewLine;
      p->SetName("Casting Yard Results");
      *pChapter << p;

      p = new rptParagraph;
      *pChapter << p;
      *p << CCastingYardMomentsTable().Build(pBroker,span,girder,pDisplayUnits) << rptNewLine;
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *p << "Load Responses - Bridge Site"<<rptNewLine;
   p->SetName("Bridge Site Results");
   *pChapter << p;
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductMomentsTable().Build(pBroker,span,girder,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   GET_IFACE2(pBroker,IUserDefinedLoads,pUDL);
   bool are_user_loads = pUDL->DoUserLoadsExist(span,girder);
   if (are_user_loads)
   {
      *p << CUserMomentsTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
   }

   // Product Shears
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductShearTable().Build(pBroker,span,girder,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   if (are_user_loads)
   {
      *p << CUserShearTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
   }

   // Product Reactions
   p = new rptParagraph;
   *pChapter << p;
   *p << CProductReactionTable().Build(pBroker,span,girder,analysisType,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;
   *p << LIVELOAD_PER_LANE << rptNewLine;
   *p << rptNewLine;
   LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

   if (are_user_loads)
   {
      *p << CUserReactionTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
   }

   // Product Displacements
   if ( bDesign )
   {
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductDisplacementsTable().Build(pBroker,span,girder,analysisType,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;
      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

      if (are_user_loads)
      {
         *p << CUserDisplacementsTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
      }

      // Product Rotations
      p = new rptParagraph;
      *pChapter << p;
      *p << CProductRotationTable().Build(pBroker,span,girder,analysisType,true,false,bDesign,bRating,bIndicateControllingLoad,pDisplayUnits) << rptNewLine;
      *p << LIVELOAD_PER_LANE << rptNewLine;
      *p << rptNewLine;
      LiveLoadTableFooter(pBroker,p,girder,bDesign,bRating);

      if (are_user_loads)
      {
         *p << CUserRotationTable().Build(pBroker,span,girder,analysisType,pDisplayUnits) << rptNewLine;
      }
   }

   GET_IFACE2( pBroker, ILibrary, pLib );
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if (bDesign && pSpecEntry->GetDoEvaluateLLDeflection())
   {
      // Optional Live Load Displacements
      p = new rptParagraph;
      p->SetName("Live Load Displacements");
      *pChapter << p;
      *p << CProductDisplacementsTable().BuildLiveLoadTable(pBroker,span,girder,pDisplayUnits) << rptNewLine;
      *p << "D1 = LRFD Design truck without lane load"<< rptNewLine;
      *p << "D2 = 0.25*(Design truck) + lane load"<< rptNewLine;
      *p << "D(Controlling) = Max(D1, D2)"<< rptNewLine;
      *p << rptNewLine;
   }

   // Load Combinations (DC, DW, etc) & Limit States
   if ( bDesign )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << "Responses - Casting Yard Stage" << rptNewLine;
      p->SetName("Combined Results - Casting Yard");
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard, analysisType);
   //   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard);
   //   CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::CastingYard);

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << "Responses - Girder Placement" << rptNewLine;
      p->SetName("Combined Results - Girder Placement");
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement, analysisType);
      CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement,   analysisType);
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::GirderPlacement,analysisType);

      GET_IFACE2(pBroker,IBridge,pBridge);
      SpanIndexType nSpans = pBridge->GetSpanCount();
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      bool bTempStrands = false;
      SpanIndexType firstSpanIdx = (span == ALL_SPANS ? 0 : span);
      SpanIndexType lastSpanIdx  = (span == ALL_SPANS ? nSpans : firstSpanIdx+1);
      for ( SpanIndexType spanIdx = firstSpanIdx; spanIdx < lastSpanIdx; spanIdx++ )
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
         GirderIndexType gdrIdx = (nGirders <= girder ? nGirders-1 : girder);
         if ( 0 < pStrandGeom->GetMaxStrands(spanIdx,gdrIdx,pgsTypes::Temporary) )
         {
            bTempStrands = true;
            break;
         }
      }
      if ( bTempStrands )
      {
         // if there can be temporary strands, report the loads at the temporary strand removal stage
         // because this is when the girder load is applied
         p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
         *pChapter << p;
         *p << "Responses - Temporary Strand Removal Stage" << rptNewLine;
         p->SetName("Combined Results - Temporary Strand Removal");
         CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, analysisType);
         CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, analysisType);
         CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::TemporaryStrandRemoval, analysisType);
      }

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << "Responses - Deck and Diaphragm Placement (Bridge Site 1)" << rptNewLine;
      p->SetName("Combined Results - Deck and Diaphragm Placement (Bridge Site 1)");
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1, analysisType);
      CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1, analysisType);
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite1, analysisType);

      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << "Responses - Superimposed Dead Loads (Bridge Site 2)" << rptNewLine;
      p->SetName("Combined Results - Superimposed Dead Loads (Bridge Site 2)");
      CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2, analysisType);
      CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2, analysisType);
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite2, analysisType);
   }

   p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
   *pChapter << p;
   *p << "Responses - Final with Live Load (Bridge Site 3)" << rptNewLine;
   p->SetName("Combined Results - Final with Live Load (Bridge Site 3)");
   CLiveLoadDistributionFactorTable().Build(pChapter,pBroker,span,girder,pDisplayUnits);
   CCombinedMomentsTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType,bDesign,bRating);
   CCombinedShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType,bDesign,bRating);
   if ( bDesign )
      CCombinedReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType,bDesign,bRating);

   if ( bDesign )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << "Live Load Reactions without Impact" << rptNewLine;
      p->SetName("Live Load Reactions without Impact");
      CLiveLoadReactionTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType);
   }

   if ( pSpecEntry->GetShearCapacityMethod() == scmVciVcw )
   {
      p = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << p;
      *p << "Concurrent Shears" << rptNewLine;
      p->SetName("Concurrent Shears");
      CConcurrentShearTable().Build(pBroker,pChapter,span,girder,pDisplayUnits,pgsTypes::BridgeSite3, analysisType);
   }

   return pChapter;
}

CChapterBuilder* CMVRChapterBuilder::Clone() const
{
   return new CMVRChapterBuilder(m_bDesign,m_bRating);
}
