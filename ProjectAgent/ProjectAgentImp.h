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

// ProjectAgentImp.h : Declaration of the CProjectAgentImp

#ifndef __PROJECTAGENT_H_
#define __PROJECTAGENT_H_

#include "resource.h"       // main symbols

#include <StrData.h>
#include <vector>
#include "CPProjectAgent.h"
#include <MathEx.h>
#include <PsgLib\LibraryManager.h>

#include <Units\SysUnits.h>

#include <EAF\EAFInterfaceCache.h>

#include <PgsExt\Keys.h>
#include <PgsExt\LoadFactors.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\LongitudinalRebarData.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\LoadManager.h>
#include <PgsExt\ClosureJointData.h>

#include "LibraryEntryObserver.h"

#include <IFace\GirderHandling.h>


class CStructuredLoad;
class ConflictList;
class CBridgeChangedHint;


/////////////////////////////////////////////////////////////////////////////
// CProjectAgentImp
class ATL_NO_VTABLE CProjectAgentImp : 
	public CComObjectRootEx<CComSingleThreadModel>,
   //public CComRefCountTracer<CProjectAgentImp,CComObjectRootEx<CComSingleThreadModel> >,
	public CComCoClass<CProjectAgentImp, &CLSID_ProjectAgent>,
	public IConnectionPointContainerImpl<CProjectAgentImp>,
   public CProxyIProjectPropertiesEventSink<CProjectAgentImp>,
   public CProxyIEnvironmentEventSink<CProjectAgentImp>,
   public CProxyIBridgeDescriptionEventSink<CProjectAgentImp>,
   public CProxyISpecificationEventSink<CProjectAgentImp>,
   public CProxyIRatingSpecificationEventSink<CProjectAgentImp>,
   public CProxyILibraryConflictEventSink<CProjectAgentImp>,
   public CProxyILoadModifiersEventSink<CProjectAgentImp>,
   public CProxyILossParametersEventSink<CProjectAgentImp>,
   public CProxyIEventsEventSink<CProjectAgentImp>,
   public IAgentEx,
   public IAgentPersist,
   public IProjectProperties,
   public IEnvironment,
   public IRoadwayData,
   public IBridgeDescription,
   public ISegmentData,
   public IShear,
   public ILongitudinalRebar,
   public ISpecification,
   public IRatingSpecification,
   public ILibraryNames,
   public ILibrary,
   public ILoadModifiers,
   public ISegmentHauling,
   public ISegmentLifting,
   public IImportProjectLibrary,
   public IUserDefinedLoadData,
   public IEvents,
   public ILimits,
   public ILoadFactors,
   public ILiveLoads,
   public IEventMap,
   public IEffectiveFlangeWidth,
   public ILossParameters,
   public IValidate
{  
public:
	CProjectAgentImp(); 
   virtual ~CProjectAgentImp();

   DECLARE_PROTECT_FINAL_CONSTRUCT();
   HRESULT FinalConstruct();
   void FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_PROJECTAGENT)

BEGIN_COM_MAP(CProjectAgentImp)
	COM_INTERFACE_ENTRY(IAgent)
   COM_INTERFACE_ENTRY(IAgentEx)
	COM_INTERFACE_ENTRY(IAgentPersist)
	COM_INTERFACE_ENTRY(IProjectProperties)
   COM_INTERFACE_ENTRY(IEnvironment)
   COM_INTERFACE_ENTRY(IRoadwayData)
   COM_INTERFACE_ENTRY(IBridgeDescription)
   COM_INTERFACE_ENTRY(ISegmentData)
   COM_INTERFACE_ENTRY(IShear)
   COM_INTERFACE_ENTRY(ILongitudinalRebar)
   COM_INTERFACE_ENTRY(ISpecification)
   COM_INTERFACE_ENTRY(IRatingSpecification)
   COM_INTERFACE_ENTRY(ILibraryNames)
   COM_INTERFACE_ENTRY(ILibrary)
   COM_INTERFACE_ENTRY(ILoadModifiers)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
   COM_INTERFACE_ENTRY(ISegmentHauling)
   COM_INTERFACE_ENTRY(ISegmentLifting)
   COM_INTERFACE_ENTRY(IImportProjectLibrary)
   COM_INTERFACE_ENTRY(IUserDefinedLoadData)
   COM_INTERFACE_ENTRY(IEvents)
   COM_INTERFACE_ENTRY(ILimits)
   COM_INTERFACE_ENTRY(ILoadFactors)
   COM_INTERFACE_ENTRY(ILiveLoads)
   COM_INTERFACE_ENTRY(IEventMap)
   COM_INTERFACE_ENTRY(IEffectiveFlangeWidth)
   COM_INTERFACE_ENTRY(ILossParameters)
   COM_INTERFACE_ENTRY(IValidate)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CProjectAgentImp)
   CONNECTION_POINT_ENTRY( IID_IProjectPropertiesEventSink )
   CONNECTION_POINT_ENTRY( IID_IEnvironmentEventSink )
   CONNECTION_POINT_ENTRY( IID_IBridgeDescriptionEventSink )
   CONNECTION_POINT_ENTRY( IID_ISpecificationEventSink )
   CONNECTION_POINT_ENTRY( IID_IRatingSpecificationEventSink )
   CONNECTION_POINT_ENTRY( IID_ILibraryConflictEventSink )
   CONNECTION_POINT_ENTRY( IID_ILoadModifiersEventSink )
   CONNECTION_POINT_ENTRY( IID_ILossParametersEventSink )
   CONNECTION_POINT_ENTRY( IID_IEventsSink )
END_CONNECTION_POINT_MAP()

   StatusCallbackIDType m_scidBridgeDescriptionInfo;
   StatusCallbackIDType m_scidBridgeDescriptionWarning;
   StatusCallbackIDType m_scidGirderDescriptionWarning;
   StatusCallbackIDType m_scidRebarStrengthWarning;
   StatusCallbackIDType m_scidLoadDescriptionWarning;

// IAgent
public:
	STDMETHOD(SetBroker)(/*[in]*/ IBroker* pBroker) override;
   STDMETHOD(RegInterfaces)() override;
	STDMETHOD(Init)() override;
	STDMETHOD(Reset)() override;
	STDMETHOD(ShutDown)() override;
   STDMETHOD(Init2)() override;
   STDMETHOD(GetClassID)(CLSID* pCLSID) override;

// IAgentPersist
public:
	STDMETHOD(Load)(/*[in]*/ IStructuredLoad* pStrLoad) override;
	STDMETHOD(Save)(/*[in]*/ IStructuredSave* pStrSave) override;

// IProjectProperties
public:
   virtual LPCTSTR GetBridgeName() const override;
   virtual void SetBridgeName(LPCTSTR name) override;
   virtual LPCTSTR GetBridgeID() const override;
   virtual void SetBridgeID(LPCTSTR bid) override;
   virtual LPCTSTR GetJobNumber() const override;
   virtual void SetJobNumber(LPCTSTR jid) override;
   virtual LPCTSTR GetEngineer() const override;
   virtual void SetEngineer(LPCTSTR eng) override;
   virtual LPCTSTR GetCompany() const override;
   virtual void SetCompany(LPCTSTR company) override;
   virtual LPCTSTR GetComments() const override;
   virtual void SetComments(LPCTSTR comments) override;

// IEnvironment
public:
   virtual enumExposureCondition GetExposureCondition() const override;
	virtual void SetExposureCondition(enumExposureCondition newVal) override;
	virtual Float64 GetRelHumidity() const override;
	virtual void SetRelHumidity(Float64 newVal) override;

// IRoadwayData
public:
   virtual void SetAlignmentData2(const AlignmentData2& data) override;
   virtual AlignmentData2 GetAlignmentData2() const override;
   virtual void SetProfileData2(const ProfileData2& data) override;
   virtual ProfileData2 GetProfileData2() const override;
   virtual void SetRoadwaySectionData(const RoadwaySectionData& data) override;
   virtual RoadwaySectionData GetRoadwaySectionData() const override;

// IBridgeDescription
public:
   virtual const CBridgeDescription2* GetBridgeDescription() override;
   virtual void SetBridgeDescription(const CBridgeDescription2& desc) override;
   virtual const CDeckDescription2* GetDeckDescription() override;
   virtual void SetDeckDescription(const CDeckDescription2& deck) override;
   virtual SpanIndexType GetSpanCount() override;
   virtual const CSpanData2* GetSpan(SpanIndexType spanIdx) override;
   virtual void SetSpan(SpanIndexType spanIdx,const CSpanData2& spanData) override;
   virtual PierIndexType GetPierCount() override;
   virtual const CPierData2* GetPier(PierIndexType pierIdx) override;
   virtual const CPierData2* FindPier(PierIDType pierID) override;
   virtual void SetPierByIndex(PierIndexType pierIdx,const CPierData2& PierData) override;
   virtual void SetPierByID(PierIDType pierID,const CPierData2& PierData) override;
   virtual SupportIndexType GetTemporarySupportCount() override;
   virtual const CTemporarySupportData* GetTemporarySupport(SupportIndexType tsIdx) override;
   virtual const CTemporarySupportData* FindTemporarySupport(SupportIDType tsID) override;
   virtual void SetTemporarySupportByIndex(SupportIndexType tsIdx,const CTemporarySupportData& tsData) override;
   virtual void SetTemporarySupportByID(SupportIDType tsID,const CTemporarySupportData& tsData) override;
   virtual GroupIndexType GetGirderGroupCount() override;
   virtual const CGirderGroupData* GetGirderGroup(GroupIndexType grpIdx) override;
   virtual const CSplicedGirderData* GetGirder(const CGirderKey& girderKey) override;
   virtual const CSplicedGirderData* FindGirder(GirderIDType gdrID) override;
   virtual void SetGirder(const CGirderKey& girderKey,const CSplicedGirderData& splicedGirder) override;
   virtual const CPTData* GetPostTensioning(const CGirderKey& girderKey) override;
   virtual void SetPostTensioning(const CGirderKey& girderKey,const CPTData& ptData) override;
   virtual const CPrecastSegmentData* GetPrecastSegmentData(const CSegmentKey& segmentKey) override;
   virtual void SetPrecastSegmentData(const CSegmentKey& segmentKey,const CPrecastSegmentData& segment) override;
   virtual const CClosureJointData* GetClosureJointData(const CSegmentKey& closureKey) override;
   virtual void SetClosureJointData(const CSegmentKey& closureKey,const CClosureJointData& closure) override;
   virtual void SetSpanLength(SpanIndexType spanIdx,Float64 newLength) override;
   virtual void MovePier(PierIndexType pierIdx,Float64 newStation,pgsTypes::MovePierOption moveOption) override;
   virtual SupportIndexType MoveTemporarySupport(SupportIndexType tsIdx,Float64 newStation) override;
   virtual void SetMeasurementType(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementType mt) override;
   virtual void SetMeasurementLocation(PierIndexType pierIdx,pgsTypes::PierFaceType pierFace,pgsTypes::MeasurementLocation ml) override;
   virtual void SetGirderSpacing(PierIndexType pierIdx,pgsTypes::PierFaceType face,const CGirderSpacing2& spacing) override;
   virtual void SetGirderSpacingAtStartOfGroup(GroupIndexType groupIdx,const CGirderSpacing2& spacing) override;
   virtual void SetGirderSpacingAtEndOfGroup(GroupIndexType groupIdx,const CGirderSpacing2& spacing) override;
   virtual void SetGirderName(const CGirderKey& girderKey, LPCTSTR strGirderName) override;
   virtual void SetGirderGroup(GroupIndexType grpIdx,const CGirderGroupData& girderGroup) override;
   virtual void SetGirderCount(GroupIndexType grpIdx,GirderIndexType nGirders) override;
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::BoundaryConditionType connectionType) override;
   virtual void SetBoundaryCondition(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType connectionType,EventIndexType castClosureEventIdx) override;
   virtual void DeletePier(PierIndexType pierIdx,pgsTypes::PierFaceType faceForSpan) override;
   virtual void InsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType pierFace, Float64 spanLength, const CSpanData2* pSpanData,const CPierData2* pPierData,bool bCreateNewGroup,EventIndexType eventIdx) override;
   virtual void InsertTemporarySupport(CTemporarySupportData* pTSData,EventIndexType erectionEventIdx,EventIndexType removalEventIdx,EventIndexType castClosureJointEventIdx) override;
   virtual void DeleteTemporarySupportByIndex(SupportIndexType tsIdx) override;
   virtual void DeleteTemporarySupportByID(SupportIDType tsID) override;
   virtual void SetLiveLoadDistributionFactorMethod(pgsTypes::DistributionFactorMethod method) override;
   virtual pgsTypes::DistributionFactorMethod GetLiveLoadDistributionFactorMethod() override;
   virtual void UseSameNumberOfGirdersInAllGroups(bool bUseSame) override;
   virtual bool UseSameNumberOfGirdersInAllGroups() override;
   virtual void SetGirderCount(GirderIndexType nGirders) override;
   virtual void UseSameGirderForEntireBridge(bool bSame) override;
   virtual bool UseSameGirderForEntireBridge() override;
   virtual void SetGirderName(LPCTSTR strGirderName) override;
   virtual void SetGirderSpacingType(pgsTypes::SupportedBeamSpacing sbs) override;
   virtual pgsTypes::SupportedBeamSpacing GetGirderSpacingType() override;
   virtual void SetGirderSpacing(Float64 spacing) override;
   virtual void SetMeasurementType(pgsTypes::MeasurementType mt) override;
   virtual pgsTypes::MeasurementType GetMeasurementType() override;
   virtual void SetMeasurementLocation(pgsTypes::MeasurementLocation ml) override;
   virtual pgsTypes::MeasurementLocation GetMeasurementLocation() override;
   virtual void SetWearingSurfaceType(pgsTypes::WearingSurfaceType wearingSurfaceType) override;
   virtual void SetSlabOffsetType(pgsTypes::SlabOffsetType offsetType) override;
   virtual void SetSlabOffset(Float64 slabOffset) override;
   virtual void SetSlabOffset(GroupIndexType grpIdx, PierIndexType pierIdx, Float64 offset) override;
   virtual void SetSlabOffset(GroupIndexType grpIdx, PierIndexType pierIdx, GirderIndexType gdrIdx, Float64 offset) override;
   virtual Float64 GetSlabOffset(GroupIndexType grpidx, PierIndexType pierIdx, GirderIndexType gdrIdx) override;
   virtual pgsTypes::SlabOffsetType GetSlabOffsetType() override;
   virtual void SetFillet( Float64 Fillet);
   virtual Float64 GetFillet();
   virtual void SetAssExcessCamberType(pgsTypes::AssExcessCamberType cType);
   virtual pgsTypes::AssExcessCamberType GetAssExcessCamberType();
   virtual void SetAssExcessCamber( Float64 assExcessCamber);
   virtual void SetAssExcessCamber(SpanIndexType spanIdx, Float64 offset);
   virtual void SetAssExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx, Float64 camber);
   virtual Float64 GetAssExcessCamber( SpanIndexType spanIdx, GirderIndexType gdrIdx);
   virtual std::vector<pgsTypes::BoundaryConditionType> GetBoundaryConditionTypes(PierIndexType pierIdx) override;
   virtual std::vector<pgsTypes::PierSegmentConnectionType> GetPierSegmentConnectionTypes(PierIndexType pierIdx) override;
   virtual const CTimelineManager* GetTimelineManager() override;
   virtual void SetTimelineManager(const CTimelineManager& timelineMbr) override;
   virtual EventIndexType AddTimelineEvent(const CTimelineEvent& timelineEvent) override;
   virtual EventIndexType GetEventCount() override;
   virtual const CTimelineEvent* GetEventByIndex(EventIndexType eventIdx) override;
   virtual const CTimelineEvent* GetEventByID(EventIDType eventID) override;
   virtual void SetEventByIndex(EventIndexType eventIdx,const CTimelineEvent& stage) override;
   virtual void SetEventByID(EventIDType eventID,const CTimelineEvent& stage) override;
   virtual void SetSegmentConstructionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx) override;
   virtual void SetSegmentConstructionEventByID(const CSegmentKey& segmentKey,EventIDType eventID) override;
   virtual EventIndexType GetSegmentConstructionEventIndex(const CSegmentKey& segmentKey) override;
   virtual EventIDType GetSegmentConstructionEventID(const CSegmentKey& segmentKey) override;
   virtual EventIndexType GetPierErectionEvent(PierIndexType pierIdx) override;
   virtual void SetPierErectionEventByIndex(PierIndexType pierIdx,EventIndexType eventIdx) override;
   virtual void SetPierErectionEventByID(PierIndexType pierIdx,IDType eventID) override;
   virtual void SetTempSupportEventsByIndex(SupportIndexType tsIdx,EventIndexType erectIdx,EventIndexType removeIdx) override;
   virtual void SetTempSupportEventsByID(SupportIDType tsID,EventIndexType erectIdx,EventIndexType removeIdx) override;
   virtual void SetSegmentErectionEventByIndex(const CSegmentKey& segmentKey,EventIndexType eventIdx) override;
   virtual void SetSegmentErectionEventByID(const CSegmentKey& segmentKey,EventIDType eventID) override;
   virtual EventIndexType GetSegmentErectionEventIndex(const CSegmentKey& segmentKey) override;
   virtual EventIDType GetSegmentErectionEventID(const CSegmentKey& segmentKey) override;
   virtual void SetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType constructionEventIdx,EventIndexType erectionEventIdx) override;
   virtual void SetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType constructionEventID,EventIDType erectionEventID) override;
   virtual void GetSegmentEventsByIndex(const CSegmentKey& segmentKey,EventIndexType* constructionEventIdx,EventIndexType* erectionEventIdx) override;
   virtual void GetSegmentEventsByID(const CSegmentKey& segmentKey,EventIDType* constructionEventID,EventIDType* erectionEventID) override;
   virtual EventIndexType GetCastClosureJointEventIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx) override;
   virtual EventIDType GetCastClosureJointEventID(GroupIndexType grpIdx,CollectionIndexType closureIdx) override;
   virtual void SetCastClosureJointEventByIndex(GroupIndexType grpIdx,CollectionIndexType closureIdx,EventIndexType eventIdx) override;
   virtual void SetCastClosureJointEventByID(GroupIndexType grpIdx,CollectionIndexType closureIdx,EventIDType eventID) override;
   virtual EventIndexType GetStressTendonEventIndex(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual EventIDType GetStressTendonEventID(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
   virtual void SetStressTendonEventByIndex(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIndexType eventIdx) override;
   virtual void SetStressTendonEventByID(const CGirderKey& girderKey,DuctIndexType ductIdx,EventIDType eventID) override;
   virtual EventIndexType GetCastDeckEventIndex() override;
   virtual EventIDType GetCastDeckEventID() override;
   virtual int SetCastDeckEventByIndex(EventIndexType eventIdx,bool bAdjustTimeline) override;
   virtual int SetCastDeckEventByID(EventIDType eventID,bool bAdjustTimeline) override;
   virtual EventIndexType GetIntermediateDiaphragmsLoadEventIndex() override;
   virtual EventIDType GetIntermediateDiaphragmsLoadEventID() override;
   virtual void SetIntermediateDiaphragmsLoadEventByIndex(EventIndexType eventIdx) override;
   virtual void SetIntermediateDiaphragmsLoadEventByID(EventIDType eventID) override;
   virtual EventIndexType GetRailingSystemLoadEventIndex() override;
   virtual EventIDType GetRailingSystemLoadEventID() override;
   virtual void SetRailingSystemLoadEventByIndex(EventIndexType eventIdx) override;
   virtual void SetRailingSystemLoadEventByID(EventIDType eventID) override;
   virtual EventIndexType GetOverlayLoadEventIndex() override;
   virtual EventIDType GetOverlayLoadEventID() override;
   virtual void SetOverlayLoadEventByIndex(EventIndexType eventIdx) override;
   virtual void SetOverlayLoadEventByID(EventIDType eventID) override;
   virtual EventIndexType GetLiveLoadEventIndex() override;
   virtual EventIDType GetLiveLoadEventID() override;
   virtual void SetLiveLoadEventByIndex(EventIndexType eventIdx) override;
   virtual void SetLiveLoadEventByID(EventIDType eventID) override;
   virtual GroupIDType GetGroupID(GroupIndexType groupIdx) override;
   virtual GirderIDType GetGirderID(const CGirderKey& girderKey) override;
   virtual SegmentIDType GetSegmentID(const CSegmentKey& segmentKey) override;
   virtual bool IsCompatibleGirder(const CGirderKey& girderKey, LPCTSTR lpszGirderName) const override;
   virtual bool AreGirdersCompatible(GroupIndexType groupIdx) const override;
   virtual bool AreGirdersCompatible(const std::vector<std::_tstring>& vGirderNames) const override;
   virtual bool AreGirdersCompatible(const CBridgeDescription2& bridgeDescription,const std::vector<std::_tstring>& vGirderNames) const override;

// ISegmentData 
public:
   virtual const matPsStrand* GetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type) const override;
   virtual void SetStrandMaterial(const CSegmentKey& segmentKey,pgsTypes::StrandType type,const matPsStrand* pmat) override;

   virtual const CGirderMaterial* GetSegmentMaterial(const CSegmentKey& segmentKey) const override;
   virtual void SetSegmentMaterial(const CSegmentKey& segmentKey,const CGirderMaterial& material) override;

   virtual const CStrandData* GetStrandData(const CSegmentKey& segmentKey) override;
   virtual void SetStrandData(const CSegmentKey& segmentKey,const CStrandData& strands) override;

   virtual const CHandlingData* GetHandlingData(const CSegmentKey& segmentKey) override;
   virtual void SetHandlingData(const CSegmentKey& segmentKey,const CHandlingData& handling) override;

// IShear
public:
   virtual std::_tstring GetSegmentStirrupMaterial(const CSegmentKey& segmentKey) const override;
   virtual void GetSegmentStirrupMaterial(const CSegmentKey& segmentKey,matRebar::Type& type,matRebar::Grade& grade) override;
   virtual void SetSegmentStirrupMaterial(const CSegmentKey& segmentKey,matRebar::Type type,matRebar::Grade grade) override;
   virtual const CShearData2* GetSegmentShearData(const CSegmentKey& segmentKey) const override;
   virtual void SetSegmentShearData(const CSegmentKey& segmentKey,const CShearData2& data) override;
   virtual std::_tstring GetClosureJointStirrupMaterial(const CClosureKey& closureKey) const override;
   virtual void GetClosureJointStirrupMaterial(const CClosureKey& closureKey,matRebar::Type& type,matRebar::Grade& grade) override;
   virtual void SetClosureJointStirrupMaterial(const CClosureKey& closureKey,matRebar::Type type,matRebar::Grade grade) override;
   virtual const CShearData2* GetClosureJointShearData(const CClosureKey& closureKey) const override;
   virtual void SetClosureJointShearData(const CClosureKey& closureKey,const CShearData2& data) override;

// ILongitudinalRebar
public:
   virtual std::_tstring GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey) const override;
   virtual void GetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type& type,matRebar::Grade& grade) override;
   virtual void SetSegmentLongitudinalRebarMaterial(const CSegmentKey& segmentKey,matRebar::Type type,matRebar::Grade grade) override;
   virtual const CLongitudinalRebarData* GetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey) const override;
   virtual void SetSegmentLongitudinalRebarData(const CSegmentKey& segmentKey,const CLongitudinalRebarData& data) override;

   virtual std::_tstring GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey) const override;
   virtual void GetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type& type,matRebar::Grade& grade) override;
   virtual void SetClosureJointLongitudinalRebarMaterial(const CClosureKey& closureKey,matRebar::Type type,matRebar::Grade grade) override;
   virtual const CLongitudinalRebarData* GetClosureJointLongitudinalRebarData(const CClosureKey& closureKey) const override;
   virtual void SetClosureJointLongitudinalRebarData(const CClosureKey& closureKey,const CLongitudinalRebarData& data) override;

// ISpecification
public:
   virtual std::_tstring GetSpecification() override;
   virtual void SetSpecification(const std::_tstring& spec) override;
   virtual void GetTrafficBarrierDistribution(GirderIndexType* pNGirders,pgsTypes::TrafficBarrierDistribution* pDistType) override;
   virtual Uint16 GetMomentCapacityMethod() override;
   virtual void SetAnalysisType(pgsTypes::AnalysisType analysisType) override;
   virtual pgsTypes::AnalysisType GetAnalysisType() override;
   virtual std::vector<arDesignOptions> GetDesignOptions(const CGirderKey& girderKey) override;
   virtual bool IsSlabOffsetDesignEnabled() override;
   virtual pgsTypes::OverlayLoadDistributionType GetOverlayLoadDistributionType() override;
   virtual pgsTypes::HaunchLoadComputationType GetHaunchLoadComputationType() override;
   virtual Float64 GetCamberTolerance() override;
   virtual Float64 GetHaunchLoadCamberFactor() override;
   virtual bool IsAssExcessCamberInputEnabled(bool considerDeckType=true) override;

// IRatingSpecification
public:
   virtual bool AlwaysLoadRate() override;
   virtual bool IsRatingEnabled() override;
   virtual bool IsRatingEnabled(pgsTypes::LoadRatingType ratingType) override;
   virtual void EnableRating(pgsTypes::LoadRatingType ratingType,bool bEnable) override;
   virtual std::_tstring GetRatingSpecification() override;
   virtual void SetADTT(Int16 adtt) override;
   virtual Int16 GetADTT() override;
   virtual void SetRatingSpecification(const std::_tstring& spec) override;
   virtual void IncludePedestrianLiveLoad(bool bInclude) override;
   virtual bool IncludePedestrianLiveLoad() override;
   virtual void SetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor) override;
   virtual void GetGirderConditionFactor(const CSegmentKey& segmentKey,pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) override;
   virtual Float64 GetGirderConditionFactor(const CSegmentKey& segmentKey) override;
   virtual void SetDeckConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor) override;
   virtual void GetDeckConditionFactor(pgsTypes::ConditionFactorType* pConditionFactorType,Float64 *pConditionFactor) override;
   virtual Float64 GetDeckConditionFactor() override;
   virtual void SetSystemFactorFlexure(Float64 sysFactor) override;
   virtual Float64 GetSystemFactorFlexure() override;
   virtual void SetSystemFactorShear(Float64 sysFactor) override;
   virtual Float64 GetSystemFactorShear() override;
   virtual void SetDeadLoadFactor(pgsTypes::LimitState ls,Float64 gDC) override;
   virtual Float64 GetDeadLoadFactor(pgsTypes::LimitState ls) override;
   virtual void SetWearingSurfaceFactor(pgsTypes::LimitState ls,Float64 gDW) override;
   virtual Float64 GetWearingSurfaceFactor(pgsTypes::LimitState ls) override;
   virtual void SetCreepFactor(pgsTypes::LimitState ls,Float64 gCR) override;
   virtual Float64 GetCreepFactor(pgsTypes::LimitState ls) override;
   virtual void SetShrinkageFactor(pgsTypes::LimitState ls,Float64 gSH) override;
   virtual Float64 GetShrinkageFactor(pgsTypes::LimitState ls) override;
   virtual void SetRelaxationFactor(pgsTypes::LimitState ls,Float64 gRE) override;
   virtual Float64 GetRelaxationFactor(pgsTypes::LimitState ls) override;
   virtual void SetSecondaryEffectsFactor(pgsTypes::LimitState ls,Float64 gPS) override;
   virtual Float64 GetSecondaryEffectsFactor(pgsTypes::LimitState ls) override;
   virtual void SetLiveLoadFactor(pgsTypes::LimitState ls,Float64 gLL) override;
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,bool bResolveIfDefault=false) override;
   virtual Float64 GetLiveLoadFactor(pgsTypes::LimitState ls,pgsTypes::SpecialPermitType specialPermitType,Int16 adtt,const RatingLibraryEntry* pRatingEntry,bool bResolveIfDefault=false) override;
   virtual void SetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType,Float64 t) override;
   virtual Float64 GetAllowableTensionCoefficient(pgsTypes::LoadRatingType ratingType) override;
   virtual void RateForStress(pgsTypes::LoadRatingType ratingType,bool bRateForStress) override;
   virtual bool RateForStress(pgsTypes::LoadRatingType ratingType) override;
   virtual void RateForShear(pgsTypes::LoadRatingType ratingType,bool bRateForShear) override;
   virtual bool RateForShear(pgsTypes::LoadRatingType ratingType) override;
   virtual void ExcludeLegalLoadLaneLoading(bool bExclude) override;
   virtual bool ExcludeLegalLoadLaneLoading() override;
   virtual void CheckYieldStress(pgsTypes::LoadRatingType ratingType,bool bCheckYieldStress) override;
   virtual bool CheckYieldStress(pgsTypes::LoadRatingType ratingType) override;
   virtual void SetYieldStressLimitCoefficient(Float64 x) override;
   virtual Float64 GetYieldStressLimitCoefficient() override;
   virtual void SetSpecialPermitType(pgsTypes::SpecialPermitType type) override;
   virtual pgsTypes::SpecialPermitType GetSpecialPermitType() override;
   virtual Float64 GetStrengthLiveLoadFactor(pgsTypes::LoadRatingType ratingType,AxleConfiguration& axleConfig) override;
   virtual Float64 GetServiceLiveLoadFactor(pgsTypes::LoadRatingType ratingType) override;
   virtual Float64 GetReactionStrengthLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) override;
   virtual Float64 GetReactionServiceLiveLoadFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) override;

// ILibraryNames
public:
   virtual void EnumGdrConnectionNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumGirderNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumGirderNames( LPCTSTR strGirderFamily, std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumConcreteNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumDiaphragmNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumTrafficBarrierNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumSpecNames( std::vector<std::_tstring>* pNames) const override;
   virtual void EnumRatingCriteriaNames( std::vector<std::_tstring>* pNames) const override;
   virtual void EnumLiveLoadNames( std::vector<std::_tstring>* pNames) const override;
   virtual void EnumDuctNames( std::vector<std::_tstring>* pNames ) const override;
   virtual void EnumHaulTruckNames( std::vector<std::_tstring>* pNames) const override;
   virtual void EnumGirderFamilyNames( std::vector<std::_tstring>* pNames ) override;
   virtual void GetBeamFactory(const std::_tstring& strBeamFamily,const std::_tstring& strBeamName,IBeamFactory** ppFactory) override;

// ILibrary
public:
   virtual void SetLibraryManager(psgLibraryManager* pNewLibMgr) override;
   virtual psgLibraryManager* GetLibraryManager() override; 
   virtual const ConnectionLibraryEntry* GetConnectionEntry(LPCTSTR lpszName ) const override;
   virtual const GirderLibraryEntry* GetGirderEntry( LPCTSTR lpszName ) const override;
   virtual const ConcreteLibraryEntry* GetConcreteEntry( LPCTSTR lpszName ) const override;
   virtual const DiaphragmLayoutEntry* GetDiaphragmEntry( LPCTSTR lpszName ) const override;
   virtual const TrafficBarrierEntry* GetTrafficBarrierEntry( LPCTSTR lpszName ) const override;
   virtual const SpecLibraryEntry* GetSpecEntry( LPCTSTR lpszName ) const override;
   virtual const LiveLoadLibraryEntry* GetLiveLoadEntry( LPCTSTR lpszName ) const override;
   virtual const DuctLibraryEntry* GetDuctEntry( LPCTSTR lpszName ) const override;
   virtual const HaulTruckLibraryEntry* GetHaulTruckEntry(LPCTSTR lpszName) const override;
   virtual ConcreteLibrary&        GetConcreteLibrary() override;
   virtual ConnectionLibrary&      GetConnectionLibrary() override;
   virtual GirderLibrary&          GetGirderLibrary() override;
   virtual DiaphragmLayoutLibrary& GetDiaphragmLayoutLibrary() override;
   virtual TrafficBarrierLibrary&  GetTrafficBarrierLibrary() override;
   virtual SpecLibrary*            GetSpecLibrary() override;
   virtual LiveLoadLibrary*        GetLiveLoadLibrary() override;
   virtual DuctLibrary*            GetDuctLibrary() override;
   virtual HaulTruckLibrary*       GetHaulTruckLibrary() override;
   virtual std::vector<libEntryUsageRecord> GetLibraryUsageRecords() const override;
   virtual void GetMasterLibraryInfo(std::_tstring& strPublisher,std::_tstring& strMasterLib,sysTime& time) const override;
   virtual RatingLibrary* GetRatingLibrary() override;
   virtual const RatingLibrary* GetRatingLibrary() const override;
   virtual const RatingLibraryEntry* GetRatingEntry( LPCTSTR lpszName ) const override;

// ILoadModifiers
public:
   virtual void SetDuctilityFactor(Level level,Float64 value) override;
   virtual void SetImportanceFactor(Level level,Float64 value) override;
   virtual void SetRedundancyFactor(Level level,Float64 value) override;
   virtual Float64 GetDuctilityFactor() override;
   virtual Float64 GetImportanceFactor() override;
   virtual Float64 GetRedundancyFactor() override;
   virtual Level GetDuctilityLevel() override;
   virtual Level GetImportanceLevel() override;
   virtual Level GetRedundancyLevel() override;

// ISegmentLifting
public:
   virtual Float64 GetLeftLiftingLoopLocation(const CSegmentKey& segmentKey) override;
   virtual Float64 GetRightLiftingLoopLocation(const CSegmentKey& segmentKey) override;
   virtual void SetLiftingLoopLocations(const CSegmentKey& segmentKey, Float64 left,Float64 right) override;

// ISegmentHauling
public:
   virtual Float64 GetLeadingOverhang(const CSegmentKey& segmentKey) override;
   virtual Float64 GetTrailingOverhang(const CSegmentKey& segmentKey) override;
   virtual void SetTruckSupportLocations(const CSegmentKey& segmentKey, Float64 leftLoc,Float64 rightLoc) override;
   virtual LPCTSTR GetHaulTruck(const CSegmentKey& segmentKey) override;
   virtual void SetHaulTruck(const CSegmentKey& segmentKey,LPCTSTR lpszHaulTruck) override;

// IImportProjectLibrary
public:
   virtual bool ImportProjectLibraries(IStructuredLoad* pLoad) override;

// IUserDefinedLoadData
public:
   virtual bool HasUserDC(const CGirderKey& girderKey) override;
   virtual bool HasUserDW(const CGirderKey& girderKey) override;
   virtual bool HasUserLLIM(const CGirderKey& girderKey) override;
   virtual CollectionIndexType GetPointLoadCount() const override;
   virtual CollectionIndexType AddPointLoad(EventIDType eventID,const CPointLoadData& pld) override;
   virtual const CPointLoadData* GetPointLoad(CollectionIndexType idx) const override;
   virtual const CPointLoadData* FindPointLoad(LoadIDType loadID) const override;
   virtual EventIndexType GetPointLoadEventIndex(LoadIDType loadID) const override;
   virtual EventIDType GetPointLoadEventID(LoadIDType loadID) const override;
   virtual void UpdatePointLoad(CollectionIndexType idx, EventIDType eventID,const CPointLoadData& pld) override;
   virtual void DeletePointLoad(CollectionIndexType idx) override;
   virtual std::vector<CPointLoadData> GetPointLoads(const CSpanKey& spanKey) const override;

   virtual CollectionIndexType GetDistributedLoadCount() const override;
   virtual CollectionIndexType AddDistributedLoad(EventIDType eventID,const CDistributedLoadData& pld) override;
   virtual const CDistributedLoadData* GetDistributedLoad(CollectionIndexType idx) const override;
   virtual const CDistributedLoadData* FindDistributedLoad(LoadIDType loadID) const override;
   virtual EventIndexType GetDistributedLoadEventIndex(LoadIDType loadID) const override;
   virtual EventIDType GetDistributedLoadEventID(LoadIDType loadID) const override;
   virtual void UpdateDistributedLoad(CollectionIndexType idx, EventIDType eventID,const CDistributedLoadData& pld) override;
   virtual void DeleteDistributedLoad(CollectionIndexType idx) override;
   virtual std::vector<CDistributedLoadData> GetDistributedLoads(const CSpanKey& spanKey) const override;

   virtual CollectionIndexType GetMomentLoadCount() const override;
   virtual CollectionIndexType AddMomentLoad(EventIDType eventID,const CMomentLoadData& pld) override;
   virtual const CMomentLoadData* GetMomentLoad(CollectionIndexType idx) const override;
   virtual const CMomentLoadData* FindMomentLoad(LoadIDType loadID) const override;
   virtual EventIndexType GetMomentLoadEventIndex(LoadIDType loadID) const override;
   virtual EventIDType GetMomentLoadEventID(LoadIDType loadID) const override;
   virtual void UpdateMomentLoad(CollectionIndexType idx, EventIDType eventID,const CMomentLoadData& pld) override;
   virtual void DeleteMomentLoad(CollectionIndexType idx) override;
   virtual std::vector<CMomentLoadData> GetMomentLoads(const CSpanKey& spanKey) const override;

   virtual void SetConstructionLoad(Float64 load) override;
   virtual Float64 GetConstructionLoad() const override;

// ILiveLoads
public:
   virtual bool IsLiveLoadDefined(pgsTypes::LiveLoadType llType) override;
   virtual PedestrianLoadApplicationType GetPedestrianLoadApplication(pgsTypes::LiveLoadType llType) override;
   virtual void SetPedestrianLoadApplication(pgsTypes::LiveLoadType llType, PedestrianLoadApplicationType PedLoad) override;
   virtual std::vector<std::_tstring> GetLiveLoadNames(pgsTypes::LiveLoadType llType) override;
   virtual void SetLiveLoadNames(pgsTypes::LiveLoadType llType,const std::vector<std::_tstring>& names) override;
   virtual Float64 GetTruckImpact(pgsTypes::LiveLoadType llType) override;
   virtual void SetTruckImpact(pgsTypes::LiveLoadType llType,Float64 impact) override;
   virtual Float64 GetLaneImpact(pgsTypes::LiveLoadType llType) override;
   virtual void SetLaneImpact(pgsTypes::LiveLoadType llType,Float64 impact) override;
   virtual void SetLldfRangeOfApplicabilityAction(LldfRangeOfApplicabilityAction action) override;
   virtual LldfRangeOfApplicabilityAction GetLldfRangeOfApplicabilityAction() override;
   virtual std::_tstring GetLLDFSpecialActionText() override; // get common string for ignore roa case
   virtual bool IgnoreLLDFRangeOfApplicability() override; // true if action is to ignore ROA

// IEvents
public:
   virtual void HoldEvents() override;
   virtual void FirePendingEvents() override;
   virtual void CancelPendingEvents() override;

// ILimits
public:
   virtual Float64 GetMaxSlabFc(pgsTypes::ConcreteType concType) override;
   virtual Float64 GetMaxSegmentFci(pgsTypes::ConcreteType concType) override;
   virtual Float64 GetMaxSegmentFc(pgsTypes::ConcreteType concType) override;
   virtual Float64 GetMaxClosureFci(pgsTypes::ConcreteType concType) override;
   virtual Float64 GetMaxClosureFc(pgsTypes::ConcreteType concType) override;
   virtual Float64 GetMaxConcreteUnitWeight(pgsTypes::ConcreteType concType) override;
   virtual Float64 GetMaxConcreteAggSize(pgsTypes::ConcreteType concType) override;

// ILoadFactors
public:
   const CLoadFactors* GetLoadFactors() const;
   void SetLoadFactors(const CLoadFactors& loadFactors) ;

// IEventMap
public:
   virtual CComBSTR GetEventName(EventIndexType eventIdx) override;  
   virtual EventIndexType GetEventIndex(CComBSTR bstrEvent) override;

// IEffectiveFlangeWidth
public:
   virtual bool IgnoreEffectiveFlangeWidthLimits() override;
   virtual void IgnoreEffectiveFlangeWidthLimits(bool bIgnore) override;

// ILossParameters
public:
   virtual pgsTypes::LossMethod GetLossMethod() override;
   virtual pgsTypes::TimeDependentModel GetTimeDependentModel() override;
   virtual void IgnoreCreepEffects(bool bIgnore) override;
   virtual bool IgnoreCreepEffects() override;
   virtual void IgnoreShrinkageEffects(bool bIgnore) override;
   virtual bool IgnoreShrinkageEffects() override;
   virtual void IgnoreRelaxationEffects(bool bIgnore) override;
   virtual bool IgnoreRelaxationEffects() override;
   virtual void IgnoreTimeDependentEffects(bool bIgnoreCreep,bool bIgnoreShrinkage,bool bIgnoreRelaxation) override;
   virtual void SetTendonPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction) override;
   virtual void GetTendonPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) override;
   virtual void SetTemporaryStrandPostTensionParameters(Float64 Dset,Float64 wobble,Float64 friction) override;
   virtual void GetTemporaryStrandPostTensionParameters(Float64* Dset,Float64* wobble,Float64* friction) override;
   virtual void UseGeneralLumpSumLosses(bool bLumpSum) override;
   virtual bool UseGeneralLumpSumLosses() override;
   virtual Float64 GetBeforeXferLosses() override;
   virtual void SetBeforeXferLosses(Float64 loss) override;
   virtual Float64 GetAfterXferLosses() override;
   virtual void SetAfterXferLosses(Float64 loss) override;
   virtual Float64 GetLiftingLosses() override;
   virtual void SetLiftingLosses(Float64 loss) override;
   virtual Float64 GetShippingLosses() override;
   virtual void SetShippingLosses(Float64 loss) override;
   virtual Float64 GetBeforeTempStrandRemovalLosses() override;
   virtual void SetBeforeTempStrandRemovalLosses(Float64 loss) override;
   virtual Float64 GetAfterTempStrandRemovalLosses() override;
   virtual void SetAfterTempStrandRemovalLosses(Float64 loss) override;
   virtual Float64 GetAfterDeckPlacementLosses() override;
   virtual void SetAfterDeckPlacementLosses(Float64 loss) override;
   virtual Float64 GetAfterSIDLLosses() override;
   virtual void SetAfterSIDLLosses(Float64 loss) override;
   virtual Float64 GetFinalLosses() override;
   virtual void SetFinalLosses(Float64 loss) override;

// IValidate
public:
   virtual UINT Orientation(LPCTSTR lpszOrientation) override;

#ifdef _DEBUG
   bool AssertValid() const;
#endif//

private:
   DECLARE_EAF_AGENT_DATA;

   // status items must be managed by span girder
   void AddSegmentStatusItem(const CSegmentKey& segmentKey, std::_tstring& message);
   void RemoveSegmentStatusItems(const CSegmentKey& segmentKey);

   // save hash value and vector of status ids
   typedef std::map<CSegmentKey, std::vector<StatusItemIDType> > StatusContainer;
   typedef StatusContainer::iterator StatusIterator;

   StatusContainer m_CurrentGirderStatusItems;

   // used for validating orientation strings
   CComPtr<IAngle> m_objAngle;
   CComPtr<IDirection> m_objDirection;

   std::_tstring m_BridgeName;
   std::_tstring m_BridgeID;
   std::_tstring m_JobNumber;
   std::_tstring m_Engineer;
   std::_tstring m_Company;
   std::_tstring m_Comments;

   pgsTypes::AnalysisType m_AnalysisType;
   bool m_bGetAnalysisTypeFromLibrary; // if true, we are reading old input... get the analysis type from the library entry

   std::vector<std::_tstring> m_GirderFamilyNames;

   bool m_bIgnoreEffectiveFlangeWidthLimits;

   // rating data
   std::_tstring m_RatingSpec;
   const RatingLibraryEntry* m_pRatingEntry;
   bool m_bExcludeLegalLoadLaneLoading;
   bool m_bIncludePedestrianLiveLoad;
   pgsTypes::SpecialPermitType m_SpecialPermitType;
   Float64 m_SystemFactorFlexure;
   Float64 m_SystemFactorShear;
   // NOTE: the next group of arrays (m_gDC, m_gDW, etc) are larger than they need to be.
   // This is because of a bug in earlier versions of PGSuper. In those version, data was
   // stored beyond the end of the array. The size of the array has been incresed so that
   // this error wont occur.
   std::array<Float64, pgsTypes::LimitStateCount> m_gDC;
   std::array<Float64, pgsTypes::LimitStateCount> m_gDW;
   std::array<Float64, pgsTypes::LimitStateCount> m_gCR;
   std::array<Float64, pgsTypes::LimitStateCount> m_gSH;
   std::array<Float64, pgsTypes::LimitStateCount> m_gRE;
   std::array<Float64, pgsTypes::LimitStateCount> m_gPS;
   std::array<Float64, pgsTypes::LimitStateCount> m_gLL;

   std::array<Float64, pgsTypes::lrLoadRatingTypeCount> m_AllowableTensionCoefficient; // index is load rating type
   std::array<bool, pgsTypes::lrLoadRatingTypeCount>    m_bCheckYieldStress; // index is load rating type
   std::array<bool, pgsTypes::lrLoadRatingTypeCount>    m_bRateForStress; // index is load rating type
   std::array<bool, pgsTypes::lrLoadRatingTypeCount>    m_bRateForShear; // index is load rating type
   std::array<bool, pgsTypes::lrLoadRatingTypeCount>    m_bEnableRating; // index is load rating type
   Int16 m_ADTT; // < 0 = Unknown
   Float64 m_AllowableYieldStressCoefficient; // fr <= xfy for Service I permit rating

   // Environment Data
   enumExposureCondition m_ExposureCondition;
   Float64 m_RelHumidity;

   // Alignment Data
   Float64 m_AlignmentOffset_Temp;
   bool m_bUseTempAlignmentOffset;

   AlignmentData2 m_AlignmentData2;
   ProfileData2   m_ProfileData2;
   RoadwaySectionData m_RoadwaySectionData;

   // Bridge Description Data
   mutable CBridgeDescription2 m_BridgeDescription;

   mutable CLoadManager m_LoadManager;

   // Prestressing Data
   void UpdateJackingForce();
   void UpdateJackingForce(const CSegmentKey& segmentKey);
   mutable bool m_bUpdateJackingForce;

   // Specification Data
   std::_tstring m_Spec;
   const SpecLibraryEntry* m_pSpecEntry;


   // Live load selection
   struct LiveLoadSelection
   {
      std::_tstring                 EntryName;
      const LiveLoadLibraryEntry* pEntry; // nullptr if this is a system defined live load (HL-93)

      LiveLoadSelection() { pEntry = nullptr; }

      bool operator==(const LiveLoadSelection& other) const
      { return pEntry == other.pEntry; }

      bool operator<(const LiveLoadSelection& other) const
      { return EntryName < other.EntryName; }
   };

   typedef std::set<LiveLoadSelection> LiveLoadSelectionContainer;
   typedef LiveLoadSelectionContainer::iterator LiveLoadSelectionIterator;

   // index is pgsTypes::LiveLoadTypes constant
   std::array<LiveLoadSelectionContainer, pgsTypes::lltLiveLoadTypeCount> m_SelectedLiveLoads;
   std::array<Float64, pgsTypes::lltLiveLoadTypeCount> m_TruckImpact;
   std::array<Float64, pgsTypes::lltLiveLoadTypeCount> m_LaneImpact;
   std::array<PedestrianLoadApplicationType,3> m_PedestrianLoadApplicationType; // lltDesign, lltPermit, lltFatigue only

   std::vector<std::_tstring> m_ReservedLiveLoads; // reserved live load names (names not found in library)
   bool IsReservedLiveLoad(const std::_tstring& strName);

   LldfRangeOfApplicabilityAction m_LldfRangeOfApplicabilityAction;
   bool m_bGetIgnoreROAFromLibrary; // if true, we are reading old input... get the Ignore ROA setting from the spec library entry

   // Load Modifiers
   ILoadModifiers::Level m_DuctilityLevel;
   ILoadModifiers::Level m_ImportanceLevel;
   ILoadModifiers::Level m_RedundancyLevel;
   Float64 m_DuctilityFactor;
   Float64 m_ImportanceFactor;
   Float64 m_RedundancyFactor;

   // Load Factors
   CLoadFactors m_LoadFactors;

   //
   // Prestress Losses
   //

   // if true, the time dependent effect is ignores in the time-step analysis
   bool m_bIgnoreCreepEffects;
   bool m_bIgnoreShrinkageEffects;
   bool m_bIgnoreRelaxationEffects;

   // General Lump Sum Losses
   bool m_bGeneralLumpSum; // if true, the loss method specified in the project criteria is ignored
                           // and general lump sum losses are used
   Float64 m_BeforeXferLosses;
   Float64 m_AfterXferLosses;
   Float64 m_LiftingLosses;
   Float64 m_ShippingLosses;
   Float64 m_BeforeTempStrandRemovalLosses;
   Float64 m_AfterTempStrandRemovalLosses;
   Float64 m_AfterDeckPlacementLosses;
   Float64 m_AfterSIDLLosses;
   Float64 m_FinalLosses;

   // Post-tension Tendon
   Float64 m_Dset_PT;
   Float64 m_WobbleFriction_PT;
   Float64 m_FrictionCoefficient_PT;

   // Post-tension Temporary Top Strand
   Float64 m_Dset_TTS;
   Float64 m_WobbleFriction_TTS;
   Float64 m_FrictionCoefficient_TTS;


   Float64 m_ConstructionLoad;

   HRESULT LoadPointLoads(IStructuredLoad* pLoad);
   HRESULT LoadDistributedLoads(IStructuredLoad* pLoad);
   HRESULT LoadMomentLoads(IStructuredLoad* pLoad);

   Uint32 m_PendingEvents;
   int m_EventHoldCount;
   bool m_bFiringEvents;
   std::map<CGirderKey,Uint32> m_PendingEventsHash; // girders that have pending events
   std::vector<CBridgeChangedHint*> m_PendingBridgeChangedHints;

   // Callback methods for structured storage map
   static HRESULT SpecificationProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT RatingSpecificationProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT UnitModeProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT AlignmentProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT ProfileProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT SuperelevationProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT PierDataProc(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT PierDataProc2(IStructuredSave*,IStructuredLoad*,IProgress*,CProjectAgentImp*);
   static HRESULT XSectionDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT XSectionDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT BridgeDescriptionProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT PrestressingDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT PrestressingDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT ShearDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT ShearDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LongitudinalRebarDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LongitudinalRebarDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LoadFactorsProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LiftingAndHaulingDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LiftingAndHaulingLoadDataProc(IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT DistFactorMethodDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT DistFactorMethodDataProc2(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT UserLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LiveLoadsDataProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT SaveLiveLoad(IStructuredSave* pSave,IProgress* pProgress,CProjectAgentImp* pObj,LPTSTR lpszUnitName,pgsTypes::LiveLoadType llType);
   static HRESULT LoadLiveLoad(IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj,LPTSTR lpszUnitName,pgsTypes::LiveLoadType llType);
   static HRESULT EffectiveFlangeWidthProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);
   static HRESULT LossesProc(IStructuredSave* pSave,IStructuredLoad* pLoad,IProgress* pProgress,CProjectAgentImp* pObj);

   void ValidateStrands(const CSegmentKey& segmentKey,CPrecastSegmentData* pSegment,bool fromLibrary);
   void ConvertLegacyDebondData(CPrecastSegmentData* pSegment, const GirderLibraryEntry* pGdrEntry);
   void ConvertLegacyExtendedStrandData(CPrecastSegmentData* pSegment, const GirderLibraryEntry* pGdrEntry);

   Float64 GetMaxPjack(const CSegmentKey& segmentKey,pgsTypes::StrandType type,StrandIndexType nStrands) const;
   Float64 GetMaxPjack(const CSegmentKey& segmentKey,StrandIndexType nStrands,const matPsStrand* pStrand) const;

   bool ResolveLibraryConflicts(const ConflictList& rList);
   void DealWithGirderLibraryChanges(bool fromLibrary);  // behavior is different if problem is caused by a library change
   
   void MoveBridge(PierIndexType pierIdx,Float64 newStation);
   void MoveBridgeAdjustPrevSpan(PierIndexType pierIdx,Float64 newStation);
   void MoveBridgeAdjustNextSpan(PierIndexType pierIdx,Float64 newStation);
   void MoveBridgeAdjustAdjacentSpans(PierIndexType pierIdx,Float64 newStation);

   void SpecificationChanged(bool bFireEvent);
   void InitSpecification(const std::_tstring& spec);

   void RatingSpecificationChanged(bool bFireEvent);
   void InitRatingSpecification(const std::_tstring& spec);

   HRESULT FireContinuityRelatedSpanChange(const CSpanKey& spanKey,Uint32 lHint); 

   void UseBridgeLibraryEntries();
   void UseGirderLibraryEntries();
   void UseDuctLibraryEntries();
   void ReleaseBridgeLibraryEntries();
   void ReleaseGirderLibraryEntries();
   void ReleaseDuctLibraryEntries();

   void UpdateConcreteMaterial();
   void UpdateTimeDependentMaterials();
   void UpdateStrandMaterial();
   void VerifyRebarGrade();

   void ValidateBridgeModel();
   IDType m_BridgeStabilityStatusItemID;

   DECLARE_STRSTORAGEMAP(CProjectAgentImp)

   psgLibraryManager* m_pLibMgr;
   pgsLibraryEntryObserver m_LibObserver;
   friend pgsLibraryEntryObserver;

   friend CProxyIProjectPropertiesEventSink<CProjectAgentImp>;
   friend CProxyIEnvironmentEventSink<CProjectAgentImp>;
   friend CProxyIBridgeDescriptionEventSink<CProjectAgentImp>;
   friend CProxyISpecificationEventSink<CProjectAgentImp>;
   friend CProxyIRatingSpecificationEventSink<CProjectAgentImp>;
   friend CProxyILibraryConflictEventSink<CProjectAgentImp>;
   friend CProxyILoadModifiersEventSink<CProjectAgentImp>;
   friend CProxyILossParametersEventSink<CProjectAgentImp>;

   // In early versions of PGSuper, the girder concrete was stored by named reference
   // to a concrete girder library entry. Then, concrete parameters where assigned to
   // each girder individually. In order to read old files and populate the data fields
   // of the GirderData object, this concrete library name needed to be stored somewhere.
   // That is the purpose of this data member. It is only used for temporary storage
   // while loading old files
   std::_tstring m_strOldGirderConcreteName;

   // Older versions of PGSuper (before version 3.0), only conventional precast girder
   // bridges where modeled. Version 3.0 added spliced girder capabilities. The
   // stages for precast girder bridges were a fixed model. This method is called
   // when loading old files and maps the old fixed stage model into the current
   // user defined staging model
   void CreatePrecastGirderBridgeTimelineEvents();

   CPrecastSegmentData* GetSegment(const CSegmentKey& segmentKey);
   const CPrecastSegmentData* GetSegment(const CSegmentKey& segmentKey) const;

   // returns true of there are user loads of the specified type defined for
   // the specified girder
   bool HasUserLoad(const CGirderKey& girderKey,UserLoads::LoadCase lcType);

   bool m_bUpdateUserDefinedLoads; // if true, the user defined loads came from PGSuper 2.9.x or earlier and the timeline has not yet been updated

   void UpdateHaulTruck(const COldHaulTruck* pOldHaulTruck);
   const HaulTruckLibraryEntry* FindHaulTruckLibraryEntry(const COldHaulTruck* pOldHaulTruck);
   const HaulTruckLibraryEntry* FindHaulTruckLibraryEntry(Float64 kTheta,const COldHaulTruck* pOldHaulTruck);
};

#endif //__PROJECTAGENT_H_

