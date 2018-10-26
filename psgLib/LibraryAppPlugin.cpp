#include "stdafx.h"
#include "PGSuperLibrary_i.h"
#include "LibraryAppPlugin.h"

#include "resource.h"

#include <PsgLib\LibraryDocTemplate.h>
#include <PsgLib\LibraryEditorDoc.h>
#include <PsgLib\LibChildFrm.h>
#include <PsgLib\LibraryEditorView.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFMainFrame.h>

#include "PGSuperLibraryMgrCATID.h"

BEGIN_MESSAGE_MAP(CMyCmdTarget,CCmdTarget)
   ON_COMMAND(ID_MANAGE_PLUGINS,OnManagePlugins)
END_MESSAGE_MAP()

void CMyCmdTarget::OnManagePlugins()
{
   m_pMyAppPlugin->ManagePlugins();
}

///////////////////////////////////////////////////////////////////
void CLibraryAppPlugin::ManagePlugins()
{
   EAFManagePlugins(_T("Manage Library Editor Plugins"),CATID_PGSuperLibraryManagerPlugin,EAFGetMainFrame());
}

BOOL CLibraryAppPlugin::Init(CEAFApp* pParent)
{
   return TRUE;
}

void CLibraryAppPlugin::Terminate()
{
}

void CLibraryAppPlugin::IntegrateWithUI(BOOL bIntegrate)
{
   CEAFMainFrame* pFrame = EAFGetMainFrame();
   CEAFMenu* pMainMenu = pFrame->GetMainMenu();

   UINT filePos = pMainMenu->FindMenuItem("&File");
   CEAFMenu* pFileMenu = pMainMenu->GetSubMenu(filePos);

   UINT managePos = pFileMenu->FindMenuItem("Manage");
   CEAFMenu* pManageMenu = pFileMenu->GetSubMenu(managePos);

   if ( bIntegrate )
   {
      pManageMenu->AppendMenu(ID_MANAGE_PLUGINS,"Manage Library Editor Plugins...",this);
   }
   else
   {
      pManageMenu->RemoveMenu(ID_MANAGE_PLUGINS,  MF_BYCOMMAND, this);
   }
}

CEAFDocTemplate* CLibraryAppPlugin::CreateDocTemplate()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   // doc template for Library editor
	CLibraryDocTemplate* pLibMgrDocTemplate = new CLibraryDocTemplate(
		IDR_LIBRARYTYPE,this,
		RUNTIME_CLASS(CLibraryEditorDoc),
		RUNTIME_CLASS(CLibChildFrame),
		RUNTIME_CLASS(CLibraryEditorView));

   pLibMgrDocTemplate->SetPlugin(this);

   HICON hIcon = AfxGetApp()->LoadIcon(IDI_LIBRARY_MANAGER);
   pLibMgrDocTemplate->CreateDefaultItem(hIcon);

   return pLibMgrDocTemplate;
}

HMENU CLibraryAppPlugin::GetSharedMenuHandle()
{
   return NULL;
}

UINT CLibraryAppPlugin::GetDocumentResourceID()
{
   return IDR_LIBRARYTYPE;
}

CString CLibraryAppPlugin::GetName()
{
   return CString("PGSuper Library Editor");
}

//////////////////////////
// IEAFCommandCallback
BOOL CLibraryAppPlugin::OnCommandMessage(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   return m_MyCmdTarget.OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);
}

BOOL CLibraryAppPlugin::GetStatusBarMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // load appropriate string
	if ( rMessage.LoadString(nID) )
	{
		// first newline terminates actual string
      rMessage.Replace('\n','\0');
	}
	else
	{
		// not found
		TRACE1("Warning: no message line prompt for ID %d.\n", nID);
	}

   return TRUE;
}

BOOL CLibraryAppPlugin::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString string;
   // load appropriate string
	if ( string.LoadString(nID) )
	{
		// tip is after first newline 
      int pos = string.Find('\n');
      if ( 0 < pos )
         rMessage = string.Mid(pos+1);
	}
	else
	{
		// not found
		TRACE1("Warning: no tool tip for ID %d.\n", nID);
	}

   return TRUE;
}

