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

// cfSplitChildFrame.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperDoc.h"
#include "BridgeModelViewChildFrame.h"
#include "BridgePlanView.h"
#include "BridgeSectionView.h"
#include "AlignmentPlanView.h"
#include "AlignmentProfileView.h"
#include "BridgeViewPrintJob.h"
#include "StationCutDlg.h"
#include "SelectItemDlg.h"
#include "MainFrm.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\EditByUI.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>
#include "EditBoundaryConditions.h"

#include "PGSuperAppPlugin\InsertSpanDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame

IMPLEMENT_DYNCREATE(CBridgeModelViewChildFrame, CSplitChildFrame)

CBridgeModelViewChildFrame::CBridgeModelViewChildFrame()
{
   m_bCutLocationInitialized = false;
   m_CurrentCutLocation = 0;
   m_bSelecting = false;
}

CBridgeModelViewChildFrame::~CBridgeModelViewChildFrame()
{
}

BOOL CBridgeModelViewChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
   // force this window to be maximized (not sure why WS_VISIBLE is required)
   cs.style |= WS_MAXIMIZE | WS_VISIBLE;

   return __super::PreCreateWindow(cs);
}

BOOL CBridgeModelViewChildFrame::Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle,
				const RECT& rect,
				CMDIFrameWnd* pParentWnd,
				CCreateContext* pContext)
{
   BOOL bResult = CSplitChildFrame::Create(lpszClassName,lpszWindowName,dwStyle,rect,pParentWnd,pContext);
   if ( bResult )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      HICON hIcon = AfxGetApp()->LoadIcon(IDR_BRIDGEMODELEDITOR);
      SetIcon(hIcon,TRUE);
   }

   return bResult;
}

BEGIN_MESSAGE_MAP(CBridgeModelViewChildFrame, CSplitChildFrame)
	//{{AFX_MSG_MAP(CBridgeModelViewChildFrame)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrintDirect)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_SPAN,   OnEditSpan)
	ON_COMMAND(ID_EDIT_PIER,   OnEditPier)
	ON_COMMAND(ID_VIEW_GIRDER, OnViewGirder)
	ON_COMMAND(ID_DELETE_PIER, OnDeletePier)
	ON_UPDATE_COMMAND_UI(ID_DELETE_PIER, OnUpdateDeletePier)
	ON_COMMAND(ID_DELETE_SPAN, OnDeleteSpan)
	ON_UPDATE_COMMAND_UI(ID_DELETE_SPAN, OnUpdateDeleteSpan)
	ON_COMMAND(ID_INSERT_SPAN, OnInsertSpan)
	ON_COMMAND(ID_INSERT_PIER, OnInsertPier)
	//}}AFX_MSG_MAP
   ON_COMMAND_RANGE(IDM_HINGE,IDM_INTEGRAL_SEGMENT_AT_PIER,OnBoundaryCondition)
   ON_UPDATE_COMMAND_UI_RANGE(IDM_HINGE,IDM_INTEGRAL_SEGMENT_AT_PIER,OnUpdateBoundaryCondition)
	ON_MESSAGE(WM_HELP, OnCommandHelp)
   ON_NOTIFY(UDN_DELTAPOS, IDC_START_SPAN_SPIN, &CBridgeModelViewChildFrame::OnStartSpanChanged)
   ON_NOTIFY(UDN_DELTAPOS, IDC_END_SPAN_SPIN, &CBridgeModelViewChildFrame::OnEndSpanChanged)
   ON_CONTROL_RANGE(BN_CLICKED,IDC_BRIDGE,IDC_ALIGNMENT,OnViewModeChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeModelViewChildFrame message handlers
CRuntimeClass* CBridgeModelViewChildFrame::GetLowerPaneClass() const
{
   return RUNTIME_CLASS(CBridgeSectionView);
}

Float64 CBridgeModelViewChildFrame::GetTopFrameFraction() const
{
   return 0.5;
}

void CBridgeModelViewChildFrame::OnFilePrint() 
{
   DoFilePrint(false);
}

void CBridgeModelViewChildFrame::OnFilePrintDirect() 
{
   DoFilePrint(true);
}

void CBridgeModelViewChildFrame::DoFilePrint(bool direct)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   
   // create a print job and do it
   CBridgeViewPrintJob pj(this,GetUpperView(), GetLowerView(), pBroker);
   pj.OnFilePrint(direct);
}

CBridgePlanView* CBridgeModelViewChildFrame::GetBridgePlanView() 
{
   CView* pView = GetUpperView();
   if ( pView->IsKindOf(RUNTIME_CLASS(CBridgePlanView)) )
   {
      return (CBridgePlanView*)pView;
   }
   return NULL;
}

CBridgeSectionView* CBridgeModelViewChildFrame::GetBridgeSectionView() 
{
   CView* pView = GetLowerView();
   if ( pView->IsKindOf(RUNTIME_CLASS(CBridgeSectionView)) )
   {
      return (CBridgeSectionView*)pView;
   }
   return NULL;
}

CAlignmentPlanView* CBridgeModelViewChildFrame::GetAlignmentPlanView()
{
   CView* pView = GetUpperView();
   if ( pView->IsKindOf(RUNTIME_CLASS(CAlignmentPlanView)) )
   {
      return (CAlignmentPlanView*)pView;
   }
   return NULL;
}

CAlignmentProfileView* CBridgeModelViewChildFrame::GetAlignmentProfileView()
{
   CView* pView = GetLowerView();
   if ( pView->IsKindOf(RUNTIME_CLASS(CAlignmentProfileView)) )
   {
      return (CAlignmentProfileView*)pView;
   }
   return NULL;
}

CBridgeViewPane* CBridgeModelViewChildFrame::GetUpperView()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane calls AssertValid, Must be in the application module state
   ASSERT_KINDOF(CBridgeViewPane,m_SplitterWnd.GetPane(0,0));
   return (CBridgeViewPane*)m_SplitterWnd.GetPane(0, 0);
}

CBridgeViewPane* CBridgeModelViewChildFrame::GetLowerView()
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane calls AssertValid, Must be in the application module state
   ASSERT_KINDOF(CBridgeViewPane,m_SplitterWnd.GetPane(1,0));
   return (CBridgeViewPane*)m_SplitterWnd.GetPane(1, 0);
}

void CBridgeModelViewChildFrame::InitSpanRange()
{
   // Can't get to the broker, and thus the bridge information in OnCreate, so we need a method
   // that can be called later to initalize the span range for viewing
   CSpinButtonCtrl* pStartSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_START_SPAN_SPIN);
   CSpinButtonCtrl* pEndSpinner   = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_END_SPAN_SPIN);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   CBridgePlanView* pPlanView = GetBridgePlanView();

   SpanIndexType startSpanIdx, endSpanIdx;
   pPlanView->GetSpanRange(&startSpanIdx,&endSpanIdx);

   startSpanIdx = (startSpanIdx == ALL_SPANS ? 0        : startSpanIdx);
   endSpanIdx   = (endSpanIdx   == ALL_SPANS ? nSpans-1 : endSpanIdx  );

   pStartSpinner->SetRange32(1,(int)nSpans);
   pEndSpinner->SetRange32((int)startSpanIdx+1,(int)nSpans);

   pStartSpinner->SetPos32((int)startSpanIdx+1);
   pEndSpinner->SetPos32((int)endSpanIdx+1);

   CString str;
   str.Format(_T("of %d Spans"),nSpans);
   m_SettingsBar.GetDlgItem(IDC_SPAN_COUNT)->SetWindowText(str);
}

#if defined _DEBUG
void CBridgeModelViewChildFrame::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());
   CSplitChildFrame::AssertValid();
}

void CBridgeModelViewChildFrame::Dump(CDumpContext& dc) const
{
   CSplitChildFrame::Dump(dc);
}
#endif 

int CBridgeModelViewChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSplitChildFrame::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	this->SetWindowText(_T("Bridge View"));

   {
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if ( !m_SettingsBar.Create( this, IDD_BRIDGEVIEW_CONTROLS, CBRS_TOP, IDD_BRIDGEVIEW_CONTROLS) )
	{
		TRACE0("Failed to create control bar\n");
		return -1;      // fail to create
	}
   }

   m_SettingsBar.CheckRadioButton(IDC_BRIDGE,IDC_ALIGNMENT,IDC_BRIDGE);


   return 0;
}

void CBridgeModelViewChildFrame::CutAt(Float64 X)
{
   UpdateCutLocation(X);
}

Float64 CBridgeModelViewChildFrame::GetNextCutStation(Float64 direction)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

   // if control key is down... move the section cut

   Float64 station = GetCurrentCutLocation();

   SpanIndexType spanIdx;
   if ( !pBridge->GetSpan(station,&spanIdx) )
   {
      return station;
   }

   Float64 back_pier = pBridge->GetPierStation(spanIdx);
   Float64 ahead_pier = pBridge->GetPierStation(spanIdx+1);
   Float64 span_length = ahead_pier - back_pier;
   Float64 inc = span_length/10;

   station = station + direction*inc;

   return station;
}

void CBridgeModelViewChildFrame::CutAtNext()
{
   CutAt(GetNextCutStation(1));
}

void CBridgeModelViewChildFrame::CutAtPrev()
{
   CutAt(GetNextCutStation(-1));
}

LPCTSTR CBridgeModelViewChildFrame::GetDeckTypeName(pgsTypes::SupportedDeckType deckType) const
{
   switch ( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
      return _T("Composite cast-in-place deck");

   case pgsTypes::sdtCompositeSIP: 
      return _T("Composite stay-in-place deck panels");

   case pgsTypes::sdtCompositeOverlay:
      return _T("Composite structural overlay");

   case pgsTypes::sdtNone:
      return _T("None");
   }

   return _T("");
}

void CBridgeModelViewChildFrame::ShowCutDlg()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();
   Float64 start = pBridge->GetPierStation(0);
   Float64 end   = pBridge->GetPierStation(nPiers-1);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   CStationCutDlg dlg(m_CurrentCutLocation,start,end,bUnitsSI);
   if ( dlg.DoModal() == IDOK )
   {
      UpdateCutLocation(dlg.GetValue());
   }
}

Float64 CBridgeModelViewChildFrame::GetMinCutLocation()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);

   return pBridge->GetPierStation(0);
}

Float64 CBridgeModelViewChildFrame::GetMaxCutLocation()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();
   return pBridge->GetPierStation(nPiers-1);
}

void CBridgeModelViewChildFrame::UpdateCutLocation(Float64 cut)
{
   m_CurrentCutLocation = cut;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

   SpanIndexType startSpanIdx, endSpanIdx;
   GetBridgePlanView()->GetSpanRange(&startSpanIdx,&endSpanIdx);

   PierIndexType startPierIdx = (PierIndexType)startSpanIdx;
   PierIndexType endPierIdx   = (PierIndexType)(endSpanIdx + 1);

   Float64 start = pBridge->GetPierStation(startPierIdx);
   Float64 end   = pBridge->GetPierStation(endPierIdx);

   m_CurrentCutLocation = ForceIntoRange(start,m_CurrentCutLocation,end);

//   UpdateBar();
   GetUpperView()->OnUpdate(NULL, HINT_BRIDGEVIEWSECTIONCUTCHANGED, NULL);
   GetLowerView()->OnUpdate(NULL, HINT_BRIDGEVIEWSECTIONCUTCHANGED, NULL);
}
   
Float64 CBridgeModelViewChildFrame::GetCurrentCutLocation()
{
   if ( !m_bCutLocationInitialized )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      SpanIndexType startSpanIdx, endSpanIdx;
      CBridgePlanView* pPlanView = GetBridgePlanView();
      pPlanView->GetSpanRange(&startSpanIdx,&endSpanIdx);
      PierIndexType startPierIdx = (PierIndexType)startSpanIdx;
      PierIndexType endPierIdx   = (PierIndexType)(endSpanIdx+1);

      GET_IFACE2(pBroker,IBridge,pBridge);
      Float64 start = pBridge->GetPierStation(startPierIdx);
      Float64 end   = pBridge->GetPierStation(endPierIdx);
      m_CurrentCutLocation = 0.5*(end-start) + start;
      m_bCutLocationInitialized = true;
   }

   return m_CurrentCutLocation;
}

void CBridgeModelViewChildFrame::OnViewGirder() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSegmentKey segmentKey;

   if ( GetBridgePlanView()->GetSelectedGirder(&segmentKey) || GetBridgePlanView()->GetSelectedSegment(&segmentKey) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      pDoc->OnViewGirderEditor();
   }
}

void CBridgeModelViewChildFrame::OnEditSpan() 
{
   SpanIndexType spanIdx;
   if ( GetBridgePlanView()->GetSelectedSpan(&spanIdx) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      pDoc->EditSpanDescription(spanIdx,ESD_GENERAL);
   }
}

void CBridgeModelViewChildFrame::OnEditPier() 
{
   PierIndexType pierIdx;
   if ( GetBridgePlanView()->GetSelectedPier(&pierIdx) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      pDoc->EditPierDescription(pierIdx,EPD_GENERAL);
   }
}

void CBridgeModelViewChildFrame::SelectSpan(SpanIndexType spanIdx)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectSpan(spanIdx);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectPier(PierIndexType pierIdx)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectPier(pierIdx);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectGirder(const CGirderKey& girderKey)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectGirder(girderKey);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectSegment(const CSegmentKey& segmentKey)
{
   if ( m_bSelecting )
      return;
   
   ATLASSERT(segmentKey.segmentIndex != INVALID_INDEX);

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectSegment(segmentKey);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectClosureJoint(const CSegmentKey& closureKey)
{
   if ( m_bSelecting )
      return;
   
   ATLASSERT(closureKey.segmentIndex != INVALID_INDEX);

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectClosureJoint(closureKey);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectTemporarySupport(SupportIDType tsID)
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectTemporarySupport(tsID);
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectDeck()
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectDeck();
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::SelectAlignment()
{
   if ( m_bSelecting )
      return;

   m_bSelecting = true;
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->SelectAlignment();
   m_bSelecting = false;
}

void CBridgeModelViewChildFrame::ClearSelection()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   pDoc->ClearSelection();
}

LRESULT CBridgeModelViewChildFrame::OnCommandHelp(WPARAM, LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGE_VIEW );
   return TRUE;
}

void CBridgeModelViewChildFrame::OnDeletePier() 
{
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
      pDoc->DeletePier(pierIdx);
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnUpdateDeletePier(CCmdUI* pCmdUI) 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   PierIndexType pierIdx;
	if ( 1 < pBridgeDesc->GetSpanCount() && GetBridgePlanView()->GetSelectedPier(&pierIdx) )
   {
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnDeleteSpan() 
{
   SpanIndexType spanIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedSpan(&spanIdx) )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();
      pDoc->DeleteSpan(spanIdx);
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnUpdateDeleteSpan(CCmdUI* pCmdUI) 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType spanIdx;
	if ( 1 < pBridgeDesc->GetSpanCount() && GetBridgePlanView()->GetSelectedSpan(&spanIdx) )
   {
      pCmdUI->Enable(TRUE);
   }
   else
   {
      pCmdUI->Enable(FALSE);
   }
}

void CBridgeModelViewChildFrame::OnInsertSpan() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   SpanIndexType spanIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedSpan(&spanIdx) )
   {
      CComPtr<IBroker> broker;
      EAFGetBroker(&broker);
      GET_IFACE2(broker,IBridgeDescription,pIBridgeDesc);
      CInsertSpanDlg dlg(pIBridgeDesc->GetBridgeDescription());
      if ( dlg.DoModal() == IDOK )
      {
         Float64 span_length = dlg.m_SpanLength;
         PierIndexType refPierIdx = dlg.m_RefPierIdx;
         pgsTypes::PierFaceType face = dlg.m_PierFace;
         bool bCreateNewGroup = dlg.m_bCreateNewGroup;
         EventIndexType eventIdx = dlg.m_EventIndex;

         CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();
         pDoc->InsertSpan(refPierIdx,face,span_length,bCreateNewGroup,eventIdx);
      }
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnInsertPier() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CComPtr<IBroker> broker;
      EAFGetBroker(&broker);
      GET_IFACE2(broker,IBridgeDescription,pIBridgeDesc);
      CInsertSpanDlg dlg(pIBridgeDesc->GetBridgeDescription());
      if ( dlg.DoModal() == IDOK )
      {
         Float64 span_length = dlg.m_SpanLength;
         PierIndexType refPierIdx = dlg.m_RefPierIdx;
         pgsTypes::PierFaceType pierFace = dlg.m_PierFace;
         bool bCreateNewGroup = dlg.m_bCreateNewGroup;
         EventIndexType eventIdx = dlg.m_EventIndex;

         CPGSDocBase* pDoc = (CPGSDocBase*)pView->GetDocument();
         pDoc->InsertSpan(refPierIdx,pierFace,span_length,bCreateNewGroup,eventIdx);

         if ( pierFace == pgsTypes::Back )
         {
            // move the pier selection ahead one pier so that it seems to
            // stay with the currently selected pier
            pView->SelectPier(pierIdx+1,true);
         }
      }
   }
   else
   {
      ATLASSERT(FALSE); // shouldn't get here
   }
}

void CBridgeModelViewChildFrame::OnBoundaryCondition(UINT nIDC)
{
#pragma Reminder("UPDATE: need to deal with InteriorPier... this is for BoundaryPier")
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      pgsTypes::BoundaryConditionType newBoundaryConditionType;
      pgsTypes::PierSegmentConnectionType newSegmentConnectionType;
      switch( nIDC )
      {
         case IDM_HINGE:                          newBoundaryConditionType = pgsTypes::bctHinge;                        break;
         case IDM_ROLLER:                         newBoundaryConditionType = pgsTypes::bctRoller;                       break;
         case IDM_CONTINUOUS_AFTERDECK:           newBoundaryConditionType = pgsTypes::bctContinuousAfterDeck;          break;
         case IDM_CONTINUOUS_BEFOREDECK:          newBoundaryConditionType = pgsTypes::bctContinuousBeforeDeck;         break;
         case IDM_INTEGRAL_AFTERDECK:             newBoundaryConditionType = pgsTypes::bctIntegralAfterDeck;            break;
         case IDM_INTEGRAL_BEFOREDECK:            newBoundaryConditionType = pgsTypes::bctIntegralBeforeDeck;           break;
         case IDM_INTEGRAL_AFTERDECK_HINGEBACK:   newBoundaryConditionType = pgsTypes::bctIntegralAfterDeckHingeBack;   break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEBACK:  newBoundaryConditionType = pgsTypes::bctIntegralBeforeDeckHingeBack;  break;
         case IDM_INTEGRAL_AFTERDECK_HINGEAHEAD:  newBoundaryConditionType = pgsTypes::bctIntegralAfterDeckHingeAhead;  break;
         case IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD: newBoundaryConditionType = pgsTypes::bctIntegralBeforeDeckHingeAhead; break;
         case IDM_CONTINUOUS_CLOSURE:             newSegmentConnectionType = pgsTypes::psctContinousClosureJoint;  break;
         case IDM_INTEGRAL_CLOSURE:               newSegmentConnectionType = pgsTypes::psctIntegralClosureJoint;   break;
         case IDM_CONTINUOUS_SEGMENT_AT_PIER:     newSegmentConnectionType = pgsTypes::psctContinuousSegment;     break;
         case IDM_INTEGRAL_SEGMENT_AT_PIER:       newSegmentConnectionType = pgsTypes::psctIntegralSegment;       break;

         default: ATLASSERT(false); // is there a new connection type?
      }

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      txnEditBoundaryConditions* pTxn;

      const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
      if ( pPier->IsBoundaryPier() )
      {
         pgsTypes::BoundaryConditionType oldConnectionType = pPier->GetBoundaryConditionType();
         pTxn = new txnEditBoundaryConditions(pierIdx,oldConnectionType,newBoundaryConditionType);
      }
      else
      {
         pgsTypes::PierSegmentConnectionType oldConnectionType = pPier->GetSegmentConnectionType();
         EventIndexType oldClosureEventIdx = INVALID_INDEX;
         EventIndexType newClosureEventIdx = INVALID_INDEX;
         const CGirderGroupData* pGroup = pPier->GetGirderGroup(pgsTypes::Back); // this is an interior pier so back/ahead are the same
         GroupIndexType grpIdx = pGroup->GetIndex();
         if ( oldConnectionType == pgsTypes::psctContinousClosureJoint || oldConnectionType == pgsTypes::psctIntegralClosureJoint )
         {
            IndexType closureIdx = pPier->GetClosureJoint(0)->GetIndex();
            oldClosureEventIdx = pIBridgeDesc->GetCastClosureJointEventIndex(grpIdx,closureIdx);
         }

         if ( newSegmentConnectionType == pgsTypes::psctContinousClosureJoint || newSegmentConnectionType == pgsTypes::psctIntegralClosureJoint )
         {
            // A new closure joint is being created. We need to supply the bridge model with the event index
            // for when the closure joint is cast. Since the user wasn't prompted (this is a response
            // handler for a right-click context menu) the first-best guess is to use the closure casting
            // event for another closure joint in this girder.

            newClosureEventIdx = pIBridgeDesc->GetCastClosureJointEventIndex(grpIdx,0);
            if ( newClosureEventIdx == INVALID_INDEX )
            {
               // there isn't another closure joint in this group with a valid event index
               // the next best guess is to use the event just before the event when the first 
               // tendon is installed.
               EventIndexType eventIdx = INVALID_INDEX;
               GirderIndexType nGirders = pGroup->GetGirderCount();
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
                  CGirderKey girderKey(pGirder->GetGirderKey());
                  const CPTData* pPT = pGirder->GetPostTensioning();
                  DuctIndexType nDucts = pPT->GetDuctCount();
                  for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
                  {
                     EventIndexType stressTendonEventIdx = pIBridgeDesc->GetStressTendonEventIndex(girderKey,ductIdx);
                     eventIdx = Min(eventIdx,stressTendonEventIdx);
                  }
               }

               if ( eventIdx != INVALID_INDEX )
               {
                  newClosureEventIdx = eventIdx-1;
               }
            }

            if ( newClosureEventIdx == INVALID_INDEX )
            {
               // the last best guess is to cast the closure when the segments are installed
               EventIndexType eventIdx = 0;
               GirderIndexType nGirders = pGroup->GetGirderCount();
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
                  SegmentIndexType nSegments = pGirder->GetSegmentCount();
                  for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
                  {
                     const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
                     CSegmentKey segmentKey(pSegment->GetSegmentKey());
                     EventIndexType erectSegmentEventIdx = pIBridgeDesc->GetSegmentErectionEventIndex(segmentKey);
                     eventIdx = Max(eventIdx,erectSegmentEventIdx);
                  }
               }

               if ( eventIdx != INVALID_INDEX )
               {
                  newClosureEventIdx = eventIdx;
               }
            }

            // if this fires then there is a use case that hasn't been considered
            ATLASSERT(newClosureEventIdx != INVALID_INDEX);
         }

         pTxn = new txnEditBoundaryConditions(pierIdx,oldConnectionType,oldClosureEventIdx,newSegmentConnectionType,newClosureEventIdx);
      }
      txnTxnManager::GetInstance()->Execute(pTxn);
   }
}

void CBridgeModelViewChildFrame::OnUpdateBoundaryCondition(CCmdUI* pCmdUI)
{
   PierIndexType pierIdx;
   CBridgePlanView* pView = GetBridgePlanView();
	if ( pView->GetSelectedPier(&pierIdx) )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
      if ( pPier->IsBoundaryPier() )
      {
         pgsTypes::BoundaryConditionType boundaryConditionType = pPier->GetBoundaryConditionType();
         switch( pCmdUI->m_nID )
         {
            case IDM_HINGE:                          pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctHinge);                        break;
            case IDM_ROLLER:                         pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctRoller);                       break;
            case IDM_CONTINUOUS_AFTERDECK:           pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctContinuousAfterDeck);          break;
            case IDM_CONTINUOUS_BEFOREDECK:          pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctContinuousBeforeDeck);         break;
            case IDM_INTEGRAL_AFTERDECK:             pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralAfterDeck);            break;
            case IDM_INTEGRAL_BEFOREDECK:            pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralBeforeDeck);           break;
            case IDM_INTEGRAL_AFTERDECK_HINGEBACK:   pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeBack);   break;
            case IDM_INTEGRAL_BEFOREDECK_HINGEBACK:  pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeBack);  break;
            case IDM_INTEGRAL_AFTERDECK_HINGEAHEAD:  pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeAhead);  break;
            case IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD: pCmdUI->SetCheck(boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeAhead); break;
            default: ATLASSERT(false); // is there a new boundary condition type?
         }
      }
      else
      {
         pgsTypes::PierSegmentConnectionType segmentConnectionType = pPier->GetSegmentConnectionType();
         switch( pCmdUI->m_nID )
         {
            case IDM_CONTINUOUS_CLOSURE:             pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctContinousClosureJoint);  break;
            case IDM_INTEGRAL_CLOSURE:               pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctIntegralClosureJoint);   break;
            case IDM_CONTINUOUS_SEGMENT_AT_PIER:     pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctContinuousSegment);     break;
            case IDM_INTEGRAL_SEGMENT_AT_PIER:       pCmdUI->SetCheck(segmentConnectionType == pgsTypes::psctIntegralSegment);       break;
            default: ATLASSERT(false); // is there a new connection type?
         }
      }
   }
}
void CBridgeModelViewChildFrame::OnStartSpanChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
   LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
   // TODO: Add your control notification handler code here
   *pResult = 0;


   CSpinButtonCtrl* pStartSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_START_SPAN_SPIN);
   int start, end;
   pStartSpinner->GetRange32(start,end);
   int newPos = pNMUpDown->iPos + pNMUpDown->iDelta;

   if ( newPos < start || end < newPos )
   {
      *pResult = 1;
      return;
   }

   SpanIndexType newStartSpanIdx = newPos - 1;

   CBridgePlanView* pPlanView = GetBridgePlanView();

   SpanIndexType startSpanIdx, endSpanIdx;
   pPlanView->GetSpanRange(&startSpanIdx,&endSpanIdx);

   CSpinButtonCtrl* pEndSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_END_SPAN_SPIN);
   if ( endSpanIdx <= newStartSpanIdx )
   {
      // new start span is greater than end span
      // force position to be the same
      endSpanIdx = newStartSpanIdx;

      pEndSpinner->SetPos32((int)endSpanIdx+1);
   }

   pPlanView->SetSpanRange(newStartSpanIdx,endSpanIdx);
}

void CBridgeModelViewChildFrame::OnEndSpanChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
   LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
   // TODO: Add your control notification handler code here
   *pResult = 0;

   CSpinButtonCtrl* pEndSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_END_SPAN_SPIN);
   int start, end;
   pEndSpinner->GetRange32(start,end);
   int newPos = pNMUpDown->iPos + pNMUpDown->iDelta;

   if ( newPos < start || end < newPos )
   {
      *pResult = 1;
      return;
   }

   SpanIndexType newEndSpanIdx = newPos - 1;

   CBridgePlanView* pPlanView = GetBridgePlanView();

   SpanIndexType startSpanIdx, endSpanIdx;
   pPlanView->GetSpanRange(&startSpanIdx,&endSpanIdx);
   
   CSpinButtonCtrl* pStartSpinner = (CSpinButtonCtrl*)m_SettingsBar.GetDlgItem(IDC_START_SPAN_SPIN);
   if ( newEndSpanIdx <= startSpanIdx )
   {
      // new end span is less than start span
      // force position to be the same
      startSpanIdx = newEndSpanIdx;

      pStartSpinner->SetPos32((int)startSpanIdx+1);
   }

   pPlanView->SetSpanRange(startSpanIdx,newEndSpanIdx);
}

void CBridgeModelViewChildFrame::OnViewModeChanged(UINT nIDC)
{
   int show = (nIDC == IDC_BRIDGE ? SW_SHOW : SW_HIDE);
   m_SettingsBar.GetDlgItem(IDC_SPAN_RANGE_LABEL)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_START_SPAN_SPIN)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_START_SPAN_EDIT)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_SPAN_RANGE_TO)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_END_SPAN_SPIN)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_END_SPAN_EDIT)->ShowWindow(show);
   m_SettingsBar.GetDlgItem(IDC_SPAN_COUNT)->ShowWindow(show);

   if ( nIDC == IDC_BRIDGE )
   {
      m_SplitterWnd.ReplaceView(0,0,RUNTIME_CLASS(CBridgePlanView));
      m_SplitterWnd.ReplaceView(1,0,RUNTIME_CLASS(CBridgeSectionView));
   }
   else
   {
      m_SplitterWnd.ReplaceView(0,0,RUNTIME_CLASS(CAlignmentPlanView));
      m_SplitterWnd.ReplaceView(1,0,RUNTIME_CLASS(CAlignmentProfileView));
   }
}
