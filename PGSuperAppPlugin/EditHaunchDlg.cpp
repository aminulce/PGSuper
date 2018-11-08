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

// EditHaunchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "EditHaunchDlg.h"

#include <EAF\EAFMainFrame.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include "PGSuperUnits.h"

#include <PgsExt\BridgeDescription2.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CEditHaunchDlg, CDialog)

CEditHaunchDlg::CEditHaunchDlg(const CBridgeDescription2* pBridgeDesc, CWnd* pParent /*=nullptr*/)
	: CDialog(CEditHaunchDlg::IDD, pParent),
   m_pBridgeDesc(pBridgeDesc),
   m_HaunchShape(pgsTypes::hsSquare),
   m_WasSlabOffsetTypeForced(false),
   m_WasAssExcessCamberTypeForced(false),
   m_WasDataIntialized(false)
{
   m_HaunchInputData.m_SlabOffsetType = pgsTypes::sotBridge;
   m_HaunchInputData.m_AssExcessCamberType = pgsTypes::aecBridge;
}

CEditHaunchDlg::~CEditHaunchDlg()
{
}

void CEditHaunchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );

   // fillet
   DDX_UnitValueAndTag( pDX, IDC_FILLET, IDC_FILLET_UNIT, m_Fillet, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore( pDX, IDC_FILLET, m_Fillet, pDisplayUnits->GetComponentDimUnit() );

   DDX_Control(pDX,IDC_HAUNCH_SHAPE,m_cbHaunchShape);
   DDX_CBItemData(pDX, IDC_HAUNCH_SHAPE, m_HaunchShape);

   DDX_CBItemData(pDX, IDC_A_TYPE, m_HaunchInputData.m_SlabOffsetType);
   DDX_CBItemData(pDX, IDC_ASSEXCESSCAMBER_TYPE, m_HaunchInputData.m_AssExcessCamberType);

   if (pDX->m_bSaveAndValidate)
   {
      // Get min A value and build error message for too small of A
      Float64 minA = m_pBridgeDesc->GetDeckDescription()->GrossDepth;
      if (pgsTypes::sdtCompositeSIP == m_pBridgeDesc->GetDeckDescription()->GetDeckType())
      {
         minA += m_pBridgeDesc->GetDeckDescription()->PanelDepth;
      }

      cmpdim.SetValue(minA);
      CString strMinValError;
      strMinValError.Format(_T("Slab Offset value must be greater or equal to slab depth (%.4f %s)"), cmpdim.GetValue(true), cmpdim.GetUnitTag().c_str() );

      // Slab offsets
      switch(m_HaunchInputData.m_SlabOffsetType)
      {
      case pgsTypes::sotBridge:
         m_HaunchSame4BridgeDlg.DownloadData(minA,strMinValError,&m_HaunchInputData,pDX);
         break;
      case pgsTypes::sotPier:
         m_HaunchSpanBySpanDlg.DownloadData(minA,strMinValError,&m_HaunchInputData,pDX);
         break;
      case pgsTypes::sotGirder:
         m_HaunchByGirderDlg.DownloadData(minA,strMinValError,&m_HaunchInputData,pDX);
         break;
      default:
         ATLASSERT(0);
         break;
      };

      // AssExcessCambers
      if (m_bCanAssExcessCamberInputBeEnabled)
      {
         switch (m_HaunchInputData.m_AssExcessCamberType)
         {
         case pgsTypes::aecBridge:
            m_AssExcessCamberSame4BridgeDlg.DownloadData(&m_HaunchInputData, pDX);
         break;
         case pgsTypes::aecSpan:
            m_AssExcessCamberSpanBySpanDlg.DownloadData(&m_HaunchInputData, pDX);
         break;
         case pgsTypes::aecGirder:
            m_AssExcessCamberByGirderDlg.DownloadData(&m_HaunchInputData, pDX);
         break;
      default:
         ATLASSERT(0);
         break;
      };
      }
   }
   else
   {
      // Set data values in embedded dialogs
      m_HaunchSame4BridgeDlg.m_ADim = m_HaunchInputData.m_SingleSlabOffset;
      m_HaunchSame4BridgeDlg.UpdateData(FALSE);
      m_HaunchSpanBySpanDlg.UploadData(this->m_HaunchInputData);
      m_HaunchByGirderDlg.UploadData(this->m_HaunchInputData);

      if (m_bCanAssExcessCamberInputBeEnabled)
      {
         m_AssExcessCamberSame4BridgeDlg.m_AssExcessCamber = m_HaunchInputData.m_SingleAssExcessCamber;
      }
      else
      {
         // disable controls in dialog
         m_AssExcessCamberSame4BridgeDlg.m_bDoEnable = false;
      }

      m_AssExcessCamberSame4BridgeDlg.UpdateData(FALSE);

      m_AssExcessCamberSpanBySpanDlg.UploadData(this->m_HaunchInputData);
      m_AssExcessCamberByGirderDlg.UploadData(this->m_HaunchInputData);
   }
}


BEGIN_MESSAGE_MAP(CEditHaunchDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_A_TYPE, &CEditHaunchDlg::OnCbnSelchangeAType)
   ON_CBN_SELCHANGE(IDC_ASSEXCESSCAMBER_TYPE, &CEditHaunchDlg::OnCbnSelchangeAssExcessCamberType)
   ON_BN_CLICKED(ID_HELP, &CEditHaunchDlg::OnBnClickedHelp)
END_MESSAGE_MAP()


// CEditHaunchDlg message handlers

BOOL CEditHaunchDlg::OnInitDialog()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISpecification, pSpec );
   m_bCanAssExcessCamberInputBeEnabled = pSpec->IsAssExcessCamberInputEnabled();

   // Initialize our data structure for current data
   InitializeData();

   // Embed dialogs for into current. A discription may be found at
   // http://www.codeproject.com/KB/dialog/embedded_dialog.aspx

   // Set up embedded dialogs
   {
      CWnd* pBox = GetDlgItem(IDC_A_BOX);
      pBox->ShowWindow(SW_HIDE);

      CRect boxRect;
      pBox->GetWindowRect(&boxRect);
      ScreenToClient(boxRect);

      VERIFY(m_HaunchSame4BridgeDlg.Create(CHaunchSame4BridgeDlg::IDD, this));
      VERIFY(m_HaunchSame4BridgeDlg.SetWindowPos(GetDlgItem(IDC_A_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      VERIFY(m_HaunchSpanBySpanDlg.Create(CHaunchSpanBySpanDlg::IDD, this));
      VERIFY(m_HaunchSpanBySpanDlg.SetWindowPos(GetDlgItem(IDC_A_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      m_HaunchByGirderDlg.InitSize(m_HaunchInputData);
      VERIFY(m_HaunchByGirderDlg.Create(CHaunchByGirderDlg::IDD, this));
      VERIFY(m_HaunchByGirderDlg.SetWindowPos(GetDlgItem(IDC_A_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      pBox = GetDlgItem(IDC_ASSEXCESSCAMBER_BOX);
      pBox->ShowWindow(SW_HIDE);

      pBox->GetWindowRect(&boxRect);
      ScreenToClient(boxRect);

      VERIFY(m_AssExcessCamberSame4BridgeDlg.Create(CAssExcessCamberSame4BridgeDlg::IDD, this));
      VERIFY(m_AssExcessCamberSame4BridgeDlg.SetWindowPos(GetDlgItem(IDC_ASSEXCESSCAMBER_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      VERIFY(m_AssExcessCamberSpanBySpanDlg.Create(CAssExcessCamberSpanBySpanDlg::IDD, this));
      VERIFY(m_AssExcessCamberSpanBySpanDlg.SetWindowPos(GetDlgItem(IDC_ASSEXCESSCAMBER_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));

      m_AssExcessCamberByGirderDlg.InitSize(m_HaunchInputData);
      VERIFY(m_AssExcessCamberByGirderDlg.Create(CAssExcessCamberByGirderDlg::IDD, this));
      VERIFY(m_AssExcessCamberByGirderDlg.SetWindowPos(GetDlgItem(IDC_ASSEXCESSCAMBER_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE));//|SWP_NOMOVE));
   }

   // Slab offset type combo
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_A_TYPE);
   int sqidx = pBox->AddString( SlabOffsetTypeAsString(pgsTypes::sotBridge));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotBridge);
   sqidx = pBox->AddString( SlabOffsetTypeAsString(pgsTypes::sotPier));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotPier);
   sqidx = pBox->AddString( SlabOffsetTypeAsString(pgsTypes::sotGirder));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::sotGirder);

   // AssExcessCamber type combo
   pBox =(CComboBox*)GetDlgItem(IDC_ASSEXCESSCAMBER_TYPE);
   sqidx = pBox->AddString( AssExcessCamberTypeAsString(pgsTypes::aecBridge));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::aecBridge);
   sqidx = pBox->AddString( AssExcessCamberTypeAsString(pgsTypes::aecSpan));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::aecSpan);
   sqidx = pBox->AddString( AssExcessCamberTypeAsString(pgsTypes::aecGirder));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::aecGirder);

   if (!m_bCanAssExcessCamberInputBeEnabled)
   {
      pBox->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSEXCESSCAMBER_GROUP)->EnableWindow(FALSE);
   }

   CDialog::OnInitDialog();

   OnCbnSelchangeAType();
   OnCbnSelchangeAssExcessCamberType();

   m_cbHaunchShape.Initialize(m_HaunchShape);
   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_HAUNCH_SHAPE,m_HaunchShape);

   if ( IsAdjacentSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
   {
      m_cbHaunchShape.EnableWindow(FALSE); // cannot change haunch shape for adjacent spacing
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditHaunchDlg::InitializeData()
{
   m_WasDataIntialized = true;

   const CDeckDescription2* pDeck = m_pBridgeDesc->GetDeckDescription();
   ATLASSERT(pDeck->GetDeckType()!= pgsTypes::sdtNone); // should not be able to edit haunch if no deck

   // Take data from project and fill our local data structures
   // Fill all slab offset types with values depending on initial type
   m_HaunchInputData.m_BearingsSlabOffset.clear();
   m_HaunchInputData.m_MaxGirdersPerSpan = 0;

   // Bridge-wide data
   ///////////////////////////////////
   m_Fillet = m_pBridgeDesc->GetFillet();

   // If girder spacing is adjacent, force haunch shape to square
   if ( IsAdjacentSpacing(m_pBridgeDesc->GetGirderSpacingType()) )
   {
      m_HaunchShape = pgsTypes::hsSquare;
   }
   else
   {
      m_HaunchShape = pDeck->HaunchShape;
   }

   m_HaunchInputData.m_SlabOffsetType = m_pBridgeDesc->GetSlabOffsetType();

   // Pier and girder based A data are treated the same for all types
   PierIndexType npiers = m_pBridgeDesc->GetPierCount();

   bool bFirst(true);
   for (PierIndexType ipier=0; ipier<npiers; ipier++)
   {
      const CPierData2* pPier = m_pBridgeDesc->GetPier(ipier);

      // We want to iterate over bearing lines. Determine how many
      pgsTypes::PierFaceType pierFaces[2];
      PierIndexType nbrglines = 1; 
      if (ipier==0)
      {
         ATLASSERT(pPier->IsAbutment());
         nbrglines = 1;
         pierFaces[0] = pgsTypes::Ahead;
      }
      else if (ipier==npiers-1)
      {
         ATLASSERT(pPier->IsAbutment());
         nbrglines = 1;
         pierFaces[0] = pgsTypes::Back;
      }
      else if (pPier->IsBoundaryPier())
      {
         nbrglines = 2;
         pierFaces[0] = pgsTypes::Back;
         pierFaces[1] = pgsTypes::Ahead;
      }
      else
      {
         ATLASSERT(pPier->IsInteriorPier());
         nbrglines = 1;
         pierFaces[0] = pgsTypes::Back;
      }

      for (PierIndexType ibrg=0; ibrg<nbrglines; ibrg++)
      {
         const CGirderGroupData* pGroup = pPier->GetGirderGroup(pierFaces[ibrg]);

         SlabOffsetBearingData brgData;

         // Save pier and group data to make updating bridge description easier
         brgData.m_PierIndex   = ipier;
         brgData.m_pGroupIndex = pGroup->GetIndex();

         if (!pPier->IsAbutment() && nbrglines==1)
         {
            // continuous interior pier
            brgData.m_PDType = SlabOffsetBearingData::pdCL;
         }
         else
         {
            brgData.m_PDType = pgsTypes::Back==pierFaces[ibrg] ? SlabOffsetBearingData::pdBack :  SlabOffsetBearingData::pdAhead;
         }

         GirderIndexType ng = pGroup->GetGirderCount();
         m_HaunchInputData.m_MaxGirdersPerSpan = max(m_HaunchInputData.m_MaxGirdersPerSpan, ng);

         for (GirderIndexType ig=0; ig<ng; ig++)
         {
            Float64 A = pGroup->GetSlabOffset(ipier, ig);
            brgData.m_AsForGirders.push_back(A);

            // Fill data for case when single A is used for entire bridge. Use bearingline 1, girder 1
            if (ipier==0 && ig==0)
            {
               m_HaunchInputData.m_SingleSlabOffset  = A;
            }
         }

         m_HaunchInputData.m_BearingsSlabOffset.push_back( brgData );
      }
   }

   // See if haunch fill type was forced. If so, set it
   if (m_WasSlabOffsetTypeForced)
   {
      m_HaunchInputData.m_SlabOffsetType = m_ForcedSlabOffsetType;
   }

   // Now AssExcessCambers
   /////////////////////////////////////////////

   m_HaunchInputData.m_AssExcessCamberSpans.clear();
   m_HaunchInputData.m_AssExcessCamberType = m_bCanAssExcessCamberInputBeEnabled ? m_pBridgeDesc->GetAssExcessCamberType() : pgsTypes::aecBridge;

   // Pier and girder based A data are treated the same for all types
   SpanIndexType nspans = m_pBridgeDesc->GetSpanCount();

   bFirst = true;
   for (SpanIndexType ispan = 0; ispan < nspans; ispan++)
   {
      const CSpanData2* pSpan = m_pBridgeDesc->GetSpan(ispan);

      AssExcessCamberSpanData fsData;
      fsData.m_SpanIndex = ispan;

      GirderIndexType ng = pSpan->GetGirderCount();
      for (GirderIndexType ig = 0; ig < ng; ig++)
      {
         Float64 AssExcessCamber = pSpan->GetAssExcessCamber(ig);
         fsData.m_AssExcessCambersForGirders.push_back(AssExcessCamber);

         // Fill data for case when single A is used for entire bridge. Use bearingline 1, girder 1
         if (ispan == 0 && ig == 0)
         {
            m_HaunchInputData.m_SingleAssExcessCamber = AssExcessCamber;
         }
      }

      m_HaunchInputData.m_AssExcessCamberSpans.push_back(fsData);
   }

   // See if haunch fill type was forced. If so, set it
   if (m_WasAssExcessCamberTypeForced)
   {
      m_HaunchInputData.m_AssExcessCamberType = m_ForcedAssExcessCamberType;
   }
}

void CEditHaunchDlg::OnCbnSelchangeAType()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_A_TYPE);
   
   m_HaunchInputData.m_SlabOffsetType = (pgsTypes::SlabOffsetType)pBox->GetCurSel();

   switch(m_HaunchInputData.m_SlabOffsetType)
   {
   case pgsTypes::sotBridge:
      m_HaunchSame4BridgeDlg.ShowWindow(SW_SHOW);
      m_HaunchSpanBySpanDlg.ShowWindow(SW_HIDE);
      m_HaunchByGirderDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::sotPier:
      m_HaunchSame4BridgeDlg.ShowWindow(SW_HIDE);
      m_HaunchSpanBySpanDlg.ShowWindow(SW_SHOW);
      m_HaunchByGirderDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::sotGirder:
      m_HaunchSame4BridgeDlg.ShowWindow(SW_HIDE);
      m_HaunchSpanBySpanDlg.ShowWindow(SW_HIDE);
      m_HaunchByGirderDlg.ShowWindow(SW_SHOW);
      break;
   default:
      ATLASSERT(0);
      break;
   };
}

void CEditHaunchDlg::OnCbnSelchangeAssExcessCamberType()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_ASSEXCESSCAMBER_TYPE);
   
   m_HaunchInputData.m_AssExcessCamberType = (pgsTypes::AssExcessCamberType)pBox->GetCurSel();

   switch(m_HaunchInputData.m_AssExcessCamberType)
   {
   case pgsTypes::aecBridge:
      m_AssExcessCamberSame4BridgeDlg.ShowWindow(SW_SHOW);
      m_AssExcessCamberSpanBySpanDlg.ShowWindow(SW_HIDE);
      m_AssExcessCamberByGirderDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::aecSpan:
      m_AssExcessCamberSame4BridgeDlg.ShowWindow(SW_HIDE);
      m_AssExcessCamberSpanBySpanDlg.ShowWindow(SW_SHOW);
      m_AssExcessCamberByGirderDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::aecGirder:
      m_AssExcessCamberSame4BridgeDlg.ShowWindow(SW_HIDE);
      m_AssExcessCamberSpanBySpanDlg.ShowWindow(SW_HIDE);
      m_AssExcessCamberByGirderDlg.ShowWindow(SW_SHOW);
      break;
   default:
      ATLASSERT(0);
      break;
   };
}

void CEditHaunchDlg::ForceToSlabOffsetType(pgsTypes::SlabOffsetType slabOffsetType)
{
   m_WasSlabOffsetTypeForced = true;
   m_ForcedSlabOffsetType = slabOffsetType;
   m_WasDataIntialized = false;
}

void CEditHaunchDlg::ForceToAssExcessCamberType(pgsTypes::AssExcessCamberType AssExcessCamberType)
{
   m_WasAssExcessCamberTypeForced = true;
   m_ForcedAssExcessCamberType = AssExcessCamberType;
   m_WasDataIntialized = false;
}

void CEditHaunchDlg::ModifyBridgeDescr(CBridgeDescription2* pBridgeDesc)
{
   if (!m_WasDataIntialized)
   {
      InitializeData();
   }

   // Haunch shape
   pBridgeDesc->GetDeckDescription()->HaunchShape = m_HaunchShape;

   pBridgeDesc->SetFillet(m_Fillet);

   // Slab offset
   if (m_HaunchInputData.m_SlabOffsetType == pgsTypes::sotBridge)
   {
      pBridgeDesc->SetSlabOffsetType(pgsTypes::sotBridge);
      pBridgeDesc->SetSlabOffset(m_HaunchInputData.m_SingleSlabOffset);
   }
   else if (m_HaunchInputData.m_SlabOffsetType == pgsTypes::sotPier)
   {
      pBridgeDesc->SetSlabOffsetType(pgsTypes::sotPier);

      // loop over each bearing line and set A
      for(SlabOffsetBearingDataConstIter hdit=m_HaunchInputData.m_BearingsSlabOffset.begin(); hdit!=m_HaunchInputData.m_BearingsSlabOffset.end(); hdit++)
      {
         const SlabOffsetBearingData& hbd = *hdit;
         
         CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(hbd.m_pGroupIndex);

         // use A from slot[0] for all girders
         Float64 A = hbd.m_AsForGirders[0];

         pGroup->SetSlabOffset(hbd.m_PierIndex, A);
      }
   }
   else if (m_HaunchInputData.m_SlabOffsetType == pgsTypes::sotGirder)
   {
      pBridgeDesc->SetSlabOffsetType(pgsTypes::sotGirder);

      // loop over each bearing line / girder and set A
      for(SlabOffsetBearingDataConstIter hdit=m_HaunchInputData.m_BearingsSlabOffset.begin(); hdit!=m_HaunchInputData.m_BearingsSlabOffset.end(); hdit++)
      {
         const SlabOffsetBearingData& hbd = *hdit;
         
         CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(hbd.m_pGroupIndex);

         GirderIndexType ng = hbd.m_AsForGirders.size();
         for (GirderIndexType ig=0; ig<ng; ig++)
         {
            pGroup->SetSlabOffset(hbd.m_PierIndex, ig, hbd.m_AsForGirders[ig]);
         }
      }
   }

   // AssExcessCamber
   if (m_bCanAssExcessCamberInputBeEnabled)
   {
      if (m_HaunchInputData.m_AssExcessCamberType == pgsTypes::aecBridge)
   {
         pBridgeDesc->SetAssExcessCamberType(pgsTypes::aecBridge);
         pBridgeDesc->SetAssExcessCamber(m_HaunchInputData.m_SingleAssExcessCamber);
   }
      else if (m_HaunchInputData.m_AssExcessCamberType == pgsTypes::aecSpan)
   {
         pBridgeDesc->SetAssExcessCamberType(pgsTypes::aecSpan);

      // loop over each bearing line and set A
         for (AssExcessCamberSpanDataConstIter hdit = m_HaunchInputData.m_AssExcessCamberSpans.begin(); hdit != m_HaunchInputData.m_AssExcessCamberSpans.end(); hdit++)
      {
            const AssExcessCamberSpanData& hbd = *hdit;
         
         CSpanData2* pSpan = pBridgeDesc->GetSpan(hbd.m_SpanIndex);

            // use AssExcessCamber from slot[0] for all girders
            Float64 AssExcessCamber = hbd.m_AssExcessCambersForGirders[0];

            pSpan->SetAssExcessCamber(AssExcessCamber);
      }
   }
      else if (m_HaunchInputData.m_AssExcessCamberType == pgsTypes::aecGirder)
   {
         pBridgeDesc->SetAssExcessCamberType(pgsTypes::aecGirder);

      // loop over each bearing line / girder and set A
         for (AssExcessCamberSpanDataConstIter hdit = m_HaunchInputData.m_AssExcessCamberSpans.begin(); hdit != m_HaunchInputData.m_AssExcessCamberSpans.end(); hdit++)
      {
            const AssExcessCamberSpanData& hbd = *hdit;
         
         CSpanData2* pSpan = pBridgeDesc->GetSpan(hbd.m_SpanIndex);

            GirderIndexType ng = hbd.m_AssExcessCambersForGirders.size();
            for (GirderIndexType ig = 0; ig < ng; ig++)
         {
               pSpan->SetAssExcessCamber(ig, hbd.m_AssExcessCambersForGirders[ig]);
            }
         }
      }
   }
}

void CEditHaunchDlg::OnBnClickedHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_HAUNCH);
}
