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

/****************************************************************************
CLASS
   CSpecCheckSummaryChapterBuilder
****************************************************************************/

#include <Reporting\SpecCheckSummaryChapterBuilder.h>

#include <PgsExt\GirderPointOfInterest.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\GirderArtifactTool.h>
#include <PgsExt\BridgeDescription2.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Artifact.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <EAF\EAFAutoProgress.h>
#include <IFace\DocumentType.h>

#include <LRFD\VersionMgr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CSpecCheckSummaryChapterBuilder::CSpecCheckSummaryChapterBuilder(bool referToDetailsReport,bool bSelect):
CPGSuperChapterBuilder(bSelect),
m_ReferToDetailsReport(referToDetailsReport)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSpecCheckSummaryChapterBuilder::GetName() const
{
   return TEXT("Specification Check Summary");
}

rptChapter* CSpecCheckSummaryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   // Report for a single girder
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   if (pGirderRptSpec != NULL)
   {
      rptChapter* pChapter = CPGSuperChapterBuilder::Build(pGirderRptSpec,level);

      CComPtr<IBroker> pBroker;
      pGirderRptSpec->GetBroker(&pBroker);
      const CGirderKey& girderKey( pGirderRptSpec->GetGirderKey() );

      GET_IFACE2(pBroker,IArtifact,pIArtifact);
      const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

      CreateContent(pChapter, pBroker, pGirderArtifact);

      return pChapter;
   }

   // Report multiple girders
   CMultiGirderReportSpecification* pMultiGirderRptSpec = dynamic_cast<CMultiGirderReportSpecification*>(pRptSpec);
   if (pMultiGirderRptSpec != NULL)
   {
      const std::vector<CGirderKey>& girderKeys( pMultiGirderRptSpec->GetGirderKeys() );

      // Give progress window a progress meter
      bool bMultiGirderReport = (1 < girderKeys.size() ? true : false);

      CComPtr<IBroker> pBroker;
      pMultiGirderRptSpec->GetBroker(&pBroker);

      GET_IFACE2(pBroker,IProgress,pProgress);
      DWORD mask = bMultiGirderReport ? PW_ALL|PW_NOCANCEL : PW_ALL|PW_NOGAUGE|PW_NOCANCEL;

      CEAFAutoProgress ap(pProgress,0,mask); 

      if (bMultiGirderReport)
         pProgress->Init(0,(short)girderKeys.size(),1);  // and for multi-girders, a gauge.

      // Build chapter and fill it
      rptChapter* pChapter = CPGSuperChapterBuilder::Build(pMultiGirderRptSpec,level);

      GET_IFACE2(pBroker,IArtifact,pIArtifact);
      GET_IFACE2(pBroker,IBridge,pBridge);
      GET_IFACE2(pBroker,IDocumentType,pDocType);

      for (std::vector<CGirderKey>::const_iterator it=girderKeys.begin(); it!=girderKeys.end(); it++)
      {
         const CGirderKey& girderKey(*it);

         const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

         rptParagraph* pParagraph = new rptParagraph( pgsReportStyleHolder::GetHeadingStyle() );
         *pChapter << pParagraph;

         if ( pDocType->IsPGSuperDocument() )
            *pParagraph << _T("Results for Span ") << LABEL_SPAN(girderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(girderKey.girderIndex);
         else
            *pParagraph << _T("Results for Group ") << LABEL_GROUP(girderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(girderKey.girderIndex);

         CreateContent(pChapter, pBroker, pGirderArtifact);

         if (bMultiGirderReport)
            pProgress->Increment();
      }

      return pChapter;
   }


   ATLASSERT(0);
   return NULL;
}

rptChapter* CSpecCheckSummaryChapterBuilder::BuildEx(CReportSpecification* pRptSpec,Uint16 level,
                    const pgsGirderArtifact* pGirderArtifact) const
{
   CGirderReportSpecification* pGirderRptSpec = dynamic_cast<CGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pGirderRptSpec->GetBroker(&pBroker);
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);
   CreateContent(pChapter, pBroker, pGirderArtifact);

   return pChapter;
}

void CSpecCheckSummaryChapterBuilder::CreateContent(rptChapter* pChapter, IBroker* pBroker,
                                                    const pgsGirderArtifact* pGirderArtifact) const
{
   rptParagraph* pPara = new rptParagraph;
   *pChapter << pPara;

   CGirderKey girderKey(pGirderArtifact->GetGirderKey());

   if( pGirderArtifact->Passed() )
   {
      *pPara << color(Green)<< _T("The Specification Check was Successful") << color(Black) << rptNewLine;
   }
   else
   {
      const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

      *pPara << color(Red) << _T("The Specification Check Was Not Successful") << color(Black) << rptNewLine;
     
      GET_IFACE2(pBroker,ILimitStateForces,pLimitStateForces);
      bool bPermit = pLimitStateForces->IsStrengthIIApplicable(girderKey);

      // Build a list of our failures
      FailureList failures;

      // Allowable stress checks
      ListStressFailures(pBroker,failures,pGirderArtifact,m_ReferToDetailsReport);

      // Moment Capacity Checks
      ListMomentCapacityFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthI);
      if ( bPermit )
         ListMomentCapacityFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthII);

      //Stirrup Checks
      ListVerticalShearFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthI);
      if ( bPermit )
         ListVerticalShearFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthII);

      ListHorizontalShearFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthI);
      if ( bPermit )
         ListHorizontalShearFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthII);

      ListStirrupDetailingFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthI);
      if ( bPermit )
         ListStirrupDetailingFailures(pBroker,failures,pGirderArtifact,pgsTypes::StrengthII);

      ListDebondingFailures(pBroker,failures,pGirderArtifact);
      ListSplittingZoneFailures(pBroker,failures,pGirderArtifact);
      ListConfinementZoneFailures(pBroker,failures,pGirderArtifact);
      ListVariousFailures(pBroker,failures,pGirderArtifact,m_ReferToDetailsReport);

      // Put failures into report
      for (FailureListIterator it=failures.begin(); it!=failures.end(); it++)
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         *pPara << *it << rptNewLine;
      }
   }

   // Stirrup lengths is not technically a spec check, but a warning
   bool bCheckStirrups = false;
   GET_IFACE2(pBroker,IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const pgsConstructabilityArtifact* pConstrArtifact = pSegmentArtifact->GetConstructabilityArtifact();
      if ( pConstrArtifact->CheckStirrupLength() )
      {
         bCheckStirrups = true;
      }
   }
   if ( bCheckStirrups )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << color(Red) << Bold(_T("WARNING: The length of stirrups that engage the bridge deck may need special attention. Refer to the Specification Check Details for more information.")) << color(Black) << rptNewLine;
   }

   GET_IFACE2(pBroker,IStirrupGeometry,pStirrupGeom);
   if ( !pStirrupGeom->AreStirrupZoneLengthsCombatible(girderKey) )
   {
      rptParagraph* pPara = new rptParagraph;
      *pChapter << pPara;
      *pPara << color(Red) << Bold(_T("WARNING: Stirrup zone lengths are not compatible with stirrup spacings. Refer to the Stirrup Layout Geometry Check for more information.")) << color(Black) << rptNewLine;
   }

#pragma Reminder("UPDATE: this needs to be completed")

   //// Negative camber is not technically a spec check, but a warning
   //GET_IFACE2(pBroker, IPointOfInterest, pPointOfInterest );
   //std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(span, gdr ,pgsTypes::BridgeSite1, POI_MIDSPAN);
   //ATLASSERT(pmid.size()==1);
   //pgsPointOfInterest poiMidSpan(pmid.front());


   //GET_IFACE2(pBroker,ICamber,pCamber);
   //GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   //const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   //Float64 C = 0;
   //if ( pBridgeDesc->GetDeckDescription()->DeckType != pgsTypes::sdtNone )
   //{
   //   C = pCamber->GetScreedCamber( poiMidSpan ) ;
   //}

   //Float64 D = 999;
   //if ( pBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
   //{
   //   D = pCamber->GetDCamberForGirderSchedule( poiMidSpan, CREEP_MAXTIME);
   //}
   //else
   //{
   //   D = 0.5*pCamber->GetDCamberForGirderSchedule( poiMidSpan, CREEP_MINTIME);
   //}

   //if ( D < C )
   //{
   //   pPara = new rptParagraph;
   //   *pChapter << pPara;

   //   *pPara << color(Red) << Bold(_T("WARNING: Screed Camber is greater than the camber at time of deck casting. The girder may end up with a sag.")) << color(Black) << rptNewLine;
   //}
   //else if ( IsEqual(C,D,::ConvertToSysUnits(0.25,unitMeasure::Inch)) )
   //{
   //   pPara = new rptParagraph;
   //   *pChapter << pPara;

   //   *pPara << color(Red) << Bold(_T("WARNING: Screed Camber is nearly equal to the camber at time of deck casting. The girder may end up with a sag.")) << color(Black) << rptNewLine;
   //}

   //Float64 excess_camber = pCamber->GetExcessCamber(poiMidSpan,CREEP_MAXTIME);
   //if ( excess_camber < 0.0 )
   //{
   //   rptParagraph* pPara = new rptParagraph;
   //   *pChapter << pPara;
   //   *pPara << color(Red) << Bold(_T("WARNING:  Excess camber is negative, indicating a potential sag in the beam. Refer to the Details Report for more information.")) << color(Black) << rptNewLine;
   //}
}

CChapterBuilder* CSpecCheckSummaryChapterBuilder::Clone() const
{
   return new CSpecCheckSummaryChapterBuilder(m_ReferToDetailsReport);
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
