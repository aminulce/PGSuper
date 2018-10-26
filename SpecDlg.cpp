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

// SpecDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "SpecDlg.h"

#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecDlg dialog


CSpecDlg::CSpecDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpecDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSpecDlg)
	//}}AFX_DATA_INIT
	m_Spec = _T("");
}


void CSpecDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecDlg)
	//}}AFX_DATA_MAP
	DDX_CBStringExactCase(pDX, IDC_SPEC, m_Spec);
}


BEGIN_MESSAGE_MAP(CSpecDlg, CDialog)
	//{{AFX_MSG_MAP(CSpecDlg)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDlg message handlers

BOOL CSpecDlg::OnInitDialog() 
{
   CComboBox* pBox = (CComboBox*)GetDlgItem( IDC_SPEC );
   ASSERT( pBox );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool bIsPGSplice = pDocType->IsPGSpliceDocument();

   GET_IFACE2(pBroker, ILibrary, pLib);
   GET_IFACE2(pBroker, ILibraryNames, pLibNames );

   std::vector<std::_tstring> specs;
   pLibNames->EnumSpecNames( &specs );

   BOOST_FOREACH(const std::_tstring& spec,specs)
   {
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec.c_str() );
      if ( (bIsPGSplice && pSpecEntry->GetLossMethod() != LOSSES_TIME_STEP) ) 
      {
         // only project criteria using time-step losses can be used with spliced girders
         continue;
      }
      pBox->AddString( spec.c_str() );
   }

   CDialog::OnInitDialog();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_DESIGNCRITERIA );
}
