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

// PGSpliceDoc.h : interface of the CPGSpliceDoc class
//
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include "PGSuperDocBase.h"
#include "PGSuperPluginMgr.h"
#include "PGSpliceCatCom.h"

#include "PGSComponentInfo.h"
typedef CEAFPluginManager<IPGSpliceComponentInfo,CPGSpliceDoc> CPGSpliceComponentInfoManager;

#define ETSD_GENERAL       0
#define ETSD_CONNECTION    1

class CPGSpliceDoc : public CPGSuperDocBase
{
protected: // create from serialization only
	CPGSpliceDoc();
	DECLARE_DYNCREATE(CPGSpliceDoc)


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSpliceDoc)
	//}}AFX_VIRTUAL

   bool EditClosurePourDescription(const CClosurePourData* pClosure,int nPage);
   bool EditGirderSegmentDescription(const CSegmentKey& segmentKey,int nPage);
   bool EditGirderDescription(const CGirderKey& girderKey,int nPage);
   bool EditTemporarySupportDescription(SupportIDType tsID,int nPage);
   void DeleteTemporarySupport(SupportIDType tsID);

// Implementation
public:
	virtual ~CPGSpliceDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

   virtual UINT GetStandardToolbarResourceID();

   CPGSpliceComponentInfoManager* GetComponentInfoManager() { return &m_ComponentInfoManager; }

// Generated message map functions
protected:
   BOOL Init();

   CPGSpliceComponentInfoManager m_ComponentInfoManager;

	//{{AFX_MSG(CPGSpliceDoc)
	afx_msg void OnDeleteSelection();
	afx_msg void OnUpdateDeleteSelection(CCmdUI* pCmdUI);
	afx_msg void OnEditBridgeDescription();
   afx_msg void OnEditSegment(UINT nID);
   afx_msg void OnEditClosurePour();
   afx_msg void OnUpdateEditClosurePour(CCmdUI* pCmdUI);
   afx_msg void OnEditGirder();
   afx_msg void OnEditGirderline();
   afx_msg void OnEditTemporarySupport();
   afx_msg void OnUpdateEditTemporarySupport(CCmdUI* pCmdUI);
   afx_msg void OnInsertTemporarySupport();
   afx_msg void OnDeleteTemporarySupport();
   afx_msg void OnEditTimeline();
   //}}AFX_MSG
   afx_msg BOOL OnEditGirderDropDown(NMHDR* pnmhdr,LRESULT* plr);
   virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

   virtual void DoIntegrateWithUI(BOOL bIntegrate);

   virtual CPGSuperPluginMgrBase* CreatePluginManager() { return new CPGSplicePluginMgr(); }

   virtual CATID GetAgentCategoryID() { return CATID_PGSpliceAgent; }
   virtual CATID GetExtensionAgentCategoryID() { return CATID_PGSpliceExtensionAgent; }
   virtual CATID GetBeamFamilyCategoryID() { return CATID_PGSpliceBeamFamily; }
   virtual CATID GetComponentInfoCategoryID() { return CATID_PGSpliceComponentInfo; }

   virtual LPCTSTR GetTemplateExtension();

   virtual BOOL InitMainMenu();

   DECLARE_MESSAGE_MAP()
};
