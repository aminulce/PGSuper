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

// PierDisplayObjectEvents.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PierDisplayObjectEvents.h"
#include "mfcdual.h"
#include "pgsuperdoc.h"
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPierDisplayObjectEvents

CPierDisplayObjectEvents::CPierDisplayObjectEvents(PierIndexType pierIdx,PierIndexType nPiers,bool bHasDeck,CBridgeModelViewChildFrame* pFrame)
{
   m_PierIdx = pierIdx;
   m_nPiers = nPiers;
   m_bHasDeck = bHasDeck;
   m_pFrame  = pFrame;
}

BEGIN_INTERFACE_MAP(CPierDisplayObjectEvents, CCmdTarget)
	INTERFACE_PART(CPierDisplayObjectEvents, IID_iDisplayObjectEvents, Events)
END_INTERFACE_MAP()

DELEGATE_CUSTOM_INTERFACE(CPierDisplayObjectEvents,Events);

void CPierDisplayObjectEvents::EditPier(iDisplayObject* pDO)
{
   CComPtr<iDisplayList> pList;
   pDO->GetDisplayList(&pList);

   CComPtr<iDisplayMgr> pDispMgr;
   pList->GetDisplayMgr(&pDispMgr);

   CDisplayView* pView = pDispMgr->GetView();
   CDocument* pDoc = pView->GetDocument();

   ((CPGSuperDoc*)pDoc)->EditPierDescription(m_PierIdx,EPD_GENERAL);
}

void CPierDisplayObjectEvents::SelectPier(iDisplayObject* pDO)
{
   // do the selection at the frame level because it will do this view
   // and the other view
   m_pFrame->SelectPier(m_PierIdx);
}

void CPierDisplayObjectEvents::SelectPrev(iDisplayObject* pDO)
{
   if ( m_PierIdx == 0 )
   {
      // this is the first pier
      if ( m_bHasDeck )
         m_pFrame->SelectDeck();  // select deck if there is one
      else
         m_pFrame->SelectPier(m_nPiers-1); // otherwise, select last pier
   }
   else
   {
      m_pFrame->SelectSpan(m_PierIdx-1);
   }
}

void CPierDisplayObjectEvents::SelectNext(iDisplayObject* pDO)
{
   if ( m_PierIdx == m_nPiers-1 )
   {
      // this is the last pier
      if ( m_bHasDeck )
         m_pFrame->SelectDeck(); // select deck if there is one 
      else
         m_pFrame->SelectPier(0); // otherwise, select first pier
   }
   else
   {
      m_pFrame->SelectSpan(m_PierIdx);
   }
}

/////////////////////////////////////////////////////////////////////////////
// CPierDisplayObjectEvents message handlers
STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnLButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   pThis->EditPier(pDO);
   return true;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnLButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnLButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnRButtonDblClk(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnRButtonDown(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return true; // acknowledge the event so that the object can become selected
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnRButtonUp(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnMouseMove(iDisplayObject* pDO,UINT nFlags,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnMouseWheel(iDisplayObject* pDO,UINT nFlags,short zDelta,CPoint point)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnKeyDown(iDisplayObject* pDO,UINT nChar, UINT nRepCnt, UINT nFlags)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);

   if ( nChar == VK_RETURN )
   {
      pThis->EditPier(pDO);
      return true;
   }
   else if ( nChar == VK_LEFT )
   {
      pThis->SelectPrev(pDO);
      return true;
   }
   else if ( nChar == VK_RIGHT )
   {
      pThis->SelectNext(pDO);
      return true;
   }
   else if ( nChar == VK_UP || nChar == VK_DOWN )
   {
      int spanIdx = (pThis->m_PierIdx == pThis->m_nPiers-1 ? pThis->m_nPiers-2 : pThis->m_PierIdx);
      pThis->m_pFrame->SelectGirder(spanIdx,0);
      return true;
   }

   return false;
}

STDMETHODIMP_(bool) CPierDisplayObjectEvents::XEvents::OnContextMenu(iDisplayObject* pDO,CWnd* pWnd,CPoint point)
{
   METHOD_PROLOGUE_(CPierDisplayObjectEvents,Events);
   AFX_MANAGE_STATE(AfxGetStaticModuleState());


   if ( pDO->IsSelected() )
   {
      CMenu menu;
      menu.LoadMenu(IDR_SELECTED_PIER_CONTEXT);
      CMenu* pSubMenu = menu.GetSubMenu(0);


      CComPtr<iDisplayList> pList;
      pDO->GetDisplayList(&pList);

      CComPtr<iDisplayMgr> pDispMgr;
      pList->GetDisplayMgr(&pDispMgr);

      CDisplayView* pView = pDispMgr->GetView();
      CPGSuperDoc* pDoc = (CPGSuperDoc*)pView->GetDocument();

      CComPtr<IBroker> pBroker;
      pDoc->GetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CPierData* pPier = pBridgeDesc->GetPier(pThis->m_PierIdx);


      pSubMenu->AppendMenu(MF_SEPARATOR);
      pSubMenu->AppendMenu(MF_STRING,IDM_HINGED,CPierData::AsString(pgsTypes::Hinged));
      pSubMenu->AppendMenu(MF_STRING,IDM_ROLLER,CPierData::AsString(pgsTypes::Roller));

      if ( pPier->GetPrevSpan() && pPier->GetNextSpan() )
      {
         pSubMenu->AppendMenu(MF_STRING,IDM_CONTINUOUS_AFTERDECK, CPierData::AsString(pgsTypes::ContinuousAfterDeck));
         pSubMenu->AppendMenu(MF_STRING,IDM_CONTINUOUS_BEFOREDECK,CPierData::AsString(pgsTypes::ContinuousBeforeDeck));
      }

      pSubMenu->AppendMenu(MF_STRING,IDM_INTEGRAL_AFTERDECK, CPierData::AsString(pgsTypes::IntegralAfterDeck));
      pSubMenu->AppendMenu(MF_STRING,IDM_INTEGRAL_BEFOREDECK,CPierData::AsString(pgsTypes::IntegralBeforeDeck));

      if ( pPier->GetPrevSpan() && pPier->GetNextSpan() )
      {
         pSubMenu->AppendMenu(MF_STRING,IDM_INTEGRAL_AFTERDECK_HINGEBACK,  CPierData::AsString(pgsTypes::IntegralAfterDeckHingeBack));
         pSubMenu->AppendMenu(MF_STRING,IDM_INTEGRAL_BEFOREDECK_HINGEBACK, CPierData::AsString(pgsTypes::IntegralBeforeDeckHingeBack));
         pSubMenu->AppendMenu(MF_STRING,IDM_INTEGRAL_AFTERDECK_HINGEAHEAD, CPierData::AsString(pgsTypes::IntegralAfterDeckHingeAhead));
         pSubMenu->AppendMenu(MF_STRING,IDM_INTEGRAL_BEFOREDECK_HINGEAHEAD,CPierData::AsString(pgsTypes::IntegralBeforeDeckHingeAhead));
      }


      pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,pThis->m_pFrame);

      return true;
   }

   return false;
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnChanged(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnDragMoved(iDisplayObject* pDO,ISize2d* offset)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnMoved(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnCopied(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnSelect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
   pThis->SelectPier(pDO);
}

STDMETHODIMP_(void) CPierDisplayObjectEvents::XEvents::OnUnselect(iDisplayObject* pDO)
{
   METHOD_PROLOGUE(CPierDisplayObjectEvents,Events);
}

