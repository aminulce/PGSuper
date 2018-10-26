///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2013  Washington State Department of Transportation
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

// ClosurePourDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin.h"
#include "ClosurePourDlg.h"

#include <LRFD\RebarPool.h>


#define IDC_CHECKBOX 100

// CClosurePourDlg

IMPLEMENT_DYNAMIC(CClosurePourDlg, CPropertySheet)

CClosurePourDlg::CClosurePourDlg(UINT nIDCaption, const CSegmentKey& closureKey,const CClosurePourData* pClosurePour,EventIndexType eventIdx,bool bEditingInGirder,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage),
   m_EventIdx(eventIdx),
   m_ClosurePour(*pClosurePour),
   m_ClosureKey(closureKey),
   m_bEditingInGirder(bEditingInGirder)
{
   Init();
}

CClosurePourDlg::CClosurePourDlg(LPCTSTR pszCaption, const CSegmentKey& closureKey,const CClosurePourData* pClosurePour, EventIndexType eventIdx, bool bEditingInGirder,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage),
   m_EventIdx(eventIdx),
   m_ClosurePour(*pClosurePour),
   m_ClosureKey(closureKey),
   m_bEditingInGirder(bEditingInGirder)
{
   Init();
}

CClosurePourDlg::~CClosurePourDlg()
{
}


BEGIN_MESSAGE_MAP(CClosurePourDlg, CPropertySheet)
      WBFL_ON_PROPSHEET_OK
END_MESSAGE_MAP()


// CClosurePourDlg message handlers
void CClosurePourDlg::Init()
{
   m_bCopyToAllClosurePours = false;

   m_psh.dwFlags                |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_General.m_psp.dwFlags      |= PSP_HASHELP;
   m_Longitudinal.m_psp.dwFlags |= PSP_HASHELP;
   m_Stirrups.m_psp.dwFlags     |= PSP_HASHELP;

   AddPage(&m_General);
   AddPage(&m_Longitudinal);
   AddPage(&m_Stirrups);
}

BOOL CClosurePourDlg::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();


   // Build the OK button
   CWnd* pOK = GetDlgItem(IDOK);
   CRect rOK;
   pOK->GetWindowRect(&rOK);

   CRect wndPage;
   GetActivePage()->GetWindowRect(&wndPage);

   CRect rect;
   rect.left = wndPage.left;
   rect.top = rOK.top;
   rect.bottom = rOK.bottom;
   rect.right = rOK.left - 7;
   ScreenToClient(&rect);
   CString strTxt(m_bEditingInGirder ? _T("Copy to all closure pours in this girder") : _T("Copy to all closure pours at this support"));
   m_CheckBox.Create(strTxt,WS_CHILD | WS_VISIBLE | BS_LEFTTEXT | BS_RIGHT | BS_AUTOCHECKBOX,rect,this,IDC_CHECKBOX);
   m_CheckBox.SetFont(GetFont());

   UpdateData(FALSE); // calls DoDataExchange

   return bResult;
}

void CClosurePourDlg::DoDataExchange(CDataExchange* pDX)
{
   CPropertySheet::DoDataExchange(pDX);
   DDX_Check_Bool(pDX,IDC_CHECKBOX,m_bCopyToAllClosurePours);
}

BOOL CClosurePourDlg::OnOK()
{
   UpdateData(TRUE);
   return FALSE; // MUST RETURN FALSE
}
