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
// DesignRatingPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "DesignRatingPage.h"
#include "RatingOptionsDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>
#include "HtmlHelp\HelpTopics.hh"

// CDesignRatingPage dialog

IMPLEMENT_DYNAMIC(CDesignRatingPage, CPropertyPage)

CDesignRatingPage::CDesignRatingPage()
	: CPropertyPage(CDesignRatingPage::IDD)
{
}

CDesignRatingPage::~CDesignRatingPage()
{
}

void CDesignRatingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   DDX_Text(pDX,IDC_STRENGTH_I_DC,m_Data.StrengthI_DC);
   DDX_Text(pDX,IDC_STRENGTH_I_DW,m_Data.StrengthI_DW);
   DDX_Keyword(pDX,IDC_STRENGTH_I_LL_INVENTORY,_T("Compute"),m_Data.StrengthI_LL_Inventory);
   DDX_Keyword(pDX,IDC_STRENGTH_I_LL_OPERATING,_T("Compute"),m_Data.StrengthI_LL_Operating);
   DDX_Text(pDX,IDC_STRENGTH_I_CR,m_Data.StrengthI_CR);
   DDX_Text(pDX,IDC_STRENGTH_I_SH,m_Data.StrengthI_SH);
   DDX_Text(pDX,IDC_STRENGTH_I_PS,m_Data.StrengthI_PS);
   
   DDX_Text(pDX,IDC_SERVICE_III_DC,m_Data.ServiceIII_DC);
   DDX_Text(pDX,IDC_SERVICE_III_DW,m_Data.ServiceIII_DW);
   DDX_Keyword(pDX,IDC_SERVICE_III_LL,_T("Compute"),m_Data.ServiceIII_LL);
   DDX_Text(pDX,IDC_SERVICE_III_CR,m_Data.ServiceIII_CR);
   DDX_Text(pDX,IDC_SERVICE_III_SH,m_Data.ServiceIII_SH);
   DDX_Text(pDX,IDC_SERVICE_III_PS,m_Data.ServiceIII_PS);

   DDX_Check_Bool(pDX,IDC_RATE_FOR_SHEAR,m_Data.bRateForShear);

   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IEAFDisplayUnits,pDisplayUnits);
   DDX_UnitValueAndTag(pDX,IDC_ALLOWABLE_TENSION,IDC_ALLOWABLE_TENSION_UNIT,m_Data.AllowableTensionCoefficient,pDisplayUnits->GetTensionCoefficientUnit());

   CString tag = pDisplayUnits->GetUnitMode() == eafTypes::umSI ? _T("sqrt( f'c (MPa) )") : _T("sqrt( f'c (KSI) )");
   DDX_Text(pDX,IDC_ALLOWABLE_TENSION_UNIT,tag);
}


BEGIN_MESSAGE_MAP(CDesignRatingPage, CPropertyPage)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CDesignRatingPage message handlers
BOOL CDesignRatingPage::OnInitDialog()
{
   EnableToolTips();

   CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDesignRatingPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   pgsTypes::LimitState limit_state;
   bool bIsLoadFactorTip = false;

   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);

      switch(nID)
      {
      case IDC_RATE_FOR_SHEAR:
         m_strTip = _T("In-service concrete bridges that show no visible signs of shear distress need not be checked for shear when rating for the design loads (MBE 6A.5.9)");
         break;

      case IDC_STRENGTH_I_LL_INVENTORY:
         limit_state = pgsTypes::StrengthI_Inventory;
         bIsLoadFactorTip = true;
         break;

      case IDC_STRENGTH_I_LL_OPERATING:
         limit_state = pgsTypes::StrengthI_Operating;
         bIsLoadFactorTip = true;
         break;

      case IDC_SERVICE_III_LL:
         limit_state = pgsTypes::ServiceIII_Inventory;
         bIsLoadFactorTip = true;
         break;

      default:
         return FALSE;
      }

      if ( bIsLoadFactorTip )
      {
         CRatingOptionsDlg* pParent = (CRatingOptionsDlg*)GetParent();
         pParent->GetLoadFactorToolTip(m_strTip,limit_state);
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = NULL;
      return TRUE;
   }
   return FALSE;
}

BOOL CDesignRatingPage::OnSetActive()
{
   if ( !CPropertyPage::OnSetActive() )
      return FALSE;

   CRatingOptionsDlg* pParent = (CRatingOptionsDlg*)GetParent();
   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2( broker, ILibrary, pLib );
   const RatingLibraryEntry* pRatingEntry = pLib->GetRatingEntry( pParent->m_GeneralPage.m_Data.CriteriaName.c_str() );

   const CLiveLoadFactorModel& inventory = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Inventory);

   CDataExchange dx(this,false);
   Float64 gLL = -1;
   if ( !inventory.AllowUserOverride() )
   {
      DDX_Keyword(&dx,IDC_STRENGTH_I_LL_INVENTORY,_T("Compute"),gLL);
      DDX_Keyword(&dx,IDC_SERVICE_III_LL,_T("Compute"),gLL);
      GetDlgItem(IDC_STRENGTH_I_LL_INVENTORY)->EnableWindow(FALSE);
      GetDlgItem(IDC_SERVICE_III_LL)->EnableWindow(FALSE);
   }
   else
   {
      GetDlgItem(IDC_STRENGTH_I_LL_INVENTORY)->EnableWindow(TRUE);
      GetDlgItem(IDC_SERVICE_III_LL)->EnableWindow(TRUE);
   }


   const CLiveLoadFactorModel& operating = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrDesign_Operating);
   if ( !operating.AllowUserOverride() )
   {
      DDX_Keyword(&dx,IDC_STRENGTH_I_LL_OPERATING,_T("Compute"),gLL);
      GetDlgItem(IDC_STRENGTH_I_LL_OPERATING)->EnableWindow(FALSE);
   }
   else
   {
      GetDlgItem(IDC_STRENGTH_I_LL_OPERATING)->EnableWindow(TRUE);
   }

   GET_IFACE2(broker,ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   if ( pSpecEntry->GetLossMethod() != pgsTypes::TIME_STEP )
   {
      GetDlgItem(IDC_STRENGTH_I_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STRENGTH_I_PS_LABEL)->ShowWindow(SW_HIDE);

      GetDlgItem(IDC_SERVICE_III_PLUS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_CR)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_CR_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_SH)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_SH_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_PS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_SERVICE_III_PS_LABEL)->ShowWindow(SW_HIDE);
   }

   return TRUE;
}

void CDesignRatingPage::OnHelp()
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_RATING_DESIGN_TAB );
}