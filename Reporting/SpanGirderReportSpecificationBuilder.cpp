///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2016  Washington State Department of Transportation
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

#include "stdafx.h"
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\SpanGirderReportDlg.h>
#include <Reporting\MultiGirderReportDlg.h>
#include "MultiViewReportDlg.h"

#include <IFace\Selection.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSpanReportSpecificationBuilder::CSpanReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CSpanReportSpecificationBuilder::~CSpanReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CSpanReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span and chapter list
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,SpanAndChapters,pRptSpec); // span only mode
   dlg.m_Span = spanIdx;

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CSpanReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.m_Span) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CSpanReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   boost::shared_ptr<CReportSpecification> pRptSpec( new CSpanReportSpecification(rptDesc.GetReportName(),m_pBroker,spanIdx) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CGirderReportSpecificationBuilder::CGirderReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CGirderReportSpecificationBuilder::~CGirderReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span and chapter list
   GET_IFACE(ISelection,pSelection);
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   gdrIdx = (gdrIdx == INVALID_INDEX ? 0 : gdrIdx );

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,GirderAndChapters,pRptSpec); // girder only mode
   dlg.m_Girder = gdrIdx;

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.m_Girder) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   ATLASSERT( gdrIdx != INVALID_INDEX );

   gdrIdx = (gdrIdx == INVALID_INDEX ? 0 : gdrIdx );
   boost::shared_ptr<CReportSpecification> pRptSpec( new CGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,gdrIdx) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CSpanGirderReportSpecificationBuilder::CSpanGirderReportSpecificationBuilder(IBroker* pBroker) :
CSpanReportSpecificationBuilder(pBroker)
{
}

CSpanGirderReportSpecificationBuilder::~CSpanGirderReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CSpanGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );

   CSpanGirderReportDlg dlg(m_pBroker,rptDesc,SpanGirderAndChapters,pRptSpec);
   dlg.m_Span = spanIdx;
   dlg.m_Girder = gdrIdx;

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CSpanGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,dlg.m_Span,dlg.m_Girder) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CSpanGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );
   boost::shared_ptr<CReportSpecification> pRptSpec( new CSpanGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,spanIdx,gdrIdx) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CMultiGirderReportSpecificationBuilder::CMultiGirderReportSpecificationBuilder(IBroker* pBroker) :
CBrokerReportSpecificationBuilder(pBroker)
{
}

CMultiGirderReportSpecificationBuilder::~CMultiGirderReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CMultiGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Prompt for span, girder, and chapter list
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );

   CMultiGirderReportDlg dlg(m_pBroker,rptDesc,pRptSpec);

   SpanGirderHashType hash = HashSpanGirder(spanIdx,gdrIdx);
   std::vector<SpanGirderHashType> gdrlist;
   gdrlist.push_back(hash);

   dlg.m_SelGdrs = gdrlist;

   if ( dlg.DoModal() == IDOK )
   {
      boost::shared_ptr<CReportSpecification> pRptSpec( new CMultiGirderReportSpecification(rptDesc.GetReportName(),m_pBroker, dlg.m_SelGdrs) );

      std::vector<std::_tstring> chList = dlg.m_ChapterList;
      rptDesc.ConfigureReportSpecification(chList,pRptSpec);

      return pRptSpec;
   }

   return boost::shared_ptr<CReportSpecification>();
}

boost::shared_ptr<CReportSpecification> CMultiGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );

   SpanGirderHashType hash = HashSpanGirder(spanIdx, gdrIdx);
   std::vector<SpanGirderHashType> gdrlist;
   gdrlist.push_back(hash);

   boost::shared_ptr<CReportSpecification> pRptSpec( new CMultiGirderReportSpecification(rptDesc.GetReportName(),m_pBroker, gdrlist) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

CMultiViewSpanGirderReportSpecificationBuilder::CMultiViewSpanGirderReportSpecificationBuilder(IBroker* pBroker) :
CSpanReportSpecificationBuilder(pBroker)
{
}

CMultiViewSpanGirderReportSpecificationBuilder::~CMultiViewSpanGirderReportSpecificationBuilder(void)
{
}

boost::shared_ptr<CReportSpecification> CMultiViewSpanGirderReportSpecificationBuilder::CreateReportSpec(const CReportDescription& rptDesc,boost::shared_ptr<CReportSpecification>& pRptSpec)
{
   // First check if we are getting a CSpanGirderReportSpecification. If so, use our bro to take care of this
   boost::shared_ptr<CSpanGirderReportSpecification> pSpanGirderRptSpec = boost::dynamic_pointer_cast<CSpanGirderReportSpecification,CReportSpecification>(pRptSpec);
   if ( pSpanGirderRptSpec != NULL )
   {
      boost::shared_ptr<CReportSpecificationBuilder> pSpanGirderRptSpecBuilder( new CSpanGirderReportSpecificationBuilder(m_pBroker) );
      return pSpanGirderRptSpecBuilder->CreateReportSpec(rptDesc, pRptSpec);
   }
   else
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      // Prompt for span, girder, and chapter list
      GET_IFACE(ISelection,pSelection);
      SpanIndexType spanIdx = pSelection->GetSpanIdx();
      GirderIndexType gdrIdx = pSelection->GetGirderIdx();

      spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
      gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );

      CMultiViewReportDlg dlg(m_pBroker,rptDesc,pRptSpec,spanIdx,gdrIdx);

      if ( dlg.DoModal() == IDOK )
      {
         std::vector<SpanGirderHashType> girderList = dlg.GetGirderList();

         boost::shared_ptr<CReportSpecification> pRptSpec( new CMultiViewSpanGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,girderList) );

         std::vector<std::_tstring> chList = dlg.m_ChapterList;
         rptDesc.ConfigureReportSpecification(chList,pRptSpec);

         return pRptSpec;
      }

      return boost::shared_ptr<CReportSpecification>();
   }
}

boost::shared_ptr<CReportSpecification> CMultiViewSpanGirderReportSpecificationBuilder::CreateDefaultReportSpec(const CReportDescription& rptDesc)
{
   // Get the selected span and girder
   GET_IFACE(ISelection,pSelection);
   SpanIndexType spanIdx = pSelection->GetSpanIdx();
   GirderIndexType gdrIdx = pSelection->GetGirderIdx();

   spanIdx = (spanIdx == INVALID_INDEX ? 0 : spanIdx );
   gdrIdx  = (gdrIdx  == INVALID_INDEX ? 0 : gdrIdx  );
   boost::shared_ptr<CReportSpecification> pRptSpec( new CSpanGirderReportSpecification(rptDesc.GetReportName(),m_pBroker,spanIdx,gdrIdx) );

   rptDesc.ConfigureReportSpecification(pRptSpec);

   return pRptSpec;
}

