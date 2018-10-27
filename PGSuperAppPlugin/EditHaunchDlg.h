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

#pragma once

#include "HaunchSpanGrid.h"
#include "HaunchSame4BridgeDlg.h"
#include "HaunchSpanBySpanDlg.h"
#include "HaunchByGirderDlg.h"
#include "AssExcessCamberSame4BridgeDlg.h"
#include "AssExcessCamberSpanBySpanDlg.h"
#include "AssExcessCamberByGirderDlg.h"
#include <PgsExt\HaunchShapeComboBox.h>

class CBridgeDescription2;

inline CString SlabOffsetTypeAsString(pgsTypes::SlabOffsetType type)
{
   if (type == pgsTypes::sotBridge)
   {
      return _T("Define single slab offset for entire Bridge");
   }
   else if (type == pgsTypes::sotPier)
   {
      return _T("Define unique slab offsets for each Pier");
   }
   else if (type == pgsTypes::sotGirder)
   {
      return _T("Define unique slab offsets for each Girder");
   }
   else
   {
      ATLASSERT(0);
      return _T("Error, bad haunch type");
   }
}

inline CString AssExcessCamberTypeAsString(pgsTypes::AssExcessCamberType type)
{
   if (type == pgsTypes::aecBridge)
   {
      return _T("Define single Assumed Excess Camber for entire Bridge");
   }
   else if (type == pgsTypes::aecSpan)
   {
      return _T("Define unique Assumed Excess Cambers for each Span");
   }
   else if (type == pgsTypes::aecGirder)
   {
      return _T("Define unique Assumed Excess Cambers for each Girder");
   }
   else
   {
      ATLASSERT(0);
      return _T("Error, bad camber type");
   }
}

// CEditHaunchDlg dialog

class CEditHaunchDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditHaunchDlg)

public:
   // constructor - holds on to bridge description while dialog is active
	CEditHaunchDlg(const CBridgeDescription2* pBridgeDesc, CWnd* pParent = nullptr);
	virtual ~CEditHaunchDlg();

// Dialog Data
	enum { IDD = IDD_EDIT_HAUNCH };

// embedded dialogs for different haunch layouts
   CHaunchSame4BridgeDlg m_HaunchSame4BridgeDlg;
   CHaunchSpanBySpanDlg  m_HaunchSpanBySpanDlg;
   CHaunchByGirderDlg    m_HaunchByGirderDlg;

// embedded dialogs for different AssExcessCamber layouts
   CAssExcessCamberSame4BridgeDlg m_AssExcessCamberSame4BridgeDlg;
   CAssExcessCamberSpanBySpanDlg  m_AssExcessCamberSpanBySpanDlg;
   CAssExcessCamberByGirderDlg    m_AssExcessCamberByGirderDlg;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnCbnSelchangeAType();
   afx_msg void OnCbnSelchangeAssExcessCamberType();

   // Force to another type than that in bridge descripton. Must be called before DoModal
   void ForceToSlabOffsetType(pgsTypes::SlabOffsetType slabOffsetType);
   void ForceToAssExcessCamberType(pgsTypes::AssExcessCamberType AssExcessCamberType);

   // Change bridge description to our haunch data
   void ModifyBridgeDescr(CBridgeDescription2* pBridgeDesc);

private:
   const CBridgeDescription2* m_pBridgeDesc;

   // Data for haunch input
   HaunchInputData m_HaunchInputData;

   Float64 m_Fillet;

   pgsTypes::HaunchShapeType m_HaunchShape;
   CHaunchShapeComboBox m_cbHaunchShape;

   bool m_WasSlabOffsetTypeForced;
   pgsTypes::SlabOffsetType m_ForcedSlabOffsetType;
   bool m_WasAssExcessCamberTypeForced;
   pgsTypes::AssExcessCamberType m_ForcedAssExcessCamberType;
   bool m_WasDataIntialized;
   bool m_bCanAssExcessCamberInputBeEnabled;

   void InitializeData();
public:
   afx_msg void OnBnClickedHelp();
};