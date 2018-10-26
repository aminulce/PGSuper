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

#include "StdAfx.h"
#include <Reporting\TimeStepDetailsChapterBuilder.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PointOfInterest.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>

#include <WBFLGenericBridgeTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// When defined, the computation details for initial strains are reported
// This is the summation part of Tadros 1977 Eqns 3 and 4.
//#define REPORT_INITIAL_STRAIN_DETAILS

//#define REPORT_PRODUCT_LOAD_DETAILS

/****************************************************************************
CLASS
   CTimeStepDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTimeStepDetailsChapterBuilder::CTimeStepDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CTimeStepDetailsChapterBuilder::GetName() const
{
   return TEXT("Time Step Details");
}

rptChapter* CTimeStepDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   CPointOfInterestReportSpecification* pPoiRptSpec = dynamic_cast<CPointOfInterestReportSpecification*>(pRptSpec);

   CComPtr<IBroker> pBroker;
   pPoiRptSpec->GetBroker(&pBroker);

   const pgsPointOfInterest& poi(pPoiRptSpec->GetPointOfInterest());
   const CGirderKey& girderKey(poi.GetSegmentKey());

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      *pPara << color(Red) << _T("Time Step analysis results not available.") << color(Black) << rptNewLine;
      return pChapter;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IProductLoads,pProductLoads);
//   GET_IFACE2(pBroker,ITendonGeometry,pTendonGeom);
//   GET_IFACE2(pBroker,IPointOfInterest,pPOI);
   GET_IFACE2(pBroker,ILosses,pLosses);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
//   GET_IFACE2(pBroker,IMaterials,pMaterials);
//   GET_IFACE2(pBroker,ILongRebarGeometry,pRebarGeom);
//
//#if !defined LUMP_STRANDS
//   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
//#endif
//
//
//   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
//
//   INIT_UV_PROTOTYPE(rptPointOfInterest,    location,   pDisplayUnits->GetSpanLengthUnit(),      true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    ecc,        pDisplayUnits->GetComponentDimUnit(),    true);
   INIT_UV_PROTOTYPE(rptLengthUnitValue,    height,        pDisplayUnits->GetComponentDimUnit(),    true);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    stress,     pDisplayUnits->GetStressUnit(),          true);
   INIT_UV_PROTOTYPE(rptLength2UnitValue,   area,       pDisplayUnits->GetAreaUnit(),            true);
   INIT_UV_PROTOTYPE(rptLength4UnitValue,   momI,       pDisplayUnits->GetMomentOfInertiaUnit(), true);
   INIT_UV_PROTOTYPE(rptStressUnitValue,    modE,       pDisplayUnits->GetModEUnit(),            true);
   INIT_UV_PROTOTYPE(rptForceUnitValue,     force,      pDisplayUnits->GetGeneralForceUnit(),    true);
   INIT_UV_PROTOTYPE(rptMomentUnitValue,    moment,     pDisplayUnits->GetSmallMomentUnit(),     true);
   INIT_UV_PROTOTYPE(rptPerLengthUnitValue, curvature,  pDisplayUnits->GetCurvatureUnit(),       true);
//   INIT_UV_PROTOTYPE(rptLengthUnitValue,    length,     pDisplayUnits->GetSpanLengthUnit(),      true);
//   INIT_UV_PROTOTYPE(rptLengthUnitValue,    deflection, pDisplayUnits->GetDeflectionUnit(),      true);
//
//   location.IncludeSpanAndGirder(true);

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      // put each interval on its own page
      *pPara << rptNewPage;

      // Heading
      pPara = new rptParagraph(pgsReportStyleHolder::GetHeadingStyle());
      *pChapter << pPara;

      CString str;
      str.Format(_T("Interval %d : %s"),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
      pPara->SetName(str);
      *pPara << pPara->GetName();

      pPara = new rptParagraph;
      *pChapter << pPara;

      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      // Interval Time Parameters
      Float64 start  = pIntervals->GetTime(intervalIdx,pgsTypes::Start);
      Float64 middle = pIntervals->GetTime(intervalIdx,pgsTypes::Middle);
      Float64 end    = pIntervals->GetTime(intervalIdx,pgsTypes::End);

      Float64 duration = pIntervals->GetDuration(intervalIdx);
      *pPara << _T("Start: ") << start << _T(" day, Middle: ") << middle << _T(" day, End: ") << end << _T(" day, Duration: ") << duration << _T(" day") << rptNewLine;

      *pPara << rptNewLine;

      // Properties
      *pPara << _T("Net Properties of Section Components") << rptNewLine;

      *pPara << _T("Girder: An = ") << area.SetValue(tsDetails.Girder.An) << _T(" ")
             << _T("In = ") << momI.SetValue(tsDetails.Girder.In) << _T(" ")
             << _T("Yn = ") << ecc.SetValue(tsDetails.Girder.Yn)  << _T(" ")
             << _T("H = ") << height.SetValue(tsDetails.Girder.H) << _T(" ") << rptNewLine;
      
      *pPara << rptNewLine;

      *pPara << _T("Deck: An = ") << area.SetValue(tsDetails.Deck.An) << _T(" ")
             << _T("In = ") << momI.SetValue(tsDetails.Deck.In) << _T(" ")
             << _T("Yn = ") << ecc.SetValue(tsDetails.Deck.Yn)  << _T(" ")
             << _T("H = ") << height.SetValue(tsDetails.Deck.H) << _T(" ") << rptNewLine;

      *pPara << rptNewLine;

      BOOST_FOREACH(const TIME_STEP_REBAR& tsRebar,tsDetails.GirderRebar)
      {
         *pPara << _T("Girder Rebar: As = ") << area.SetValue(tsRebar.As) << _T(" ")
            << _T("Ys = ") << ecc.SetValue(tsRebar.Ys) << rptNewLine;
      }

      *pPara << rptNewLine;

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
         if ( matType == pgsTypes::drmTop )
         {
            *pPara << _T("Deck Top Mat Rebar: Es = ");
         }
         else
         {
            *pPara << _T("Deck Bottom Mat Rebar: Es = ");
         }

         *pPara << _T("As = ") << area.SetValue(tsDetails.DeckRebar[matType].As) << _T(" ")
                << _T("Ys = ") << ecc.SetValue(tsDetails.DeckRebar[matType].Ys) << rptNewLine;
      }

      *pPara << rptNewLine;

      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         if ( strandType == pgsTypes::Straight )
         {
            *pPara << _T("Straight Strands: Eps = ");
         }
         else if ( strandType == pgsTypes::Harped )
         {
            *pPara << _T("Harped Strands: Eps = ");
         }
         else if ( strandType == pgsTypes::Temporary )
         {
            *pPara << _T("Temporary Strands: Eps = ");
         }

         *pPara << _T("As = ") << area.SetValue(tsDetails.Strands[strandType].As) << _T(" ")
                << _T("Ys = ") << area.SetValue(tsDetails.Strands[strandType].Ys) << rptNewLine;
      }

      *pPara << rptNewLine;
   
      BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
      {
         *pPara << _T("Tendon: As = ") << area.SetValue(tsTendon.As) << _T(" ")
            << _T("Ys = ") << ecc.SetValue(tsTendon.Ys) << rptNewLine;
      }

      *pPara << rptNewLine;

      *pPara << _T("Transformed Composite Properties: Atr = ") << area.SetValue(tsDetails.Atr) << _T(" ")
         << _T("Itr = ") << momI.SetValue(tsDetails.Itr) << _T(" ")
         << _T("Ytr = ") << ecc.SetValue(tsDetails.Ytr) << rptNewLine;

      *pPara << rptNewLine;

      // Incremental Strains (Tadros Eqn 3 & 4)

      *pPara << _T("Incremental Strain and Curvature Due to Loads Applied During this Interval") << rptNewLine;

      ColumnIndexType nColumns = 0;
      nColumns++; // Load Name
      nColumns += 2; // P & M for Composite
      nColumns += 2; // P & M for Girder
      nColumns += 3; // Straight, Harped, Temporary
      nColumns += tsDetails.Tendons.size();
      nColumns += tsDetails.GirderRebar.size();
      nColumns += 2; // P & M for Deck
      nColumns += 2; // top bottom deck rebar

      rptRcTable* pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(nColumns);
      *pPara << pLayoutTable << rptNewLine;
      ColumnIndexType colIdx = 0;
      RowIndexType rowIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Load");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Composite");
      colIdx++;//(*pLayoutTable)(rowIdx,colIdx++) << SKIP
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Girder");
      colIdx++;//(*pLayoutTable)(rowIdx,colIdx++) << SKIP

      (*pLayoutTable)(rowIdx,colIdx++) << _T("Straight");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Harped");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Temporary");

      BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
      {
         (*pLayoutTable)(rowIdx,colIdx++) << _T("Tendon");
      }

      BOOST_FOREACH(const TIME_STEP_REBAR& tsRebar,tsDetails.GirderRebar)
      {
         (*pLayoutTable)(rowIdx,colIdx++) << _T("Girder Rebar");
      }

      (*pLayoutTable)(rowIdx,colIdx++) << _T("Deck");
      colIdx++;//(*pLayoutTable)(rowIdx,colIdx++) << SKIP
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Top Deck Rebar");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Top Deck Rebar");
      rowIdx++;

      colIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dM");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP*(Eg*An)/(Eg*Atr)");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dM*(Eg*In)/(Eg*Itr)");

      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP*(Eps*Aps)/(Eg*Atr)");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP*(Eps*Aps)/(Eg*Atr)");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP*(Eps*Aps)/(Eg*Atr)");

      BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
      {
         (*pLayoutTable)(rowIdx,colIdx++) << _T("dP*(Ept*Apt)/(Eg*Atr)");
      }

      BOOST_FOREACH(const TIME_STEP_REBAR& tsRebar,tsDetails.GirderRebar)
      {
         (*pLayoutTable)(rowIdx,colIdx++) << _T("dP*(E*A)/(Eg*Atr)");
      }

      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP*(Ed*An)/(Eg*Atr)");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dM*(Ed*In)/(Eg*Itr)");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP*(E*A)/(Eg*Atr)");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP*(E*A)/(Eg*Atr)");
      rowIdx++;

      int nLoads = sizeof(tsDetails.Girder.dPi)/sizeof(tsDetails.Girder.dPi[0]);
      //nLoads -= 3; // don't include creep, shrinkage, or relaxation
      Float64 dP = 0;
      Float64 dM = 0;
      for ( int i = 0; i < nLoads; i++, rowIdx++ )
      {
         colIdx = 0;
         ProductForceType pfType = (ProductForceType)i;
         (*pLayoutTable)(rowIdx,colIdx++) << pProductLoads->GetProductLoadName(pfType);
         (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.dPi[pfType]);
         (*pLayoutTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.dMi[pfType]);
         (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.dPi[pfType]);
         (*pLayoutTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Girder.dMi[pfType]);

         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
            (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Strands[strandType].dPi[pfType]);
         }

         BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
         {
            (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsTendon.dPi[pfType]);
         }

         BOOST_FOREACH(const TIME_STEP_REBAR& tsRebar,tsDetails.GirderRebar)
         {
            (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsRebar.dPi[pfType]);
         }

         (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.dPi[pfType]);
         (*pLayoutTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Deck.dMi[pfType]);

         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
            (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.DeckRebar[matType].dPi[pfType]);
         }


         dP += tsDetails.dPi[pfType];
         dM += tsDetails.dMi[pfType];
      }

      colIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Total");
      (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(dP);
      (*pLayoutTable)(rowIdx,colIdx++) << moment.SetValue(dM);
      (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.dP);
      (*pLayoutTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Girder.dM);

      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Strands[strandType].dP);
      }

      BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
      {
         (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsTendon.dP);
      }

      BOOST_FOREACH(const TIME_STEP_REBAR& tsRebar,tsDetails.GirderRebar)
      {
         (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsRebar.dP);
      }

      (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.dP);
      (*pLayoutTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Deck.dM);

      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
         (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.DeckRebar[matType].dP);
      }

      rowIdx++;

      colIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP/(Eg*Atr) = (") << force.SetValue(dP) << _T(")/[(") << modE.SetValue(tsDetails.E) << _T(")(") << area.SetValue(tsDetails.Atr) << _T(")] = ") << (IsZero(tsDetails.E*tsDetails.Atr) ? 0 : dP/(tsDetails.E*tsDetails.Atr));
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dM/(Eg*Itr) = (") << moment.SetValue(dM) << _T(")/[(") << modE.SetValue(tsDetails.E) << _T(")(") << momI.SetValue(tsDetails.Itr) << _T(")] = ") << curvature.SetValue(IsZero(tsDetails.E*tsDetails.Itr) ? 0 : dM/(tsDetails.E*tsDetails.Itr));

      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP/(Eg*An) = (") << force.SetValue(tsDetails.Girder.dP) << _T(")/[(") << modE.SetValue(tsDetails.Girder.E) << _T(")(") << area.SetValue(tsDetails.Girder.An) << _T(")] = ") << tsDetails.Girder.e;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dM/(Eg*In) = (") << moment.SetValue(tsDetails.Girder.dM) << _T(")/[(") << modE.SetValue(tsDetails.Girder.E) << _T(")(") << momI.SetValue(tsDetails.Girder.In) << _T(")] = ") << curvature.SetValue(tsDetails.Girder.r);
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;

         Float64 dP = tsDetails.Strands[strandType].dP;
         Float64 E  = tsDetails.Strands[strandType].E;
         Float64 As = tsDetails.Strands[strandType].As;
         Float64 e  = tsDetails.Strands[strandType].e;
         (*pLayoutTable)(rowIdx,colIdx++) << _T("dP/(Eps*Aps) = (") << force.SetValue(dP) << _T(")/[(") << modE.SetValue(E) << _T(")(") << area.SetValue(As) << _T(")] = ") << e;
      }

      BOOST_FOREACH(const TIME_STEP_STRAND& tsTendon,tsDetails.Tendons)
      {
         Float64 dP = tsTendon.dP;
         Float64 E  = tsTendon.E;
         Float64 As = tsTendon.As;
         Float64 e  = tsTendon.e;
         (*pLayoutTable)(rowIdx,colIdx++) << _T("dP/(Ept*Aps) = (") << force.SetValue(dP) << _T(")/[(") << modE.SetValue(E) << _T(")(") << area.SetValue(As) << _T(")] = ") << e;
      }

      BOOST_FOREACH(const TIME_STEP_REBAR& tsRebar,tsDetails.GirderRebar)
      {
         (*pLayoutTable)(rowIdx,colIdx++) << _T("dP/(Eg*An) = (") << force.SetValue(tsRebar.dP) << _T(")/[(") << modE.SetValue(tsRebar.E) << _T(")(") << area.SetValue(tsRebar.As) << _T(")] = ") << tsRebar.e;
      }

      (*pLayoutTable)(rowIdx,colIdx++) << _T("dP/(Eg*An) = (") << force.SetValue(tsDetails.Deck.dP) << _T(")/[(") << modE.SetValue(tsDetails.Deck.E) << _T(")(") << area.SetValue(tsDetails.Deck.An) << _T(")] = ") << tsDetails.Deck.e;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("dM/(Eg*In) = (") << moment.SetValue(tsDetails.Deck.dM) << _T(")/[(") << modE.SetValue(tsDetails.Deck.E) << _T(")(") << momI.SetValue(tsDetails.Deck.In) << _T(")] = ") << curvature.SetValue(tsDetails.Deck.r);
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::DeckRebarMatType matType = (pgsTypes::DeckRebarMatType)i;
         (*pLayoutTable)(rowIdx,colIdx++) << _T("dP/(Eg*An) = (") << force.SetValue(tsDetails.DeckRebar[matType].dP) << _T(")/[(") << modE.SetValue(tsDetails.DeckRebar[matType].E) << _T(")(") << area.SetValue(tsDetails.DeckRebar[matType].As) << _T(")] = ") << tsDetails.DeckRebar[matType].e;
      }

      rowIdx++;


      *pPara << _T("Incremental Strain and Curvature Due to Loads Applied in Previous Intervals") << rptNewLine;

      pLayoutTable = pgsReportStyleHolder::CreateLayoutTable(5);
      *pPara << pLayoutTable << rptNewLine;

      rowIdx = 0;
      colIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Interval");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Girder");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Deck");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("");
      rowIdx++;

      colIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("e");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("r");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("e");
      (*pLayoutTable)(rowIdx,colIdx++) << _T("r");
      rowIdx++;

      std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator girder_strain_iter(tsDetails.Girder.ec.begin());
      std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator girder_strain_iter_end(tsDetails.Girder.ec.end());
      std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator girder_curvature_iter(tsDetails.Girder.rc.begin());
      std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator girder_curvature_iter_end(tsDetails.Girder.rc.end());

      std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator deck_strain_iter(tsDetails.Deck.ec.begin());
      std::vector<TIME_STEP_CONCRETE::CREEP_STRAIN>::const_iterator deck_strain_iter_end(tsDetails.Deck.ec.end());
      std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator deck_curvature_iter(tsDetails.Deck.rc.begin());
      std::vector<TIME_STEP_CONCRETE::CREEP_CURVATURE>::const_iterator deck_curvature_iter_end(tsDetails.Deck.rc.end());

      IntervalIndexType j = 0;
      for ( ; girder_strain_iter != girder_strain_iter_end; girder_strain_iter++, girder_curvature_iter++, deck_strain_iter++, deck_curvature_iter++, j++, rowIdx++ )
      {
         colIdx = 0;
         const TIME_STEP_CONCRETE::CREEP_STRAIN& girder_creep_strain(*girder_strain_iter);
         const TIME_STEP_CONCRETE::CREEP_CURVATURE& girder_creep_curvature(*girder_curvature_iter);

         const TIME_STEP_CONCRETE::CREEP_STRAIN& deck_creep_strain(*deck_strain_iter);
         const TIME_STEP_CONCRETE::CREEP_CURVATURE& deck_creep_curvature(*deck_curvature_iter);

         (*pLayoutTable)(rowIdx,colIdx++) << LABEL_INTERVAL(j) << rptNewLine;
         (*pLayoutTable)(rowIdx,colIdx++) << _T("(") << force.SetValue(girder_creep_strain.P) << _T("/(") << area.SetValue(girder_creep_strain.A) << _T(" * ") << modE.SetValue(girder_creep_strain.E) << _T("))*(") << girder_creep_strain.Ce << _T(" - ") << girder_creep_strain.Cs << _T(")") << rptNewLine;
         (*pLayoutTable)(rowIdx,colIdx++) << _T("(") << moment.SetValue(girder_creep_curvature.M) << _T("/(") << momI.SetValue(girder_creep_curvature.I) << _T(" * ") << modE.SetValue(girder_creep_curvature.E) << _T("))*(") << girder_creep_curvature.Ce << _T(" - ") << girder_creep_curvature.Cs << _T(")") << rptNewLine;
         (*pLayoutTable)(rowIdx,colIdx++) << _T("(") << force.SetValue(deck_creep_strain.P) << _T("/(") << area.SetValue(deck_creep_strain.A) << _T(" * ") << modE.SetValue(deck_creep_strain.E) << _T("))*(") << deck_creep_strain.Ce << _T(" - ") << deck_creep_strain.Cs << _T(")") << rptNewLine;
         (*pLayoutTable)(rowIdx,colIdx++) << _T("(") << moment.SetValue(deck_creep_curvature.M) << _T("/(") << momI.SetValue(deck_creep_curvature.I) << _T(" * ") << modE.SetValue(deck_creep_curvature.E) << _T("))*(") << deck_creep_curvature.Ce << _T(" - ") << deck_creep_curvature.Cs << _T(")") << rptNewLine;
      }

      colIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Creep Strain");
      (*pLayoutTable)(rowIdx,colIdx++) << _T(" = ") << tsDetails.Girder.eci;
      (*pLayoutTable)(rowIdx,colIdx++) << _T(" = ") << tsDetails.Girder.rci;
      (*pLayoutTable)(rowIdx,colIdx++) << _T(" = ") << tsDetails.Deck.eci;
      (*pLayoutTable)(rowIdx,colIdx++) << _T(" = ") << tsDetails.Deck.rci;
      rowIdx++;

      colIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Creep");
      (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.PrCreep);
      (*pLayoutTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Girder.MrCreep);
      (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.PrCreep);
      (*pLayoutTable)(rowIdx,colIdx++) << moment.SetValue(tsDetails.Deck.MrCreep);
      rowIdx++;

      colIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Shrinkage Strain");
      (*pLayoutTable)(rowIdx,colIdx++) << tsDetails.Girder.esi;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("");
      (*pLayoutTable)(rowIdx,colIdx++) << tsDetails.Deck.esi;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("");
      rowIdx++;

      colIdx = 0;
      (*pLayoutTable)(rowIdx,colIdx++) << _T("Shrinkage");
      (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Girder.PrShrinkage);
      (*pLayoutTable)(rowIdx,colIdx++) << _T("");
      (*pLayoutTable)(rowIdx,colIdx++) << force.SetValue(tsDetails.Deck.PrShrinkage);
      (*pLayoutTable)(rowIdx,colIdx++) << _T("");
      rowIdx++;

      // Relaxation
      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         if ( strandType == pgsTypes::Straight )
         {
            *pPara << _T("Straight Strand") << rptNewLine;
         }
         else if (strandType == pgsTypes::Harped)
         {
            *pPara << _T("Harped Strand") << rptNewLine;
         }
         else if (strandType == pgsTypes::Temporary)
         {
            *pPara << _T("Temporary Strand") << rptNewLine;
         }

         *pPara << _T("fr = ") << stress.SetValue(tsDetails.Strands[strandType].fr) << rptNewLine;
         *pPara << _T("e = fr/Eps = ") << stress.SetValue(tsDetails.Strands[strandType].fr) << _T("/") << modE.SetValue(tsDetails.Strands[strandType].E) << _T(" = ") << tsDetails.Strands[strandType].er << rptNewLine;
         *pPara << _T("Pr = ") << force.SetValue(tsDetails.Strands[strandType].PrRelaxation) << rptNewLine;
         *pPara << rptNewLine;
      }

      // Restraining forces
      *pPara << _T("Total Artificial Restraining Forces") << rptNewLine;
      *pPara << _T("Creep: N = ") << force.SetValue(tsDetails.Pr[TIMESTEP_CR]) << _T(", M = ") << moment.SetValue(tsDetails.Mr[TIMESTEP_CR]) << rptNewLine;
      *pPara << _T("Shrinkage: N = ") << force.SetValue(tsDetails.Pr[TIMESTEP_SH]) << _T(", M = ") << moment.SetValue(tsDetails.Mr[TIMESTEP_SH]) << rptNewLine;
      *pPara << _T("Relaxation: N = ") << force.SetValue(tsDetails.Pr[TIMESTEP_RE]) << _T(", M = ") << moment.SetValue(tsDetails.Mr[TIMESTEP_RE]) << rptNewLine;
      
      *pPara << rptNewLine;

      // Distribution of internal forces to each element of the cross section

   } // next interval

   return pChapter;
}

CChapterBuilder* CTimeStepDetailsChapterBuilder::Clone() const
{
   return new CTimeStepDetailsChapterBuilder;
}
