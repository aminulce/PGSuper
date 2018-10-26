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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSImportPluginDocTemplateBase.h"
#include "SelectItemDlg.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CMyTemplateItem : public CEAFTemplateItem
{
public:
   CMyTemplateItem(CEAFDocTemplate* pDocTemplate,LPCTSTR name,LPCTSTR path,HICON hIcon,IPGSProjectImporter* pImporter) :
      CEAFTemplateItem(pDocTemplate,name,path,hIcon)
      {
         m_Importer = pImporter;
      }

   virtual CEAFTemplateItem* Clone() const
   {
      CMyTemplateItem* pClone = new CMyTemplateItem(m_pDocTemplate,m_Name,m_Path,m_hIcon,m_Importer);
      return pClone;
   }

   CComPtr<IPGSProjectImporter> m_Importer;
   DECLARE_DYNAMIC(CMyTemplateItem)
};

IMPLEMENT_DYNAMIC(CMyTemplateItem,CEAFTemplateItem)


IMPLEMENT_DYNAMIC(CPGSImportPluginDocTemplateBase,CEAFDocTemplate)

CPGSImportPluginDocTemplateBase::CPGSImportPluginDocTemplateBase(UINT nIDResource,
                                                                 IEAFCommandCallback* pCallback,
  																                 CRuntimeClass* pDocClass,
  																                 CRuntimeClass* pFrameClass,
 																                 CRuntimeClass* pViewClass,
                                                                 HMENU hSharedMenu,
                                                                 int maxViewCount) :
CEAFDocTemplate(nIDResource,pCallback,pDocClass,pFrameClass,pViewClass,hSharedMenu,maxViewCount)
{
   USES_CONVERSION;

   m_pProjectImporterMgr = NULL;
}

CPGSImportPluginDocTemplateBase::~CPGSImportPluginDocTemplateBase()
{
   GetProjectImporterManager()->UnloadImporters();
   delete m_pProjectImporterMgr;
}

CDocTemplate::Confidence CPGSImportPluginDocTemplateBase::MatchDocType(LPCTSTR lpszPathName,CDocument*& rpDocMatch)
{
   // This document template doesn't open any document files from disk
   // return noAttempt so that the document manager does not conclude this is the
   // best match
   return CDocTemplate::noAttempt;
}

BOOL CPGSImportPluginDocTemplateBase::DoOpenDocumentFile(LPCTSTR lpszPathName,BOOL bMakeVisible,CEAFDocument* pDocument,CFrameWnd* pFrame)
{
   // Importers "open" documents completely different then the default base class method
   // Don't call the base class version of this method
   // CEAFDocTemplate::DoOpenDocumentFile(lpszPathName,bMakeVisible,pDocument,pFrame);

   // Creating a new document
   ASSERT_KINDOF(CMyTemplateItem,m_pTemplateItem);
   CMyTemplateItem* pTemplateItem = (CMyTemplateItem*)m_pTemplateItem;

   // create a new document - with default document name
	SetDefaultTitle(pDocument);

	// avoid creating temporary compound file when starting up invisible
	if (!bMakeVisible)
   {
		pDocument->m_bEmbedded = TRUE;
   }

	if (!pDocument->OnNewDocument())
	{
		// user has be alerted to what failed in OnNewDocument
		TRACE(traceAppMsg, 0, "CPGSImportPluginDocTemplateBase::OnNewDocument returned FALSE.\n");
      return FALSE;
	}

   // Hold the UI events (release in CPGSuperDoc::OnCreateFinalize)
   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   ATLASSERT(broker != NULL);
   GET_IFACE2(broker,IEvents,pEvents);
   GET_IFACE2(broker,IUIEvents,pUIEvents);
   pEvents->HoldEvents();
   pUIEvents->HoldEvents();

   // do the importing
   try
   {
      if ( FAILED(pTemplateItem->m_Importer->Import(broker)) )
      {
         return FALSE;
      }
   }
   catch(...)
   {
   }

   // it worked, now bump untitled count (for untitled documents... from MFC for multidoc applications)
	m_nUntitledCount++;

   // Can't release events here because the views have not yet been created
   // Events are released in CPGSuperDoc::OnCreateFinalize

   return TRUE;
}

CString CPGSImportPluginDocTemplateBase::GetTemplateGroupItemDescription(const CEAFTemplateItem* pItem) const
{
   CString strName = pItem->GetName();
   CString strDescription;
   strDescription.Format(_T("Create a new %s project using the %s importer"),GetProjectImporterManager()->GetAppName(),strName);
   return strDescription;
}

BOOL CPGSImportPluginDocTemplateBase::GetDocString(CString& rString,enum DocStringIndex index) const
{
   if ( index == CDocTemplate::fileNewName )
   {
      rString.Format(_T("%s Project Importers"),GetProjectImporterManager()->GetAppName());
      return TRUE;
   }
   else if ( index == CDocTemplate::filterExt || index == CDocTemplate::filterName )
   {
      rString.Empty();
      return TRUE;
   }

   return CEAFDocTemplate::GetDocString(rString,index);
}

CPGSProjectImporterMgrBase* CPGSImportPluginDocTemplateBase::GetProjectImporterManager() const
{
   if ( m_pProjectImporterMgr == NULL )
   {
      m_pProjectImporterMgr = CreateProjectImporterMgr();

      m_pProjectImporterMgr->LoadImporters();

      CollectionIndexType nImporters = m_pProjectImporterMgr->GetImporterCount();
      for ( CollectionIndexType idx = 0; idx < nImporters; idx++ )
      {
         CComPtr<IPGSProjectImporter> importer;
         m_pProjectImporterMgr->GetImporter(idx,&importer);

         CComBSTR bstrText;
         importer->GetItemText(&bstrText);

         HICON hIcon;
         importer->GetIcon(&hIcon);
         m_TemplateGroup.AddItem( new CMyTemplateItem((CEAFDocTemplate*)this,OLE2T(bstrText),NULL,hIcon,importer) );
      }
   }

   return m_pProjectImporterMgr;
}
