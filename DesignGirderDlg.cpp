///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2018  Washington State Department of Transportation
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

// DesignGirderDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "DesignGirderDlg.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <PsgLib\GirderLibraryEntry.h>
#include <EAF\EAFDocument.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDesignGirderDlg dialog


CDesignGirderDlg::CDesignGirderDlg(GroupIndexType grpIdx,GirderIndexType gdrIdx, bool enableA, arSlabOffsetDesignType designA, 
                                   IBroker* pBroker, CWnd* pParent /*=nullptr*/)
	: CDialog(CDesignGirderDlg::IDD, pParent),
   m_EnableA(enableA),
   m_DesignSlabOffset(designA)
   , m_DesignRadioNum(0)
   , m_StartWithCurrentStirrupLayout(FALSE)
{
   m_pBroker = pBroker;

	//{{AFX_DATA_INIT(CDesignGirderDlg)
   m_Group = grpIdx;
	m_Girder = gdrIdx;
	m_DesignForFlexure = TRUE;
	m_DesignForShear = TRUE;
	//}}AFX_DATA_INIT

   m_strToolTip = "Eug�ne Freyssinet\r\nFather of Prestressed Concrete";
}


void CDesignGirderDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CDesignGirderDlg)
   DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_Girder);
   DDX_CBIndex(pDX, IDC_SPAN, (int&)m_Group);
   DDX_CBEnum(pDX, IDC_DESIGN_A, (int&)m_DesignSlabOffset);
   DDX_Check(pDX, IDC_DESIGN_FLEXURE, m_DesignForFlexure);
   DDX_Check(pDX, IDC_DESIGN_SHEAR, m_DesignForShear);
   DDX_Radio(pDX, IDC_RADIO_SINGLE, m_DesignRadioNum);
   DDX_Check(pDX, IDC_START_WITH_LAYOUT, m_StartWithCurrentStirrupLayout);
   //}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate)
   {
      if (m_DesignForFlexure==FALSE && m_DesignForShear==FALSE)
      {
         ::AfxMessageBox(_T("No design requested. Please select Flexural and/or Shear design."),MB_OK | MB_ICONWARNING);
         pDX->Fail();
      }

      // build girder list based on input type
      if (m_DesignRadioNum==0)
      {
         // Single girder
         std::vector<CGirderKey> list_of_one;
         CGirderKey girderKey(m_Group, m_Girder);
         list_of_one.push_back(girderKey);
         m_GirderKeys = list_of_one;
      }
      else
      {
         // Girder list was stored/passed from grid
         if (m_GirderKeys.empty())
         {
            ::AfxMessageBox(_T("No girders selected. Please select at least one girder"),MB_OK | MB_ICONWARNING);
            pDX->Fail();
         }
      }

      if (m_DesignForFlexure==FALSE)
      {
         // No A design if no flexure design
         m_DesignSlabOffset = sodNoADesign;
      }
   }
}


BEGIN_MESSAGE_MAP(CDesignGirderDlg, CDialog)
	//{{AFX_MSG_MAP(CDesignGirderDlg)
	ON_BN_CLICKED(IDC_HELPME, OnHelp)
	ON_CBN_SELCHANGE(IDC_SPAN, OnSpanChanged)
	ON_BN_CLICKED(IDC_DESIGN_FLEXURE, OnDesignFlexure)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_SELECT_GIRDERS, &CDesignGirderDlg::OnBnClickedSelectGirders)
   ON_BN_CLICKED(IDC_RADIO_SINGLE, &CDesignGirderDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO_MULTIPLE, &CDesignGirderDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_DESIGN_SHEAR, &CDesignGirderDlg::OnBnClickedDesignShear)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDesignGirderDlg message handlers

BOOL CDesignGirderDlg::OnInitDialog() 
{
   // Load up the combo boxes with span and girder information
   GET_IFACE(IBridge,pBridge);

   CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
   CComboBox* pGdrBox  = (CComboBox*)GetDlgItem( IDC_GIRDER );

   SpanIndexType cSpan = pBridge->GetSpanCount();
   for ( SpanIndexType i = 0; i < cSpan; i++ )
   {
      CString strSpan;
      strSpan.Format(_T("Span %d"),LABEL_SPAN(i));
      pSpanBox->AddString(strSpan);
   }

   pSpanBox->SetCurSel((int)m_Group);
   UpdateGirderComboBox(m_Group);

   CButton* pA= (CButton*)GetDlgItem( IDC_DESIGN_A );
   pA->EnableWindow(m_EnableA?TRUE:FALSE);

	CDialog::OnInitDialog();

   EnableToolTips(TRUE);

   OnBnClickedRadio();
   OnBnClickedDesignShear();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDesignGirderDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_DESIGNGIRDER );
}

void CDesignGirderDlg::OnSpanChanged() 
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
   SpanIndexType spanIdx = (SpanIndexType)pCB->GetCurSel();

   UpdateGirderComboBox(spanIdx);
}

void CDesignGirderDlg::UpdateGirderComboBox(SpanIndexType spanIdx)
{
   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirder = pBridge->GetGirderCount( spanIdx );
   for ( GirderIndexType j = 0; j < cGirder; j++ )
   {
      CString strGdr;
      strGdr.Format( _T("Girder %s"), LABEL_GIRDER(j));
      pGdrBox->AddString( strGdr );
   }

   if ( pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR )
      pGdrBox->SetCurSel(0);
}

void CDesignGirderDlg::OnDesignFlexure() 
{
   // enable A design only if flexure is on
   UpdateADimCtrl();
}

BOOL CDesignGirderDlg::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_PICTURE:
         pTTT->lpszText = m_strToolTip.LockBuffer();
         pTTT->hinst = nullptr;
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip

      return TRUE;
   }
   return FALSE;
}

void CDesignGirderDlg::OnBnClickedSelectGirders()
{
   CMultiGirderSelectDlg dlg;
   dlg.m_GirderKeys = m_GirderKeys;

   if (dlg.DoModal()==IDOK)
   {
      m_GirderKeys = dlg.m_GirderKeys;

      // update button text
      CString msg;
      msg.Format(_T("Select Girders\n(%d Selected)"), m_GirderKeys.size());
      GetDlgItem(IDC_SELECT_GIRDERS)->SetWindowText(msg);

      UpdateADimCtrl();
   }
}

void CDesignGirderDlg::OnBnClickedRadio()
{
   BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO_SINGLE) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

   GetDlgItem(IDC_SPAN)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_GIRDER)->EnableWindow(enab_sgl);

   GetDlgItem(IDC_SELECT_GIRDERS)->EnableWindow(enab_mpl);

   UpdateADimCtrl();

   if ( enab_mpl && m_GirderKeys.size() == 0 )
   {
      OnBnClickedSelectGirders();
   }
}

void CDesignGirderDlg::UpdateADimCtrl()
{
   if (m_EnableA)
   {
      // only enable A if flexure is selected
      CButton* pAFlex = (CButton*)GetDlgItem( IDC_DESIGN_FLEXURE );
      BOOL flexure_checked = (pAFlex->GetCheck()==1) ? TRUE:FALSE;

      CButton* pA= (CButton*)GetDlgItem( IDC_DESIGN_A );
      pA->EnableWindow(flexure_checked);
   }
}

void CDesignGirderDlg::OnBnClickedDesignShear()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_DESIGN_SHEAR) == BST_CHECKED;

   GetDlgItem( IDC_START_WITH_LAYOUT )->EnableWindow(bEnable);
}
