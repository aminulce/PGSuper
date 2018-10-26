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

// VoidedSlabDistFactorEngineer.cpp : Implementation of CVoidedSlabDistFactorEngineer
#include "stdafx.h"
#include "VoidedSlabDistFactorEngineer.h"
#include "..\PGSuperException.h"
#include <Units\SysUnits.h>
#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\GirderLabel.h>
#include <Reporting\ReportStyleHolder.h>
#include <PgsExt\StatusItem.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DistributionFactors.h>
#include <IFace\StatusCenter.h>
#include "helper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVoidedSlabFactory
HRESULT CVoidedSlabDistFactorEngineer::FinalConstruct()
{
   return S_OK;
}

void CVoidedSlabDistFactorEngineer::BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDisplayUnits)
{
   SPANDETAILS span_lldf;
   GetSpanDF(span,gdr,pgsTypes::StrengthI,-1,&span_lldf);

   PierIndexType pier1 = span;
   PierIndexType pier2 = span+1;
   PIERDETAILS pier1_lldf, pier2_lldf;
   GetPierDF(pier1, gdr, pgsTypes::StrengthI, pgsTypes::Ahead, -1, &pier1_lldf);
   GetPierDF(pier2, gdr, pgsTypes::StrengthI, pgsTypes::Back,  -1, &pier2_lldf);

   REACTIONDETAILS reaction1_lldf, reaction2_lldf;
   GetPierReactionDF(pier1, gdr, pgsTypes::StrengthI, -1, &reaction1_lldf);
   GetPierReactionDF(pier2, gdr, pgsTypes::StrengthI, -1, &reaction2_lldf);

   // do a sanity check to make sure the fundimental values are correct
   ATLASSERT(span_lldf.Method  == pier1_lldf.Method);
   ATLASSERT(span_lldf.Method  == pier2_lldf.Method);
   ATLASSERT(pier1_lldf.Method == pier2_lldf.Method);

   ATLASSERT(span_lldf.bExteriorGirder  == pier1_lldf.bExteriorGirder);
   ATLASSERT(span_lldf.bExteriorGirder  == pier2_lldf.bExteriorGirder);
   ATLASSERT(pier1_lldf.bExteriorGirder == pier2_lldf.bExteriorGirder);

   // Grab the interfaces that are needed
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ILiveLoads,pLiveLoads);

   // determine continuity
   bool bContinuous, bContinuousAtStart, bContinuousAtEnd;
   pBridge->IsContinuousAtPier(pier1,&bContinuous,&bContinuousAtStart);
   pBridge->IsContinuousAtPier(pier2,&bContinuousAtEnd,&bContinuous);

   bool bIntegral, bIntegralAtStart, bIntegralAtEnd;
   pBridge->IsIntegralAtPier(pier1,&bIntegral,&bIntegralAtStart);
   pBridge->IsIntegralAtPier(pier2,&bIntegralAtEnd,&bIntegral);

   rptParagraph* pPara;

   bool bSIUnits = IS_SI_UNITS(pDisplayUnits);
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    location, pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,     pDisplayUnits->GetAreaUnit(),            true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );
   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim2,    pDisplayUnits->GetComponentDimUnit(),    true );
   INIT_UV_PROTOTYPE( rptLength4UnitValue,   inertia,  pDisplayUnits->GetMomentOfInertiaUnit(), true );
   INIT_UV_PROTOTYPE( rptAngleUnitValue,     angle,    pDisplayUnits->GetAngleUnit(),           true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   std::string strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdr);

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pPara) << "Method of Computation:"<<rptNewLine;
   (*pChapter) << pPara;
   pPara = new rptParagraph;
   (*pChapter) << pPara;
   (*pPara) << GetComputationDescription(span,gdr,
                                         strGirderName,
                                         pDeck->DeckType,
                                         pDeck->TransverseConnectivity);

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << "Distribution Factor Parameters" << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Girder Spacing: " << Sub2("S","avg") << " = " << xdim.SetValue(span_lldf.Savg) << rptNewLine;
   Float64 station,offset;
   pBridge->GetStationAndOffset(pgsPointOfInterest(span,gdr,span_lldf.ControllingLocation),&station, &offset);
   Float64 supp_dist = span_lldf.ControllingLocation - pBridge->GetGirderStartConnectionLength(span,gdr);
   (*pPara) << "Measurement of Girder Spacing taken at " << location.SetValue(supp_dist)<< " from left support, measured along girder, or station = "<< rptRcStation(station, &pDisplayUnits->GetStationFormat() ) << rptNewLine;
   (*pPara) << "Span Length: L = " << xdim.SetValue(span_lldf.L) << rptNewLine;
   (*pPara) << "Deck Width: W = " << xdim.SetValue(span_lldf.W) << rptNewLine;
   (*pPara) << "Moment of Inertia: I = " << inertia.SetValue(span_lldf.I) << rptNewLine;
   (*pPara) << "St. Venant torsional inertia constant: J = " << inertia.SetValue(span_lldf.J) << rptNewLine;
   (*pPara) << "Beam Width: b = " << xdim2.SetValue(span_lldf.b) << rptNewLine;
   (*pPara) << "Beam Depth: d = " << xdim2.SetValue(span_lldf.d) << rptNewLine;
   Float64 de = span_lldf.Side==dfLeft ? span_lldf.leftDe:span_lldf.rightDe;
   (*pPara) << "Distance from exterior web of exterior beam to curb line: d" << Sub("e") << " = " << xdim.SetValue(de) << rptNewLine;
   (*pPara) << "Possion Ratio: " << symbol(mu) << " = " << span_lldf.PossionRatio << rptNewLine;
//   (*pPara) << "Skew Angle at start: " << symbol(theta) << " = " << angle.SetValue(fabs(span_lldf.skew1)) << rptNewLine;
//   (*pPara) << "Skew Angle at end: " << symbol(theta) << " = " << angle.SetValue(fabs(span_lldf.skew2)) << rptNewLine;
   (*pPara) << "Number of Design Lanes: N" << Sub("L") << " = " << span_lldf.Nl << rptNewLine;
   (*pPara) << "Lane Width: wLane = " << xdim.SetValue(span_lldf.wLane) << rptNewLine;
   (*pPara) << "Number of Girders: N" << Sub("b") << " = " << span_lldf.Nb << rptNewLine;

   if (pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::LeverRule)
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pPara) << "St. Venant torsional inertia constant";
      (*pChapter) << pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      if ( span_lldf.nVoids == 0 )
      {
         (*pPara) << "Polar Moment of Inertia: I" << Sub("p") << " = " << inertia.SetValue(span_lldf.Jsolid.Ip) << rptNewLine;
         (*pPara) << "Area: A" << " = " << area.SetValue(span_lldf.Jsolid.A) << rptNewLine;
         (*pPara) << "Torsional Constant: " << rptRcImage(strImagePath + "J Equation.gif") << rptTab
                  << "J" << " = " << inertia.SetValue(span_lldf.J) << rptNewLine;
      }
      else
      {
         (*pPara) << rptRcImage(strImagePath + "J_Equation_closed_thin_walled.gif") << rptNewLine;
         (*pPara) << rptRcImage(strImagePath + "VoidedSlab_TorsionalConstant.gif") << rptNewLine;
         (*pPara) << "Area enclosed by centerlines of elements: " << Sub2("A","o") << " = " << area.SetValue(span_lldf.Jvoid.Ao) << rptNewLine;

         rptRcTable* p_table = pgsReportStyleHolder::CreateDefaultTable(3,"");
         (*pPara) << p_table;

         (*p_table)(0,0) << "Element";
         (*p_table)(0,1) << "s";
         (*p_table)(0,2) << "t";

         RowIndexType row = p_table->GetNumberOfHeaderRows();
         std::vector<VOIDEDSLAB_J_VOID::Element>::iterator iter;
         for ( iter = span_lldf.Jvoid.Elements.begin(); iter != span_lldf.Jvoid.Elements.end(); iter++ )
         {
            VOIDEDSLAB_J_VOID::Element& element = *iter;
            (*p_table)(row,0) << row;
            (*p_table)(row,1) << xdim2.SetValue(element.first);
            (*p_table)(row,2) << xdim2.SetValue(element.second);

            row++;
         }
         (*pPara) << symbol(SUM) << "s/t = " << span_lldf.Jvoid.S_over_T << rptNewLine;
         (*pPara) << "Torsional Constant: J = " << inertia.SetValue(span_lldf.J) << rptNewLine;
      }
   }

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pPara) << "Strength and Service Limit States";
      (*pChapter) << pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
   }


   //////////////////////////////////////////////////////
   // Moments
   //////////////////////////////////////////////////////

   if ( bContinuousAtStart || bIntegralAtStart )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Negative Moment over Pier " << long(pier1+1) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((pier1_lldf.skew1 + pier1_lldf.skew2)/2)) << rptNewLine;
      (*pPara) << "Span Length: L = " << xdim.SetValue(pier1_lldf.L) << rptNewLine << rptNewLine;

      // Negative moment DF from pier1_lldf
      ReportMoment(pPara,
                   pier1_lldf,
                   pier1_lldf.gM1,
                   pier1_lldf.gM2,
                   pier1_lldf.gM,
                   bSIUnits,pDisplayUnits);
   }

   // Positive moment DF
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   if ( bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd )
      (*pPara) << "Distribution Factor for Positive and Negative Moment in Span " << LABEL_SPAN(span) << rptNewLine;
   else
      (*pPara) << "Distribution Factor for Positive Moment in Span " << LABEL_SPAN(span) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((span_lldf.skew1 + span_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << "Span Length: L = " << xdim.SetValue(span_lldf.L) << rptNewLine << rptNewLine;

   ReportMoment(pPara,
                span_lldf,
                span_lldf.gM1,
                span_lldf.gM2,
                span_lldf.gM,
                bSIUnits,pDisplayUnits);

   if ( bContinuousAtEnd || bIntegralAtEnd )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Negative Moment over Pier " << long(pier2+1) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((pier2_lldf.skew1 + pier2_lldf.skew2)/2)) << rptNewLine;
      (*pPara) << "Span Length: L = " << xdim.SetValue(pier2_lldf.L) << rptNewLine << rptNewLine;

      // Negative moment DF from pier2_lldf
      ReportMoment(pPara,
                   pier2_lldf,
                   pier2_lldf.gM1,
                   pier2_lldf.gM2,
                   pier2_lldf.gM,
                   bSIUnits,pDisplayUnits);
   }
   

   //////////////////////////////////////////////////////////////
   // Shears
   //////////////////////////////////////////////////////////////
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << "Distribution Factor for Shear in Span " << LABEL_SPAN(span) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((span_lldf.skew1 + span_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << "Span Length: L = " << xdim.SetValue(span_lldf.L) << rptNewLine << rptNewLine;

   ReportShear(pPara,
               span_lldf,
               span_lldf.gV1,
               span_lldf.gV2,
               span_lldf.gV,
               bSIUnits,pDisplayUnits);

   //////////////////////////////////////////////////////////////
   // Reactions
   //////////////////////////////////////////////////////////////
   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << "Distribution Factor for Reaction at Pier " << long(pier1+1) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((reaction1_lldf.skew1 + reaction1_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << "Span Length: L = " << xdim.SetValue(reaction1_lldf.L) << rptNewLine << rptNewLine;

   ReportShear(pPara,
               reaction1_lldf,
               reaction1_lldf.gR1,
               reaction1_lldf.gR2,
               reaction1_lldf.gR,
               bSIUnits,pDisplayUnits);

     ///////

   pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
   (*pChapter) << pPara;
   (*pPara) << "Distribution Factor for Reaction at Pier " << long(pier2+1) << rptNewLine;
   pPara = new rptParagraph;
   (*pChapter) << pPara;

   (*pPara) << "Average Skew Angle: " << symbol(theta) << " = " << angle.SetValue(fabs((reaction2_lldf.skew1 + reaction2_lldf.skew2)/2)) << rptNewLine;
   (*pPara) << "Span Length: L = " << xdim.SetValue(reaction2_lldf.L) << rptNewLine << rptNewLine;

   ReportShear(pPara,
               reaction2_lldf,
               reaction2_lldf.gR1,
               reaction2_lldf.gR2,
               reaction2_lldf.gR,
               bSIUnits,pDisplayUnits);

   ////////////////////////////////////////////////////////////////////////////
   // Fatigue limit states
   ////////////////////////////////////////////////////////////////////////////
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pPara) << "Fatigue Limit States";
      (*pChapter) << pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      std::string superscript;

      rptRcScalar scalar2 = scalar;

      //////////////////////////////////////////////////////////////
      // Moments
      //////////////////////////////////////////////////////////////
      if ( bContinuousAtEnd || bIntegralAtEnd )
      {
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << "Distribution Factor for Negative Moment over Pier " << LABEL_PIER(pier1) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (pier1_lldf.bExteriorGirder ? "ME" : "MI");
         (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(pier1_lldf.gM1.mg) << "/1.2 = " << scalar2.SetValue(pier1_lldf.gM1.mg/1.2);
      }

      // Positive moment DF
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      if ( bContinuousAtStart || bContinuousAtEnd || bIntegralAtStart || bIntegralAtEnd )
         (*pPara) << "Distribution Factor for Positive and Negative Moment in Span " << LABEL_SPAN(span) << rptNewLine;
      else
         (*pPara) << "Distribution Factor for Positive Moment in Span " << LABEL_SPAN(span) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (span_lldf.bExteriorGirder ? "ME" : "MI");
      (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(span_lldf.gM1.mg) << "/1.2 = " << scalar2.SetValue(span_lldf.gM1.mg/1.2);

      if ( bContinuousAtEnd || bIntegralAtEnd )
      {
         pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
         (*pChapter) << pPara;
         (*pPara) << "Distribution Factor for Negative Moment over Pier " << LABEL_PIER(pier2) << rptNewLine;
         pPara = new rptParagraph;
         (*pChapter) << pPara;

         superscript = (pier2_lldf.bExteriorGirder ? "ME" : "MI");
         (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(pier2_lldf.gM1.mg) << "/1.2 = " << scalar2.SetValue(pier2_lldf.gM1.mg/1.2);
      }

      //////////////////////////////////////////////////////////////
      // Shears
      //////////////////////////////////////////////////////////////
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Shear in Span " << LABEL_SPAN(span) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (span_lldf.bExteriorGirder ? "VE" : "VI");
      (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(span_lldf.gV1.mg) << "/1.2 = " << scalar2.SetValue(span_lldf.gV1.mg/1.2);

      //////////////////////////////////////////////////////////////
      // Reactions
      //////////////////////////////////////////////////////////////
      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Reaction at Pier " << LABEL_PIER(pier1) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (reaction1_lldf.bExteriorGirder ? "VE" : "VI");
      (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(reaction1_lldf.gR1.mg) << "/1.2 = " << scalar2.SetValue(reaction1_lldf.gR1.mg/1.2);

        ///////

      pPara = new rptParagraph(pgsReportStyleHolder::GetSubheadingStyle());
      (*pChapter) << pPara;
      (*pPara) << "Distribution Factor for Reaction at Pier " << LABEL_PIER(pier2) << rptNewLine;
      pPara = new rptParagraph;
      (*pChapter) << pPara;

      superscript = (reaction2_lldf.bExteriorGirder ? "VE" : "VI");
      (*pPara) << "g" << superscript << Sub("Fatigue") << " = " << "mg" << superscript << Sub("1") << "/m =" << scalar.SetValue(reaction2_lldf.gR1.mg) << "/1.2 = " << scalar2.SetValue(reaction2_lldf.gR1.mg/1.2);
   }
}

lrfdLiveLoadDistributionFactorBase* CVoidedSlabDistFactorEngineer::GetLLDFParameters(SpanIndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,VOIDEDSLAB_LLDFDETAILS* plldf)
{
   GET_IFACE(IBridgeMaterial,   pMaterial);
   GET_IFACE(ISectProp2,        pSectProp2);
   GET_IFACE(IGirder,           pGirder);
   GET_IFACE(ILibrary,          pLib);
   GET_IFACE(ISpecification,    pSpec);
   GET_IFACE(IRoadwayData,      pRoadway);
   GET_IFACE(IBridge,           pBridge);
   GET_IFACE(IPointOfInterest,  pPOI);
   GET_IFACE(IStatusCenter,pStatusCenter);
   GET_IFACE(IBarriers,pBarriers);

   // Determine span/pier index... This is the index of a pier and the next span.
   // If this is the last pier, span index is for the last span
   SpanIndexType span = INVALID_INDEX;
   PierIndexType pier = INVALID_INDEX;
   SpanIndexType prev_span = INVALID_INDEX;
   SpanIndexType next_span = INVALID_INDEX;
   PierIndexType prev_pier = INVALID_INDEX;
   PierIndexType next_pier = INVALID_INDEX;
   GetIndicies(spanOrPier,dfType,span,pier,prev_span,next_span,prev_pier,next_pier);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   ATLASSERT( pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::DirectlyInput );

   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   GirderIndexType nGirders = pSpan->GetGirderCount();

   if ( nGirders <= gdr )
   {
      ATLASSERT(0);
      gdr = nGirders-1;
   }

   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);

   // determine girder spacing
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();

   ///////////////////////////////////////////////////////////////////////////
   // Determine overhang and spacing information
   GetGirderSpacingAndOverhang(span,gdr,dfType, plldf);

   // put a poi at controlling location from spacing comp
   pgsPointOfInterest poi(span,gdr,plldf->ControllingLocation);

   // Throws exception if fails requirement (no need to catch it)
   GET_IFACE(ILiveLoadDistributionFactors, pDistFactors);
   pDistFactors->VerifyDistributionFactorRequirements(poi);

   Float64 Height       = pGirderEntry->GetDimension("H");
   Float64 Width        = pGirderEntry->GetDimension("W");
   Float64 VoidSpacing  = pGirderEntry->GetDimension("Void_Spacing");
   Float64 VoidDiameter = pGirderEntry->GetDimension("Void_Diameter");
   Int16 nVoids         = (Int16)pGirderEntry->GetDimension("Number_of_Voids");

   // thickness of exterior "web"
   Float64 t_ext;
   if (nVoids>0)
   {
      t_ext = (Width - (nVoids-1)*(VoidSpacing) - VoidDiameter)/2;
   }
   else
   {
      t_ext = Width;
   }

   plldf->b            = Width;
   plldf->d            = Height;

   if (fcgdr>0)
   {
      plldf->I            = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi,fcgdr);
   }
   else
   {
      plldf->I            = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi);
   }

   plldf->PossionRatio = 0.2;
   plldf->nVoids = nVoids; // need to make WBFL::LRFD consistent
   plldf->TransverseConnectivity = pDeck->TransverseConnectivity;

   pgsTypes::TrafficBarrierOrientation side = pBarriers->GetNearestBarrier(span,gdr);
   Float64 curb_offset = pBarriers->GetInterfaceWidth(side);

   // compute de (inside edge of barrier to CL of exterior web)
   Float64 wd = pGirder->GetCL2ExteriorWebDistance(poi); // cl beam to cl web
   ATLASSERT(wd>=0.0);

   // Note this is not exactly correct because opposite exterior beam might be different, but we won't be using this data set for that beam
   plldf->leftDe  = plldf->leftCurbOverhang - wd;  
   plldf->rightDe = plldf->rightCurbOverhang - wd; 

   plldf->L = GetEffectiveSpanLength(spanOrPier,gdr,dfType);

   lrfdLiveLoadDistributionFactorBase* pLLDF;

   if ( nVoids == 0 )
   {
      // solid slab

      Float64 Ix, Iy, A, Ip;
      if (fcgdr>0)
      {
         Ix = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi,fcgdr);
         Iy = pSectProp2->GetIy(pgsTypes::BridgeSite3,poi,fcgdr);
         A  = pSectProp2->GetAg(pgsTypes::BridgeSite3,poi,fcgdr);
      }
      else
      {
         Ix = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi);
         Iy = pSectProp2->GetIy(pgsTypes::BridgeSite3,poi);
         A  = pSectProp2->GetAg(pgsTypes::BridgeSite3,poi);
      }

      Ip = Ix + Iy;

      VOIDEDSLAB_J_SOLID Jsolid;
      Jsolid.A = A;
      Jsolid.Ip = Ip;

      plldf->Jsolid = Jsolid;
      plldf->J = A*A*A*A/(40.0*Ip);
   }
   else
   {
      // voided slab

      // Deterine J

      VOIDEDSLAB_J_VOID Jvoid;

      Float64 Sum_s_over_t = 0;

      // NOTE: t_ext is computed above

      // thickness of interior vertical elements
      Float64 t_int = VoidSpacing - VoidDiameter;

      // length of internal, vertical elements between voids
      Float64 s_int = (Height + VoidDiameter)/2;

      // s and t for top and bottom
      Float64 s_top = Width - t_ext;
      Float64 t_top = (Height - VoidDiameter)/2;

      Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_top,t_top)); // top
      Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_top,t_top)); // bottom
      Sum_s_over_t += 2*(s_top/t_top);

      Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_int,t_ext)); // left edge
      Sum_s_over_t += (s_int/t_ext);

      // between voids
      for ( long i = 0; i < nVoids-1; i++ )
      {
         Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_int,t_int));
         Sum_s_over_t += (s_int/t_int);
      }

      Jvoid.Elements.push_back(VOIDEDSLAB_J_VOID::Element(s_int,t_ext)); // right edge
      Sum_s_over_t += (s_int/t_ext);

      Jvoid.S_over_T = Sum_s_over_t;

      Float64 Ao = s_top * s_int;

      Jvoid.Ao = Ao;

      Float64 J = 4.0*Ao*Ao/Sum_s_over_t;

      plldf->Jvoid = Jvoid;
      plldf->J = J;
   }

   // WSDOT deviation doesn't apply to this type of cross section because it isn't slab on girder construction
   if (plldf->Method == LLDF_TXDOT)
   {
         pLLDF = new lrfdTxdotVoidedSlab(plldf->gdrNum,
                                         plldf->Savg,
                                         plldf->gdrSpacings,
                                         plldf->leftCurbOverhang,
                                         plldf->rightCurbOverhang,
                                         plldf->Nl, 
                                         plldf->wLane,
                                         plldf->L,
                                         plldf->W,
                                         plldf->I,
                                         plldf->J,
                                         plldf->b,
                                         plldf->d,
                                         plldf->leftDe,
                                         plldf->rightDe,
                                         plldf->PossionRatio,
                                         plldf->skew1, 
                                         plldf->skew2);
   }
   else
   {
      if ( pDeck->TransverseConnectivity == pgsTypes::atcConnectedAsUnit )
      {

         pLLDF = new lrfdLldfTypeF(plldf->gdrNum,
                                   plldf->Savg,
                                   plldf->gdrSpacings,
                                   plldf->leftCurbOverhang,
                                   plldf->rightCurbOverhang,
                                   plldf->Nl, 
                                   plldf->wLane,
                                   plldf->L,
                                   plldf->W,
                                   plldf->I,
                                   plldf->J,
                                   plldf->b,
                                   plldf->d,
                                   plldf->leftDe,
                                   plldf->rightDe,
                                   plldf->PossionRatio,
                                   false,
                                   plldf->skew1, 
                                   plldf->skew2);
      }
      else
      {

         pLLDF = new lrfdLldfTypeG(plldf->gdrNum,
                            plldf->Savg,
                            plldf->gdrSpacings,
                            plldf->leftCurbOverhang,
                            plldf->rightCurbOverhang,
                            plldf->Nl, 
                            plldf->wLane,
                            plldf->L,
                            plldf->W,
                            plldf->I,
                            plldf->J,
                            plldf->b,
                            plldf->d,
                            plldf->leftDe,
                            plldf->rightDe,
                            plldf->PossionRatio,
                            false,
                            plldf->skew1, 
                            plldf->skew2);
            
            
      }
   }

   GET_IFACE(ILiveLoads,pLiveLoads);
   pLLDF->SetRangeOfApplicabilityAction( pLiveLoads->GetLldfRangeOfApplicabilityAction() );

   plldf->bExteriorGirder = pBridge->IsExteriorGirder(span,gdr);

   return pLLDF;
}

void CVoidedSlabDistFactorEngineer::ReportMoment(rptParagraph* pPara,VOIDEDSLAB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,double gM,bool bSIUnits,IDisplayUnits* pDisplayUnits)
{
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   GET_IFACE(IBridge,pBridge);

   if ( lldf.bExteriorGirder )
   {
      if (gM1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane") << rptNewLine;
         (*pPara) << Bold("Lever Rule") << rptNewLine;
         ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if (gM1.EqnData.bWasUsed)
      {
         (*pPara) << Bold("1 Loaded Lane: Equation") << rptNewLine;

         if (lldf.Method == LLDF_TXDOT && !(gM1.ControllingMethod & LEVER_RULE))
         {
            (*pPara) << "For TxDOT Method, Use "<<"mg" << Super("MI") << Sub("1")<<". And,do not apply skew correction factor."<< rptNewLine;

            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_MI_Type_G_SI.gif" : "mg_1_MI_Type_G_US.gif")) << rptNewLine;
            ATLASSERT(gM1.ControllingMethod & S_OVER_D_METHOD);
            (*pPara)<< "K = "<< gM1.EqnData.K << rptNewLine;
            (*pPara)<< "C = "<< gM1.EqnData.C << rptNewLine;
            (*pPara)<< "D = "<< xdim.SetValue(gM1.EqnData.D) << rptNewLine;
            (*pPara) << rptNewLine;

            (*pPara) << "mg" << Super("ME") << Sub("1") << " = " << scalar.SetValue(gM1.mg) << rptNewLine;

         }
         else
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_ME_Type_G_SI.gif" : "mg_1_ME_Type_G_US.gif")) << rptNewLine;
 
            if ( lldf.TransverseConnectivity == pgsTypes::atcConnectedAsUnit )
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_MI_Type_F_SI.gif" : "mg_1_MI_Type_F_US.gif")) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_MI_Type_G_SI.gif" : "mg_1_MI_Type_G_US.gif")) << rptNewLine;
               ATLASSERT(gM1.ControllingMethod & S_OVER_D_METHOD);
               (*pPara)<< "K = "<< gM1.EqnData.K << rptNewLine;
               (*pPara)<< "C = "<< gM1.EqnData.C << rptNewLine;
               (*pPara)<< "D = "<< xdim.SetValue(gM1.EqnData.D) << rptNewLine;
               (*pPara) << rptNewLine;
            }

            (*pPara) << "mg" << Super("MI") << Sub("1") << " = " << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
            (*pPara) << "e = " << gM1.EqnData.e << rptNewLine;
            (*pPara) << "mg" << Super("ME") << Sub("1") << " = " << scalar.SetValue(gM1.EqnData.mg*gM1.EqnData.e) << rptNewLine;
         }
      }

      if ( gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
         (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

       
      if ( 2 <= lldf.Nl )
      {

         if (gM2.LeverRuleData.bWasUsed)
         {
            ATLASSERT(gM2.ControllingMethod & LEVER_RULE);
            (*pPara) << rptNewLine;
            (*pPara) << Bold("2+ Loaded Lanes: Lever Rule") << rptNewLine;
            ReportLeverRule(pPara,true,1.0,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if (gM2.EqnData.bWasUsed)
         {
            if (lldf.Method == LLDF_TXDOT && !(gM2.ControllingMethod & LEVER_RULE))
            {
               (*pPara) << Bold("2+ Loaded Lanes: Equation Method") << rptNewLine;
               (*pPara) << "Same as for 1 Loaded Lane" << rptNewLine;
               (*pPara) << "mg" << Super("ME") << Sub("2") << " = " << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
            }
            else
            {
               (*pPara) << Bold("2+ Loaded Lane") << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_2_ME_Type_G_SI.gif" : "mg_2_ME_Type_G_US.gif")) << rptNewLine;

               if ( lldf.TransverseConnectivity == pgsTypes::atcConnectedAsUnit )
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_2_MI_Type_F_SI.gif" : "mg_2_MI_Type_F_US.gif")) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_2_MI_Type_G_SI.gif" : "mg_2_MI_Type_G_US.gif")) << rptNewLine;
                  ATLASSERT(gM2.ControllingMethod & S_OVER_D_METHOD);
                  (*pPara)<< "K = "<< gM2.EqnData.K << rptNewLine;
                  (*pPara)<< "C = "<< gM2.EqnData.C << rptNewLine;
                  (*pPara)<< "D = "<< xdim.SetValue(gM2.EqnData.D) << rptNewLine;
                  (*pPara) << rptNewLine;
               }

               (*pPara) << "mg" << Super("MI") << Sub("2+") << " = " << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
               (*pPara) << "e = " << gM2.EqnData.e << rptNewLine;
               (*pPara) << "mg" << Super("ME") << Sub("2+") << " = " << scalar.SetValue(gM2.EqnData.mg*gM2.EqnData.e) << rptNewLine;
            }
         }

         if ( gM2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
            (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      (*pPara) << Bold("Skew Correction") << rptNewLine;
      if(lldf.Method != LLDF_TXDOT)
      {
         (*pPara) << rptRcImage(strImagePath + "Skew Correction for Moment Type C.gif") << rptNewLine;
      }

      (*pPara) << "Skew Correction Factor: = " << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
      (*pPara) << "Skew Corrected Factor: mg" << Super("ME") << Sub("1") << " = " << scalar.SetValue(gM1.mg);
      (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
      if ( lldf.Nl >= 2 )
      {
         (*pPara) << "Skew Corrected Factor: mg" << Super("ME") << Sub("2+") << " = " << scalar.SetValue(gM2.mg);
         (gM2.mg > gM1.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
      }
   }
   else
   {
      // Interior Girder
      if ( gM1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Lever Rule") << rptNewLine;
         ReportLeverRule(pPara,true,1.0,gM1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if (gM1.EqnData.bWasUsed)
      {
         (*pPara) << Bold("1 Loaded Lane: Equation") << rptNewLine;
         if (lldf.Method == LLDF_TXDOT && !(gM1.ControllingMethod & LEVER_RULE))
         {
            (*pPara) << "For TxDOT Method, do not apply skew correction factor."<< rptNewLine;

            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_MI_Type_G_SI.gif" : "mg_1_MI_Type_G_US.gif")) << rptNewLine;
            ATLASSERT(gM1.ControllingMethod & S_OVER_D_METHOD);
            (*pPara)<< "K = "<< gM1.EqnData.K << rptNewLine;
            (*pPara)<< "C = "<< gM1.EqnData.C << rptNewLine;
            (*pPara)<< "D = "<< xdim.SetValue(gM1.EqnData.D) << rptNewLine;
            (*pPara) << rptNewLine;

            (*pPara) << "mg" << Super("MI") << Sub("1") << " = " << scalar.SetValue(gM1.mg) << rptNewLine;
         }
         else
         {
            if ( lldf.TransverseConnectivity == pgsTypes::atcConnectedAsUnit )
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_MI_Type_F_SI.gif" : "mg_1_MI_Type_F_US.gif")) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_MI_Type_G_SI.gif" : "mg_1_MI_Type_G_US.gif")) << rptNewLine;
               ATLASSERT(gM1.ControllingMethod & S_OVER_D_METHOD);
               (*pPara)<< "K = "<< gM1.EqnData.K << rptNewLine;
               (*pPara)<< "C = "<< gM1.EqnData.C << rptNewLine;
               (*pPara)<< "D = "<< xdim.SetValue(gM1.EqnData.D) << rptNewLine;
               (*pPara) << rptNewLine;
            }

            (*pPara) << "mg" << Super("MI") << Sub("1") << " = " << scalar.SetValue(gM1.EqnData.mg) << rptNewLine;
         }
      }

      if ( gM1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
         (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gM1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      if ( 2 <= lldf.Nl )
      {
         if ( gM2.LeverRuleData.bWasUsed )
         {
            (*pPara) << rptNewLine;
            (*pPara) << Bold("2+ Loaded Lanes") << rptNewLine;
            (*pPara) << Bold("Lever Rule") << rptNewLine;
            ReportLeverRule(pPara,true,1.0,gM2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if (gM2.EqnData.bWasUsed)
         {
            if (lldf.Method == LLDF_TXDOT && !(gM2.ControllingMethod & LEVER_RULE))
            {
               (*pPara) << rptNewLine;

               (*pPara) << Bold("2+ Loaded Lanes: Equation Method") << rptNewLine;
               (*pPara) << "Same as for 1 Loaded Lane" << rptNewLine;
               (*pPara) << "mg" << Super("MI") << Sub("2") << " = " << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
            }
            else
            {
               (*pPara) << rptNewLine;

               (*pPara) << Bold("2+ Loaded Lanes") << rptNewLine;

               if ( lldf.TransverseConnectivity == pgsTypes::atcConnectedAsUnit )
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_2_MI_Type_F_SI.gif" : "mg_2_MI_Type_F_US.gif")) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_2_MI_Type_G_SI.gif" : "mg_2_MI_Type_G_US.gif")) << rptNewLine;
                  ATLASSERT(gM2.ControllingMethod & S_OVER_D_METHOD);
                  (*pPara)<< "K = "<< gM2.EqnData.K << rptNewLine;
                  (*pPara)<< "C = "<< gM2.EqnData.C << rptNewLine;
                  (*pPara)<< "D = "<< xdim.SetValue(gM2.EqnData.D) << rptNewLine;
                  (*pPara) << rptNewLine;
               }

               (*pPara) << "mg" << Super("MI") << Sub("2+") << " = " << scalar.SetValue(gM2.EqnData.mg) << rptNewLine;
            }
         }

         if ( gM2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
            (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gM2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      (*pPara) << Bold("Skew Correction") << rptNewLine;
      if(lldf.Method != LLDF_TXDOT)
      {
         (*pPara) << rptRcImage(strImagePath + "Skew Correction for Moment Type C.gif") << rptNewLine;
      }

      (*pPara) << "Skew Correction Factor: = " << scalar.SetValue(gM1.SkewCorrectionFactor) << rptNewLine;
      (*pPara) << "Skew Corrected Factor: mg" << Super("MI") << Sub("1") << " = " << scalar.SetValue(gM1.mg);
      (lldf.Nl == 1 || gM1.mg >= gM2.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
      if ( lldf.Nl >= 2 )
      {
         (*pPara) << "Skew Corrected Factor: mg" << Super("MI") << Sub("2+") << " = " << scalar.SetValue(gM2.mg);
         (gM2.mg > gM1.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
      }
   }
}

void CVoidedSlabDistFactorEngineer::ReportShear(rptParagraph* pPara,VOIDEDSLAB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,double gV,bool bSIUnits,IDisplayUnits* pDisplayUnits)
{
   std::string strImagePath(pgsReportStyleHolder::GetImagePath());

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    xdim,     pDisplayUnits->GetSpanLengthUnit(),      true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Fixed );
   scalar.SetWidth(6);
   scalar.SetPrecision(3);
   scalar.SetTolerance(1.0e-6);

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( lldf.bExteriorGirder )
   {
      if ( gV1.EqnData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Equation") << rptNewLine;
         if( lldf.Method==LLDF_TXDOT )
         {
            (*pPara) << "For TxDOT Method, Use "<<"mg" << Super("MI") << Sub("1")<<". And,do not apply skew correction factor."<< rptNewLine;

            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_MI_Type_G_SI.gif" : "mg_1_MI_Type_G_US.gif")) << rptNewLine;
            ATLASSERT(gV1.ControllingMethod & S_OVER_D_METHOD);
            (*pPara)<< "K = "<< gV1.EqnData.K << rptNewLine;
            (*pPara)<< "C = "<< gV1.EqnData.C << rptNewLine;
            (*pPara)<< "D = "<< xdim.SetValue(gV1.EqnData.D) << rptNewLine;
            (*pPara) << rptNewLine;

            (*pPara) << "mg" << Super("VE") << Sub("1") << " = " << scalar.SetValue(gV1.mg) << rptNewLine;
         }
         else 
         {

            if (gV1.ControllingMethod & MOMENT_OVERRIDE)
            {
               (*pPara) << "Overriden by moment factor because J or I was out of range for shear equation"<<rptNewLine;
               (*pPara) << "mg" << Super("VE") << Sub("1") << " = " << "mg" << Super("ME") << Sub("1") << " = " << scalar.SetValue(gV1.EqnData.mg*gV1.EqnData.e) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_VE_Type_G_SI.gif" : "mg_1_VE_Type_G_US.gif")) << rptNewLine;
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_VI_Type_G_SI.gif" : "mg_1_VI_Type_G_US.gif")) << rptNewLine;
               (*pPara) << "mg" << Super("VI") << Sub("1") << " = " << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
               (*pPara) << "e = " << gV1.EqnData.e << rptNewLine;
               (*pPara) << "mg" << Super("VE") << Sub("1") << " = " << scalar.SetValue(gV1.EqnData.mg*gV1.EqnData.e) << rptNewLine;
            }
         }

      }

      if ( gV1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Lever Rule") << rptNewLine;
         ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
         (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      if ( 2 <= lldf.Nl )
      {
         (*pPara) << rptNewLine;
         if ( gV2.ControllingMethod & LEVER_RULE )
         {
            (*pPara) << Bold("2+ Loaded Lane: Lever Rule") << rptNewLine;
            ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gV1.EqnData.bWasUsed )
         {
            if( lldf.Method==LLDF_TXDOT )
            {
               (*pPara) << Bold("2+ Loaded Lane") << rptNewLine;
               (*pPara) << "Same as for 1 Loaded Lane" << rptNewLine;
               (*pPara) << "mg" << Super("VE") << Sub("2") << " = " << scalar.SetValue(gV2.mg) << rptNewLine;
            }
            else 
            {
               (*pPara) << Bold("2+ Loaded Lane: Equation") << rptNewLine;

               if (gV2.ControllingMethod & MOMENT_OVERRIDE)
               {
                  (*pPara) << "Overriden by moment factor because J or I was out of range for shear equation"<<rptNewLine;
                  (*pPara) << "mg" << Super("VE") << Sub("2") << " = " << "mg" << Super("ME") << Sub("2") << " = " << scalar.SetValue(gV2.EqnData.mg*gV2.EqnData.e) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_2_VE_Type_G_SI.gif" : "mg_2_VE_Type_G_US.gif")) << rptNewLine;
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_2_VI_Type_G_SI.gif" : "mg_2_VI_Type_G_US.gif")) << rptNewLine;
                  (*pPara) << "mg" << Super("VI") << Sub("2+") << " = " << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
                  (*pPara) << "e = " << gV2.EqnData.e << rptNewLine;
                  (*pPara) << "mg" << Super("VE") << Sub("2+") << " = " << scalar.SetValue(gV2.EqnData.mg*gV2.EqnData.e) << rptNewLine;
               }
            }
         }

         if ( gV2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
            (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }

         (*pPara) << rptNewLine;

         (*pPara) << Bold("Skew Correction") << rptNewLine;
         if(lldf.Method != LLDF_TXDOT)
         {
            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "Skew_Correction_for_Shear_Type_F_SI.gif" : "Skew_Correction_for_Shear_Type_F_US.gif")) << rptNewLine;
         }

         (*pPara) << "Skew Correction Factor: = " << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
         (*pPara) << "Skew Corrected Factor: mg" << Super("VE") << Sub("1") << " = " << scalar.SetValue(gV1.mg);
         (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
         if ( lldf.Nl >= 2 )
         {
            (*pPara) << "Skew Corrected Factor: mg" << Super("VE") << Sub("2+") << " = " << scalar.SetValue(gV2.mg);
            (gV2.mg > gV1.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
         }
      }
   }
   else
   {
      // Interior Girder
      //
      // Shear
      //

      if ( gV1.EqnData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Equation") << rptNewLine;
         if( lldf.Method==LLDF_TXDOT )
         {
            (*pPara) << "For TxDOT Method, Use "<<"mg" << Super("MI") << Sub("1")<<". And,do not apply shear correction factor."<< rptNewLine;

            (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_MI_Type_G_SI.gif" : "mg_1_MI_Type_G_US.gif")) << rptNewLine;
            ATLASSERT(gV1.ControllingMethod &   S_OVER_D_METHOD);
            (*pPara)<< "K = "<< gV1.EqnData.K << rptNewLine;
            (*pPara)<< "C = "<< gV1.EqnData.C << rptNewLine;
            (*pPara)<< "D = "<< xdim.SetValue(gV1.EqnData.D) << rptNewLine;
            (*pPara) << rptNewLine;

            (*pPara) << "mg" << Super("VI") << Sub("1") << " = " << scalar.SetValue(gV1.mg) << rptNewLine;
         }
         else 
         {
            if (gV1.ControllingMethod & MOMENT_OVERRIDE)
            {
               (*pPara) << "Overriden by moment factor because J or I was out of range for shear equation"<<rptNewLine;
               (*pPara) << "mg" << Super("VI") << Sub("1") << " = " << "mg" << Super("MI") << Sub("1") << " = " << scalar.SetValue(gV1.mg) << rptNewLine;
            }
            else
            {
               (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_1_VI_Type_G_SI.gif" : "mg_1_VI_Type_G_US.gif")) << rptNewLine;
               (*pPara) << "mg" << Super("VI") << Sub("1") << " = " << scalar.SetValue(gV1.EqnData.mg) << rptNewLine;
            }
         }
      }

      if ( gV1.LeverRuleData.bWasUsed )
      {
         (*pPara) << Bold("Lever Rule") << rptNewLine;
         ReportLeverRule(pPara,false,1.0,gV1.LeverRuleData,m_pBroker,pDisplayUnits);
      }

      if ( gV1.LanesBeamsData.bWasUsed )
      {
         (*pPara) << Bold("1 Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
         (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
         ReportLanesBeamsMethod(pPara,gV1.LanesBeamsData,m_pBroker,pDisplayUnits);
      }

      if ( 2 <= lldf.Nl )
      {
         if ( gV2.EqnData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lane: Equation") << rptNewLine;
            if( lldf.Method==LLDF_TXDOT )
            {
               (*pPara) << Bold("2+ Loaded Lane") << rptNewLine;
               (*pPara) << "Same as for 1 Loaded Lane" << rptNewLine;
               (*pPara) << "mg" << Super("VI") << Sub("2") << " = " << scalar.SetValue(gV2.mg) << rptNewLine;
            }
            else
            {
               if (gV2.ControllingMethod & MOMENT_OVERRIDE)
               {
                  (*pPara) << "Overriden by moment factor because J or I was out of range for shear equation"<<rptNewLine;
                  (*pPara) << "mg" << Super("VI") << Sub("2") << " = " << "mg" << Super("MI") << Sub("2") << " = " << scalar.SetValue(gV2.mg) << rptNewLine;
               }
               else
               {
                  (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "mg_2_VI_Type_G_SI.gif" : "mg_2_VI_Type_G_US.gif")) << rptNewLine;
                  (*pPara) << "mg" << Super("VI") << Sub("2+") << " = " << scalar.SetValue(gV2.EqnData.mg) << rptNewLine;
               }
            }
         }

         if ( gV2.LeverRuleData.bWasUsed)
         {
            (*pPara) << Bold("2+ Loaded Lane: Lever Rule") << rptNewLine;
            ReportLeverRule(pPara,false,1.0,gV2.LeverRuleData,m_pBroker,pDisplayUnits);
         }

         if ( gV2.LanesBeamsData.bWasUsed )
         {
            (*pPara) << Bold("2+ Loaded Lane: Number of Lanes over Number of Beams - Factor cannot be less than this") << rptNewLine;
            (*pPara) << "Skew correction is not applied to Lanes/Beams method"<< rptNewLine;
            ReportLanesBeamsMethod(pPara,gV2.LanesBeamsData,m_pBroker,pDisplayUnits);
         }
      }

      (*pPara) << rptNewLine;

      (*pPara) << Bold("Skew Correction") << rptNewLine;
      (*pPara) << rptRcImage(strImagePath + (bSIUnits ? "Skew_Correction_for_Shear_Type_F_SI.gif" : "Skew_Correction_for_Shear_Type_F_US.gif")) << rptNewLine;
      (*pPara) << "Skew Correction Factor: = " << scalar.SetValue(gV1.SkewCorrectionFactor) << rptNewLine;
      (*pPara) << rptNewLine;
      (*pPara) << "Skew Corrected Factor: mg" << Super("VI") << Sub("1") << " = " << scalar.SetValue(gV1.mg);
      (lldf.Nl == 1 || gV1.mg >= gV2.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
      if ( lldf.Nl >= 2 )
      {
         (*pPara) << "Skew Corrected Factor: mg" << Super("VI") << Sub("2+") << " = " << scalar.SetValue(gV2.mg);
         (gV2.mg > gV1.mg) ? (*pPara) << Bold(" < Controls") << rptNewLine : (*pPara) << rptNewLine;
      }
   }
}

std::string CVoidedSlabDistFactorEngineer::GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::string& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect)
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 lldfMethod = pSpecEntry->GetLiveLoadDistributionMethod();

   std::string descr;
   if ( lldfMethod == LLDF_TXDOT )
   {
      descr += std::string("TxDOT Section 3.7 modifications (no skew correction for moment or shear). Regardless of input connectivity or deck type, use AASHTO Type (g) connected only enough to prevent relative vertical displacement.");
   }
   else if ( lldfMethod == LLDF_LRFD || lldfMethod == LLDF_WSDOT  )
   {
      if (decktype == pgsTypes::sdtCompositeOverlay || decktype == pgsTypes::sdtNone)
      {
         descr += std::string("AASHTO Type (f) using AASHTO LRFD Method per Article 4.6.2.2");
      }
      else
      {
         descr += std::string("AASHTO Type (g) using AASHTO LRFD Method per Article 4.6.2.2 with determination of transverse connectivity.");
      }
   }
   else
   {
      ATLASSERT(0);
   }

   // Special text if ROA is ignored
   GET_IFACE(ILiveLoads,pLiveLoads);
   std::string straction = pLiveLoads->GetLLDFSpecialActionText();
   if ( !straction.empty() )
   {
      descr += straction;
   }

   return descr;
}
