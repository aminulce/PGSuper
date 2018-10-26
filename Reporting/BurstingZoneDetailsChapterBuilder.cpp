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
#include <Reporting\BurstingZoneDetailsChapterBuilder.h>
#include <Reporting\ReportStyleHolder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include <PgsExt\GirderArtifact.h>

#include <PsgLib\SpecLibraryEntry.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CSplittingZoneDetailsChapterBuilder
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CSplittingZoneDetailsChapterBuilder::CSplittingZoneDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CSplittingZoneDetailsChapterBuilder::GetKey() const
{
   return TEXT("Splitting Resistance Details");
}

LPCTSTR CSplittingZoneDetailsChapterBuilder::GetName() const
{
   if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
      return TEXT("Splitting Resistance Details");
   else
      return TEXT("Bursting Resistance Details");
}

rptChapter* CSplittingZoneDetailsChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   CSpanGirderReportSpecification* pSGRptSpec = dynamic_cast<CSpanGirderReportSpecification*>(pRptSpec);
   CComPtr<IBroker> pBroker;
   pSGRptSpec->GetBroker(&pBroker);
   SpanIndexType span = pSGRptSpec->GetSpan();
   GirderIndexType girder = pSGRptSpec->GetGirder();

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   bool bInitialRelaxation = ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 || 
                               pSpecEntry->GetLossMethod() == LOSSES_WSDOT_REFINED                    ||
                               pSpecEntry->GetLossMethod() == LOSSES_TXDOT_REFINED_2004               ||
                               pSpecEntry->GetLossMethod() == LOSSES_WSDOT_LUMPSUM ? true : false );

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,girder);
   const pgsSplittingZoneArtifact* pArtifact = gdrArtifact->GetSplittingZoneArtifact();

   INIT_UV_PROTOTYPE( rptLengthUnitValue,    length, pDisplayUnits->GetSpanLengthUnit(),   true );
   INIT_UV_PROTOTYPE( rptStressUnitValue,    stress, pDisplayUnits->GetStressUnit(),       true );
   INIT_UV_PROTOTYPE( rptAreaUnitValue,      area,   pDisplayUnits->GetAreaUnit(),         true );
   INIT_UV_PROTOTYPE( rptForceUnitValue,     force,  pDisplayUnits->GetGeneralForceUnit(), true );

   rptRcScalar scalar;
   scalar.SetFormat( sysNumericFormatTool::Automatic );
   scalar.SetWidth(4);
   scalar.SetPrecision(1);

   rptParagraph* pPara;

   pPara = new rptParagraph;
   *pChapter << pPara;

   std::_tstring strName;
   if ( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() )
      strName = _T("Splitting");
   else
      strName = _T("Bursting");

   if (!pArtifact->GetIsApplicable())
   {
      (*pPara) << _T("Check for ")<<strName<<_T(" resistance (LRFD 5.10.10.1) is disabled in Project Criteria library.") << rptNewLine;
   }
   else
   {
      (*pPara) << _T("LRFD 5.10.10.1") << rptNewLine;
      (*pPara) << strName << _T(" Dimension: h = ") << length.SetValue(pArtifact->GetH()) << rptNewLine;
      (*pPara) << strName << _T(" Length: h/") << scalar.SetValue(pArtifact->GetSplittingZoneLengthFactor()) << _T(" = ") << length.SetValue(pArtifact->GetSplittingZoneLength()) << rptNewLine;
      (*pPara) << strName << _T(" Direction: ") << (pArtifact->GetSplittingDirection() == pgsTypes::sdVertical ? _T("Vertical") : _T("Horizontal")) << rptNewLine;
      (*pPara) << strName << _T(" Force: P = 0.04(A") << Sub(_T("ps")) << _T(")(") << RPT_FPJ << _T(" - ") ;
      
      if ( bInitialRelaxation )
      {
         if ( pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 )
            (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR1")) << _T(" - ");
         else
            (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pR0")) << _T(" - ");
      }
      
      (*pPara) << symbol(DELTA) << RPT_STRESS(_T("pES"))  << _T(") = ");
      (*pPara) << _T("0.04(") << area.SetValue(pArtifact->GetAps()) << _T(")(") << stress.SetValue(pArtifact->GetFpj()) << _T(" - ");
      (*pPara) << stress.SetValue(pArtifact->GetLossesAfterTransfer()) << _T(" ) ");

      (*pPara) << _T(" = ") << force.SetValue(pArtifact->GetSplittingForce()) << rptNewLine;
      (*pPara) << strName << _T(" Resistance: P") << Sub(_T("r")) << _T(" = ")
               << RPT_STRESS(_T("s")) << Sub2(_T("A"),_T("s")) << _T(" = ")
               << _T("(") << stress.SetValue(pArtifact->GetFs()) << _T(")(") << area.SetValue(pArtifact->GetAvs()) << _T(") = ")
               << force.SetValue(pArtifact->GetSplittingResistance()) << rptNewLine;
   }

   return pChapter;
}

CChapterBuilder* CSplittingZoneDetailsChapterBuilder::Clone() const
{
   return new CSplittingZoneDetailsChapterBuilder;
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
