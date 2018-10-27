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

// PGSuperDocBase.cpp : implementation of the CPGSDocBase class
//
#include "PGSuperAppPlugin\stdafx.h"

#include "PGSuperAppPlugin\PGSuperApp.h"

#include <BridgeLink.h>

#include <WBFLDManip.h>
#include <WBFLDManipTools.h>

#include <objbase.h>
#include <initguid.h>

#include "PGSuperDocBase.h"
#include "PGSuperUnits.h"
#include "PGSuperBaseAppPlugin.h"
#include "PGSuperDocTemplateBase.h"

#include "BridgeSectionView.h"
#include "BridgePlanView.h"

#include <WBFLCore_i.c>
#include <WBFLReportManagerAgent_i.c>
#include <WBFLGraphManagerAgent_i.c>
#include <WBFLTools_i.c>
#include <WBFLUnitServer_i.c>
#include <WBFLGeometry_i.c>
#include <WBFLCogo_i.c>
#include <WBFLDManip_i.c>
#include <WBFLDManipTools_i.c>

#include <PGSuperAppPlugin.h>
#include <PGSuperProjectImporterAppPlugin.h>

#include <PsgLib\PsgLib.h>
#include <PsgLib\BeamFamilyManager.h>

#include <IFace\Test1250.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\Artifact.h>
#include <IFace\TxDOTCadExport.h>
#include <IFace\Transactions.h>
#include <IFace\EditByUI.h>
#include <IFace\VersionInfo.h>
#include <IFace\Project.h>
#include <IFace\Alignment.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PrestressForce.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Allowables.h>
#include <IFace\StatusCenter.h>
#include <IFace\RatingSpecification.h>
#include <IFace\DistributionFactors.h>
#include <IFace\Intervals.h>
#include <EAF\EAFUIIntegration.h>
#include "PGSuperDocProxyAgent.h"

#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>


#include "SupportDrawStrategy.h"
#include "SectionCutDrawStrategy.h"
#include "PointLoadDrawStrategy.h"
#include "DistributedLoadDrawStrategy.h"
#include "MomentLoadDrawStrategy.h"
#include "GevEditLoad.h"

#include "Convert.h"

#include <ComCat.h>

#include "BridgeLinkCATID.h"

#include "Hints.h"

#include "PGSuperException.h"
#include <System\FileStream.h>
#include <System\StructuredLoadXmlPrs.h>

#include <MFCTools\AutoRegistry.h>

// Helpers
#include <MathEx.h>

// Agents
#include "PGSuperDocProxyAgent.h"

// Dialogs
#include "PGSuperAppPlugin\AboutDlg.h"
#include "ProjectPropertiesDlg.h"
#include "EnvironmentDlg.h"
#include "BridgeDescDlg.h"
#include "SpecDlg.h"
#include "BridgeEditorSettingsSheet.h"
#include "GirderEditorSettingsSheet.h"
#include "CopyGirderDlg.h"
#include "LiveLoadDistFactorsDlg.h"
#include "LiveLoadSelectDlg.h"
#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"
#include "AlignmentDescriptionDlg.h"
#include "SelectItemDlg.h"
#include "SpanDetailsDlg.h"
#include "PierDetailsDlg.h"
#include "GirderLabelFormatDlg.h"
#include "RatingOptionsDlg.h"
#include "ConstructionLoadDlg.h"
#include "GirderSelectStrandsDlg.h"
#include "PGSuperAppPlugin\GirderSegmentStrandsPage.h"
#include "PGSuperAppPlugin\InsertSpanDlg.h"
#include "PGSuperAppPlugin\LoadFactorsDlg.h"
#include "PGSuperAppPlugin\LossParametersDlg.h"
#include "PGSuperAppPlugin\EditTimelineDlg.h"
#include "PGSuperAppPlugin\EditHaunchDlg.h"

#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpanGirderReportSpecification.h>

#include <EAF\EAFAutoProgress.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\GirderDesignArtifact.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\DesignConfigUtil.h>



// Transactions
#include <PgsExt\EditBridge.h>
#include "EditAlignment.h"
#include "EditPier.h"
#include "EditSpan.h"
#include "InsertDeleteSpan.h"
#include "EditLLDF.h"
#include "EditEnvironment.h"
#include "EditProjectCriteria.h"
#include "EditRatingCriteria.h"
#include "EditLiveLoad.h"
#include "EditConstructionLoad.h"
#include <PgsExt\InsertDeleteLoad.h>
#include "EditEffectiveFlangeWidth.h"
#include "EditLoadFactors.h"
#include "EditLoadModifiers.h"
#include "PGSuperAppPlugin\EditLossParameters.h"
#include "PGSuperAppPlugin\EditPrecastSegment.h"
#include "EditProjectProperties.h"
#include "PGSuperAppPlugin\EditTimeline.h"

// Logging
#include <iostream>
#include <fstream>
#include <System\Time.h>

#include <algorithm>

#include <PgsExt\StatusItem.h>
#include <PgsExt\MacroTxn.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// cause the resource control values to be defined
#define APSTUDIO_INVOKED
#undef APSTUDIO_READONLY_SYMBOLS

#include "resource.h"       // main symbols 

#define PGSUPER_PLUGIN_COMMAND_COUNT 256

static const Float64 FILE_VERSION = 3.0;

static bool DoesFolderExist(const CString& dirname);
static bool DoesFileExist(const CString& filname);

#pragma Reminder("UPDATE: UpdatePrestressForce should be part of the strand editing dialogs")
// Function to update prestress force after editing strands
static void UpdatePrestressForce(pgsTypes::StrandType type, const CSegmentKey& segmentKey,
                                 CPrecastSegmentData& newSegmentData,const CPrecastSegmentData& oldSegmentData, 
                                 IPretensionForce* pPrestress)
{

      // If going from no strands - always compute pjack automatically
      if(newSegmentData.Strands.IsPjackCalculated(type) ||
         (0 == oldSegmentData.Strands.GetStrandCount(type) &&
          0 < newSegmentData.Strands.GetStrandCount(type)))
      {
         newSegmentData.Strands.IsPjackCalculated(type,true);
         newSegmentData.Strands.SetPjack(type, pPrestress->GetPjackMax(segmentKey, 
                                                                 *(newSegmentData.Strands.GetStrandMaterial(type)),
                                                                 newSegmentData.Strands.GetStrandCount(type)));
      }
}

/////////////////////////////////////////////////////////////////////////////
// CPGSDocBase

BEGIN_MESSAGE_MAP(CPGSDocBase, CEAFBrokerDocument)
	//{{AFX_MSG_MAP(CPGSDocBase)
	ON_COMMAND(ID_FILE_PROJECT_PROPERTIES, OnFileProjectProperties)
	ON_COMMAND(ID_PROJECT_ENVIRONMENT, OnProjectEnvironment)
   ON_COMMAND(ID_PROJECT_EFFECTIVEFLANGEWIDTH, OnEffectiveFlangeWidth)
	ON_COMMAND(ID_PROJECT_SPEC, OnProjectSpec)
	ON_COMMAND(ID_RATING_SPEC,  OnRatingSpec)
	ON_UPDATE_COMMAND_UI(EAFID_TOGGLE_AUTOCALC, OnUpdateAutoCalc)
	ON_COMMAND(IDM_EXPORT_TEMPLATE, OnExportToTemplateFile)
	ON_COMMAND(ID_VIEWSETTINGS_BRIDGEMODELEDITOR, OnViewsettingsBridgemodelEditor)
	ON_COMMAND(ID_LOADS_LOADMODIFIERS, OnLoadsLoadModifiers)
   ON_COMMAND(ID_LOADS_LOADFACTORS, OnLoadsLoadFactors)
	ON_COMMAND(ID_VIEWSETTINGS_GIRDEREDITOR, OnViewsettingsGirderEditor)
	ON_COMMAND(IDM_COPY_GIRDER_PROPS, OnCopyGirderProps)
	ON_COMMAND(IDM_IMPORT_PROJECT_LIBRARY, OnImportProjectLibrary)
	ON_COMMAND(ID_ADD_POINT_LOAD, OnAddPointload)
	ON_COMMAND(ID_ADD_DISTRIBUTED_LOAD, OnAddDistributedLoad)
	ON_COMMAND(ID_ADD_MOMENT_LOAD, OnAddMomentLoad)
   ON_COMMAND(ID_CONSTRUCTION_LOADS,OnConstructionLoads)
   ON_COMMAND(ID_PROJECT_ALIGNMENT, OnProjectAlignment)
   ON_COMMAND(ID_PROJECT_BARRIER, OnProjectBarriers)
   ON_COMMAND(ID_PROJECT_PROFILE, OnProjectProfile)
	ON_COMMAND(ID_PROJECT_PIERDESC, OnEditPier)
	ON_COMMAND(ID_PROJECT_SPANDESC, OnEditSpan)
	ON_COMMAND(ID_DELETE, OnDeleteSelection)
	ON_UPDATE_COMMAND_UI(ID_DELETE, OnUpdateDeleteSelection)
	ON_COMMAND(ID_LOADS_LLDF, OnLoadsLldf)
   ON_COMMAND(ID_LIVE_LOADS,OnLiveLoads)
	ON_COMMAND(ID_INSERT, OnInsert)
	ON_COMMAND(ID_OPTIONS_LABELS, OnOptionsLabels)
   ON_COMMAND(ID_PROJECT_LOSSES,OnLosses)
   ON_COMMAND(ID_EDIT_TIMELINE,OnEditTimeline)

   ON_COMMAND(ID_VIEW_BRIDGEMODELEDITOR, OnViewBridgeModelEditor)
   ON_COMMAND(ID_VIEW_GIRDEREDITOR, OnViewGirderEditor)
   ON_COMMAND(ID_VIEW_LIBRARYEDITOR, OnViewLibraryEditor)

	ON_COMMAND(ID_EDIT_USERLOADS, OnEditUserLoads)
	ON_COMMAND(ID_EDIT_HAUNCH, OnEditHaunch)
   ON_UPDATE_COMMAND_UI(ID_EDIT_HAUNCH,OnUpdateEditHaunch)
   //}}AFX_MSG_MAP

   // autocalc command implementations
   ON_COMMAND(EAFID_TOGGLE_AUTOCALC, OnAutoCalc)
   ON_UPDATE_COMMAND_UI(EAFID_TOGGLE_AUTOCALC,OnUpdateAutoCalc)
   ON_COMMAND(EAFID_AUTOCALC_UPDATENOW, OnUpdateNow)
	ON_UPDATE_COMMAND_UI(EAFID_AUTOCALC_UPDATENOW, OnUpdateUpdateNow)

	ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
	ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileSendMail)


   ON_UPDATE_COMMAND_UI_RANGE(EAFID_VIEW_STATUSCENTER, EAFID_VIEW_STATUSCENTER3, CEAFBrokerDocument::OnUpdateViewStatusCenter)
	ON_COMMAND_RANGE(EAFID_VIEW_STATUSCENTER, EAFID_VIEW_STATUSCENTER3,OnViewStatusCenter)

   ON_UPDATE_COMMAND_UI(ID_VIEW_REPORTS,OnUpdateViewReports)
   ON_UPDATE_COMMAND_UI(ID_VIEW_GRAPHS,OnUpdateViewGraphs)

	ON_UPDATE_COMMAND_UI(FIRST_DATA_IMPORTER_PLUGIN, OnImportMenu)
   ON_COMMAND_RANGE(FIRST_DATA_IMPORTER_PLUGIN,LAST_DATA_IMPORTER_PLUGIN, OnImport)
	ON_UPDATE_COMMAND_UI(FIRST_DATA_EXPORTER_PLUGIN, OnExportMenu)
   ON_COMMAND_RANGE(FIRST_DATA_EXPORTER_PLUGIN,LAST_DATA_EXPORTER_PLUGIN, OnExport)

   ON_COMMAND(ID_HELP_ABOUT, OnAbout)
   ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)

   // this doesn't work for documents... see OnCmdMsg for handling of WM_NOTIFY
   //ON_NOTIFY(TBN_DROPDOWN,ID_STDTOOLBAR,OnViewReports)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPGSDocBase construction/destruction

CPGSDocBase::CPGSDocBase():
m_DesignSlabOffset(sodAandAssExcessCamber),
m_bAutoCalcEnabled(true)
{
	EnableAutomation();

	AfxOleLockApp();

   m_Selection.Type       = CSelection::None;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   m_LibMgr.SetName( _T("PGSuper Library") );

   CEAFAutoCalcDocMixin::SetDocument(this);

   m_bShowProjectProperties = true;

   m_pPGSuperDocProxyAgent = nullptr;

   m_pPluginMgr = nullptr;

   m_CallbackID = 0;

   SetCustomReportHelpID(eafTypes::crhCustomReport,IDH_CUSTOM_REPORT);
   SetCustomReportHelpID(eafTypes::crhFavoriteReport,IDH_FAVORITE_REPORT);
   SetCustomReportDefinitionHelpID(IDH_CUSTOM_REPORT_DEFINITION);


   // Reserve a range of command IDs for extension agent commands (which are currently supported)
   // and EAFDocumentPlugin objects (which are not currently supported in PGSuper)
   UINT nCommands = GetPluginCommandManager()->ReserveCommandIDRange(PGSUPER_PLUGIN_COMMAND_COUNT);
   ATLASSERT(nCommands == PGSUPER_PLUGIN_COMMAND_COUNT);

   m_bSelectingGirder = false;
   m_bSelectingSegment = false;
   m_bClearingSelection = false;

   txnTxnManager::SetTransactionManagerFactory(&m_TxnMgrFactory);
}

CPGSDocBase::~CPGSDocBase()
{
   m_DocUnitSystem.Release();
   m_pPluginMgr->UnloadPlugins();
   delete m_pPluginMgr;
   m_pPluginMgr = nullptr;
   AfxOleUnlockApp();
}

// CEAFAutoCalcDocMixin overrides
bool CPGSDocBase::IsAutoCalcEnabled() const
{
   return m_bAutoCalcEnabled;
}

void CPGSDocBase::EnableAutoCalc(bool bEnable)
{
   if ( m_bAutoCalcEnabled != bEnable )
   {
      bool bWasDisabled = !IsAutoCalcEnabled();
      m_bAutoCalcEnabled = bEnable;

      CPGSuperStatusBar* pStatusBar = ((CPGSuperStatusBar*)EAFGetMainFrame()->GetStatusBar());
      pStatusBar->AutoCalcEnabled( m_bAutoCalcEnabled );

      // If AutoCalc was off and now it is on,
      // Update the views.
      if ( bWasDisabled && IsAutoCalcEnabled() )
      {
        OnUpdateNow();
      }
   }
}

void CPGSDocBase::OnUpdateNow()
{
   CEAFAutoCalcDocMixin::OnUpdateNow();
}

void CPGSDocBase::OnUpdateUpdateNow(CCmdUI* pCmdUI)
{
   CEAFAutoCalcDocMixin::OnUpdateUpdateNow(pCmdUI);
}

void CPGSDocBase::OnViewStatusCenter(UINT nID)
{
   CEAFBrokerDocument::OnViewStatusCenter();
}

void CPGSDocBase::OnLibMgrChanged(psgLibraryManager* pNewLibMgr)
{
   GET_IFACE( ILibrary, pLib );
   pLib->SetLibraryManager( pNewLibMgr );
}

// libISupportLibraryManager implementation
CollectionIndexType CPGSDocBase::GetNumberOfLibraryManagers() const
{
   return 1;
}

libLibraryManager* CPGSDocBase::GetLibraryManager(CollectionIndexType num)
{
   PRECONDITION( num == 0 );
   return &m_LibMgr;
}

libLibraryManager* CPGSDocBase::GetTargetLibraryManager()
{
   return &m_LibMgr;
}

void CPGSDocBase::GetDocUnitSystem(IDocUnitSystem** ppDocUnitSystem)
{
   (*ppDocUnitSystem) = m_DocUnitSystem;
   (*ppDocUnitSystem)->AddRef();
}

void CPGSDocBase::EditAlignmentDescription(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IRoadwayData,pAlignment);
   CAlignmentDescriptionDlg dlg(_T("Alignment Description"),m_pBroker);

   dlg.m_AlignmentPage.m_AlignmentData = pAlignment->GetAlignmentData2();
   dlg.m_ProfilePage.m_ProfileData = pAlignment->GetProfileData2();
   dlg.m_CrownSlopePage.m_RoadwaySectionData = pAlignment->GetRoadwaySectionData();

   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      txnEditAlignment* pTxn = new txnEditAlignment(pAlignment->GetAlignmentData2(),     dlg.m_AlignmentPage.m_AlignmentData,
                                                    pAlignment->GetProfileData2(),       dlg.m_ProfilePage.m_ProfileData,
                                                    pAlignment->GetRoadwaySectionData(), dlg.m_CrownSlopePage.m_RoadwaySectionData );

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSDocBase::EditBridgeDescription(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IEnvironment, pEnvironment );

   const CBridgeDescription2* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   enumExposureCondition oldExposureCondition = pEnvironment->GetExposureCondition();
   Float64 oldRelHumidity = pEnvironment->GetRelHumidity();

   CBridgeDescDlg dlg(*pOldBridgeDesc);

   dlg.m_EnvironmentalPage.m_Exposure    = oldExposureCondition == expNormal ? 0 : 1;
   dlg.m_EnvironmentalPage.m_RelHumidity = oldRelHumidity;

   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {

      txnTransaction* pTxn = new txnEditBridge(*pOldBridgeDesc,      dlg.GetBridgeDescription(),
                                              oldExposureCondition, dlg.m_EnvironmentalPage.m_Exposure == 0 ? expNormal : expSevere,
                                              oldRelHumidity,       dlg.m_EnvironmentalPage.m_RelHumidity);


      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         txnMacroTxn* pMacro = new pgsMacroTxn;
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(pTxn);
         pMacro->AddTransaction(pExtensionTxn);
         pTxn = pMacro;
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSDocBase::EditPierDescription(PierIndexType pierIdx, int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // This dialog makes a copy of the bridge model because it changes it.
   // If the user presses the Cancel button, we don't have to figure out
   // what got changed.
   CPierDetailsDlg dlg(pBridgeDesc,pierIdx);
   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      txnTransaction* pTxn = new txnEditPier(pierIdx,*pBridgeDesc,*dlg.GetBridgeDescription());
      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn != nullptr )
      {
         txnMacroTxn* pMacro = new pgsMacroTxn;
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(pTxn);
         pMacro->AddTransaction(pExtensionTxn);
         pTxn = pMacro;
      }

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }

   return true;
}

bool CPGSDocBase::EditSpanDescription(SpanIndexType spanIdx, int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // This dialog makes a copy of the bridge model because it changes it.
   // If the user presses the Cancel button, we don't have to figure out
   // what got changed.
   CSpanDetailsDlg dlg(pBridgeDesc,spanIdx);
   dlg.SetActivePage(nPage);

   if ( dlg.DoModal() == IDOK )
   {
      txnTransaction* pTxn = new txnEditSpan(spanIdx,*pBridgeDesc,*dlg.GetBridgeDescription());

      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();
      if ( pExtensionTxn )
      {
         txnMacroTxn* pMacro = new pgsMacroTxn;
         pMacro->Name(pTxn->Name());
         pMacro->AddTransaction(pTxn);
         pMacro->AddTransaction(pExtensionTxn);
         pTxn = pMacro;
      }
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }

   return true;
}

void CPGSDocBase::OnEditHaunch() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CEditHaunchDlg dlg(pOldBridgeDesc);
   if ( dlg.DoModal() == IDOK )
   {
      GET_IFACE(IEnvironment, pEnvironment );
      enumExposureCondition oldExposureCondition = pEnvironment->GetExposureCondition();
      Float64 oldRelHumidity = pEnvironment->GetRelHumidity();
      CBridgeDescription2 newBridgeDesc = *pOldBridgeDesc;

      // dialog modifies descr
      dlg.ModifyBridgeDescr(&newBridgeDesc);

      txnTransaction* pTxn = new txnEditBridge(*pOldBridgeDesc,     newBridgeDesc,
                                              oldExposureCondition, oldExposureCondition, 
                                              oldRelHumidity,       oldRelHumidity);


      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSDocBase::OnUpdateEditHaunch(CCmdUI* pCmdUI)
{
   GET_IFACE_NOCHECK(IBridge,pBridge);
   pCmdUI->Enable( pBridge->GetDeckType()==pgsTypes::sdtNone ? FALSE : TRUE );
}

bool CPGSDocBase::EditDirectSelectionPrestressing(const CSegmentKey& segmentKey)
{
   // this method doesn't apply to PGSplice, however it must be accessable to the PGSuperDocProxyAgent
   // that is why it is in this class and not in CPGSuperDoc
   ATLASSERT(IsKindOf(RUNTIME_CLASS(CPGSuperDoc)));

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE(IStrandGeometry,pStrandGeometry);

   const CBridgeDescription2* pBridgeDesc  = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager*    pTimelineMgr = pBridgeDesc->GetTimelineManager();
   const CPrecastSegmentData* pSegment     = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

   txnEditPrecastSegmentData oldSegmentData;
   oldSegmentData.m_SegmentKey  = segmentKey;
   oldSegmentData.m_SegmentData = *pSegment;
   oldSegmentData.m_TimelineMgr = *pTimelineMgr;

   if (pSegment->Strands.GetStrandDefinitionType() != CStrandData::sdtDirectSelection )
   {
      // We can go no further
      ::AfxMessageBox(_T("Programmer Error: EditDirectSelectionPrestressing - can only be called for Direct Select strand fill"),MB_OK | MB_ICONWARNING);
      return false;
   }

   std::_tstring strGirderName = pSegment->GetGirder()->GetGirderName();
   const GirderLibraryEntry* pGdrEntry = pLib->GetGirderEntry(strGirderName.c_str());

   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   bool allowEndAdjustment = 0.0 <= pStrandGeometry->GetHarpedEndOffsetIncrement(segmentKey);
   bool allowHpAdjustment  = 0.0 <= pStrandGeometry->GetHarpedHpOffsetIncrement(segmentKey);

   // Legacy strand adjustments cause problems in UI. Fix them if need be
   if (allowEndAdjustment)
   {
      DealWithLegacyEndHarpedStrandAdjustment(segmentKey, oldSegmentData.m_SegmentData, pStrandGeometry);
   }

   if (allowHpAdjustment)
   {
      DealWithLegacyHpHarpedStrandAdjustment(segmentKey, oldSegmentData.m_SegmentData, pStrandGeometry);
   }

   // Get current offset input values - dialog will force in bounds if needed
   HarpedStrandOffsetType endMeasureType = pSegment->Strands.GetHarpStrandOffsetMeasurementAtEnd();
   HarpedStrandOffsetType hpMeasureType  = pSegment->Strands.GetHarpStrandOffsetMeasurementAtHarpPoint();

   Float64 hpOffsetAtStart = pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metStart);
   Float64 hpOffsetAtHp1   = pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(pgsTypes::metStart);
   Float64 hpOffsetAtHp2   = pSegment->Strands.GetHarpStrandOffsetAtHarpPoint(pgsTypes::metEnd);
   Float64 hpOffsetAtEnd   = pSegment->Strands.GetHarpStrandOffsetAtEnd(pgsTypes::metEnd);

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi = pPOI->GetPointsOfInterest(segmentKey,POI_START_FACE | POI_END_FACE);
   ATLASSERT(vPoi.size() == 2);
   pgsPointOfInterest startPoi(vPoi.front());
   pgsPointOfInterest endPoi(vPoi.back());

   GET_IFACE(IGirder,pGdr);
   Float64 HgStart = pGdr->GetHeight(startPoi);
   Float64 HgEnd   = pGdr->GetHeight(endPoi);

   vPoi = pPOI->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT);
   ATLASSERT( 0 <= vPoi.size() && vPoi.size() <= 2 );
   pgsPointOfInterest hp1_poi;
   pgsPointOfInterest hp2_poi;

   if ( 0 < vPoi.size() )
   {
      hp1_poi = vPoi.front();
      hp2_poi = vPoi.back();
   }

   Float64 HgHp1, HgHp2;
   if ( 0 < vPoi.size() )
   {
      HgHp1 = pGdr->GetHeight(hp1_poi);
      HgHp2 = pGdr->GetHeight(hp2_poi);
   }
   else
   {
      HgHp1 = HgStart;
      HgHp2 = HgEnd;
   }


   // Max debond length is 1/2 girder length
   Float64 maxDebondLength = pBridge->GetSegmentLength(segmentKey)/2.0;

   // Fire up dialog
   txnEditPrecastSegmentData newSegmentData = oldSegmentData;

   CGirderSelectStrandsDlg dlg;
#pragma Reminder("UPDATE: clean up this initialization... the same code is in multiple locations")
   // this initialization is here, in BridgeDescPrestressPage.cpp and in GirderSegmentDlg.cpp
   // make the page more self-sufficient (most all of this data can come from the "Strands" object
   dlg.m_SelectStrandsPage.InitializeData(segmentKey, &newSegmentData.m_SegmentData.Strands, pSpecEntry, pGdrEntry,
                      allowEndAdjustment, allowHpAdjustment, endMeasureType, hpMeasureType, 
                      hpOffsetAtStart, hpOffsetAtHp1, hpOffsetAtHp2, hpOffsetAtEnd,
                      maxDebondLength,
                      HgStart,HgHp1,HgHp2,HgEnd);

   if ( dlg.DoModal() == IDOK )
   {
      // The dialog does not deal with Pjack. Update pjack here
#pragma Reminder("UPDATE: dialog should deal with Pjack")
      GET_IFACE(IPretensionForce, pPrestress );

      UpdatePrestressForce(pgsTypes::Straight,  segmentKey, newSegmentData.m_SegmentData, oldSegmentData.m_SegmentData, pPrestress);
      UpdatePrestressForce(pgsTypes::Harped,    segmentKey, newSegmentData.m_SegmentData, oldSegmentData.m_SegmentData, pPrestress);
      UpdatePrestressForce(pgsTypes::Temporary, segmentKey, newSegmentData.m_SegmentData, oldSegmentData.m_SegmentData, pPrestress);

      // Fire our transaction
      txnEditPrecastSegment* pTxn = new txnEditPrecastSegment(segmentKey,newSegmentData);

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);

      return true;
   }
   else
   {
     return true;
   }
}


bool CPGSDocBase::EditDirectInputPrestressing(const CSegmentKey& segmentKey)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc  = pIBridgeDesc->GetBridgeDescription();
   const CTimelineManager*    pTimelineMgr = pBridgeDesc->GetTimelineManager();

   SegmentIDType segmentID = pIBridgeDesc->GetSegmentID(segmentKey);


   CSplicedGirderData girder = *pIBridgeDesc->GetGirder(segmentKey);
   girder.SetIndex(segmentKey.girderIndex);
   CPrecastSegmentData* pSegment = girder.GetSegment(segmentKey.segmentIndex);

   txnEditPrecastSegmentData oldSegmentData;
   oldSegmentData.m_SegmentKey  = segmentKey;
   oldSegmentData.m_SegmentData = *pSegment;
   oldSegmentData.m_TimelineMgr = *pTimelineMgr;

   if (pSegment->Strands.GetStrandDefinitionType() != CStrandData::sdtDirectInput )
   {
      // We can go no further
      ::AfxMessageBox(_T("Programmer Error: EditDirectInputPrestressing - can only be called for Direct Input strand definition"),MB_OK | MB_ICONWARNING);
      return false;
   }

   // Fire up dialog
   txnEditPrecastSegmentData newSegmentData = oldSegmentData;

   CPropertySheet dlg(_T("Define Strands"));
   CGirderSegmentStrandsPage page;
   page.Init(pSegment);
   dlg.AddPage(&page);
   if ( dlg.DoModal() == IDOK )
   {
      newSegmentData.m_SegmentData = *pSegment;

      // The dialog does not deal with Pjack. Update pjack here
#pragma Reminder("UPDATE: dialog should deal with Pjack")
      GET_IFACE(IPretensionForce, pPrestress );

      UpdatePrestressForce(pgsTypes::Straight,  segmentKey, newSegmentData.m_SegmentData, oldSegmentData.m_SegmentData, pPrestress);
      UpdatePrestressForce(pgsTypes::Harped,    segmentKey, newSegmentData.m_SegmentData, oldSegmentData.m_SegmentData, pPrestress);
      UpdatePrestressForce(pgsTypes::Temporary, segmentKey, newSegmentData.m_SegmentData, oldSegmentData.m_SegmentData, pPrestress);

      // Fire our transaction
      txnEditPrecastSegment* pTxn = new txnEditPrecastSegment(segmentKey,newSegmentData);

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);

      return true;
   }
   else
   {
     return true;
   }
}
   
void CPGSDocBase::AddPointLoad(const CPointLoadData& loadData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CEditPointLoadDlg dlg(loadData,pTimelineMgr);
   if ( dlg.DoModal() == IDOK )
   {
      txnInsertPointLoad* pTxn = new txnInsertPointLoad(dlg.m_Load,dlg.m_EventID,dlg.m_bWasNewEventCreated ? &dlg.m_TimelineMgr : nullptr);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSDocBase::EditPointLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CPointLoadData* pLoadData = pUserDefinedLoads->GetPointLoad(loadIdx);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   EventIDType eventID = pTimelineMgr->FindUserLoadEventID(pLoadData->m_ID);

   CEditPointLoadDlg dlg(*pLoadData,pTimelineMgr);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (*pLoadData != dlg.m_Load)
      {
         txnEditPointLoad* pTxn = new txnEditPointLoad(loadIdx,*pLoadData,eventID,dlg.m_Load,dlg.m_EventID,dlg.m_bWasNewEventCreated ? &dlg.m_TimelineMgr : nullptr);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSDocBase::DeletePointLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   txnDeletePointLoad* pTxn = new txnDeletePointLoad(loadIdx);

   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSDocBase::AddDistributedLoad(const CDistributedLoadData& loadData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CEditDistributedLoadDlg dlg(loadData,pTimelineMgr);
   if ( dlg.DoModal() == IDOK )
   {
      txnInsertDistributedLoad* pTxn = new txnInsertDistributedLoad(dlg.m_Load,dlg.m_EventID,&dlg.m_bWasNewEventCreated ? &dlg.m_TimelineMgr : nullptr);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSDocBase::EditDistributedLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CDistributedLoadData* pLoadData = pUserDefinedLoads->GetDistributedLoad(loadIdx);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   EventIDType eventID = pTimelineMgr->FindUserLoadEventID(pLoadData->m_ID);

   CEditDistributedLoadDlg dlg(*pLoadData,pTimelineMgr);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (*pLoadData != dlg.m_Load)
      {
         txnEditDistributedLoad* pTxn = new txnEditDistributedLoad(loadIdx,*pLoadData,eventID,dlg.m_Load,dlg.m_EventID,dlg.m_bWasNewEventCreated ? &dlg.m_TimelineMgr : nullptr);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSDocBase::DeleteDistributedLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   txnDeleteDistributedLoad* pTxn = new txnDeleteDistributedLoad(loadIdx);

   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSDocBase::AddMomentLoad(const CMomentLoadData& loadData)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CEditMomentLoadDlg dlg(loadData,pTimelineMgr);
   if ( dlg.DoModal() == IDOK )
   {
      txnInsertMomentLoad* pTxn = new txnInsertMomentLoad(dlg.m_Load,dlg.m_EventID,dlg.m_bWasNewEventCreated ? &dlg.m_TimelineMgr : nullptr);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

bool CPGSDocBase::EditMomentLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IUserDefinedLoadData, pUserDefinedLoads);
   const CMomentLoadData* pLoadData = pUserDefinedLoads->GetMomentLoad(loadIdx);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   EventIDType eventID = pTimelineMgr->FindUserLoadEventID(pLoadData->m_ID);

   CEditMomentLoadDlg dlg(*pLoadData,pTimelineMgr);
   if (dlg.DoModal() == IDOK)
   {
      // only update if changed
      if (*pLoadData != dlg.m_Load)
      {
         txnEditMomentLoad* pTxn = new txnEditMomentLoad(loadIdx,*pLoadData,eventID,dlg.m_Load,dlg.m_EventID,dlg.m_bWasNewEventCreated ? &dlg.m_TimelineMgr : nullptr);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSDocBase::DeleteMomentLoad(CollectionIndexType loadIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   txnDeleteMomentLoad* pTxn = new txnDeleteMomentLoad(loadIdx);

   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

bool CPGSDocBase::EditTimeline()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CEditTimelineDlg dlg;
   dlg.m_TimelineManager = *pBridgeDesc->GetTimelineManager();

   if (dlg.DoModal() == IDOK)
   {
      txnEditTimeline* pTxn = new txnEditTimeline(*pBridgeDesc->GetTimelineManager(), dlg.m_TimelineManager);
      GET_IFACE(IEAFTransactions, pTransactions);
      pTransactions->Execute(pTxn);
      return true;
   }

   return false;
}

bool CPGSDocBase::EditEffectiveFlangeWidth()
{
   GET_IFACE(IEffectiveFlangeWidth, pEFW);
   CString strQuestion(_T("The LRFD General Effective Flange Width provisions (4.6.2.6.1) are considered applicable for skew angles less than 75 degress, L/S greater than or equal to 2.0 and overhang widths less than or equal to 0.5S. In unusual cases where these limits are violated, a refined analysis should be used."));
   CString strResponses(_T("Stop analysis if structure violates these limits\nIgnore these limits"));

   CEAFHelpHandler helpHandler(GetDocumentationSetName(), IDH_EFFECTIVE_FLANGE_WIDTH);
   int choice = pEFW->IgnoreEffectiveFlangeWidthLimits() ? 1 : 0;
   int new_choice = AfxChoose(_T("Effective Flange Width"), strQuestion, strResponses, choice, TRUE, &helpHandler);
   if (choice != new_choice && 0 <= new_choice)
   {
      txnEditEffectiveFlangeWidth* pTxn = new txnEditEffectiveFlangeWidth(pEFW->IgnoreEffectiveFlangeWidthLimits(), new_choice == 0 ? false : true);
      GET_IFACE(IEAFTransactions, pTransactions);
      pTransactions->Execute(pTxn);
      return true;
   }

   return false;
}

bool CPGSDocBase::SelectProjectCriteria()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CSpecDlg dlg;

   GET_IFACE(ISpecification, pSpec);
   std::_tstring cur_spec = pSpec->GetSpecification();
   dlg.m_Spec = cur_spec;
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pCurrentSpecEntry = pLib->GetSpecEntry(cur_spec.c_str());
   if (dlg.DoModal())
   {
      if (dlg.m_Spec != cur_spec)
      {
         GET_IFACE(IBridge, pBridge);
         const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(dlg.m_Spec.c_str());

         pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
         pgsTypes::AnalysisType newAnalysisType = analysisType;
         pgsTypes::WearingSurfaceType wearingSurfaceType = pBridge->GetWearingSurfaceType();
         pgsTypes::WearingSurfaceType newWearingSurfaceType = wearingSurfaceType;
         if (pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP)
         {
            if (analysisType != pgsTypes::Continuous)
            {
               newAnalysisType = pgsTypes::Continuous;
               CString strMsg;
               strMsg.Format(_T("The \"%s\" Project Criteria uses the time-step method for computing losses. The Structural Analysis Method must be set to Continuous for the time-step loss method.\n\nWould you like to change the Structural Analysis Method and continue?"), dlg.m_Spec.c_str());
               if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION) == IDNO)
               {
                  return false;
               }
            }

            if (wearingSurfaceType == pgsTypes::wstFutureOverlay)
            {
               newWearingSurfaceType = pgsTypes::wstOverlay;
               CString strMsg(_T("The bridge is modeled with a future overlay wearing surface. This is not a valid wearing surface type for time-step analysis.\n\nWould you like to change the wearing surface type to Overlay and continue?"));
               if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION) == IDNO)
               {
                  return false;
               }
            }
         }


         if (pCurrentSpecEntry->GetLossMethod() == LOSSES_TIME_STEP && pSpecEntry->GetLossMethod() != LOSSES_TIME_STEP)
         {
            // switching from time-step to regular loss method... the timeline will be reset
#if defined _DEBUG
            GET_IFACE(IDocumentType, pDocType);
            ATLASSERT(pDocType->IsPGSuperDocument()); // this will only happen in a PGSuper project
#endif
            CString strMsg(_T("The Construction Sequence Timeline will be reset. Would you like to continue?"));
            if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONQUESTION) == IDNO)
            {
               return false;
            }
         }

         txnEditProjectCriteria* pTxn = new txnEditProjectCriteria(cur_spec.c_str(), dlg.m_Spec.c_str(), analysisType, newAnalysisType, wearingSurfaceType, newWearingSurfaceType);
         GET_IFACE(IEAFTransactions, pTransactions);
         pTransactions->Execute(pTxn);
         return true;
      }
   }

   return false;
}

void CPGSDocBase::EditGirderViewSettings(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   UINT settings = GetGirderEditorSettings();

	CGirderEditorSettingsSheet dlg(IDS_GM_VIEW_SETTINGS);
   dlg.SetSettings(settings);
   dlg.SetActivePage(nPage);

   INT_PTR st = dlg.DoModal();
   if (st==IDOK)
   {
      settings = dlg.GetSettings();
      SetGirderEditorSettings(settings);

      // tell the world we've changed settings
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      UpdateAllViews( 0, HINT_GIRDERVIEWSETTINGSCHANGED, 0 );
   }
}

void CPGSDocBase::EditBridgeViewSettings(int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   UINT bridgeViewSettings = GetBridgeEditorSettings();
   UINT alignmentViewSettings = GetAlignmentEditorSettings();

	CBridgeEditorSettingsSheet dlg(IDS_BM_VIEW_SETTINGS);
   dlg.SetBridgeEditorSettings(bridgeViewSettings);
   dlg.SetAlignmentEditorSettings(alignmentViewSettings);
   dlg.SetActivePage(nPage);

   INT_PTR st = dlg.DoModal();
   if (st==IDOK)
   {
      bridgeViewSettings = dlg.GetBridgeEditorSettings();
      bridgeViewSettings |= IDB_PV_DRAW_ISOTROPIC;
      SetBridgeEditorSettings(bridgeViewSettings);

      alignmentViewSettings = dlg.GetAlignmentEditorSettings();
      alignmentViewSettings |= IDB_PV_DRAW_ISOTROPIC;
      SetAlignmentEditorSettings(alignmentViewSettings);
   }

   // tell the world we've changed settings
   {
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      UpdateAllViews( 0, HINT_BRIDGEVIEWSETTINGSCHANGED, 0 );
   }
}

void CPGSDocBase::ModifyTemplate(LPCTSTR strTemplate)
{
   // called during UpdateTemplates
   // add code here, or override in a parent class to
   // add code to modify the template
   // For example, if you want to ensure that overlay depth in all templates
   // is a specific value, you can code that here.

   //GET_IFACE(IBridgeDescription, pIBridgeDesc);
   //CDeckDescription2 deck = *(pIBridgeDesc->GetDeckDescription());
   //deck.bInputAsDepthAndDensity = true;
   //deck.OverlayDepth = ::ConvertToSysUnits(3.0, unitMeasure::Inch);
   //pIBridgeDesc->SetDeckDescription(deck);

   //// Update seed values to match library
   //GET_IFACE(IDocumentType, pDocType);
   //if (pDocType->IsPGSuperDocument())
   //{
   //   // only PGSuper documents have seed values
   //   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   //   CBridgeDescription2 bridgeDesc = *(pIBridgeDesc->GetBridgeDescription());
   //   GroupIndexType nGroups = bridgeDesc.GetGirderGroupCount();
   //   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   //   {
   //      CGirderGroupData* pGroup = bridgeDesc.GetGirderGroup(grpIdx);
   //      GirderIndexType nGirders = pGroup->GetGirderCount();
   //      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   //      {
   //         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

   //         SegmentIndexType nSegments = pGirder->GetSegmentCount();
   //         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   //         {
   //            CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
   //            pSegment->LongitudinalRebarData.CopyGirderEntryData(*(pGirder->GetGirderLibraryEntry()));
   //            pSegment->ShearData.CopyGirderEntryData(*(pGirder->GetGirderLibraryEntry()));
   //         }
   //      }
   //   }
   //   pIBridgeDesc->SetBridgeDescription(bridgeDesc);
   //}
}

BOOL CPGSDocBase::UpdateTemplates(IProgress* pProgress,LPCTSTR lpszDir)
{
   CFileFind dir_finder;
   BOOL bMoreDir = dir_finder.FindFile(CString(lpszDir)+_T("\\*"));

   // recursively go through the directories
   while ( bMoreDir )
   {
      bMoreDir = dir_finder.FindNextFile();
      CString strDir = dir_finder.GetFilePath();

      if ( !dir_finder.IsDots() && dir_finder.IsDirectory() )
      {
         UpdateTemplates(pProgress,strDir);
      }
   }

   // done with the directories below this leave. Process the templates at this level
   CString strMessage;
   strMessage.Format(_T("Updating templates in %s"),lpszDir);
   pProgress->UpdateMessage(strMessage);

   CFileFind template_finder;
   BOOL bMoreTemplates = template_finder.FindFile(CString(lpszDir) + _T("\\*") + CString(GetTemplateExtension()));
   while ( bMoreTemplates )
   {
      bMoreTemplates      = template_finder.FindNextFile();
      CString strTemplate = template_finder.GetFilePath();

      strMessage.Format(_T("Updating %s"),template_finder.GetFileTitle());
      pProgress->UpdateMessage(strMessage);

      if ( !OpenTheDocument(strTemplate) )
      {
         return FALSE;
      }

      ModifyTemplate(strTemplate);

      CEAFBrokerDocument::SaveTheDocument(strTemplate);

      m_pBroker->Reset();

      CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pInit(m_pBroker);
      pInit->InitAgents();
   }

   return TRUE;
}

BOOL CPGSDocBase::UpdateTemplates()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSuper = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CString workgroup_folder;
   pPGSuper->GetTemplateFolders(workgroup_folder);

   if  ( !Init() ) // load the agents and other necessary stuff
   {
      return FALSE;
   }

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   // we want the template files to be "clean" with only PGSuper data in them
   // Tell the broker to not save data from any missing extension agents
   CComQIPtr<IBrokerPersist2> broker_persist(m_pBroker);
   ATLASSERT(broker_persist);
   VARIANT_BOOL bFlag;
   broker_persist->GetSaveMissingAgentDataFlag(&bFlag);
   broker_persist->SetSaveMissingAgentDataFlag(VARIANT_FALSE);

   UpdateTemplates(pProgress,workgroup_folder);

   // restore the flag to its previous state
   broker_persist->SetSaveMissingAgentDataFlag(bFlag);


   return FALSE; // didn't really open a file
}

IDType CPGSDocBase::RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_BridgePlanViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterBridgePlanViewCallback(IDType ID)
{
   std::map<IDType,IBridgePlanViewEventCallback*>::iterator found = m_BridgePlanViewCallbacks.find(ID);
   if ( found == m_BridgePlanViewCallbacks.end() )
   {
      return false;
   }

   m_BridgePlanViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IBridgePlanViewEventCallback*>& CPGSDocBase::GetBridgePlanViewCallbacks()
{
   return m_BridgePlanViewCallbacks;
}

IDType CPGSDocBase::RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_BridgeSectionViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterBridgeSectionViewCallback(IDType ID)
{
   std::map<IDType,IBridgeSectionViewEventCallback*>::iterator found = m_BridgeSectionViewCallbacks.find(ID);
   if ( found == m_BridgeSectionViewCallbacks.end() )
   {
      return false;
   }

   m_BridgeSectionViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IBridgeSectionViewEventCallback*>& CPGSDocBase::GetBridgeSectionViewCallbacks()
{
   return m_BridgeSectionViewCallbacks;
}

IDType CPGSDocBase::RegisterAlignmentPlanViewCallback(IAlignmentPlanViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_AlignmentPlanViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterAlignmentPlanViewCallback(IDType ID)
{
   std::map<IDType,IAlignmentPlanViewEventCallback*>::iterator found = m_AlignmentPlanViewCallbacks.find(ID);
   if ( found == m_AlignmentPlanViewCallbacks.end() )
   {
      return false;
   }

   m_AlignmentPlanViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IAlignmentPlanViewEventCallback*>& CPGSDocBase::GetAlignmentPlanViewCallbacks()
{
   return m_AlignmentPlanViewCallbacks;
}

IDType CPGSDocBase::RegisterAlignmentProfileViewCallback(IAlignmentProfileViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_AlignmentProfileViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterAlignmentProfileViewCallback(IDType ID)
{
   std::map<IDType,IAlignmentProfileViewEventCallback*>::iterator found = m_AlignmentProfileViewCallbacks.find(ID);
   if ( found == m_AlignmentProfileViewCallbacks.end() )
   {
      return false;
   }

   m_AlignmentProfileViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IAlignmentProfileViewEventCallback*>& CPGSDocBase::GetAlignmentProfileViewCallbacks()
{
   return m_AlignmentProfileViewCallbacks;
}

IDType CPGSDocBase::RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_GirderElevationViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterGirderElevationViewCallback(IDType ID)
{
   std::map<IDType,IGirderElevationViewEventCallback*>::iterator found = m_GirderElevationViewCallbacks.find(ID);
   if ( found == m_GirderElevationViewCallbacks.end() )
   {
      return false;
   }

   m_GirderElevationViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IGirderElevationViewEventCallback*>& CPGSDocBase::GetGirderElevationViewCallbacks()
{
   return m_GirderElevationViewCallbacks;
}

IDType CPGSDocBase::RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_GirderSectionViewCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterGirderSectionViewCallback(IDType ID)
{
   std::map<IDType,IGirderSectionViewEventCallback*>::iterator found = m_GirderSectionViewCallbacks.find(ID);
   if ( found == m_GirderSectionViewCallbacks.end() )
   {
      return false;
   }

   m_GirderSectionViewCallbacks.erase(found);

   return true;
}

const std::map<IDType,IGirderSectionViewEventCallback*>& CPGSDocBase::GetGirderSectionViewCallbacks()
{
   return m_GirderSectionViewCallbacks;
}

IDType CPGSDocBase::RegisterEditPierCallback(IEditPierCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditPierCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterEditPierCallback(IDType ID)
{
   std::map<IDType,IEditPierCallback*>::iterator found = m_EditPierCallbacks.find(ID);
   if ( found == m_EditPierCallbacks.end() )
   {
      return false;
   }

   m_EditPierCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditPierCallback*>& CPGSDocBase::GetEditPierCallbacks()
{
   return m_EditPierCallbacks;
}

IDType CPGSDocBase::RegisterEditTemporarySupportCallback(IEditTemporarySupportCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditTemporarySupportCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterEditTemporarySupportCallback(IDType ID)
{
   std::map<IDType,IEditTemporarySupportCallback*>::iterator found = m_EditTemporarySupportCallbacks.find(ID);
   if ( found == m_EditTemporarySupportCallbacks.end() )
   {
      return false;
   }

   m_EditTemporarySupportCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditTemporarySupportCallback*>& CPGSDocBase::GetEditTemporarySupportCallbacks()
{
   return m_EditTemporarySupportCallbacks;
}

IDType CPGSDocBase::RegisterEditSpanCallback(IEditSpanCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditSpanCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterEditSpanCallback(IDType ID)
{
   std::map<IDType,IEditSpanCallback*>::iterator found = m_EditSpanCallbacks.find(ID);
   if ( found == m_EditSpanCallbacks.end() )
   {
      return false;
   }

   m_EditSpanCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditSpanCallback*>& CPGSDocBase::GetEditSpanCallbacks()
{
   return m_EditSpanCallbacks;
}

IDType CPGSDocBase::RegisterEditGirderCallback(IEditGirderCallback* pCallback,ICopyGirderPropertiesCallback * pCopyCallback)
{
   IDType key = m_CallbackID++;
   m_EditGirderCallbacks.insert(std::make_pair(key,pCallback));

   if ( pCopyCallback )
   {
      m_CopyGirderPropertiesCallbacks.insert(std::make_pair(key,pCopyCallback));
   }

   return key;
}

bool CPGSDocBase::UnregisterEditGirderCallback(IDType ID)
{
   std::map<IDType,IEditGirderCallback*>::iterator foundCallback = m_EditGirderCallbacks.find(ID);
   if ( foundCallback == m_EditGirderCallbacks.end() )
   {
      return false;
   }

   m_EditGirderCallbacks.erase(foundCallback);

   std::map<IDType,ICopyGirderPropertiesCallback*>::iterator foundCopyCallback = m_CopyGirderPropertiesCallbacks.find(ID);
   if ( foundCopyCallback != m_CopyGirderPropertiesCallbacks.end() )
   {
      m_CopyGirderPropertiesCallbacks.erase(foundCopyCallback);
   }

   return true;
}

const std::map<IDType,IEditGirderCallback*>& CPGSDocBase::GetEditGirderCallbacks()
{
   return m_EditGirderCallbacks;
}

const std::map<IDType,ICopyGirderPropertiesCallback*>& CPGSDocBase::GetCopyGirderPropertiesCallbacks()
{
   return m_CopyGirderPropertiesCallbacks;
}

IDType CPGSDocBase::RegisterEditSplicedGirderCallback(IEditSplicedGirderCallback* pCallback,ICopyGirderPropertiesCallback* pCopyCallback)
{
   IDType key = m_CallbackID++;
   m_EditSplicedGirderCallbacks.insert(std::make_pair(key,pCallback));

   if ( pCopyCallback )
   {
      m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(key,pCopyCallback));
   }

   return key;
}

bool CPGSDocBase::UnregisterEditSplicedGirderCallback(IDType ID)
{
   std::map<IDType,IEditSplicedGirderCallback*>::iterator found = m_EditSplicedGirderCallbacks.find(ID);
   if ( found == m_EditSplicedGirderCallbacks.end() )
   {
      return false;
   }

   m_EditSplicedGirderCallbacks.erase(found);

   std::map<IDType,ICopyGirderPropertiesCallback*>::iterator foundCopyCallback = m_CopySplicedGirderPropertiesCallbacks.find(ID);
   if ( foundCopyCallback != m_CopySplicedGirderPropertiesCallbacks.end() )
   {
      m_CopySplicedGirderPropertiesCallbacks.erase(foundCopyCallback);
   }

   return true;
}

const std::map<IDType,IEditSplicedGirderCallback*>& CPGSDocBase::GetEditSplicedGirderCallbacks()
{
   return m_EditSplicedGirderCallbacks;
}

const std::map<IDType,ICopyGirderPropertiesCallback*>& CPGSDocBase::GetCopySplicedGirderPropertiesCallbacks()
{
   return m_CopySplicedGirderPropertiesCallbacks;
}

IDType CPGSDocBase::RegisterEditSegmentCallback(IEditSegmentCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditSegmentCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterEditSegmentCallback(IDType ID)
{
   std::map<IDType,IEditSegmentCallback*>::iterator found = m_EditSegmentCallbacks.find(ID);
   if ( found == m_EditSegmentCallbacks.end() )
   {
      return false;
   }

   m_EditSegmentCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditSegmentCallback*>& CPGSDocBase::GetEditSegmentCallbacks()
{
   return m_EditSegmentCallbacks;
}

IDType CPGSDocBase::RegisterEditClosureJointCallback(IEditClosureJointCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditClosureJointCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterEditClosureJointCallback(IDType ID)
{
   std::map<IDType,IEditClosureJointCallback*>::iterator found = m_EditClosureJointCallbacks.find(ID);
   if ( found == m_EditClosureJointCallbacks.end() )
   {
      return false;
   }

   m_EditClosureJointCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditClosureJointCallback*>& CPGSDocBase::GetEditClosureJointCallbacks()
{
   return m_EditClosureJointCallbacks;
}

IDType CPGSDocBase::RegisterEditBridgeCallback(IEditBridgeCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditBridgeCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterEditBridgeCallback(IDType ID)
{
   std::map<IDType,IEditBridgeCallback*>::iterator found = m_EditBridgeCallbacks.find(ID);
   if ( found == m_EditBridgeCallbacks.end() )
   {
      return false;
   }

   m_EditBridgeCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditBridgeCallback*>& CPGSDocBase::GetEditBridgeCallbacks()
{
   return m_EditBridgeCallbacks;
}

IDType CPGSDocBase::RegisterEditLoadRatingOptionsCallback(IEditLoadRatingOptionsCallback* pCallback)
{
   IDType key = m_CallbackID++;
   m_EditLoadRatingOptionsCallbacks.insert(std::make_pair(key,pCallback));
   return key;
}

bool CPGSDocBase::UnregisterEditLoadRatingOptionsCallback(IDType ID)
{
   std::map<IDType,IEditLoadRatingOptionsCallback*>::iterator found = m_EditLoadRatingOptionsCallbacks.find(ID);
   if ( found == m_EditLoadRatingOptionsCallbacks.end() )
   {
      return false;
   }

   m_EditLoadRatingOptionsCallbacks.erase(found);

   return true;
}

const std::map<IDType,IEditLoadRatingOptionsCallback*>& CPGSDocBase::GetEditLoadRatingOptionsCallbacks()
{
   return m_EditLoadRatingOptionsCallbacks;
}

BOOL CPGSDocBase::OnNewDocumentFromTemplate(LPCTSTR lpszPathName)
{
   if ( !CEAFDocument::OnNewDocumentFromTemplate(lpszPathName) )
   {
      return FALSE;
   }

   InitProjectProperties();
   return TRUE;
}

void CPGSDocBase::OnCloseDocument()
{
   CEAFBrokerDocument::OnCloseDocument();

   CBeamFamilyManager::Reset();
}

void CPGSDocBase::InitProjectProperties()
{
   CBridgeLinkApp* pApp = (CBridgeLinkApp*)EAFGetApp();
   IBridgeLink* pBL = (IBridgeLink*)pApp;
   CString engineer_name, company;
   pBL->GetUserInfo(&engineer_name,&company);

   GET_IFACE( IProjectProperties, pProjProp );

   pProjProp->SetEngineer(engineer_name);
   pProjProp->SetCompany(company);

   if ( ShowProjectPropertiesOnNewProject() )
   {
      OnFileProjectProperties();
   }
}

void CPGSDocBase::OnCreateInitialize()
{
   // called before any data is loaded/created in the document
   CEAFBrokerDocument::OnCreateInitialize();

   // Cant' hold events here because this is before any document
   // initialization happens. ie., the broker hasn't been
   // created yet
}

void CPGSDocBase::OnCreateFinalize()
{
   CEAFBrokerDocument::OnCreateFinalize();

   // Register callbacks for status items
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidInformationalError  = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusWarning)); 
   m_StatusGroupID = pStatusCenter->CreateStatusGroupID();


   //// do this here, instead of in Init()... SetFavoriteReports updates views, but during Init, the views
   //// haven't been initialized yet. We got lucky that it didn't cause crashes.
   //// Views are fully created here
   //CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   //CComPtr<IEAFAppPlugin> pAppPlugin;
   //pTemplate->GetPlugin(&pAppPlugin);
   //CPGSAppPluginBase* pPGSuper = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   //// Transfer report favorites and custom reports data from CPGSAppPluginBase to CEAFBrokerDocument (this)
   //bool doDisplayFavorites = pPGSuper->GetDoDisplayFavoriteReports();
   //std::vector<std::_tstring> Favorites = pPGSuper->GetFavoriteReports();

   //SetDoDisplayFavoriteReports(doDisplayFavorites);
   //SetFavoriteReports(Favorites);

   //CEAFCustomReports customs = pPGSuper->GetCustomReports();
   //SetCustomReports(customs);

   //IntegrateCustomReports();

   PopulateReportMenu();
   PopulateGraphMenu();

#pragma Reminder("REVIEW")
/* This option works if Outlook and PGSuper are running at the same UAC level

   // if user is on Windows Vista or Windows 7, the Send Email feature doesn't work
   // so we will remove it from the menu

   OSVERSIONINFO osInfo;
   osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osInfo);
   bool bRemoveEmailOption = false;
   if ( 6 < osInfo.dwMajorVersion || (osInfo.dwMajorVersion == 6 && 1 <= osInfo.dwMinorVersion) )
   {
      // this is Windows 7 or a future OS... assume feature isn't supported
      bRemoveEmailOption = true;
   }

   if ( bRemoveEmailOption ) 
   {
      // get main menu
      CEAFMenu* pMainMenu = GetMainMenu();

      // find position of file menu
      UINT filePos = pMainMenu->FindMenuItem(_T("&File"));

      // get the file menu
      CEAFMenu* pFileMenu = pMainMenu->GetSubMenu(filePos);

      // get the text string from the email command
      CString strEmail;
      if ( pFileMenu->GetMenuString(ID_FILE_SEND_MAIL,strEmail,MF_BYCOMMAND) )
      {
	      // find the position of the email command
	      UINT emailPos = pFileMenu->FindMenuItem(strEmail);
	
	      // remove the email command and the adjacent separator
	      pFileMenu->RemoveMenu(emailPos,MF_BYPOSITION,nullptr);
	      pFileMenu->RemoveMenu(emailPos,MF_BYPOSITION,nullptr);
      }
   }
*/
   // Set the AutoCalc state on the status bar
   CPGSuperStatusBar* pStatusBar = ((CPGSuperStatusBar*)EAFGetMainFrame()->GetStatusBar());
   pStatusBar->AutoCalcEnabled( IsAutoCalcEnabled() );

   // views have been initilized so fire any pending events
   GET_IFACE(IEvents,pEvents);
   GET_IFACE(IUIEvents,pUIEvents);
   pEvents->FirePendingEvents(); 
   pUIEvents->HoldEvents(false);
}

BOOL CPGSDocBase::CreateBroker()
{
   if ( !CEAFBrokerDocument::CreateBroker() )
   {
      return FALSE;
   }

   // map old PGSuper (pre version 3.0) CLSID to current CLSID
   // CLSID's were changed so that pre version 3.0 installations could co-exist with 3.0 and later installations
   CComQIPtr<ICLSIDMap> clsidMap(m_pBroker);
   clsidMap->AddCLSID(CComBSTR("{BE55D0A2-68EC-11D2-883C-006097C68A9C}"),CComBSTR("{DD1ECB24-F46E-4933-8EE4-1DC0BC67410D}")); // Analysis Agent
   clsidMap->AddCLSID(CComBSTR("{59753CA0-3B7B-11D2-8EC5-006097DF3C68}"),CComBSTR("{3FD393DD-8AF4-4CB2-A1C5-71E46C436BA0}")); // Bridge Agent
   clsidMap->AddCLSID(CComBSTR("{B455A760-6DAF-11D2-8EE9-006097DF3C68}"),CComBSTR("{73922319-9243-4974-BA54-CF22593EC9C4}")); // Eng Agent
   clsidMap->AddCLSID(CComBSTR("{3DA9045D-7C49-4591-AD14-D560E7D95581}"),CComBSTR("{B4639189-ED38-4A68-8A18-38026202E9DE}")); // Graph Agent
   clsidMap->AddCLSID(CComBSTR("{59D50426-265C-11D2-8EB0-006097DF3C68}"),CComBSTR("{256B5B5B-762C-4693-8802-6B0351290FEA}")); // Project Agent
   clsidMap->AddCLSID(CComBSTR("{3D5066F2-27BE-11D2-8EB2-006097DF3C68}"),CComBSTR("{1FFED5EC-7A32-4837-A1F1-99481AFF2825}")); // PGSuper Report Agent
   clsidMap->AddCLSID(CComBSTR("{EC915470-6E76-11D2-8EEB-006097DF3C68}"),CComBSTR("{F510647E-1F4F-4FEF-8257-6914DE7B07C8}")); // Spec Agent
   clsidMap->AddCLSID(CComBSTR("{433B5860-71BF-11D3-ADC5-00105A9AF985}"),CComBSTR("{7D692AAD-39D0-4E73-842C-854457EA0EE6}")); // Test Agent

   return TRUE;
}

HINSTANCE CPGSDocBase::GetResourceInstance()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return AfxGetInstanceHandle();
}

BOOL CPGSDocBase::OnOpenDocument(LPCTSTR lpszPathName)
{
   CString file_ext;
   CString file_name(lpszPathName);
	int charpos = file_name.ReverseFind('.');
	if (0 <= charpos)
   {
		file_ext = file_name.Right(file_name.GetLength() - charpos);
   }

   if ( file_ext.CompareNoCase(GetTemplateExtension()) == 0 )
   {
      // this is a template file
      return OnNewDocumentFromTemplate(lpszPathName);
   }
   else
   {
      return CEAFBrokerDocument::OnOpenDocument(lpszPathName);
   }
}

BOOL CPGSDocBase::OpenTheDocument(LPCTSTR lpszPathName)
{
   // don't fire UI events as the UI isn't completely built when the document is created
   // (view classes haven't been initialized)
   m_pPGSuperDocProxyAgent->HoldEvents();
   // Events are released in OnCreateFinalize()

   if ( !CEAFBrokerDocument::OpenTheDocument(lpszPathName) )
   {
      return FALSE;
   }

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   m_DocUnitSystem->put_UnitMode( IS_US_UNITS(pDisplayUnits) ? umUS : umSI );
  
   return TRUE;
}


HRESULT CPGSDocBase::ConvertTheDocument(LPCTSTR lpszPathName, CString* prealFileName)
{
   // Open the document and look at the second line
   // If the version tag is 0.80, then the document needs to be converted
   std::_tifstream ifile(lpszPathName);
   if ( !ifile )
   {
      return E_INVALIDARG;
   }

   TCHAR line[50];
   ifile.getline(line,50);
   ifile.getline(line,50);
   CString strLine(line);
   strLine.TrimLeft();
   int loc = strLine.Find(_T(">"),0);
   if (loc!=-1)
   {
      strLine = strLine.Left(loc+1);
      if ( strLine == CString(_T("<PGSuper version=\"0.8\">")) ||
           strLine == CString(_T("<PGSuper version=\"0.83\">"))  )
      {
         ifile.close();

         _PgsFileConvert1 convert;
         if ( !convert.CreateDispatch(_T("convert.PgsFileConvert1")) )
         {
            return REGDB_E_CLASSNOTREG;
         }

         BSTR bstrPathName = CString(lpszPathName).AllocSysString();
         BSTR bstrRealName=0;
         long convert_status = convert.Convert( &bstrPathName, &bstrRealName );
         if ( convert_status == -1  )
         {
            ::SysFreeString(bstrPathName);
            ::SysFreeString(bstrRealName);
            return E_FAIL;
            return -1;
         }
         else if (convert_status==1)
         {
            // file was converted
            ::SysFreeString(bstrPathName);
            *prealFileName = CString(bstrRealName);
            ::SysFreeString(bstrRealName);
            return S_OK;
         }
         else
         {
            // converter did not convert file - this should be impossible becuase
            // we are checking versions above. must be wrong version of the converter
            ATLASSERT(false);
            ::SysFreeString(bstrPathName);
            ::SysFreeString(bstrRealName);
         }
      }
      else
      {
         // no file conversion needed - give realFile same name as MFC's file
         *prealFileName = CString(lpszPathName);
         return S_FALSE; // succeeded, but not converted
      }
   }

   return E_FAIL;
}

CString CPGSDocBase::GetRootNodeName()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSuper = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);
   return pPGSuper->GetAppName();
}

Float64 CPGSDocBase::GetRootNodeVersion()
{
   return FILE_VERSION;
}

HRESULT CPGSDocBase::WriteTheDocument(IStructuredSave* pStrSave)
{
   // before the standard broker document persistence, write out the version
   // number of the application that created this document
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CPGSuperAppPluginApp* pApp = (CPGSuperAppPluginApp*)AfxGetApp();
   HRESULT hr = pStrSave->put_Property(_T("Version"),CComVariant(pApp->GetVersion(true)));
   if ( FAILED(hr) )
   {
      return hr;
   }

   hr = CEAFBrokerDocument::WriteTheDocument(pStrSave);
   if ( FAILED(hr) )
   {
      return hr;
   }


   return S_OK;
}

HRESULT CPGSDocBase::LoadTheDocument(IStructuredLoad* pStrLoad)
{
   Float64 version;
   HRESULT hr = pStrLoad->get_Version(&version);
   if ( FAILED(hr) )
   {
      return hr;
   }

   if ( 1.0 < version )
   {
      CComVariant var;
      var.vt = VT_BSTR;
      hr = pStrLoad->get_Property(_T("Version"),&var);
      if ( FAILED(hr) )
      {
         return hr;
      }

   #if defined _DEBUG
      TRACE(_T("Loading data saved with PGSuper Version %s\n"),CComBSTR(var.bstrVal));
   }
   else
   {
      TRACE(_T("Loading data saved with PGSuper Version 2.1 or earlier\n"));
   #endif
   } // clses the bracket for if ( 1.0 < version )

   return CEAFBrokerDocument::LoadTheDocument(pStrLoad);
}

void CPGSDocBase::OnErrorDeletingBadSave(LPCTSTR lpszPathName,LPCTSTR lpszBackup)
{
   CString msg;

   GET_IFACE(IEAFProjectLog,pLog);

   pLog->LogMessage(_T(""));
   pLog->LogMessage(_T("An error occured while recovering your last successful save."));
   msg.Format(_T("It is highly likely that the file %s is corrupt."), lpszPathName);
   pLog->LogMessage( msg );
   pLog->LogMessage(_T("To recover from this error,"));
   msg.Format(_T("   1. Delete %s"), lpszPathName );
   pLog->LogMessage( msg );
   msg.Format(_T("   2. Rename %s to %s"), lpszBackup, lpszPathName );
   pLog->LogMessage( msg );
   pLog->LogMessage(_T(""));

   std::_tstring strLogFileName = pLog->GetName();

   AfxFormatString2( msg, IDS_E_SAVERECOVER1, lpszPathName, CString(strLogFileName.c_str()) );
   AfxMessageBox(msg );
}

void CPGSDocBase::OnErrorRenamingSaveBackup(LPCTSTR lpszPathName,LPCTSTR lpszBackup)
{
   CString msg;

   GET_IFACE(IEAFProjectLog,pLog);

   pLog->LogMessage(_T(""));
   pLog->LogMessage(_T("An error occured while recovering your last successful save."));
   msg.Format(_T("It is highly likely that the file %s no longer exists."), lpszPathName);
   pLog->LogMessage( msg );
   pLog->LogMessage(_T("To recover from this error,"));
   msg.Format(_T("   1. If %s exists, delete it."), lpszPathName );
   pLog->LogMessage( msg );
   msg.Format(_T("   2. Rename %s to %s"), lpszBackup, lpszPathName );
   pLog->LogMessage( msg );
   pLog->LogMessage(_T(""));

   std::_tstring strLogFileName = pLog->GetName();

   AfxFormatString2( msg, IDS_E_SAVERECOVER2, lpszPathName, CString(strLogFileName.c_str()) );
   AfxMessageBox( msg );
}

/////////////////////////////////////////////////////////////////////////////
// CPGSDocBase diagnostics

#ifdef _DEBUG
void CPGSDocBase::AssertValid() const
{
	CEAFBrokerDocument::AssertValid();
}

void CPGSDocBase::Dump(CDumpContext& dc) const
{
	CEAFBrokerDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPGSDocBase commands

BOOL CPGSDocBase::Init()
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSuper = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   // must happen before calling base class Init()
   m_pPluginMgr = CreatePluginManager();
   m_pPluginMgr->LoadPlugins(); // these are the data importers and exporters

   if ( !CEAFBrokerDocument::Init() )
   {
      return FALSE;
   }

   // Application start up can be improved if this call
   // is executed in its own thread... Need to add some
   // code that indicates if the call fails.. then throw
   // a shut down exception
   if ( FAILED(CBeamFamilyManager::Init(GetBeamFamilyCategoryID())) )
   {
      return FALSE;
   }

   // load up the library manager
   if ( !LoadMasterLibrary() )
   {
      return FALSE;
   }

   // Setup the library manager (same as if it changed)
   OnLibMgrChanged( &m_LibMgr );

   // Set up the document unit system
   CComPtr<IAppUnitSystem> appUnitSystem;
   pPGSuper->GetAppUnitSystem(&appUnitSystem);
   CreateDocUnitSystem(appUnitSystem,&m_DocUnitSystem);


   // register the standard copy girder callback objects
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderType));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderMaterials));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderPrestressing));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderRebar));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderStirrups));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderHandling));
   m_CopyGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderSlabOffset));

#pragma Reminder("REVIEW: is this correct for spliced girders?")
   // the copy girder properties features really hasn't been tested or made to work for spliced
   // girders yet.
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderType));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderMaterials));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderPrestressing));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderRebar));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderStirrups));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderHandling));
   m_CopySplicedGirderPropertiesCallbacks.insert(std::make_pair(m_CallbackID++,&m_CopyGirderSlabOffset));

   return TRUE;
}

BOOL CPGSDocBase::LoadSpecialAgents(IBrokerInitEx2* pBrokerInit)
{
   if ( !CEAFBrokerDocument::LoadSpecialAgents(pBrokerInit) )
   {
      return FALSE;
   }

   CComObject<CPGSuperDocProxyAgent>* pDocProxyAgent;
   CComObject<CPGSuperDocProxyAgent>::CreateInstance(&pDocProxyAgent);
   m_pPGSuperDocProxyAgent = dynamic_cast<CPGSuperDocProxyAgent*>(pDocProxyAgent);
   m_pPGSuperDocProxyAgent->SetDocument( this );

   CComPtr<IAgentEx> pAgent(m_pPGSuperDocProxyAgent);
   
   HRESULT hr = pBrokerInit->AddAgent( pAgent );
   if ( FAILED(hr) )
   {
      return FALSE;
   }

   // we want to use some special agents
   CLSID clsid[] = {CLSID_ReportManagerAgent,CLSID_GraphManagerAgent};
   if ( !CEAFBrokerDocument::LoadAgents(pBrokerInit, clsid, sizeof(clsid)/sizeof(CLSID) ) )
   {
      return FALSE;
   }

   return TRUE;
}

void CPGSDocBase::OnFileProjectProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( IProjectProperties, pProjProp );

   CProjectPropertiesDlg dlg;

   dlg.m_Bridge    = pProjProp->GetBridgeName();
   dlg.m_BridgeID  = pProjProp->GetBridgeID();
   dlg.m_JobNumber = pProjProp->GetJobNumber();
   dlg.m_Engineer  = pProjProp->GetEngineer();
   dlg.m_Company   = pProjProp->GetCompany();
   dlg.m_Comments  = pProjProp->GetComments();
   dlg.m_bShowProjectProperties = ShowProjectPropertiesOnNewProject();


   if ( dlg.DoModal() == IDOK )
   {
      txnEditProjectProperties* pTxn = new txnEditProjectProperties( pProjProp->GetBridgeName(), dlg.m_Bridge,
                                                                     pProjProp->GetBridgeID(),   dlg.m_BridgeID,
                                                                     pProjProp->GetJobNumber(),  dlg.m_JobNumber,
                                                                     pProjProp->GetEngineer(),   dlg.m_Engineer,
                                                                     pProjProp->GetCompany(),    dlg.m_Company,
                                                                     pProjProp->GetComments(),   dlg.m_Comments );

         
      ShowProjectPropertiesOnNewProject(dlg.m_bShowProjectProperties);

      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}


void CPGSDocBase::HandleOpenDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   // Skipping the default functionality and replacing it with something better
   //CEAFBrokerDocument::HandleOpenDocumentError(hr,lpszPathName);

   GET_IFACE( IEAFProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format(_T("The following error occured while opening %s"),lpszPathName );
   pLog->LogMessage( log_msg_header );

   if ( hr == STRLOAD_E_USERDEFINED )
   {
      // a user defined error occured. an error message should have been displayed
      // at the point where the error occured. 
      // Do nothing here! return because we are done
      return;
   }

   CPGSuperDocTemplateBase* pTemplate = (CPGSuperDocTemplateBase*)GetDocTemplate();


   CString strLog;
   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("CLSID_StructuredLoad not registered") );
      AfxFormatString1(msg1,IDS_E_BADINSTALL, pTemplate->GetAppName() );
      break;

   case STRLOAD_E_CANTOPEN:
      pLog->LogMessage( TEXT("Could not open file") );
      AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      break;

   case STRLOAD_E_FILENOTFOUND:
      pLog->LogMessage( TEXT("File Not Found") );
      AfxFormatString1( msg1, IDS_E_FILENOTFOUND, lpszPathName );
      break;

   case STRLOAD_E_INVALIDFORMAT:
      strLog.Format(_T("File does not have a valid %s format"),pTemplate->GetAppName());
      pLog->LogMessage(strLog);
      AfxFormatString2( msg1, IDS_E_INVALIDFORMAT, lpszPathName,pTemplate->GetAppName() );
      break;

   case STRLOAD_E_BADVERSION:
      strLog.Format(_T("This file came from a newer version of %s, please upgrade"),pTemplate->GetAppName());
      pLog->LogMessage(strLog);
      AfxFormatString2( msg1, IDS_E_INVALIDVERSION, lpszPathName,pTemplate->GetAppName() );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format(_T("An unknown error occured while opening the file (hr = %d)"),hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      }
      break;
   }

   CString msg;
   CString msg2;
   std::_tstring strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );
}

void CPGSDocBase::HandleSaveDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   // Skipping the default functionality and replacing it with something better
   //CEAFBrokerDocument::HandleSaveDocumentError(hr,lpszPathName);

   GET_IFACE( IEAFProjectLog, pLog );

   CString log_msg_header;
   log_msg_header.Format(_T("The following error occured while saving %s"),lpszPathName );
   pLog->LogMessage( log_msg_header );

   CPGSuperDocTemplateBase* pTemplate = (CPGSuperDocTemplateBase*)GetDocTemplate();

   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("CLSID_StructuredSave not registered") );
      AfxFormatString1(msg1,IDS_E_BADINSTALL, pTemplate->GetAppName() );
      break;

   case STRSAVE_E_CANTOPEN:
      pLog->LogMessage( TEXT("Could not open file") );
      AfxFormatString1( msg1, IDS_E_FILENOTFOUND, lpszPathName );
      break;

   case STRSAVE_E_BADWRITE:
      pLog->LogMessage( TEXT("Error Writing to File") );
      AfxFormatString1( msg1, IDS_E_WRITE, lpszPathName );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format(_T("An unknown error occured while closing the file (hr = %d)"),hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_WRITE, lpszPathName );
      }
      break;
   }

   CString msg;
   CString msg2;
   std::_tstring strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );
}

void CPGSDocBase::HandleConvertDocumentError( HRESULT hr, LPCTSTR lpszPathName )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Skipping the default functionality and replacing it with something better
   //CEAFBrokerDocument::HandleConvertDocumentError(hr,lpszPathName);

   GET_IFACE( IEAFProjectLog, pLog );

   CPGSuperDocTemplateBase* pTemplate = (CPGSuperDocTemplateBase*)GetDocTemplate();

   CString log_msg_header;
   log_msg_header.Format(_T("The following error occured while converting %s"),lpszPathName );
   pLog->LogMessage( log_msg_header );

   CString msg1;
   switch( hr )
   {
   case REGDB_E_CLASSNOTREG:
      pLog->LogMessage( TEXT("File converter is not registered") );
      AfxFormatString1(msg1,IDS_E_BADINSTALL, pTemplate->GetAppName() );
      break;

   case E_INVALIDARG:
      msg1.Format( _T("%s could not be opened"),lpszPathName);
      pLog->LogMessage( msg1 );
      break;

   default:
      {
         CString log_msg;
         log_msg.Format(_T("An unknown error occured while converting the file (hr = %d)"),hr);
         pLog->LogMessage( log_msg );
         AfxFormatString1( msg1, IDS_E_READ, lpszPathName );
      }
      break;
   }

   CString msg;
   CString msg2;
   std::_tstring strLogFileName = pLog->GetName();
   AfxFormatString1( msg2, IDS_E_PROBPERSISTS, CString(strLogFileName.c_str()) );
   AfxFormatString2(msg, IDS_E_FORMAT, msg1, msg2 );
   AfxMessageBox( msg );
}

void CPGSDocBase::OnProjectEnvironment() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( IEnvironment, pEnvironment );

   enumExposureCondition ec = pEnvironment->GetExposureCondition();
   int expCond = ( ec == expNormal ? 0 : 1 );
   
   Float64 relHumidity = pEnvironment->GetRelHumidity();

   CEnvironmentDlg dlg;
   dlg.m_Exposure = expCond;
   dlg.m_RelHumidity = relHumidity;

   if ( dlg.DoModal() )
   {
      if ( expCond != dlg.m_Exposure || relHumidity != dlg.m_RelHumidity )
      {
         txnEditEnvironment* pTxn = new txnEditEnvironment(ec,dlg.m_Exposure == 0 ? expNormal : expSevere,relHumidity,dlg.m_RelHumidity);
         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
      }
   }
}

void CPGSDocBase::OnEffectiveFlangeWidth()
{
   EditEffectiveFlangeWidth();
}

/*--------------------------------------------------------------------*/
void CPGSDocBase::OnProjectSpec() 
{
   SelectProjectCriteria();
}

void CPGSDocBase::OnRatingSpec()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( ILibraryNames, pLibNames );

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CRatingOptionsDlg dlg;
   txnRatingCriteriaData oldData;

   std::vector<std::_tstring> specs;
   pLibNames->EnumRatingCriteriaNames( &specs );
   dlg.m_GeneralPage.m_RatingSpecs = specs;

   std::vector<std::_tstring> all_names;
   pLibNames->EnumLiveLoadNames( &all_names );
   dlg.m_LegalPage.m_AllNames  = all_names;
   dlg.m_PermitPage.m_AllNames = all_names;

   GET_IFACE( IRatingSpecification, pSpec );
   std::_tstring cur_spec = pSpec->GetRatingSpecification();
   oldData.m_General.CriteriaName  = cur_spec;
   oldData.m_General.bIncludePedestrianLiveLoad = pSpec->IncludePedestrianLiveLoad();
   oldData.m_General.TimelineMgr = *pTimelineMgr;

   oldData.m_General.SystemFactorFlexure = pSpec->GetSystemFactorFlexure();
   oldData.m_General.SystemFactorShear   = pSpec->GetSystemFactorShear();

   oldData.m_General.ADTT = pSpec->GetADTT();

   oldData.m_General.bDesignRating = pSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory);
   oldData.m_General.bLegalRating  = (pSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine)  || pSpec->IsRatingEnabled(pgsTypes::lrLegal_Special));
   oldData.m_General.bPermitRating = (pSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) || pSpec->IsRatingEnabled(pgsTypes::lrPermit_Special));

   oldData.m_Design.StrengthI_DC           = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_DW           = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_LL_Inventory = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_LL_Operating = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthI_Operating);
   oldData.m_Design.StrengthI_CR           = pSpec->GetCreepFactor(         pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_SH           = pSpec->GetShrinkageFactor(     pgsTypes::StrengthI_Inventory);
   oldData.m_Design.StrengthI_PS           = pSpec->GetSecondaryEffectsFactor(     pgsTypes::StrengthI_Inventory);

   oldData.m_Design.ServiceIII_DC          = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_DW          = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_LL          = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_CR          = pSpec->GetCreepFactor(         pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_SH          = pSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_Inventory);
   oldData.m_Design.ServiceIII_PS          = pSpec->GetSecondaryEffectsFactor(     pgsTypes::ServiceIII_Inventory);

   oldData.m_Design.AllowableTensionCoefficient = pSpec->GetAllowableTensionCoefficient(pgsTypes::lrDesign_Inventory);
   oldData.m_Design.bRateForShear = pSpec->RateForShear(pgsTypes::lrDesign_Inventory);

   GET_IFACE(ILiveLoads,pLiveLoads);
   oldData.m_Legal.IM_Truck_Routine = pLiveLoads->GetTruckImpact(pgsTypes::lltLegalRating_Routine);
   oldData.m_Legal.IM_Lane_Routine  = pLiveLoads->GetLaneImpact( pgsTypes::lltLegalRating_Routine);
   oldData.m_Legal.IM_Truck_Special = pLiveLoads->GetTruckImpact(pgsTypes::lltLegalRating_Special);
   oldData.m_Legal.IM_Lane_Special  = pLiveLoads->GetLaneImpact( pgsTypes::lltLegalRating_Special);
   oldData.m_Legal.RoutineNames     = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Routine);
   oldData.m_Legal.SpecialNames = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Special);
   oldData.m_Legal.EmergencyNames = pLiveLoads->GetLiveLoadNames(pgsTypes::lltLegalRating_Emergency);

   oldData.m_Legal.StrengthI_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthI_LegalRoutine);
   oldData.m_Legal.StrengthI_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthI_LegalRoutine);
   oldData.m_Legal.StrengthI_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthI_LegalRoutine);
   oldData.m_Legal.StrengthI_LL_Special = pSpec->GetLiveLoadFactor(pgsTypes::StrengthI_LegalSpecial);
   oldData.m_Legal.StrengthI_LL_Emergency = pSpec->GetLiveLoadFactor(pgsTypes::StrengthI_LegalEmergency);
   oldData.m_Legal.StrengthI_CR         = pSpec->GetCreepFactor(         pgsTypes::StrengthI_LegalSpecial);
   oldData.m_Legal.StrengthI_SH         = pSpec->GetShrinkageFactor(     pgsTypes::StrengthI_LegalSpecial);
   oldData.m_Legal.StrengthI_PS         = pSpec->GetSecondaryEffectsFactor(     pgsTypes::StrengthI_LegalSpecial);

   oldData.m_Legal.ServiceIII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_LegalRoutine);
   oldData.m_Legal.ServiceIII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_LegalRoutine);
   oldData.m_Legal.ServiceIII_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_LegalRoutine);
   oldData.m_Legal.ServiceIII_LL_Special = pSpec->GetLiveLoadFactor(pgsTypes::ServiceIII_LegalSpecial);
   oldData.m_Legal.ServiceIII_LL_Emergency = pSpec->GetLiveLoadFactor(pgsTypes::ServiceIII_LegalEmergency);
   oldData.m_Legal.ServiceIII_CR         = pSpec->GetCreepFactor(         pgsTypes::ServiceIII_LegalSpecial);
   oldData.m_Legal.ServiceIII_SH         = pSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_LegalSpecial);
   oldData.m_Legal.ServiceIII_PS         = pSpec->GetSecondaryEffectsFactor(     pgsTypes::ServiceIII_LegalSpecial);

   oldData.m_Legal.AllowableTensionCoefficient = pSpec->GetAllowableTensionCoefficient(pgsTypes::lrLegal_Routine);
   oldData.m_Legal.bRateForShear    = pSpec->RateForShear(pgsTypes::lrLegal_Routine);
   oldData.m_Legal.bExcludeLaneLoad = pSpec->ExcludeLegalLoadLaneLoading();

   oldData.m_Permit.RoutinePermitNames = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Routine);
   oldData.m_Permit.SpecialPermitNames = pLiveLoads->GetLiveLoadNames(pgsTypes::lltPermitRating_Special);

   oldData.m_Permit.StrengthII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_CR         = pSpec->GetCreepFactor(         pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_SH         = pSpec->GetShrinkageFactor(     pgsTypes::StrengthII_PermitRoutine);
   oldData.m_Permit.StrengthII_PS         = pSpec->GetSecondaryEffectsFactor(     pgsTypes::StrengthII_PermitRoutine);

   oldData.m_Permit.StrengthII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_CR         = pSpec->GetCreepFactor(         pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_SH         = pSpec->GetShrinkageFactor(     pgsTypes::StrengthII_PermitSpecial);
   oldData.m_Permit.StrengthII_PS         = pSpec->GetSecondaryEffectsFactor(     pgsTypes::StrengthII_PermitSpecial);

   oldData.m_Permit.ServiceI_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_CR         = pSpec->GetCreepFactor(         pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_SH         = pSpec->GetShrinkageFactor(     pgsTypes::ServiceI_PermitRoutine);
   oldData.m_Permit.ServiceI_PS         = pSpec->GetSecondaryEffectsFactor(     pgsTypes::ServiceI_PermitRoutine);

   oldData.m_Permit.ServiceIII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_PermitRoutine);
   oldData.m_Permit.ServiceIII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_PermitRoutine);
   oldData.m_Permit.ServiceIII_LL_Routine = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_PermitRoutine);
   oldData.m_Permit.ServiceIII_CR         = pSpec->GetCreepFactor(         pgsTypes::ServiceIII_PermitRoutine);
   oldData.m_Permit.ServiceIII_SH         = pSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_PermitRoutine);
   oldData.m_Permit.ServiceIII_PS         = pSpec->GetSecondaryEffectsFactor(     pgsTypes::ServiceIII_PermitRoutine);

   oldData.m_Permit.ServiceI_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_CR         = pSpec->GetCreepFactor(         pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_SH         = pSpec->GetShrinkageFactor(     pgsTypes::ServiceI_PermitSpecial);
   oldData.m_Permit.ServiceI_PS         = pSpec->GetSecondaryEffectsFactor(     pgsTypes::ServiceI_PermitSpecial);

   oldData.m_Permit.ServiceIII_DC         = pSpec->GetDeadLoadFactor(      pgsTypes::ServiceIII_PermitSpecial);
   oldData.m_Permit.ServiceIII_DW         = pSpec->GetWearingSurfaceFactor(pgsTypes::ServiceIII_PermitSpecial);
   oldData.m_Permit.ServiceIII_LL_Special = pSpec->GetLiveLoadFactor(      pgsTypes::ServiceIII_PermitSpecial);
   oldData.m_Permit.ServiceIII_CR         = pSpec->GetCreepFactor(         pgsTypes::ServiceIII_PermitSpecial);
   oldData.m_Permit.ServiceIII_SH         = pSpec->GetShrinkageFactor(     pgsTypes::ServiceIII_PermitSpecial);
   oldData.m_Permit.ServiceIII_PS         = pSpec->GetSecondaryEffectsFactor(     pgsTypes::ServiceIII_PermitSpecial);

   oldData.m_Permit.IM_Truck_Routine = pLiveLoads->GetTruckImpact(pgsTypes::lltPermitRating_Routine);
   oldData.m_Permit.IM_Lane_Routine  = pLiveLoads->GetLaneImpact( pgsTypes::lltPermitRating_Routine);

   oldData.m_Permit.IM_Truck_Special = pLiveLoads->GetTruckImpact(pgsTypes::lltPermitRating_Special);
   oldData.m_Permit.IM_Lane_Special  = pLiveLoads->GetLaneImpact( pgsTypes::lltPermitRating_Special);

   oldData.m_Permit.bRateForShear = pSpec->RateForShear(pgsTypes::lrPermit_Routine);
   oldData.m_Permit.bRateForStress = pSpec->RateForStress(pgsTypes::lrPermit_Routine);
   oldData.m_Permit.AllowableTensionCoefficient = pSpec->GetAllowableTensionCoefficient(pgsTypes::lrPermit_Routine);
   oldData.m_Permit.bCheckReinforcementYielding = pSpec->CheckYieldStress(pgsTypes::lrPermit_Routine);
   oldData.m_Permit.YieldStressCoefficient = pSpec->GetYieldStressLimitCoefficient();
   oldData.m_Permit.SpecialPermitType = pSpec->GetSpecialPermitType();

   dlg.m_GeneralPage.m_Data = oldData.m_General;
   dlg.m_DesignPage.m_Data  = oldData.m_Design;
   dlg.m_LegalPage.m_Data   = oldData.m_Legal;
   dlg.m_PermitPage.m_Data  = oldData.m_Permit;


   if ( dlg.DoModal() == IDOK )
   {
      txnRatingCriteriaData newData;

      newData.m_General = dlg.m_GeneralPage.m_Data;
      newData.m_Design  = dlg.m_DesignPage.m_Data;
      newData.m_Legal   = dlg.m_LegalPage.m_Data;
      newData.m_Permit  = dlg.m_PermitPage.m_Data;

      txnTransaction* pExtensionTxn = dlg.GetExtensionPageTransaction();

      if ( oldData != newData || pExtensionTxn )
      {
         txnTransaction* pTxn = new txnEditRatingCriteria(oldData,newData);
         if ( pExtensionTxn )
         {
            txnMacroTxn* pMacro = new pgsMacroTxn;
            pMacro->Name(pTxn->Name());
            pMacro->AddTransaction(pTxn);
            pMacro->AddTransaction(pExtensionTxn);
            pTxn = pMacro;
         }

         GET_IFACE(IEAFTransactions,pTransactions);
         pTransactions->Execute(pTxn);
      }
   }
}

void CPGSDocBase::OnAutoCalc()
{
   CEAFAutoCalcDocMixin::OnAutoCalc();   
}

void CPGSDocBase::OnUpdateAutoCalc(CCmdUI* pCmdUI)
{
   CEAFAutoCalcDocMixin::OnUpdateAutoCalc(pCmdUI);   
}

/*--------------------------------------------------------------------*/
void CPGSDocBase::OnExportToTemplateFile() 
{
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSuper = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // select inital directory to try and save in
   CString extension;
   pTemplate->GetDocString(extension,CDocTemplate::filterExt);
   int pos = 0;
   CString ext = extension.Tokenize(_T(";"),pos); // extension of project file
   ext = extension.Tokenize(_T(";"),pos); // extension of template file

   CString default_name;
   default_name.Format(_T("%s%s"),pPGSuper->GetAppName(),ext);
   CString initial_filespec;
   CString initial_dir;
   
   // prompt user to save current project to a template file
   CString strFilter;
   strFilter.Format(_T("%s Template Files (*%s)|*%s||"),pPGSuper->GetAppName(),ext,ext);
   CFileDialog  fildlg(FALSE,extension,default_name,OFN_HIDEREADONLY,strFilter);

#if defined _DEBUG
   // If this is a debug build, then the developers are probably running
   // the software and they want the workgroup folder most often.
   CString workgroup_folder;
   pPGSuper->GetTemplateFolders(workgroup_folder);
   fildlg.m_ofn.lpstrInitialDir = workgroup_folder;
#else
   fildlg.m_ofn.lpstrInitialDir = initial_dir;
#endif // _DEBUG

   INT_PTR stf = fildlg.DoModal();
   if (stf==IDOK)
   {
      // try loop with a new file name
      CString file_path = fildlg.GetPathName();

      // check if file exists and prompt user if he wants to overwrite
      if (DoesFileExist(file_path))
      {
         CString msg(_T(" The file: "));
         msg += file_path + _T(" exists. Overwrite it?");
         int stm = AfxMessageBox(msg,MB_YESNOCANCEL|MB_ICONQUESTION);
         if (stm!=IDYES)
         {
            return;
         }
      }

      // write the file.
      SaveTheDocument( file_path );
   }
}

bool DoesFolderExist(const CString& dirname)
{
   if (dirname.IsEmpty())
   {
      return false;
   }
   else
   {
      CFileFind finder;
      BOOL is_file;
      CString nam = dirname + CString(_T("\\*.*"));
      is_file = finder.FindFile(nam);
      return (is_file!=0);
   }
}

bool DoesFileExist(const CString& filename)
{
   if (filename.IsEmpty())
   {
      return false;
   }
   else
   {
      CFileFind finder;
      BOOL is_file;
      is_file = finder.FindFile(filename);
      return (is_file!=0);
   }
}


void CPGSDocBase::OnViewsettingsBridgemodelEditor() 
{
   EditBridgeViewSettings(0);
}

void CPGSDocBase::OnViewsettingsGirderEditor() 
{
   EditGirderViewSettings(0);
}


void CPGSDocBase::OnLoadsLoadModifiers() 
{
   GET_IFACE(ILoadModifiers,pLoadModifiers);

   txnEditLoadModifiers::LoadModifiers loadModifiers;
   loadModifiers.DuctilityLevel    = pLoadModifiers->GetDuctilityLevel();
   loadModifiers.DuctilityFactor   = pLoadModifiers->GetDuctilityFactor();
   loadModifiers.ImportanceLevel   = pLoadModifiers->GetImportanceLevel();
   loadModifiers.ImportanceFactor  = pLoadModifiers->GetImportanceFactor();
   loadModifiers.RedundancyLevel   = pLoadModifiers->GetRedundancyLevel();
   loadModifiers.RedundancyFactor  = pLoadModifiers->GetRedundancyFactor();

   CEAFHelpHandler helpHandler(GetDocumentationSetName(),IDH_DIALOG_LOADMODIFIERS);
   CLoadModifiersDlg dlg;
   dlg.SetHelpData( &helpHandler, &helpHandler, &helpHandler );

   dlg.SetLoadModifiers( loadModifiers.DuctilityFactor,
                         ((loadModifiers.DuctilityLevel == ILoadModifiers::High)  ? 0 : (loadModifiers.DuctilityLevel  == ILoadModifiers::Normal ? 1 : 2)),
                         loadModifiers.RedundancyFactor,
                         ((loadModifiers.RedundancyLevel == ILoadModifiers::High) ? 0 : (loadModifiers.RedundancyLevel == ILoadModifiers::Normal ? 1 : 2)),
                         loadModifiers.ImportanceFactor,
                         ((loadModifiers.ImportanceLevel == ILoadModifiers::High) ? 0 : (loadModifiers.ImportanceLevel == ILoadModifiers::Normal ? 1 : 2))
                        );

   if ( dlg.DoModal() == IDOK )
   {
      txnEditLoadModifiers::LoadModifiers newLoadModifiers;

      Int16 d,r,i;
      dlg.GetLoadModifiers(&newLoadModifiers.DuctilityFactor, &d,
                           &newLoadModifiers.RedundancyFactor,&r,
                           &newLoadModifiers.ImportanceFactor,&i);

      newLoadModifiers.DuctilityLevel  = (d == 0 ? ILoadModifiers::High : (d == 1 ? ILoadModifiers::Normal : ILoadModifiers::Low));
      newLoadModifiers.RedundancyLevel = (r == 0 ? ILoadModifiers::High : (r == 1 ? ILoadModifiers::Normal : ILoadModifiers::Low));
      newLoadModifiers.ImportanceLevel = (i == 0 ? ILoadModifiers::High : (i == 1 ? ILoadModifiers::Normal : ILoadModifiers::Low));

      txnEditLoadModifiers* pTxn = new txnEditLoadModifiers(loadModifiers,newLoadModifiers);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSDocBase::OnLoadsLoadFactors()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(ILoadFactors,pLoadFactors);

   CLoadFactors loadFactors = *pLoadFactors->GetLoadFactors();
   CLoadFactorsDlg dlg;
   dlg.m_LoadFactors = loadFactors;
   if ( dlg.DoModal() == IDOK )
   {
      txnEditLoadFactors* pTxn = new txnEditLoadFactors(loadFactors,dlg.m_LoadFactors);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSDocBase::UpdateAnalysisTypeStatusIndicator()
{
   CPGSuperStatusBar* pStatusBar = (CPGSuperStatusBar*)(EAFGetMainFrame()->GetStatusBar());

   GET_IFACE(ISpecification,pSpec);
   pStatusBar->SetAnalysisTypeStatusIndicator(pSpec->GetAnalysisType());
}

bool CPGSDocBase::LoadMasterLibrary()
{
   WATCH(_T("Loading Master Library"));

   // Load the master library
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSuper = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CString strMasterLibaryFile = pPGSuper->GetCachedMasterLibraryFile();

   std::_tstring strPublisher = pPGSuper->GetMasterLibraryPublisher();
   std::_tstring strMasterLibFile = pPGSuper->GetMasterLibraryFile();

   m_LibMgr.SetMasterLibraryInfo(strPublisher.c_str(),strMasterLibFile.c_str());

   return DoLoadMasterLibrary(strMasterLibaryFile);
}

bool CPGSDocBase::DoLoadMasterLibrary(const CString& strMasterLibraryFile)
{
   if ( strMasterLibraryFile.GetLength() == 0 )
   {
      return true;
   }

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // loop until a library file is opened or user gives up trying to find it
   CString strFile = strMasterLibraryFile;
   CString err_msg;

   bool bSuccess = false;
   while(!bSuccess)
   {
      eafTypes::UnitMode unitMode;
      HRESULT hr = pgslibLoadLibrary(strFile,&m_LibMgr,&unitMode);
      if ( FAILED(hr) )
      {
         WATCH(_T("Failed to load master library"));
         AfxFormatString1(err_msg, IDS_CORRUPTED_LIBRARY_FILE, strFile);

         // if we are here, an error occured. Issue the message and give
         // the user a chance to load another library file

         if ( AfxMessageBox(err_msg,MB_YESNO|MB_ICONSTOP) == IDYES )
         {
            CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
            CComPtr<IEAFAppPlugin> pAppPlugin;
            pTemplate->GetPlugin(&pAppPlugin);
            CPGSAppPluginBase* pPGSuper = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

#pragma Reminder("WORKING HERE - start up") // need a replacement for the call below
            //pPGSuper->UpdateProgramSettings(TRUE);
            strFile = pPGSuper->GetCachedMasterLibraryFile();
         }
         else
         {
            bSuccess = true;
         }
      }
      else
      {
         bSuccess = true;
      }
   }

   // make all entries in master library read-only
   m_LibMgr.EnableEditingForAllEntries(false);


   // Remove all girder entries that are not associated with
   // the beam family ID (basically removing PGSuper girders
   // for PGSplice and visa-versa)
   CATID catid = GetBeamFamilyCategoryID();
   CComPtr<ICatRegister> pICatReg = 0;
   HRESULT hr;
   hr = ::CoCreateInstance( CLSID_StdComponentCategoriesMgr,
                            nullptr,
                            CLSCTX_INPROC_SERVER,
                            IID_ICatRegister,
                            (void**)&pICatReg );

   CComPtr<ICatInformation> pICatInfo;
   pICatReg->QueryInterface(IID_ICatInformation,(void**)&pICatInfo);

   GirderLibrary& gdrLib = m_LibMgr.GetGirderLibrary();
   libKeyListType keyList;
   gdrLib.KeyList(keyList);
   CollectionIndexType nEntries = gdrLib.GetCount();
   for ( CollectionIndexType i = 0; i < nEntries; i++ )
   {
      std::_tstring strName = keyList[i];
      const libLibraryEntry* pEntry = gdrLib.GetEntry(strName.c_str());
      const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pEntry);
      CComPtr<IBeamFactory> beamFactory;
      pGdrEntry->GetBeamFactory(&beamFactory);
      CLSID clsid = beamFactory->GetFamilyCLSID();

      HRESULT result = pICatInfo->IsClassOfCategories(clsid,1,&catid,0,nullptr);
      if ( result == S_FALSE )
      { 
         gdrLib.RemoveEntry(strName.c_str());
      }
   }

   return true; // the only way out alive!
}

CSelection CPGSDocBase::GetSelection()
{
   return m_Selection;
}

void CPGSDocBase::SetSelection(const CSelection& selection,BOOL bNotify)
{
   if ( m_Selection != selection )
   {
      m_Selection = selection;
      if ( bNotify )
      {
         UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&m_Selection);
      }
   }
}

void CPGSDocBase::SelectPier(PierIndexType pierIdx,BOOL bNotify)
{
   if ( m_Selection.Type == CSelection::Pier && m_Selection.PierIdx == pierIdx )
   {
      return;
   }

   m_Selection.Type       = CSelection::Pier;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = pierIdx;
   m_Selection.tsID       = INVALID_INDEX;

   CSelection selection = m_Selection;
   if ( bNotify )
   {
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
   }
}

void CPGSDocBase::SelectSpan(SpanIndexType spanIdx,BOOL bNotify)
{
   if ( m_Selection.Type == CSelection::Span && m_Selection.SpanIdx == spanIdx )
   {
      return;
   }

   m_Selection.Type       = CSelection::Span;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.SpanIdx    = spanIdx;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   CSelection selection = m_Selection;
   if ( bNotify )
   {
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
   }
}

void CPGSDocBase::SelectGirder(const CGirderKey& girderKey,BOOL bNotify)
{
   if ( m_Selection.Type == CSelection::Girder && m_Selection.GroupIdx == girderKey.groupIndex && m_Selection.GirderIdx == girderKey.girderIndex )
   {
      return;
   }

   if ( m_bSelectingSegment || m_bClearingSelection )
   {
      return;
   }

   m_Selection.Type       = CSelection::Girder;
   m_Selection.GroupIdx   = girderKey.groupIndex;
   m_Selection.GirderIdx  = girderKey.girderIndex;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;

      if ( bNotify )
      {
         m_bSelectingGirder = true;
         UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
         m_bSelectingGirder = false;
      }
      bProcessingSelectionChanged = false;
   }
}

void CPGSDocBase::SelectSegment(const CSegmentKey& segmentKey,BOOL bNotify)
{
   if ( m_Selection.Type == CSelection::Segment && m_Selection.GroupIdx == segmentKey.groupIndex && m_Selection.GirderIdx == segmentKey.girderIndex && m_Selection.SegmentIdx == segmentKey.segmentIndex )
   {
      return;
   }

   if ( m_bSelectingGirder || m_bClearingSelection )
   {
      return;
   }

   m_Selection.Type       = CSelection::Segment;
   m_Selection.GroupIdx   = segmentKey.groupIndex;
   m_Selection.GirderIdx  = segmentKey.girderIndex;
   m_Selection.SegmentIdx = segmentKey.segmentIndex;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      if ( bNotify )
      {
         m_bSelectingSegment = true;
         UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
         m_bSelectingSegment = false;
      }
      bProcessingSelectionChanged = false;
   }
}

void CPGSDocBase::SelectClosureJoint(const CClosureKey& closureKey,BOOL bNotify)
{
   if ( m_Selection.Type == CSelection::ClosureJoint && m_Selection.GroupIdx == closureKey.groupIndex && m_Selection.GirderIdx == closureKey.girderIndex && m_Selection.SegmentIdx == closureKey.segmentIndex )
   {
      return;
   }

   m_Selection.Type       = CSelection::ClosureJoint;
   m_Selection.GroupIdx   = closureKey.groupIndex;
   m_Selection.GirderIdx  = closureKey.girderIndex;
   m_Selection.SegmentIdx = closureKey.segmentIndex;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      if ( bNotify )
      {
         UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
      }
      bProcessingSelectionChanged = false;
   }
}

void CPGSDocBase::SelectTemporarySupport(SupportIDType tsID,BOOL bNotify)
{
   if ( m_Selection.Type == CSelection::TemporarySupport && m_Selection.tsID == tsID )
   {
      return;
   }

   m_Selection.Type       = CSelection::TemporarySupport;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = tsID;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      if ( bNotify )
      {
         UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
      }
      bProcessingSelectionChanged = false;
   }
}

void CPGSDocBase::SelectDeck(BOOL bNotify)
{
   if ( m_Selection.Type == CSelection::Deck )
   {
      return;
   }

   m_Selection.Type       = CSelection::Deck;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   CSelection selection = m_Selection;
   if ( bNotify )
   {
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
   }
}

void CPGSDocBase::SelectAlignment(BOOL bNotify)
{
   if ( m_Selection.Type == CSelection::Alignment )
   {
      return;
   }

   m_Selection.Type       = CSelection::Alignment;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   CSelection selection = m_Selection;
   if ( bNotify )
   {
      UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
   }
}

void CPGSDocBase::SelectTrafficBarrier(pgsTypes::TrafficBarrierOrientation orientation,BOOL bNotify)
{
   if ( (orientation == pgsTypes::tboLeft && m_Selection.Type == CSelection::LeftRailingSystem) || (orientation == pgsTypes::tboRight && m_Selection.Type == CSelection::RightRailingSystem) )
   {
      return;
   }

   m_Selection.Type = (orientation == pgsTypes::tboLeft ? CSelection::LeftRailingSystem : CSelection::RightRailingSystem);
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx = INVALID_INDEX;
   m_Selection.GroupIdx = INVALID_INDEX;
   m_Selection.SpanIdx = INVALID_INDEX;
   m_Selection.PierIdx = INVALID_INDEX;
   m_Selection.tsID = INVALID_INDEX;

   CSelection selection = m_Selection;
   if (bNotify)
   {
      UpdateAllViews(0, HINT_SELECTIONCHANGED, (CObject*)&selection);
   }
}

void CPGSDocBase::ClearSelection(BOOL bNotify)
{
   if ( m_Selection.Type == CSelection::None )
   {
      return;
   }

   m_Selection.Type       = CSelection::None;
   m_Selection.SegmentIdx = INVALID_INDEX;
   m_Selection.GirderIdx  = INVALID_INDEX;
   m_Selection.GroupIdx   = INVALID_INDEX;
   m_Selection.SpanIdx    = INVALID_INDEX;
   m_Selection.PierIdx    = INVALID_INDEX;
   m_Selection.tsID       = INVALID_INDEX;

   static bool bProcessingSelectionChanged = false;
   if ( !bProcessingSelectionChanged )
   {
      bProcessingSelectionChanged = true;
      CSelection selection = m_Selection;
      if ( bNotify )
      {
         m_bClearingSelection = true;
         UpdateAllViews(0,HINT_SELECTIONCHANGED,(CObject*)&selection);
         m_bClearingSelection = false;
      }
      bProcessingSelectionChanged = false;
   }
}

void CPGSDocBase::OnCopyGirderProps() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CCopyGirderDlg dlg(m_pBroker,m_CopyGirderPropertiesCallbacks);
   dlg.DoModal();
}

void CPGSDocBase::OnImportProjectLibrary() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CDocTemplate* pTemplate = GetDocTemplate();
   CString strFilterExt;
   pTemplate->GetDocString(strFilterExt,CDocTemplate::filterExt);
   strFilterExt.Replace(_T("."),_T("*."));

   CString strFilter;
   pTemplate->GetDocString(strFilter,CDocTemplate::filterName);

   CString strFilter2;
   strFilter2.Format(_T("%s|%s||"),strFilter,strFilterExt);

	// ask user for file name
   CFileDialog  fileDlg(TRUE,nullptr,nullptr,OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,strFilter2);
   if (fileDlg.DoModal() == IDOK)
   {
      CString rPath;
      rPath = fileDlg.GetPathName();

      GET_IFACE( IImportProjectLibrary, pImport );

      CString real_file_name; // name of actual file to be read may be different than lpszPathName
      HRESULT convert_status = ConvertTheDocument(rPath, &real_file_name);
      // convert document. if file was converted, then we need to delete the converted file at the end
      if ( FAILED(convert_status) )
      {
         HandleOpenDocumentError( STRLOAD_E_INVALIDFORMAT, rPath );
         ASSERT(FALSE);
      }
      else
      {
         // NOTE: Although it looks innocent, this control block is very important!! 
         // This is because the IStructuredLoad must be destroyed before the temp 
         // file can be deleted

         CComPtr<IStructuredLoad> pStrLoad;
         HRESULT hr = ::CoCreateInstance( CLSID_StructuredLoad, nullptr, CLSCTX_INPROC_SERVER, IID_IStructuredLoad, (void**)&pStrLoad );
         if ( FAILED(hr) )
         {
            // We are not aggregating so we should CoCreateInstance should
            // never fail with this HRESULT
            ASSERT( hr != CLASS_E_NOAGGREGATION );

            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }

         hr = pStrLoad->Open( real_file_name );
         if ( FAILED(hr) )
         {
            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }


         // advance the structured load pointer to the correct point for agent
         hr = pgslibReadProjectDocHeader(GetRootNodeName(),pStrLoad);
         if ( FAILED(hr) )
         {
            HandleOpenDocumentError(hr,rPath);
            ASSERT(FALSE);
         }

         if ( !pImport->ImportProjectLibraries( pStrLoad ) )
         {
            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }

         hr = pStrLoad->Close();
         if ( FAILED(hr) )
         {
            HandleOpenDocumentError( hr, rPath );
            ASSERT(FALSE);
         }
      }

      if (convert_status == S_OK)
      {
         // file was converted and written to a temporary file. delete the temp file
         CFile::Remove(real_file_name);
      }


      SetModifiedFlag();
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      UpdateAllViews(nullptr, HINT_LIBRARYCHANGED);

      AfxMessageBox(_T("Done getting library entries"));
   }
}

void CPGSDocBase::OnLoadsLldf() 
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   pgsTypes::DistributionFactorMethod method = pBridgeDesc->GetDistributionFactorMethod();
   LldfRangeOfApplicabilityAction roaAction = pLiveLoads->GetLldfRangeOfApplicabilityAction();
                  
   OnLoadsLldf(method,roaAction);
}

void CPGSDocBase::OnLoadsLldf(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pOldBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CLiveLoadDistFactorsDlg dlg;
   dlg.m_BridgeDesc = *pOldBridgeDesc;
   dlg.m_BridgeDesc.SetDistributionFactorMethod(method);
   dlg.m_LldfRangeOfApplicabilityAction = roaAction;
   dlg.m_pBroker = m_pBroker;

   if ( dlg.DoModal() == IDOK )
   {
      GET_IFACE(ILiveLoads,pLiveLoads);

      txnEditLLDF* pTxn = new txnEditLLDF(*pOldBridgeDesc,dlg.m_BridgeDesc,
                                          pLiveLoads->GetLldfRangeOfApplicabilityAction(),dlg.m_LldfRangeOfApplicabilityAction);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSDocBase::OnAddPointload() 
{
   CPointLoadData load;
   AddPointLoad(load);
}

void CPGSDocBase::OnAddDistributedLoad() 
{
   CDistributedLoadData load;
   AddDistributedLoad(load);
}

void CPGSDocBase::OnAddMomentLoad() 
{
   CMomentLoadData load;
   AddMomentLoad(load);
}

void CPGSDocBase::OnConstructionLoads()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CConstructionLoadDlg dlg;
   GET_IFACE(IUserDefinedLoadData,pLoads);
   Float64 load = pLoads->GetConstructionLoad();
   dlg.m_Load = load;

   if ( dlg.DoModal() == IDOK )
   {
      txnEditConstructionLoad* pTxn = new txnEditConstructionLoad(load,dlg.m_Load);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSDocBase::OnProjectAlignment() 
{
   EditAlignmentDescription(EBD_ROADWAY);
}

void CPGSDocBase::OnProjectBarriers()
{
   EditBridgeDescription(EBD_RAILING);
}

void CPGSDocBase::OnProjectProfile()
{
   EditAlignmentDescription(EBD_PROFILE);
}

void CPGSDocBase::OnLiveLoads() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE( ILibraryNames, pLibNames );
   GET_IFACE( ILiveLoads, pLiveLoad );
   GET_IFACE( IProductLoads,pProductLoads);
   GET_IFACE( IBridgeDescription,pIBridgeDesc);

   std::vector<std::_tstring> all_names;
   pLibNames->EnumLiveLoadNames( &all_names );

   std::vector<std::_tstring> design_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltDesign);
   std::vector<std::_tstring> fatigue_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltFatigue);
   std::vector<std::_tstring> permit_names = pLiveLoad->GetLiveLoadNames(pgsTypes::lltPermit);

   CLiveLoadSelectDlg dlg(all_names, design_names, fatigue_names, permit_names);

   dlg.m_bHasPedestrianLoad = (pProductLoads->GetPedestrianLoadPerSidewalk(pgsTypes::tboLeft)>0.0 ||
                               pProductLoads->GetPedestrianLoadPerSidewalk(pgsTypes::tboRight)>0.0);

   EventIndexType oldLiveLoadEvent = pIBridgeDesc->GetTimelineManager()->GetLiveLoadEventIndex();
   dlg.m_LiveLoadEvent = oldLiveLoadEvent;

   dlg.m_DesignTruckImpact = pLiveLoad->GetTruckImpact(pgsTypes::lltDesign);
   dlg.m_DesignLaneImpact = pLiveLoad->GetLaneImpact(pgsTypes::lltDesign);
   dlg.m_DesignPedesType = pLiveLoad->GetPedestrianLoadApplication(pgsTypes::lltDesign);

   dlg.m_FatigueTruckImpact = pLiveLoad->GetTruckImpact(pgsTypes::lltFatigue);
   dlg.m_FatigueLaneImpact = pLiveLoad->GetLaneImpact(pgsTypes::lltFatigue);
   dlg.m_FatiguePedesType = pLiveLoad->GetPedestrianLoadApplication(pgsTypes::lltFatigue);

   dlg.m_PermitLaneImpact = pLiveLoad->GetLaneImpact(pgsTypes::lltPermit);
   dlg.m_PermitTruckImpact = pLiveLoad->GetTruckImpact(pgsTypes::lltPermit);
   dlg.m_PermitPedesType = pLiveLoad->GetPedestrianLoadApplication(pgsTypes::lltPermit);

   txnEditLiveLoadData oldDesign, oldFatigue, oldPermit;
   oldDesign.m_VehicleNames = dlg.m_DesignNames;
   oldDesign.m_TruckImpact  = dlg.m_DesignTruckImpact;
   oldDesign.m_LaneImpact   = dlg.m_DesignLaneImpact;
   oldDesign.m_PedestrianLoadApplicationType = dlg.m_DesignPedesType;

   oldFatigue.m_VehicleNames = dlg.m_FatigueNames;
   oldFatigue.m_TruckImpact  = dlg.m_FatigueTruckImpact;
   oldFatigue.m_LaneImpact   = dlg.m_FatigueLaneImpact;
   oldFatigue.m_PedestrianLoadApplicationType = dlg.m_FatiguePedesType;

   oldPermit.m_VehicleNames = dlg.m_PermitNames;
   oldPermit.m_TruckImpact  = dlg.m_PermitTruckImpact;
   oldPermit.m_LaneImpact   = dlg.m_PermitLaneImpact;
   oldPermit.m_PedestrianLoadApplicationType = dlg.m_PermitPedesType;

   if ( dlg.DoModal() == IDOK)
   {
      txnEditLiveLoadData newDesign, newFatigue, newPermit;
      newDesign.m_VehicleNames                  = dlg.m_DesignNames;
      newDesign.m_TruckImpact                   = dlg.m_DesignTruckImpact;
      newDesign.m_LaneImpact                    = dlg.m_DesignLaneImpact;
      newDesign.m_PedestrianLoadApplicationType = dlg.m_DesignPedesType;

      newFatigue.m_VehicleNames                  = dlg.m_FatigueNames;
      newFatigue.m_TruckImpact                   = dlg.m_FatigueTruckImpact;
      newFatigue.m_LaneImpact                    = dlg.m_FatigueLaneImpact;
      newFatigue.m_PedestrianLoadApplicationType = dlg.m_FatiguePedesType;

      newPermit.m_VehicleNames                  = dlg.m_PermitNames;
      newPermit.m_TruckImpact                   = dlg.m_PermitTruckImpact;
      newPermit.m_LaneImpact                    = dlg.m_PermitLaneImpact;
      newPermit.m_PedestrianLoadApplicationType = dlg.m_PermitPedesType;

      txnEditLiveLoad* pTxn = new txnEditLiveLoad(oldDesign,newDesign,oldFatigue,newFatigue,oldPermit,newPermit,oldLiveLoadEvent,dlg.m_LiveLoadEvent);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

BOOL CPGSDocBase::GetStatusBarMessageString(UINT nID,CString& rMessage) const
{
   USES_CONVERSION;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( __super::GetStatusBarMessageString(nID,rMessage) )
   {
      return TRUE;
   }

   CPGSDocBase* pThis = const_cast<CPGSDocBase*>(this);
   
   CComPtr<IPGSDataExporter> exporter;
   pThis->m_pPluginMgr->GetExporter(nID,false,&exporter);
   if ( exporter )
   {
      CComBSTR bstr;
      exporter->GetCommandHintText(&bstr);
      rMessage = OLE2T(bstr);
      rMessage.Replace('\n','\0');

      return TRUE;
   }
   
   CComPtr<IPGSDataImporter> importer;
   pThis->m_pPluginMgr->GetImporter(nID,false,&importer);
   if ( importer )
   {
      CComBSTR bstr;
      importer->GetCommandHintText(&bstr);
      rMessage = OLE2T(bstr);
      rMessage.Replace('\n','\0');

      return TRUE;
   }

   return FALSE;
}

BOOL CPGSDocBase::GetToolTipMessageString(UINT nID, CString& rMessage) const
{
   USES_CONVERSION;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( __super::GetToolTipMessageString(nID,rMessage) )
   {
      return TRUE;
   }

   CPGSDocBase* pThis = const_cast<CPGSDocBase*>(this);
   
   CComPtr<IPGSDataExporter> exporter;
   pThis->m_pPluginMgr->GetExporter(nID,false,&exporter);
   if ( exporter )
   {
      CComBSTR bstr;
      exporter->GetCommandHintText(&bstr);
      CString string( OLE2T(bstr) );
      int pos = string.Find('\n');
      if ( 0 < pos )
      {
         rMessage = string.Mid(pos+1);
      }

      return TRUE;
   }
   
   CComPtr<IPGSDataImporter> importer;
   pThis->m_pPluginMgr->GetImporter(nID,false,&importer);
   if ( importer )
   {
      CComBSTR bstr;
      importer->GetCommandHintText(&bstr);
      CString string( OLE2T(bstr) );
      int pos = string.Find('\n');
      if ( 0 < pos )
      {
         rMessage = string.Mid(pos+1);
      }

      return TRUE;
   }

   return FALSE;
}

void CPGSDocBase::CreateReportView(CollectionIndexType rptIdx,BOOL bPrompt)
{
   if ( !bPrompt && m_Selection.Type == CSelection::None)
   {
      // this is a quick report... make sure there is a current span and girder
      m_Selection.Type       = CSelection::Girder;
      m_Selection.GroupIdx   = 0;
      m_Selection.GirderIdx  = 0;
      m_Selection.SpanIdx    = INVALID_INDEX;
      m_Selection.PierIdx    = INVALID_INDEX;
      m_Selection.SegmentIdx = INVALID_INDEX;
      m_Selection.tsID       = INVALID_ID;
   }

   m_pPGSuperDocProxyAgent->CreateReportView(rptIdx,bPrompt);

   // the base class does nothing so we won't bother calling it
}

void CPGSDocBase::CreateGraphView(CollectionIndexType graphIdx)
{
   m_pPGSuperDocProxyAgent->CreateGraphView(graphIdx);

   // the base class does nothing so we won't bother calling it
}

void CPGSDocBase::OnViewBridgeModelEditor()
{
   m_pPGSuperDocProxyAgent->CreateBridgeModelView();
}

void CPGSDocBase::OnViewGirderEditor()
{
   CGirderKey girderKey(m_Selection.GroupIdx,m_Selection.GirderIdx);
   m_pPGSuperDocProxyAgent->CreateGirderView(girderKey);
}

void CPGSDocBase::OnEditUserLoads()
{
   m_pPGSuperDocProxyAgent->CreateLoadsView();
}

void CPGSDocBase::OnViewLibraryEditor()
{
   m_pPGSuperDocProxyAgent->CreateLibraryEditorView();
}

void CPGSDocBase::OnEditPier() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CString strItems;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CString strItem;
      strItem.Format(_T("%s %d\n"),(pierIdx == 0 || pierIdx == nPiers-1 ? _T("Abutment") : _T("Pier")),LABEL_PIER(pierIdx));

      strItems += strItem;
   }

   CSelectItemDlg dlg;
   dlg.m_strTitle = _T("Select Abutment/Pier");
   dlg.m_strItems = strItems;
   dlg.m_strLabel = _T("Select an abutment or pier to edit");
   dlg.m_ItemIdx = m_Selection.PierIdx;

   if ( dlg.DoModal() == IDOK )
   {
      EditPierDescription(dlg.m_ItemIdx,EPD_GENERAL);
   }
}

void CPGSDocBase::OnEditSpan() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   if (nSpans>1)
   {
      CString strItems;
      for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
      {
         CString strItem;
         strItem.Format(_T("Span %d\n"),LABEL_SPAN(spanIdx));

         strItems += strItem;
      }

      CSelectItemDlg dlg;
      dlg.m_strTitle = _T("Select span to edit");
      dlg.m_strItems = strItems;
      dlg.m_strLabel = _T("Select span to edit");
      dlg.m_ItemIdx = m_Selection.SpanIdx;

      if ( dlg.DoModal() == IDOK )
      {
         EditSpanDescription(dlg.m_ItemIdx,ESD_GENERAL);
      }
   }
   else
   {
      // No reason to ask which span if there is only one
      EditSpanDescription(0,ESD_GENERAL);
   }
}

void CPGSDocBase::DeletePier(PierIndexType pierIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // deleting a pier

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSelectItemDlg dlg;

   PierIndexType nPiers = pBridgeDesc->GetPierCount();

   CString strTitle;
   strTitle.Format(_T("Deleting Pier %d"),LABEL_PIER(pierIdx));
   dlg.m_strTitle = strTitle;

   CString strLabel;
   strLabel.Format(_T("%s. Select the span to be deleted with the pier"),strTitle);
   dlg.m_strLabel = strLabel;

   CString strItems;
   if ( pierIdx == 0 )
   {
      strItems.Format(_T("%s"),_T("Span 1\n"));
   }
   else if ( pierIdx == nPiers-1)
   {
      strItems.Format(_T("Span %d\n"),LABEL_SPAN(pierIdx-1));
   }
   else
   {
      strItems.Format(_T("Span %d\nSpan %d\n"),LABEL_SPAN(pierIdx-1),LABEL_SPAN(pierIdx));
   }

   dlg.m_strItems = strItems;
   if ( dlg.DoModal() == IDOK )
   {
      if ( pierIdx == 0 )
      {
         DeletePier(pierIdx,pgsTypes::Ahead);
      }
      else if ( pierIdx == nPiers-1 )
      {
         DeletePier(pierIdx,pgsTypes::Back);
      }
      else
      {
         DeletePier(pierIdx,dlg.m_ItemIdx == 0 ? pgsTypes::Back : pgsTypes::Ahead);
      }
   }
}

void CPGSDocBase::DeleteSpan(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // deleting a span

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CSelectItemDlg dlg;
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();

   PierIndexType prevPierIdx = (PierIndexType)spanIdx;
   PierIndexType nextPierIdx = prevPierIdx + 1;

   CString strPier1Label = pBridgeDesc->GetPier(prevPierIdx)->IsAbutment() ? _T("Abutment") : _T("Pier");
   CString strPier2Label = pBridgeDesc->GetPier(nextPierIdx)->IsAbutment() ? _T("Abutment") : _T("Pier");

   CString strTitle;
   strTitle.Format(_T("Deleting Span %d"),LABEL_SPAN(spanIdx));
   dlg.m_strTitle = strTitle;

   CString strLabel;
   strLabel.Format(_T("%s. Select the pier to be deleted with the span"),strTitle);
   dlg.m_strLabel = strLabel;

   CString strItems;
   strItems.Format(_T("%s %d\n%s %d"),strPier1Label,LABEL_PIER(prevPierIdx),strPier2Label,LABEL_PIER(nextPierIdx));
   dlg.m_strItems = strItems;
   if ( dlg.DoModal() == IDOK )
   {
      DeleteSpan(spanIdx,dlg.m_ItemIdx == 0 ? pgsTypes::PrevPier : pgsTypes::NextPier );
   }
}

void CPGSDocBase::OnDeleteSelection() 
{
   if (  m_Selection.Type == CSelection::Pier )
   {
      DeletePier(m_Selection.PierIdx);
   }
   else if ( m_Selection.Type == CSelection::Span )
   {
      DeleteSpan(m_Selection.SpanIdx);
   }
   else
   {
      ASSERT(FALSE); // should not get here
   }
}

void CPGSDocBase::OnUpdateDeleteSelection(CCmdUI* pCmdUI) 
{
   CView* pView = EAFGetActiveView();

   // command doesn't apply if there isn't an active view
   if ( pView == nullptr )
   {
      pCmdUI->Enable(FALSE);
      return;
   }

   // command only applies to Bridge Section and Plan views
   if ( !pView->IsKindOf(RUNTIME_CLASS(CBridgeSectionView)) && !pView->IsKindOf(RUNTIME_CLASS(CBridgePlanView)))
   {
      pCmdUI->Enable(FALSE);
      return;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
   if ( nSpans == 1 )
   {
      pCmdUI->Enable(FALSE);
      // can't delete the last span
      return;
   }

   if ( m_Selection.Type == CSelection::Pier )
   {
      PierIndexType nPiers = pBridgeDesc->GetPierCount();
      CString strLabel;
      strLabel.Format(_T("Delete Pier %d"),LABEL_PIER(m_Selection.PierIdx));

      pCmdUI->SetText(strLabel);
      pCmdUI->Enable(TRUE);
   }
   else if ( m_Selection.Type == CSelection::Span  )
   {
      // only span is selected
      CString strLabel;
      strLabel.Format(_T("Delete Span %d"),LABEL_SPAN(m_Selection.SpanIdx));
      pCmdUI->SetText(strLabel);
      pCmdUI->Enable(TRUE);
   }
   else
   {
      // can't delete alignment, bridge, deck, or section cut tool
      pCmdUI->SetText(_T("Delete"));
      pCmdUI->Enable(FALSE);
   }
}

void CPGSDocBase::DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType face)
{
   txnDeleteSpan* pTxn = new txnDeleteSpan(pierIdx,face);
   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSDocBase::DeleteSpan(SpanIndexType spanIdx,pgsTypes::RemovePierType pierRemoveType)
{
   PierIndexType pierIdx = (pierRemoveType == pgsTypes::PrevPier ? spanIdx : spanIdx+1);
   pgsTypes::PierFaceType pierFace = (pierRemoveType == pgsTypes::PrevPier ? pgsTypes::Ahead : pgsTypes::Back);
   DeletePier(pierIdx,pierFace);
}

void CPGSDocBase::OnInsert() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   PierIndexType nPiers = pBridgeDesc->GetPierCount();
   IndexType n = 0; 
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
      if ( !pPier->HasCantilever() )
      {
         n++;
      }
   }
   if ( n == 0 )
   {
      AfxMessageBox(_T("A span cannot be inserted into the bridge.\r\nRemove one of the cantilevers and try again."),MB_OK | MB_ICONINFORMATION);
      return;
   }

   CInsertSpanDlg dlg(pIBridgeDesc->GetBridgeDescription());
   if ( dlg.DoModal() == IDOK )
   {
      Float64 span_length         = dlg.m_SpanLength;
      PierIndexType refPierIdx    = dlg.m_RefPierIdx;
      pgsTypes::PierFaceType face = dlg.m_PierFace;
      bool bCreateNewGroup        = dlg.m_bCreateNewGroup;
      EventIndexType eventIdx     = dlg.m_EventIndex;
      InsertSpan(refPierIdx,face,span_length,bCreateNewGroup,eventIdx);
   }
}

void CPGSDocBase::InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace,Float64 spanLength,bool bCreateNewGroup,EventIndexType eventIdx)
{
   txnInsertSpan* pTxn = new txnInsertSpan(refPierIdx,pierFace,spanLength,bCreateNewGroup,eventIdx);
   GET_IFACE(IEAFTransactions,pTransactions);
   pTransactions->Execute(pTxn);
}

void CPGSDocBase::OnOptionsLabels() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CGirderLabelFormatDlg dlg;
   dlg.m_Format = (pgsGirderLabel::UseAlphaLabel() ? 0 : 1);
   if ( dlg.DoModal() )
   {
      bool bUseAlpha = dlg.m_Format == 0 ? true : false;
      if ( bUseAlpha != pgsGirderLabel::UseAlphaLabel() )
      {
         AFX_MANAGE_STATE(AfxGetAppModuleState());

         pgsGirderLabel::UseAlphaLabel(bUseAlpha);
         UpdateAllViews(nullptr,HINT_GIRDERLABELFORMATCHANGED);
      }
   }
}

void CPGSDocBase::OnLosses()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CLossParametersDlg dlg;
   GET_IFACE(ILossParameters,pLossParameters);
   txnEditLossParametersData oldData;
   pLossParameters->GetTendonPostTensionParameters(&oldData.Dset_PT,&oldData.WobbleFriction_PT,&oldData.FrictionCoefficient_PT);
   pLossParameters->GetTemporaryStrandPostTensionParameters(&oldData.Dset_TTS,&oldData.WobbleFriction_TTS,&oldData.FrictionCoefficient_TTS);

   oldData.bIgnoreCreepEffects      = pLossParameters->IgnoreCreepEffects();
   oldData.bIgnoreShrinkageEffects  = pLossParameters->IgnoreShrinkageEffects();
   oldData.bIgnoreRelaxationEffects = pLossParameters->IgnoreRelaxationEffects();

   oldData.bUseLumpSumLosses             = pLossParameters->UseGeneralLumpSumLosses();
   oldData.BeforeXferLosses              = pLossParameters->GetBeforeXferLosses();
   oldData.AfterXferLosses               = pLossParameters->GetAfterXferLosses();
   oldData.LiftingLosses                 = pLossParameters->GetLiftingLosses();
   oldData.ShippingLosses                = pLossParameters->GetShippingLosses();
   oldData.BeforeTempStrandRemovalLosses = pLossParameters->GetBeforeTempStrandRemovalLosses();
   oldData.AfterTempStrandRemovalLosses  = pLossParameters->GetAfterTempStrandRemovalLosses();
   oldData.AfterDeckPlacementLosses      = pLossParameters->GetAfterDeckPlacementLosses();
   oldData.AfterSIDLLosses               = pLossParameters->GetAfterSIDLLosses();
   oldData.FinalLosses                   = pLossParameters->GetFinalLosses();

   dlg.m_TimeStepProperties.m_bIgnoreCreepEffects      = oldData.bIgnoreCreepEffects;
   dlg.m_TimeStepProperties.m_bIgnoreShrinkageEffects  = oldData.bIgnoreShrinkageEffects;
   dlg.m_TimeStepProperties.m_bIgnoreRelaxationEffects = oldData.bIgnoreRelaxationEffects;

   dlg.m_PostTensioning.Dset_PT                = oldData.Dset_PT;
   dlg.m_PostTensioning.WobbleFriction_PT      = oldData.WobbleFriction_PT;
   dlg.m_PostTensioning.FrictionCoefficient_PT = oldData.FrictionCoefficient_PT;

   dlg.m_PostTensioning.Dset_TTS                = oldData.Dset_TTS;
   dlg.m_PostTensioning.WobbleFriction_TTS      = oldData.WobbleFriction_TTS;
   dlg.m_PostTensioning.FrictionCoefficient_TTS = oldData.FrictionCoefficient_TTS;

   dlg.m_Pretensioning.bUseLumpSumLosses             = oldData.bUseLumpSumLosses;
   dlg.m_Pretensioning.BeforeXferLosses              = oldData.BeforeXferLosses;
   dlg.m_Pretensioning.AfterXferLosses               = oldData.AfterXferLosses;
   dlg.m_Pretensioning.LiftingLosses                 = oldData.LiftingLosses;
   dlg.m_Pretensioning.ShippingLosses                = oldData.ShippingLosses;
   dlg.m_Pretensioning.BeforeTempStrandRemovalLosses = oldData.BeforeTempStrandRemovalLosses;
   dlg.m_Pretensioning.AfterTempStrandRemovalLosses  = oldData.AfterTempStrandRemovalLosses;
   dlg.m_Pretensioning.AfterDeckPlacementLosses      = oldData.AfterDeckPlacementLosses;
   dlg.m_Pretensioning.AfterSIDLLosses               = oldData.AfterSIDLLosses;
   dlg.m_Pretensioning.FinalLosses                   = oldData.FinalLosses;

   if ( dlg.DoModal() == IDOK )
   {
      txnEditLossParametersData newData;
      newData.bIgnoreCreepEffects      = dlg.m_TimeStepProperties.m_bIgnoreCreepEffects;
      newData.bIgnoreShrinkageEffects  = dlg.m_TimeStepProperties.m_bIgnoreShrinkageEffects;
      newData.bIgnoreRelaxationEffects = dlg.m_TimeStepProperties.m_bIgnoreRelaxationEffects;

      newData.Dset_PT                = dlg.m_PostTensioning.Dset_PT;
      newData.WobbleFriction_PT      = dlg.m_PostTensioning.WobbleFriction_PT;
      newData.FrictionCoefficient_PT = dlg.m_PostTensioning.FrictionCoefficient_PT;

      newData.Dset_TTS                = dlg.m_PostTensioning.Dset_TTS;
      newData.WobbleFriction_TTS      = dlg.m_PostTensioning.WobbleFriction_TTS;
      newData.FrictionCoefficient_TTS = dlg.m_PostTensioning.FrictionCoefficient_TTS;

      newData.bUseLumpSumLosses             = dlg.m_Pretensioning.bUseLumpSumLosses;
      newData.BeforeXferLosses              = dlg.m_Pretensioning.BeforeXferLosses;
      newData.AfterXferLosses               = dlg.m_Pretensioning.AfterXferLosses;
      newData.LiftingLosses                 = dlg.m_Pretensioning.LiftingLosses;
      newData.ShippingLosses                = dlg.m_Pretensioning.ShippingLosses;
      newData.BeforeTempStrandRemovalLosses = dlg.m_Pretensioning.BeforeTempStrandRemovalLosses;
      newData.AfterTempStrandRemovalLosses  = dlg.m_Pretensioning.AfterTempStrandRemovalLosses;
      newData.AfterDeckPlacementLosses      = dlg.m_Pretensioning.AfterDeckPlacementLosses;
      newData.AfterSIDLLosses               = dlg.m_Pretensioning.AfterSIDLLosses;
      newData.FinalLosses                   = dlg.m_Pretensioning.FinalLosses;

      txnEditLossParameters* pTxn = new txnEditLossParameters(oldData,newData);
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pTxn);
   }
}

void CPGSDocBase::OnEditTimeline()
{
   EditTimeline();
}

BOOL CPGSDocBase::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
    // document classes can't process ON_NOTIFY
    // see http://www.codeproject.com/KB/docview/NotifierApp.aspx for details
    if ( HIWORD(nCode) == WM_NOTIFY )
    {
       // verify that this is a WM_NOTIFY message
        WORD wCode = LOWORD(nCode) ;

        AFX_NOTIFY * notify = reinterpret_cast<AFX_NOTIFY*>(pExtra) ;
        if ( notify->pNMHDR->code == TBN_DROPDOWN )
        {
           if ( notify->pNMHDR->idFrom == m_pPGSuperDocProxyAgent->GetStdToolBarID() && ((NMTOOLBAR*)(notify->pNMHDR))->iItem == ID_VIEW_REPORTS )
           {
              return OnViewReports(notify->pNMHDR,notify->pResult); 
           }

           if ( notify->pNMHDR->idFrom == m_pPGSuperDocProxyAgent->GetStdToolBarID() && ((NMTOOLBAR*)(notify->pNMHDR))->iItem == ID_VIEW_GRAPHS )
           {
              return OnViewGraphs(notify->pNMHDR,notify->pResult); 
           }
        }
    }
	
	return CEAFBrokerDocument::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CPGSDocBase::PopulateReportMenu()
{
   CEAFMenu* pMainMenu = GetMainMenu();

   UINT viewPos = pMainMenu->FindMenuItem(_T("&View"));
   ASSERT( 0 <= viewPos );

   CEAFMenu* pViewMenu = pMainMenu->GetSubMenu(viewPos);
   ASSERT( pViewMenu != nullptr );

   UINT reportsPos = pViewMenu->FindMenuItem(_T("&Reports"));
   ASSERT( 0 <= reportsPos );

   // Get the reports menu
   CEAFMenu* pReportsMenu = pViewMenu->GetSubMenu(reportsPos);
   ASSERT(pReportsMenu != nullptr);

   CEAFBrokerDocument::PopulateReportMenu(pReportsMenu);
}

void CPGSDocBase::PopulateGraphMenu()
{
   CEAFMenu* pMainMenu = GetMainMenu();

   UINT viewPos = pMainMenu->FindMenuItem(_T("&View"));
   ASSERT( 0 <= viewPos );

   CEAFMenu* pViewMenu = pMainMenu->GetSubMenu(viewPos);
   ASSERT( pViewMenu != nullptr );

   UINT graphsPos = pViewMenu->FindMenuItem(_T("Gr&aphs"));
   ASSERT( 0 <= graphsPos );

   // Get the graphs menu
   CEAFMenu* pGraphMenu = pViewMenu->GetSubMenu(graphsPos);
   ASSERT(pGraphMenu != nullptr);

   CEAFBrokerDocument::PopulateGraphMenu(pGraphMenu);
}

void CPGSDocBase::LoadDocumentSettings()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEAFBrokerDocument::LoadDocumentSettings();

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(pPGSBase->GetAppName(),pApp);

   CString strAutoCalcDefault = pApp->GetLocalMachineString(_T("Settings"),_T("AutoCalc"), _T("On"));
   CString strAutoCalc = pApp->GetProfileString(_T("Settings"),_T("AutoCalc"),strAutoCalcDefault);
   if ( strAutoCalc.CompareNoCase(_T("Off")) == 0 )
   {
      m_bAutoCalcEnabled = false;
   }
   else
   {
      m_bAutoCalcEnabled = true;
   }

   // bridge model editor settings
   // turn on all settings for default
   UINT def_bm = IDB_PV_LABEL_PIERS     |
                 IDB_PV_LABEL_ALIGNMENT |
                 IDB_PV_LABEL_GIRDERS   |
                 IDB_PV_LABEL_BEARINGS  |
                 IDB_PV_LABEL_TICKMARKS |
                 IDB_PV_SHOW_TICKMARKS  |
                 IDB_PV_DRAW_ISOTROPIC  |
                 IDB_CS_LABEL_GIRDERS   |
                 IDB_CS_SHOW_DIMENSIONS |
                 IDB_CS_DRAW_ISOTROPIC;

   m_BridgeModelEditorSettings = pApp->GetProfileInt(_T("Settings"),_T("BridgeEditor"),def_bm);

   // the default north up setting for the alignment is whatever the user has for the bridge plan view
   UINT def_ap = IDA_AP_DRAW_BRIDGE | IDP_AP_DRAW_BRIDGE;
   m_AlignmentEditorSettings   = pApp->GetProfileInt(_T("Settings"),_T("AlignmentEditor"), def_ap | ((m_BridgeModelEditorSettings&IDB_PV_NORTH_UP)!=0 ? IDA_AP_NORTH_UP : 0));

   m_GirderModelEditorSettings = pApp->GetProfileInt(_T("Settings"),_T("GirderEditor"),def_bm);

   CString strDefaultGirderLabelFormat = pApp->GetLocalMachineString(_T("Settings"),_T("GirderLabelFormat"),     _T("Alpha"));
   CString strGirderLabelFormat = pApp->GetProfileString(_T("Settings"),_T("GirderLabelFormat"),strDefaultGirderLabelFormat);
   if ( strGirderLabelFormat.CompareNoCase(_T("Alpha")) == 0 )
   {
      pgsGirderLabel::UseAlphaLabel(true);
   }
   else
   {
      pgsGirderLabel::UseAlphaLabel(false);
   }


   CString strShowProjectProperties = pApp->GetLocalMachineString(_T("Settings"),_T("ShowProjectProperties"), _T("On"));
   CString strProjectProperties = pApp->GetProfileString(_T("Settings"),_T("ShowProjectProperties"),strShowProjectProperties);
   if ( strProjectProperties.CompareNoCase(_T("Off")) == 0 )
   {
      m_bShowProjectProperties = false;
   }
   else
   {
      m_bShowProjectProperties = true;
   }


   CString strDefaultReportCoverImage = pApp->GetLocalMachineString(_T("Settings"),_T("ReportCoverImage"),_T(""));
   rptStyleManager::SetReportCoverImage(pApp->GetProfileString(_T("Settings"),_T("ReportCoverImage"),strDefaultReportCoverImage));
}

void CPGSDocBase::SaveDocumentSettings()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CEAFBrokerDocument::SaveDocumentSettings();

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);
   CAutoRegistry autoReg(pPGSBase->GetAppName());

   CWinApp* pApp = AfxGetApp();

   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("AutoCalc"),m_bAutoCalcEnabled ? _T("On") : _T("Off") ));

   // bridge editor view
   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("BridgeEditor"),m_BridgeModelEditorSettings));

   // alignment editor view
   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("AlignmentEditor"),m_AlignmentEditorSettings));

   // girder editor view
   VERIFY(pApp->WriteProfileInt(_T("Settings"),_T("GirderEditor"),m_GirderModelEditorSettings));

   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("GirderLabelFormat"),pgsGirderLabel::UseAlphaLabel() ? _T("Alpha") : _T("Numeric") ));

   VERIFY(pApp->WriteProfileString( _T("Settings"),_T("ShowProjectProperties"),m_bShowProjectProperties ? _T("On") : _T("Off") ));
}

void CPGSDocBase::LoadDocumentationMap()
{
   // Load up the normal documentation maps
   CEAFBrokerDocument::LoadDocumentationMap();

   // Load up the document maps for our plugins (importers/exporters)
   m_pPluginMgr->LoadDocumentationMaps();
}

CString CPGSDocBase::GetDocumentationRootLocation()
{
   CEAFApp* pApp = EAFGetApp();
   return pApp->GetDocumentationRootLocation();
}

eafTypes::HelpResult CPGSDocBase::GetDocumentLocation(LPCTSTR lpszDocSetName,UINT nHID,CString& strURL)
{
   // do the normal work first
   eafTypes::HelpResult helpResult = CEAFBrokerDocument::GetDocumentLocation(lpszDocSetName,nHID,strURL);

   if ( helpResult == eafTypes::hrOK || helpResult== eafTypes::hrTopicNotFound )
   {
      // if we have a good help document location or if the doc set was found but the HID was bad,
      // we are done... return the result
      return helpResult;
   }

   // Check our plugins
   return m_pPluginMgr->GetDocumentLocation(lpszDocSetName,nHID,strURL);
}

void CPGSDocBase::OnLogFileOpened()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFBrokerDocument::OnLogFileOpened();

   GET_IFACE(IEAFProjectLog,pLog);
   CString strMsg;

   CPGSuperAppPluginApp* pApp = (CPGSuperAppPluginApp*)AfxGetApp();
   strMsg.Format(_T("PGSuper version %s"),pApp->GetVersion(false).GetBuffer(100));
   pLog->LogMessage(strMsg);
}

void CPGSDocBase::BrokerShutDown()
{
   CEAFBrokerDocument::BrokerShutDown();

   m_pPGSuperDocProxyAgent = nullptr;
}

void CPGSDocBase::OnStatusChanged()
{
   CEAFBrokerDocument::OnStatusChanged();
   if ( m_pPGSuperDocProxyAgent )
   {
      m_pPGSuperDocProxyAgent->OnStatusChanged();
   }
}

void CPGSDocBase::LoadToolbarState()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   __super::LoadToolbarState();
}

void CPGSDocBase::SaveToolbarState()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   __super::SaveToolbarState();
}

CString CPGSDocBase::GetToolbarSectionName()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);
   CAutoRegistry autoReg(pPGSBase->GetAppName());

   CString strToolbarSection;
   strToolbarSection.Format(_T("%s"),pPGSBase->GetAppName());

   return strToolbarSection;
}

void CPGSDocBase::OnUpdateViewGraphs(CCmdUI* pCmdUI)
{
   GET_IFACE(IGraphManager,pGraphMgr);
   pCmdUI->Enable( 0 < pGraphMgr->GetGraphBuilderCount() );
}

BOOL CPGSDocBase::OnViewGraphs(NMHDR* pnmhdr,LRESULT* plr) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // This method gets called when the down arrow toolbar button is used
   // It creates the drop down menu with the report names on it
   NMTOOLBAR* pnmtb = (NMTOOLBAR*)(pnmhdr);
   if ( pnmtb->iItem != ID_VIEW_GRAPHS )
   {
      return FALSE; // not our button
   }

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_GRAPHS) );
   CMenu* pMenu = menu.GetSubMenu(0);
   pMenu->RemoveMenu(0,MF_BYPOSITION); // remove the placeholder

   CEAFMenu contextMenu(pMenu->Detach(),GetPluginCommandManager());


   BuildGraphMenu(&contextMenu);

   GET_IFACE(IEAFToolbars,pToolBars);
   CEAFToolBar* pToolBar = pToolBars->GetToolBar( m_pPGSuperDocProxyAgent->GetStdToolBarID() );
   int idx = pToolBar->CommandToIndex(ID_VIEW_GRAPHS,nullptr);
   CRect rect;
   pToolBar->GetItemRect(idx,&rect);

   CPoint point(rect.left,rect.bottom);
   pToolBar->ClientToScreen(&point);
   contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x,point.y, EAFGetMainFrame() );

   return TRUE;
}

void CPGSDocBase::OnUpdateViewReports(CCmdUI* pCmdUI)
{
   GET_IFACE(IReportManager,pReportMgr);
   pCmdUI->Enable( 0 < pReportMgr->GetReportBuilderCount() );
}

BOOL CPGSDocBase::OnViewReports(NMHDR* pnmhdr,LRESULT* plr) 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // This method gets called when the down arrow toolbar button is used
   // It creates the drop down menu with the report names on it
   NMTOOLBAR* pnmtb = (NMTOOLBAR*)(pnmhdr);
   if ( pnmtb->iItem != ID_VIEW_REPORTS )
   {
      return FALSE; // not our button
   }

   CMenu menu;
   VERIFY( menu.LoadMenu(IDR_REPORTS) );
   CMenu* pMenu = menu.GetSubMenu(0);
   pMenu->RemoveMenu(0,MF_BYPOSITION); // remove the placeholder

   CEAFMenu contextMenu(pMenu->Detach(),GetPluginCommandManager());

   CEAFBrokerDocument::PopulateReportMenu(&contextMenu);

   GET_IFACE(IEAFToolbars,pToolBars);
   CEAFToolBar* pToolBar = pToolBars->GetToolBar( m_pPGSuperDocProxyAgent->GetStdToolBarID() );
   int idx = pToolBar->CommandToIndex(ID_VIEW_REPORTS,nullptr);
   CRect rect;
   pToolBar->GetItemRect(idx,&rect);

   CPoint point(rect.left,rect.bottom);
   pToolBar->ClientToScreen(&point);
   contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x,point.y, EAFGetMainFrame() );

   return TRUE;
}


void CPGSDocBase::OnImportMenu(CCmdUI* pCmdUI)
{
   USES_CONVERSION;


   if ( pCmdUI->m_pMenu == nullptr && pCmdUI->m_pSubMenu == nullptr )
   {
      return;
   }

   CMenu* pMenu = (pCmdUI->m_pSubMenu ? pCmdUI->m_pSubMenu : pCmdUI->m_pMenu);
   UINT nItems = pMenu->GetMenuItemCount();
   for ( UINT i = 1; i < nItems; i++ )
   {
      pMenu->DeleteMenu(i,MF_BYPOSITION);
   }

   CollectionIndexType nImporters = m_pPluginMgr->GetImporterCount();
   if ( nImporters == 0 )
   {
      pCmdUI->SetText(_T("Custom importers not installed"));
      pCmdUI->Enable(FALSE);
      return;
   }
   else
   {
      CollectionIndexType idx;
      // clean up the menu
      for ( idx = 0; idx < nImporters; idx++ )
      {
         pMenu->DeleteMenu(pCmdUI->m_nID+(UINT)idx,MF_BYCOMMAND);
      }

      // populate the menu
      for ( idx = 0; idx < nImporters; idx++ )
      {
         CComPtr<IPGSDataImporter> importer;
         m_pPluginMgr->GetImporter(idx,true,&importer);

         UINT cmdID = m_pPluginMgr->GetImporterCommand(idx);

         CComBSTR bstrMenuText;
         importer->GetMenuText(&bstrMenuText);
         pMenu->InsertMenu(pCmdUI->m_nIndex,MF_BYPOSITION | MF_STRING,cmdID,OLE2T(bstrMenuText));

         const CBitmap* pBmp = m_pPluginMgr->GetImporterBitmap(idx);
         pMenu->SetMenuItemBitmaps(cmdID,MF_BYCOMMAND,pBmp,nullptr);

   	   pCmdUI->m_nIndexMax = pMenu->GetMenuItemCount();

         pCmdUI->m_nIndex++;
      }
   }

   pCmdUI->m_nIndex--; // point to last menu added
}

void CPGSDocBase::OnExportMenu(CCmdUI* pCmdUI)
{
   USES_CONVERSION;


   if ( pCmdUI->m_pMenu == nullptr && pCmdUI->m_pSubMenu == nullptr )
   {
      return;
   }

   CMenu* pMenu = (pCmdUI->m_pSubMenu ? pCmdUI->m_pSubMenu : pCmdUI->m_pMenu);
   UINT nItems = pMenu->GetMenuItemCount();
   for ( UINT i = 1; i < nItems; i++ )
   {
      pMenu->DeleteMenu(i,MF_BYPOSITION);
   }

   CollectionIndexType nExporters = m_pPluginMgr->GetExporterCount();
   if ( nExporters == 0 )
   {
      pCmdUI->SetText(_T("Custom exporters not installed"));
      pCmdUI->Enable(FALSE);
      return;
   }
   else
   {
      CollectionIndexType idx;
      for ( idx = 0; idx < nExporters; idx++ )
      {
         pMenu->DeleteMenu(pCmdUI->m_nID+(UINT)idx,MF_BYCOMMAND);
      }

      for ( idx = 0; idx < nExporters; idx++ )
      {
         CComPtr<IPGSDataExporter> exporter;
         m_pPluginMgr->GetExporter(idx,true,&exporter);

         UINT cmdID = m_pPluginMgr->GetExporterCommand(idx);

         CComBSTR bstrMenuText;
         exporter->GetMenuText(&bstrMenuText);

         pMenu->InsertMenu(pCmdUI->m_nIndex,MF_BYPOSITION | MF_STRING,cmdID,OLE2T(bstrMenuText));

         const CBitmap* pBmp = m_pPluginMgr->GetExporterBitmap(idx);
         pMenu->SetMenuItemBitmaps(cmdID,MF_BYCOMMAND,pBmp,nullptr);

         pCmdUI->m_nIndexMax = pMenu->GetMenuItemCount();
         pCmdUI->m_nIndex++;
      }
   }

	// update end menu count
	pCmdUI->m_nIndex--; // point to last menu added
}

void CPGSDocBase::OnImport(UINT nID)
{
   CComPtr<IPGSDataImporter> importer;
   m_pPluginMgr->GetImporter(nID,false,&importer);

   if ( importer )
   {
      importer->Import(m_pBroker);
   }
}

void CPGSDocBase::OnExport(UINT nID)
{
   CComPtr<IPGSDataExporter> exporter;
   m_pPluginMgr->GetExporter(nID,false,&exporter);

   if ( exporter )
   {
      exporter->Export(m_pBroker);
   }
}

void CPGSDocBase::OnHelpFinder()
{
   EAFHelp(GetDocumentationSetName(),IDH_PGSUPER);
}

void CPGSDocBase::OnAbout()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   UINT resourceID = pTemplate->GetResourceID();

   CAboutDlg dlg(resourceID);
   dlg.DoModal();
}

UINT CPGSDocBase::GetBridgeEditorSettings() const
{
   return m_BridgeModelEditorSettings;
}

void CPGSDocBase::SetBridgeEditorSettings(UINT settings)
{
   m_BridgeModelEditorSettings = settings;
}

UINT CPGSDocBase::GetAlignmentEditorSettings() const
{
   return m_AlignmentEditorSettings;
}

void CPGSDocBase::SetAlignmentEditorSettings(UINT settings)
{
   m_AlignmentEditorSettings = settings;
}

UINT CPGSDocBase::GetGirderEditorSettings() const
{
   return m_GirderModelEditorSettings;
}

void CPGSDocBase::SetGirderEditorSettings(UINT settings)
{
   m_GirderModelEditorSettings = settings;
}

void CPGSDocBase::ResetUIHints()
{
   __super::ResetUIHints();
   m_pPGSuperDocProxyAgent->OnResetHints();
}

bool CPGSDocBase::ShowProjectPropertiesOnNewProject()
{
   return m_bShowProjectProperties;
}

void CPGSDocBase::ShowProjectPropertiesOnNewProject(bool bShow)
{
   m_bShowProjectProperties = bShow;
}

void CPGSDocBase::DeleteContents()
{
   __super::DeleteContents();
}

BOOL CPGSDocBase::LoadAgents()
{
   // set up the registry stuff so we read from the correct location
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)GetDocTemplate();
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(pPGSBase->GetAppName(),pApp);

   return __super::LoadAgents();
}

long CPGSDocBase::GetReportViewKey()
{
   return m_pPGSuperDocProxyAgent->GetReportViewKey();
}


void CPGSDocBase::OnChangedFavoriteReports(BOOL bIsFavorites,BOOL bFromMenu)
{
   // update main menu submenu
   __super::OnChangedFavoriteReports(bIsFavorites,bFromMenu);
   PopulateReportMenu();
}

void CPGSDocBase::ShowCustomReportHelp(eafTypes::CustomReportHelp helpType)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   __super::ShowCustomReportHelp(helpType);
}

void CPGSDocBase::ShowCustomReportDefinitionHelp()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   __super::ShowCustomReportDefinitionHelp();
}