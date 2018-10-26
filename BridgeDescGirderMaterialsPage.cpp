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

// BridgeDescGirderMaterialsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperDoc.h"
#include "PGSuperColors.h"
#include "PGSuperUnits.h"
#include "BridgeDescGirderMaterialsPage.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#include "SelectItemDlg.h"

#include "GirderDescDlg.h"

#include "HtmlHelp\HelpTopics.hh"

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ConcreteDetailsDlg.h>

#include <Atlddx.h>
#include <system\tokenizer.h>

#include "PGSuperUnits.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDescGeneralPage property page

IMPLEMENT_DYNCREATE(CGirderDescGeneralPage, CPropertyPage)

CGirderDescGeneralPage::CGirderDescGeneralPage() : CPropertyPage(CGirderDescGeneralPage::IDD)

{
	//{{AFX_DATA_INIT(CGirderDescGeneralPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGirderDescGeneralPage::~CGirderDescGeneralPage()
{
}

void CGirderDescGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescGeneralPage)
	DDX_Control(pDX, IDC_EC,      m_ctrlEc);
	DDX_Control(pDX, IDC_ECI,     m_ctrlEci);
	DDX_Control(pDX, IDC_MOD_EC,  m_ctrlEcCheck);
	DDX_Control(pDX, IDC_MOD_ECI, m_ctrlEciCheck);
   DDX_Control(pDX, IDC_GIRDER_FC, m_ctrlFc);
	DDX_Control(pDX, IDC_FCI, m_ctrlFci);
	//}}AFX_DATA_MAP

   DDX_Control(pDX,IDC_GIRDERNAME_NOTE,m_GirderTypeHyperLink);
   DDX_Control(pDX,IDC_SLABOFFSET_NOTE,m_SlabOffsetHyperLink);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   DDX_CBStringExactCase(pDX,IDC_GIRDER_NAME,pParent->m_strGirderName);

   // concrete material
   ExchangeConcreteData(pDX);

   DDX_UnitValueAndTag( pDX, IDC_FCI, IDC_FCI_UNIT, pParent->m_pSegment->Material.Concrete.Fci, pDisplayUnits->GetStressUnit() );
   // Validation: 0 < f'ci <= f'c   
   DDV_UnitValueLimitOrLess( pDX, IDC_FCI, pParent->m_pSegment->Material.Concrete.Fci, pParent->m_pSegment->Material.Concrete.Fc, pDisplayUnits->GetStressUnit() );

   // Slab Offset
   DDX_Tag(pDX, IDC_ADIM_START_UNIT, pDisplayUnits->GetComponentDimUnit() );
   DDX_Tag(pDX, IDC_ADIM_END_UNIT,   pDisplayUnits->GetComponentDimUnit() );

   GET_IFACE2(pBroker,IBridge,pBridge);
   if ( pBridge->GetDeckType() != pgsTypes::sdtNone )
   {
      DDX_UnitValueAndTag( pDX, IDC_ADIM_START, IDC_ADIM_START_UNIT, m_SlabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit() );
      DDX_UnitValueAndTag( pDX, IDC_ADIM_END,   IDC_ADIM_END_UNIT,   m_SlabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit() );

      // validate slab offset... (must be greater or equal gross deck thickess)
      if ( pDX->m_bSaveAndValidate )
      {
         Float64 Lg = pBridge->GetSegmentLength(pParent->m_SegmentKey);
         pgsPointOfInterest poiStart(pParent->m_SegmentKey,0.0);
         pgsPointOfInterest poiEnd(pParent->m_SegmentKey,Lg);
         Float64 grossDeckThicknessStart = pBridge->GetGrossSlabDepth(poiStart) + pBridge->GetFillet();
         Float64 grossDeckThicknessEnd   = pBridge->GetGrossSlabDepth(poiEnd) + pBridge->GetFillet();
         
         m_SlabOffset[pgsTypes::metStart] = (IsEqual(m_SlabOffset[pgsTypes::metStart],grossDeckThicknessStart) ? grossDeckThicknessStart : m_SlabOffset[pgsTypes::metStart]);
         m_SlabOffset[pgsTypes::metEnd]   = (IsEqual(m_SlabOffset[pgsTypes::metEnd],  grossDeckThicknessEnd  ) ? grossDeckThicknessEnd   : m_SlabOffset[pgsTypes::metEnd]);

         if ( m_SlabOffset[pgsTypes::metStart] < grossDeckThicknessStart )
         {
            pDX->PrepareEditCtrl(IDC_ADIM_START);
            CString msg;
            msg.Format(_T("The slab offset at the start of the girder must be at equal to the slab + fillet depth of %s"),FormatDimension(grossDeckThicknessStart,pDisplayUnits->GetComponentDimUnit()));
            AfxMessageBox(msg,MB_ICONEXCLAMATION);
            pDX->Fail();
         }

         if ( m_SlabOffset[pgsTypes::metEnd] < grossDeckThicknessEnd )
         {
            pDX->PrepareEditCtrl(IDC_ADIM_END);
            CString msg;
            msg.Format(_T("The slab offset at the end of the girder must be at equal to the slab + fillet depth of %s"),FormatDimension(grossDeckThicknessEnd,pDisplayUnits->GetComponentDimUnit()));
            AfxMessageBox(msg,MB_ICONEXCLAMATION);
            pDX->Fail();
         }
      }
   }

   DDX_CBEnum(pDX, IDC_CONDITION_FACTOR_TYPE, pParent->m_ConditionFactorType);
   DDX_Text(pDX,   IDC_CONDITION_FACTOR,      pParent->m_ConditionFactor);
}

void CGirderDescGeneralPage::ExchangeConcreteData(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);


   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int value;
      if ( !pDX->m_bSaveAndValidate )
      {
         value = pParent->m_pSegment->Material.Concrete.bBasePropertiesOnInitialValues ? 0 : 1;
      }
      
      DDX_Radio(pDX,IDC_FC1,value);

      if ( pDX->m_bSaveAndValidate )
      {
         pParent->m_pSegment->Material.Concrete.bBasePropertiesOnInitialValues = (value == 0 ? true : false);
      }
   }

   DDX_UnitValueAndTag( pDX, IDC_GIRDER_FC,  IDC_GIRDER_FC_UNIT,   pParent->m_pSegment->Material.Concrete.Fc , pDisplayUnits->GetStressUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_GIRDER_FC,pParent->m_pSegment->Material.Concrete.Fc, pDisplayUnits->GetStressUnit() );

   DDX_Check_Bool(pDX, IDC_MOD_ECI, pParent->m_pSegment->Material.Concrete.bUserEci);
   DDX_UnitValueAndTag( pDX, IDC_ECI,  IDC_ECI_UNIT,   pParent->m_pSegment->Material.Concrete.Eci , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_ECI,pParent->m_pSegment->Material.Concrete.Eci, pDisplayUnits->GetModEUnit() );

   DDX_Check_Bool(pDX, IDC_MOD_EC,  pParent->m_pSegment->Material.Concrete.bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_EC,  IDC_EC_UNIT, pParent->m_pSegment->Material.Concrete.Ec , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_EC, pParent->m_pSegment->Material.Concrete.Ec, pDisplayUnits->GetModEUnit() );

   if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == 1 )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }

   if ( pDX->m_bSaveAndValidate && m_ctrlEciCheck.GetCheck() == 1 )
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
   }
}

BEGIN_MESSAGE_MAP(CGirderDescGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDescGeneralPage)
   ON_BN_CLICKED(IDC_MOD_ECI, OnUserEci)
   ON_BN_CLICKED(IDC_MOD_EC, OnUserEc)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_EN_CHANGE(IDC_FCI, OnChangeFci)
	ON_EN_CHANGE(IDC_GIRDER_FC, OnChangeGirderFc)
	ON_EN_CHANGE(IDC_ECI, OnChangeEci)
	ON_EN_CHANGE(IDC_EC, OnChangeEc)
	ON_BN_CLICKED(IDC_MORE, OnMoreConcreteProperties)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	ON_WM_CTLCOLOR()
   ON_REGISTERED_MESSAGE(MsgChangeSameGirderType,OnChangeSameGirderType)
   ON_REGISTERED_MESSAGE(MsgChangeSlabOffsetType,OnChangeSlabOffsetType)
   ON_CBN_SELCHANGE(IDC_GIRDER_NAME,OnChangeGirderName)
   ON_CBN_DROPDOWN(IDC_GIRDER_NAME,OnBeforeChangeGirderName)
   ON_BN_CLICKED(IDC_FC1,OnConcreteStrength)
   ON_BN_CLICKED(IDC_FC2,OnConcreteStrength)
   ON_CBN_SELCHANGE(IDC_CONDITION_FACTOR_TYPE, OnConditionFactorTypeChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDescGeneralPage message handlers

BOOL CGirderDescGeneralPage::OnInitDialog() 
{
   FillGirderComboBox();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );
   m_LossMethod = pSpecEntry->GetLossMethod();
   m_TimeDependentModel = pSpecEntry->GetTimeDependentModel();

   if ( m_SlabOffsetType == pgsTypes::sotBridge || m_SlabOffsetType == pgsTypes::sotPier )
   {
      m_SlabOffsetTypeCache = pgsTypes::sotGirder;
   }
   else
   {
      m_SlabOffsetTypeCache = pgsTypes::sotBridge;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_strSlabOffsetCache[pgsTypes::metStart].Format(_T("%s"),FormatDimension(m_SlabOffset[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false));
   m_strSlabOffsetCache[pgsTypes::metEnd].Format(  _T("%s"),FormatDimension(m_SlabOffset[pgsTypes::metEnd],  pDisplayUnits->GetComponentDimUnit(),false));

   // Initialize the condition factor combo box
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);
   pcbConditionFactor->AddString(_T("Good or Satisfactory (Structure condition rating 6 or higher)"));
   pcbConditionFactor->AddString(_T("Fair (Structure condition rating of 5)"));
   pcbConditionFactor->AddString(_T("Poor (Structure condition rating 4 or lower)"));
   pcbConditionFactor->AddString(_T("Other"));
   pcbConditionFactor->SetCurSel(0);

   CPropertyPage::OnInitDialog();

   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
      SegmentIDType segmentID = pParent->m_pSegment->GetID();
      ATLASSERT(segmentID != INVALID_ID);

      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
      EventIndexType eventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(segmentID);
      m_AgeAtRelease = pTimelineMgr->GetEventByIndex(eventIdx)->GetConstructSegmentsActivity().GetAgeAtRelease();

      // hide regular text label
      GetDlgItem(IDC_FC_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FCI_LABEL)->ShowWindow(SW_HIDE);
   }
   else
   {
      // hide radio buttons
      GetDlgItem(IDC_FC1)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_FC2)->ShowWindow(SW_HIDE);
   }

   OnConditionFactorTypeChanged();


   UpdateGirderTypeHyperLink();
   UpdateGirderTypeControls();

   UpdateSlabOffsetHyperLink();
   UpdateSlabOffsetControls();

   if ( m_strUserEc == _T("") )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }
	
   if ( m_strUserEci == _T("") )
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
   }

   OnUserEci();
   OnUserEc();
   OnConcreteStrength();

   EnableToolTips(TRUE);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   if ( pIBridgeDesc->GetDeckDescription()->DeckType == pgsTypes::sdtNone )
   {
      // disable slab offset input if there isn't a deck
      m_SlabOffsetHyperLink.EnableWindow(FALSE);

      GetDlgItem(IDC_ADIM_START_LABEL)->EnableWindow(FALSE);
      GetDlgItem(IDC_ADIM_START)->EnableWindow(FALSE);
      GetDlgItem(IDC_ADIM_START_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDC_ADIM_END_LABEL)->EnableWindow(FALSE);
      GetDlgItem(IDC_ADIM_END)->EnableWindow(FALSE);
      GetDlgItem(IDC_ADIM_END_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDC_ADIM_START)->SetWindowText(_T(""));
      GetDlgItem(IDC_ADIM_END)->SetWindowText(_T(""));
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CGirderDescGeneralPage::OnUserEci()
{
   BOOL bEnable = ((CButton*)GetDlgItem(IDC_MOD_ECI))->GetCheck();
   GetDlgItem(IDC_ECI)->EnableWindow(bEnable);
   GetDlgItem(IDC_ECI_UNIT)->EnableWindow(bEnable);

   if (bEnable==FALSE)
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
      UpdateEci();
   }
   else
   {
      m_ctrlEci.SetWindowText(m_strUserEci);
   }
}

void CGirderDescGeneralPage::OnUserEc()
{
   BOOL bEnable = ((CButton*)GetDlgItem(IDC_MOD_EC))->GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   if (bEnable==FALSE)
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
      UpdateEc();
   }
   else
   {
      m_ctrlEc.SetWindowText(m_strUserEc); 
   }
}

void CGirderDescGeneralPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_GENERAL );
}

void CGirderDescGeneralPage::OnChangeFci() 
{
   UpdateEci();
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      UpdateFc();
      UpdateEc();
   }
}

void CGirderDescGeneralPage::OnChangeGirderFc() 
{
   UpdateEc();
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      UpdateFci();
      UpdateEci();
   }
}

void CGirderDescGeneralPage::OnChangeEc() 
{
   if ( m_LossMethod == pgsTypes::TIME_STEP && m_ctrlEcCheck.GetCheck() == TRUE)
   {
      UpdateEci();
   }
}

void CGirderDescGeneralPage::OnChangeEci() 
{
   if ( m_LossMethod == pgsTypes::TIME_STEP && m_ctrlEciCheck.GetCheck() == TRUE)
   {
      UpdateEc();
   }
}

void CGirderDescGeneralPage::UpdateEci()
{
   int method = 0;
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
      if ( i == IDC_FC2 )
      {
         // concrete model is based on f'c
         if ( m_ctrlEcCheck.GetCheck() == TRUE )
         {
            // Ec box is checked... user has input a value for Ec
            // Eci is based on user value for Ec not f'ci
            method = 0;
         }
         else
         {
            // Ec box is not checked... Ec is computed from f'c, compute Eci from f'ci
            method = 1;
         }
      }
      else
      {
         if ( m_ctrlEciCheck.GetCheck() == FALSE )// not checked
         {
            method = 1;
         }
         else
         {
            method = -1; // don't compute... it has user input
         }
      }
   }
   else
   {
      if ( m_ctrlEciCheck.GetCheck() == FALSE )
      {
         method = 1;
      }
      else
      {
         method = -1;
      }
   }

   if ( method == 0 )
   {
      // Eci is based on the user input value of Ec and not f'ci
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strEc;
      m_ctrlEc.GetWindowText(strEc);
      Float64 Ec;
      sysTokenizer::ParseDouble(strEc,&Ec);
      Ec = ::ConvertToSysUnits(Ec,pDisplayUnits->GetModEUnit().UnitOfMeasure);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      Float64 Eci;
      if ( m_TimeDependentModel == TDM_AASHTO || m_TimeDependentModel == TDM_ACI209 )
      {
         matACI209Concrete concrete;
         concrete.UserEc28(true);
         concrete.SetEc28(Ec);
         concrete.SetA(pParent->m_pSegment->Material.Concrete.A);
         concrete.SetBeta(pParent->m_pSegment->Material.Concrete.B);
         concrete.SetTimeAtCasting(0);
         concrete.SetFc28(pParent->m_pSegment->Material.Concrete.Fc);
         concrete.SetStrengthDensity(pParent->m_pSegment->Material.Concrete.StrengthDensity);
         Eci = concrete.GetEc(m_AgeAtRelease);
      }
      else
      {
         ATLASSERT(m_TimeDependentModel == TDM_CEBFIP);
         matCEBFIPConcrete concrete;
         concrete.UserEc28(true);
         concrete.SetEc28(Ec);
         concrete.SetTimeAtCasting(0);
         concrete.SetFc28(pParent->m_pSegment->Material.Concrete.Fc);
         concrete.SetStrengthDensity(pParent->m_pSegment->Material.Concrete.StrengthDensity);
         concrete.SetS(pParent->m_pSegment->Material.Concrete.S);
         concrete.SetBetaSc(pParent->m_pSegment->Material.Concrete.BetaSc);
         Eci = concrete.GetEc(m_AgeAtRelease);
      }

      CString strEci;
      strEci.Format(_T("%s"),FormatDimension(Eci,pDisplayUnits->GetModEUnit(),false));
      m_ctrlEci.SetWindowText(strEci);
   }
   else if ( method == 1)
   {
      // need to manually parse strength and density values
      CString strFci, strDensity, strK1, strK2;
      m_ctrlFci.GetWindowText(strFci);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_pSegment->Material.Concrete.StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_pSegment->Material.Concrete.EcK1);
      strK2.Format(_T("%f"),pParent->m_pSegment->Material.Concrete.EcK2);

      CString strEci = CConcreteDetailsDlg::UpdateEc(strFci,strDensity,strK1,strK2);
      m_ctrlEci.SetWindowText(strEci);
   }
}

void CGirderDescGeneralPage::UpdateEc()
{
   int method = 0;
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);

      if ( i == IDC_FC1 )
      {
         // concrete model is based on f'ci
         if ( m_ctrlEciCheck.GetCheck() == TRUE )
         {
            // Eci box is checked... user has input a value for Eci
            // Ec is based on the user input value of Eci and not f'c
            method = 0;
         }
         else
         {
            // Eci box is not checked... Eci is computed from f'ci, compute Ec from f'c
            method = 1;
         }
      }
      else
      {
         if (m_ctrlEcCheck.GetCheck() == FALSE) // not checked
         {
            method = 1;
         }
         else
         {
            method = -1; // don't compute.... it is user input
         }
      }
   }
   else
   {
      if ( m_ctrlEcCheck.GetCheck() == FALSE )
      {
         method = 1;
      }
      else
      {
         method = -1;
      }
   }

   if ( method == 0 )
   {
      // Ec is based on the user input value of Eci and not f'c
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strEci;
      m_ctrlEci.GetWindowText(strEci);
      Float64 Eci;
      sysTokenizer::ParseDouble(strEci,&Eci);
      Eci = ::ConvertToSysUnits(Eci,pDisplayUnits->GetModEUnit().UnitOfMeasure);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      Float64 Ec;
      if ( m_TimeDependentModel == TDM_AASHTO || m_TimeDependentModel == TDM_ACI209 )
      {
         Ec = matACI209Concrete::ComputeEc28(Eci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.A,pParent->m_pSegment->Material.Concrete.B);
      }
      else
      {
         ATLASSERT( m_TimeDependentModel == TDM_CEBFIP );
         Ec = matCEBFIPConcrete::ComputeEc28(Eci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.S);
      }

      CString strEc;
      strEc.Format(_T("%s"),FormatDimension(Ec,pDisplayUnits->GetModEUnit(),false));
      m_ctrlEc.SetWindowText(strEc);
   }
   else if (method == 1)
   {
      // need to manually parse strength and density values
      CString strFc, strDensity, strK1, strK2;
      m_ctrlFc.GetWindowText(strFc);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_pSegment->Material.Concrete.StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_pSegment->Material.Concrete.EcK1);
      strK2.Format(_T("%f"),pParent->m_pSegment->Material.Concrete.EcK2);

      CString strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1,strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}

void CGirderDescGeneralPage::UpdateFc()
{
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
      if ( i == IDC_FC1 )
      {
         // concrete model is based on f'ci... compute f'c
         // Get f'ci from edit control
         CString strFci;
         m_ctrlFci.GetWindowText(strFci);

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

         Float64 fci;
         sysTokenizer::ParseDouble(strFci, &fci);
         fci = ::ConvertToSysUnits(fci,pDisplayUnits->GetStressUnit().UnitOfMeasure);

         CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
         Float64 fc;

         if ( m_TimeDependentModel == TDM_AASHTO || m_TimeDependentModel == TDM_ACI209 )
         {
            fc = matACI209Concrete::ComputeFc28(fci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.A,pParent->m_pSegment->Material.Concrete.B);
         }
         else
         {
            ATLASSERT(m_TimeDependentModel == TDM_CEBFIP);
            fc = matCEBFIPConcrete::ComputeFc28(fci,m_AgeAtRelease,pParent->m_pSegment->Material.Concrete.S);
         }

         CString strFc;
         strFc.Format(_T("%s"),FormatDimension(fc,pDisplayUnits->GetStressUnit(),false));
         m_ctrlFc.SetWindowText(strFc);
      }
   }
}

void CGirderDescGeneralPage::UpdateFci()
{
   if ( m_LossMethod == pgsTypes::TIME_STEP )
   {
      int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
      if ( i == IDC_FC2 )
      {
         // concrete model is based on f'ci... compute f'c
         // Get f'c from edit control
         CString strFc;
         m_ctrlFc.GetWindowText(strFc);

         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

         Float64 fc;
         sysTokenizer::ParseDouble(strFc, &fc);
         fc = ::ConvertToSysUnits(fc,pDisplayUnits->GetStressUnit().UnitOfMeasure);

         CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

         Float64 fci;
         if ( m_TimeDependentModel == TDM_AASHTO || m_TimeDependentModel == TDM_ACI209 )
         {
            matACI209Concrete concrete;
            concrete.SetTimeAtCasting(0);
            concrete.SetFc28(fc);
            concrete.SetA(pParent->m_pSegment->Material.Concrete.A);
            concrete.SetBeta(pParent->m_pSegment->Material.Concrete.B);
            fci = concrete.GetFc(m_AgeAtRelease);
         }
         else
         {
            matCEBFIPConcrete concrete;
            concrete.SetTimeAtCasting(0);
            concrete.SetFc28(fc);
            concrete.SetS(pParent->m_pSegment->Material.Concrete.S);
            concrete.SetBetaSc(pParent->m_pSegment->Material.Concrete.BetaSc);
            fci = concrete.GetFc(m_AgeAtRelease);
         }

         CString strFci;
         strFci.Format(_T("%s"),FormatDimension(fci,pDisplayUnits->GetStressUnit(),false));
         m_ctrlFci.SetWindowText(strFci);
      }
   }
}

void CGirderDescGeneralPage::OnMoreConcreteProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CConcreteDetailsDlg dlg(true/*f'c*/);

   CDataExchange dx(this,TRUE);
   ExchangeConcreteData(&dx);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   dlg.m_fc28 = pParent->m_pSegment->Material.Concrete.Fc;
   dlg.m_Ec28 = pParent->m_pSegment->Material.Concrete.Ec;
   dlg.m_bUserEc28 = pParent->m_pSegment->Material.Concrete.bUserEc;

   dlg.m_General.m_Type        = pParent->m_pSegment->Material.Concrete.Type;
   dlg.m_General.m_AggSize     = pParent->m_pSegment->Material.Concrete.MaxAggregateSize;
   dlg.m_General.m_Ds          = pParent->m_pSegment->Material.Concrete.StrengthDensity;
   dlg.m_General.m_Dw          = pParent->m_pSegment->Material.Concrete.WeightDensity;

   dlg.m_AASHTO.m_EccK1       = pParent->m_pSegment->Material.Concrete.EcK1;
   dlg.m_AASHTO.m_EccK2       = pParent->m_pSegment->Material.Concrete.EcK2;
   dlg.m_AASHTO.m_CreepK1     = pParent->m_pSegment->Material.Concrete.CreepK1;
   dlg.m_AASHTO.m_CreepK2     = pParent->m_pSegment->Material.Concrete.CreepK2;
   dlg.m_AASHTO.m_ShrinkageK1 = pParent->m_pSegment->Material.Concrete.ShrinkageK1;
   dlg.m_AASHTO.m_ShrinkageK2 = pParent->m_pSegment->Material.Concrete.ShrinkageK2;
   dlg.m_AASHTO.m_bHasFct     = pParent->m_pSegment->Material.Concrete.bHasFct;
   dlg.m_AASHTO.m_Fct         = pParent->m_pSegment->Material.Concrete.Fct;

   dlg.m_ACI.m_bUserParameters = pParent->m_pSegment->Material.Concrete.bACIUserParameters;
   dlg.m_ACI.m_A               = pParent->m_pSegment->Material.Concrete.A;
   dlg.m_ACI.m_B               = pParent->m_pSegment->Material.Concrete.B;
   dlg.m_ACI.m_CureMethod      = pParent->m_pSegment->Material.Concrete.CureMethod;
   dlg.m_ACI.m_CementType      = pParent->m_pSegment->Material.Concrete.ACI209CementType;

   dlg.m_CEBFIP.m_bUserParameters = pParent->m_pSegment->Material.Concrete.bCEBFIPUserParameters;
   dlg.m_CEBFIP.m_S               = pParent->m_pSegment->Material.Concrete.S;
   dlg.m_CEBFIP.m_BetaSc          = pParent->m_pSegment->Material.Concrete.BetaSc;
   dlg.m_CEBFIP.m_CementType      = pParent->m_pSegment->Material.Concrete.CEBFIPCementType;

   dlg.m_General.m_strUserEc  = m_strUserEc;

   if ( dlg.DoModal() == IDOK )
   {
      pParent->m_pSegment->Material.Concrete.Fc               = dlg.m_fc28;
      pParent->m_pSegment->Material.Concrete.Ec               = dlg.m_Ec28;
      pParent->m_pSegment->Material.Concrete.bUserEc          = dlg.m_bUserEc28;

      pParent->m_pSegment->Material.Concrete.Type             = dlg.m_General.m_Type;
      pParent->m_pSegment->Material.Concrete.MaxAggregateSize = dlg.m_General.m_AggSize;
      pParent->m_pSegment->Material.Concrete.StrengthDensity  = dlg.m_General.m_Ds;
      pParent->m_pSegment->Material.Concrete.WeightDensity    = dlg.m_General.m_Dw;

      pParent->m_pSegment->Material.Concrete.EcK1             = dlg.m_AASHTO.m_EccK1;
      pParent->m_pSegment->Material.Concrete.EcK2             = dlg.m_AASHTO.m_EccK2;
      pParent->m_pSegment->Material.Concrete.CreepK1          = dlg.m_AASHTO.m_CreepK1;
      pParent->m_pSegment->Material.Concrete.CreepK2          = dlg.m_AASHTO.m_CreepK2;
      pParent->m_pSegment->Material.Concrete.ShrinkageK1      = dlg.m_AASHTO.m_ShrinkageK1;
      pParent->m_pSegment->Material.Concrete.ShrinkageK2      = dlg.m_AASHTO.m_ShrinkageK2;
      pParent->m_pSegment->Material.Concrete.bHasFct          = dlg.m_AASHTO.m_bHasFct;
      pParent->m_pSegment->Material.Concrete.Fct              = dlg.m_AASHTO.m_Fct;

      pParent->m_pSegment->Material.Concrete.bACIUserParameters = dlg.m_ACI.m_bUserParameters;
      pParent->m_pSegment->Material.Concrete.A                  = dlg.m_ACI.m_A;
      pParent->m_pSegment->Material.Concrete.B                  = dlg.m_ACI.m_B;
      pParent->m_pSegment->Material.Concrete.CureMethod         = dlg.m_ACI.m_CureMethod;
      pParent->m_pSegment->Material.Concrete.ACI209CementType   = dlg.m_ACI.m_CementType;

      pParent->m_pSegment->Material.Concrete.bCEBFIPUserParameters = dlg.m_CEBFIP.m_bUserParameters;
      pParent->m_pSegment->Material.Concrete.S                     = dlg.m_CEBFIP.m_S;
      pParent->m_pSegment->Material.Concrete.BetaSc                = dlg.m_CEBFIP.m_BetaSc;
      pParent->m_pSegment->Material.Concrete.CEBFIPCementType      = dlg.m_CEBFIP.m_CementType;

      m_strUserEc  = dlg.m_General.m_strUserEc;
      m_ctrlEc.SetWindowText(m_strUserEc);

      dx.m_bSaveAndValidate = FALSE;
      ExchangeConcreteData(&dx);

      UpdateFci();
      UpdateFc();
      UpdateEci();
      UpdateEc();

      UpdateConcreteControls();
   }
	
}

void CGirderDescGeneralPage::UpdateConcreteControls()
{
   if ( m_LossMethod == LOSSES_TIME_STEP )
   {
      int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
      INT idFci[5] = {IDC_FCI,       IDC_FCI_UNIT,       IDC_MOD_ECI, IDC_ECI, IDC_ECI_UNIT};
      INT idFc[5]  = {IDC_GIRDER_FC, IDC_GIRDER_FC_UNIT, IDC_MOD_EC,  IDC_EC,  IDC_EC_UNIT };

      BOOL bEnableFci = (i == IDC_FC1);

      for ( int j = 0; j < 5; j++ )
      {
         GetDlgItem(idFci[j])->EnableWindow(  bEnableFci );
         GetDlgItem(idFc[j] )->EnableWindow( !bEnableFci );
      }

      if ( i == IDC_FC1 ) // input based on f'ci
      {
         m_ctrlEciCheck.SetCheck(m_ctrlEcCheck.GetCheck());
         m_ctrlEcCheck.SetCheck(FALSE); // can't check Ec
      }

      if ( i == IDC_FC2 ) // input is based on f'ci
      {
         m_ctrlEcCheck.SetCheck(m_ctrlEciCheck.GetCheck());
         m_ctrlEciCheck.SetCheck(FALSE); // can't check Eci
      }
   }

   BOOL bEnable = m_ctrlEcCheck.GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   bEnable = m_ctrlEciCheck.GetCheck();
   GetDlgItem(IDC_ECI)->EnableWindow(bEnable);
   GetDlgItem(IDC_ECI_UNIT)->EnableWindow(bEnable);


   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   CString strLabel( ConcreteDescription(pParent->m_pSegment->Material.Concrete));
   GetDlgItem(IDC_CONCRETE_TYPE_LABEL)->SetWindowText( strLabel );
}


BOOL CGirderDescGeneralPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_MORE:
         UpdateConcreteParametersToolTip();
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = NULL;
      return TRUE;
   }
   return FALSE;
}

void CGirderDescGeneralPage::UpdateConcreteParametersToolTip()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   const unitmgtDensityData& density = pDisplayUnits->GetDensityUnit();
   const unitmgtLengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const unitmgtStressData&  stress  = pDisplayUnits->GetStressUnit();
   const unitmgtScalar&      scalar  = pDisplayUnits->GetScalarFormat();

   CString strTip;
   strTip.Format(_T("%-20s %s\r\n%-20s %s\r\n%-20s %s\r\n%-20s %s"),
      _T("Type"), matConcrete::GetTypeName((matConcrete::Type)pParent->m_pSegment->Material.Concrete.Type,true).c_str(),
      _T("Unit Weight"),FormatDimension(pParent->m_pSegment->Material.Concrete.StrengthDensity,density),
      _T("Unit Weight (w/ reinforcement)"),  FormatDimension(pParent->m_pSegment->Material.Concrete.WeightDensity,density),
      _T("Max Aggregate Size"),  FormatDimension(pParent->m_pSegment->Material.Concrete.MaxAggregateSize,aggsize)
      );

   if ( pParent->m_pSegment->Material.Concrete.Type != pgsTypes::Normal && pParent->m_pSegment->Material.Concrete.bHasFct )
   {
      CString strLWC;
      strLWC.Format(_T("\r\n%-20s %s"),
         _T("fct"),FormatDimension(pParent->m_pSegment->Material.Concrete.Fct,stress));

      strTip += strLWC;
   }

   CString strPress(_T("\r\n\r\nPress button to edit"));
   strTip += strPress;

   m_strTip = strTip;
}


void CGirderDescGeneralPage::FillGirderComboBox()
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   std::_tstring strGirderFamilyName = pBridgeDesc->GetGirderFamilyName();

   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames );
   std::vector<std::_tstring> names;
   std::vector<std::_tstring>::iterator iter;
   
   pLibNames->EnumGirderNames(strGirderFamilyName.c_str(), &names );
   for ( iter = names.begin(); iter < names.end(); iter++ )
   {
      std::_tstring& name = *iter;

      pCB->AddString( name.c_str() );
   }

   UpdateGirderTypeControls();
}

HBRUSH CGirderDescGeneralPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
	
   switch( pWnd->GetDlgCtrlID() )
   {
   case IDC_GIRDERNAME_NOTE:
   case IDC_SLABOFFSET_NOTE:
      pDC->SetTextColor(HYPERLINK_COLOR);
      break;
   };

   return hbr;
}

LRESULT CGirderDescGeneralPage::OnChangeSameGirderType(WPARAM wParam,LPARAM lParam)
{
   m_bUseSameGirderType = !m_bUseSameGirderType;
   UpdateGirderTypeHyperLink();
   UpdateGirderTypeControls();

   return 0;
}

void CGirderDescGeneralPage::UpdateGirderTypeControls()
{
   GetDlgItem(IDC_GIRDER_NAME)->EnableWindow( m_bUseSameGirderType ? FALSE : TRUE );
}

LRESULT CGirderDescGeneralPage::OnChangeSlabOffsetType(WPARAM wParam,LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsTypes::SlabOffsetType temp = m_SlabOffsetType;
   m_SlabOffsetType = m_SlabOffsetTypeCache;
   m_SlabOffsetTypeCache = temp;

   UpdateSlabOffsetHyperLink();
   UpdateSlabOffsetControls();

   CWnd* pwndStart = GetDlgItem(IDC_ADIM_START);
   CWnd* pwndEnd   = GetDlgItem(IDC_ADIM_END);
   if ( m_SlabOffsetType == pgsTypes::sotGirder )
   {
      // going into girder by girder slab offset mode
      CString strTempStart = m_strSlabOffsetCache[pgsTypes::metStart];
      CString strTempEnd   = m_strSlabOffsetCache[pgsTypes::metEnd];

      pwndStart->GetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      pwndEnd->GetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);

      pwndStart->SetWindowText(strTempStart);
      pwndEnd->SetWindowText(strTempEnd);
   }
   else if ( m_SlabOffsetType == pgsTypes::sotPier )
   {
      //pwndStart->SetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      //pwndEnd->SetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);
   }
   else
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 slabOffset[2];
      CDataExchange dx(this,TRUE);
      DDX_UnitValueAndTag(&dx, IDC_ADIM_START,IDC_ADIM_START_UNIT, slabOffset[pgsTypes::metStart], pDisplayUnits->GetComponentDimUnit());
      DDX_UnitValueAndTag(&dx, IDC_ADIM_END,  IDC_ADIM_END_UNIT,   slabOffset[pgsTypes::metEnd],   pDisplayUnits->GetComponentDimUnit());

      Float64 slab_offset = slabOffset[pgsTypes::metStart];

      if ( !IsEqual(slabOffset[pgsTypes::metStart],slabOffset[pgsTypes::metEnd]) )
      {
         // going to a single slab offset for the entire bridge, but the current start and end are different
         // make the user choose one
         CSelectItemDlg dlg;
         dlg.m_ItemIdx = 0;
         dlg.m_strTitle = _T("Select Slab Offset");
         dlg.m_strLabel = _T("A single slab offset will be used for the entire bridge. Select a value.");
         
         CString strItems;
         strItems.Format(_T("Start of Span (%s)\nEnd of Span (%s)"),
                         ::FormatDimension(slabOffset[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit()),
                         ::FormatDimension(slabOffset[pgsTypes::metEnd],  pDisplayUnits->GetComponentDimUnit()));

         dlg.m_strItems = strItems;
         if ( dlg.DoModal() == IDOK )
         {
            if ( dlg.m_ItemIdx == 0 )
               slab_offset = slabOffset[pgsTypes::metStart];
            else
               slab_offset = slabOffset[pgsTypes::metEnd];
         }
         else
         {
            return 0;
         }
      }

      GetDlgItem(IDC_ADIM_START)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metStart]);
      GetDlgItem(IDC_ADIM_END)->GetWindowText(m_strSlabOffsetCache[pgsTypes::metEnd]);

      GetDlgItem(IDC_ADIM_START)->SetWindowText( ::FormatDimension(slab_offset,pDisplayUnits->GetComponentDimUnit(),false) );
      GetDlgItem(IDC_ADIM_END)->SetWindowText( ::FormatDimension(slab_offset,pDisplayUnits->GetComponentDimUnit(),false) );
   }

   return 0;
}

void CGirderDescGeneralPage::UpdateSlabOffsetControls()
{
   // Enable/Disable Slab Offset controls
   BOOL bEnable = (m_SlabOffsetType == pgsTypes::sotGirder ? TRUE : FALSE);

   GetDlgItem(IDC_ADIM_START_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_START)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_START_UNIT)->EnableWindow(bEnable);

   GetDlgItem(IDC_ADIM_END_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_END)->EnableWindow(bEnable);
   GetDlgItem(IDC_ADIM_END_UNIT)->EnableWindow(bEnable);
}

void CGirderDescGeneralPage::UpdateGirderTypeHyperLink()
{
   if ( m_bUseSameGirderType )
   {
      // girder name is shared with the entire bridge
      m_GirderTypeHyperLink.SetWindowText(_T("This girder type is used for the entire bridge"));
      m_GirderTypeHyperLink.SetURL(_T("Click to change the type of this girder"));
   }
   else
   {
      m_GirderTypeHyperLink.SetWindowText(_T("This girder type is assigned to this girder"));
      m_GirderTypeHyperLink.SetURL(_T("Click to use this girder type for the entire bridge"));
   }
}

void CGirderDescGeneralPage::UpdateSlabOffsetHyperLink()
{
   if ( m_SlabOffsetType == pgsTypes::sotGirder )
   {
      // slab offset is by girder
      m_SlabOffsetHyperLink.SetWindowText(_T("Slab Offsets are defined girder by girder"));
      if ( m_SlabOffsetTypeCache == pgsTypes::sotBridge )
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this Slab Offset for the entire bridge"));
      else
         m_SlabOffsetHyperLink.SetURL(_T("Click to use this Slab Offset for this span"));
   }
   else if ( m_SlabOffsetType == pgsTypes::sotBridge )
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("A single Slab Offset is used for the entire bridge"));
      m_SlabOffsetHyperLink.SetURL(_T("Click to define Slab Offsets by girder"));
   }
   else
   {
      m_SlabOffsetHyperLink.SetWindowText(_T("A unique Slab Offset is used in each span"));
      m_SlabOffsetHyperLink.SetURL(_T("Click to define Slab Offsets by girder"));
   }
}

void CGirderDescGeneralPage::OnBeforeChangeGirderName()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);
   m_GirderNameIdx = pCB->GetCurSel();
}

void CGirderDescGeneralPage::OnChangeGirderName()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_NAME);
   int result = AfxMessageBox(_T("Changing the girder type will reset the strands, stirrups, and longitudinal rebar to default values.\n\nIs that OK?"),MB_YESNO);
   if ( result == IDNO )
   {
      pCB->SetCurSel(m_GirderNameIdx);
      return;
   }

   CString newName;
   pCB->GetWindowText(newName);

   CGirderDescDlg* pParent = (CGirderDescDlg*)GetParent();
   pParent->m_strGirderName = newName;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibrary, pLib );
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(newName);

   // reset prestress data
   pParent->m_pSegment->Strands.ResetPrestressData();

   // Must change adjustable strand type if it is incompatible with library
   pgsTypes::AdjustableStrandType libAdjType = pGdrEntry->GetAdjustableStrandType();
   pgsTypes::AdjustableStrandType adjType = pParent->m_pSegment->Strands.GetAdjustableStrandType();
   if (adjType == pgsTypes::asHarped && libAdjType == pgsTypes::asStraight)
   {
      pParent->m_pSegment->Strands.SetAdjustableStrandType(pgsTypes::asStraight);
   }
   else if (adjType == pgsTypes::asStraight && libAdjType == pgsTypes::asHarped)
   {
      pParent->m_pSegment->Strands.SetAdjustableStrandType(pgsTypes::asHarped);
   }

   // reset stirrups to library
   pParent->m_Shear.m_CurGrdName = newName;
   pParent->m_Shear.DoRestoreDefaults();

   pParent->m_LongRebar.m_CurGrdName = newName;
   pParent->m_LongRebar.RestoreToLibraryDefaults();

   // add/remove property pages if needed
   GET_IFACE2( pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   bool bCanExtendStrands = pSpecEntry->AllowStraightStrandExtensions();
   bool bCanDebond = pGdrEntry->CanDebondStraightStrands();

   pParent->OnGirderTypeChanged(bCanExtendStrands,bCanDebond);
}

void CGirderDescGeneralPage::OnConcreteStrength()
{
   UpdateConcreteControls();
}

void CGirderDescGeneralPage::OnConditionFactorTypeChanged()
{
   CEdit* pEdit = (CEdit*)GetDlgItem(IDC_CONDITION_FACTOR);
   CComboBox* pcbConditionFactor = (CComboBox*)GetDlgItem(IDC_CONDITION_FACTOR_TYPE);

   int idx = pcbConditionFactor->GetCurSel();
   switch(idx)
   {
   case 0:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("1.00"));
      break;
   case 1:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.95"));
      break;
   case 2:
      pEdit->EnableWindow(FALSE);
      pEdit->SetWindowText(_T("0.85"));
      break;
   case 3:
      pEdit->EnableWindow(TRUE);
      break;
   }
}
