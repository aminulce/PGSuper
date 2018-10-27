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

// BoxBeamFactory.h : Declaration of the CBoxBeamFactory2

#ifndef __BOXBEAMFACTORY2_H_
#define __BOXBEAMFACTORY2_H_

#include "resource.h"       // main symbols
#include "IFace\BeamFactory.h"
#include "IBeamFactory.h" // CLSID
#include "BoxBeamFactoryImpl.h"


/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFactory2
class ATL_NO_VTABLE CBoxBeamFactory2 : 
   public CBoxBeamFactoryImpl,
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CBoxBeamFactory2, &CLSID_BoxBeam2Factory>
{
public:
	CBoxBeamFactory2()
	{
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_BOXBEAMFACTORY2)
DECLARE_CLASSFACTORY_SINGLETON(CBoxBeamFactory2)

BEGIN_COM_MAP(CBoxBeamFactory2)
   COM_INTERFACE_ENTRY(IBeamFactory)
END_COM_MAP()

public:
   // IBeamFactory
   virtual void CreateGirderSection(IBroker* pBroker,StatusItemIDType statusID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) override;
   virtual bool ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) override;
   virtual void SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) override;
   virtual IBeamFactory::Dimensions LoadSectionDimensions(sysIStructuredLoad* pLoad) override;
   virtual Float64 GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey) override;
   virtual void CreateStrandMover(const IBeamFactory::Dimensions& dimensions, Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) override;
   virtual std::_tstring GetImage() override;
   virtual CLSID GetCLSID() override;
   virtual LPCTSTR GetImageResourceName() override;
   virtual HICON GetIcon() override;
   virtual bool IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) override;
   virtual void GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) override;
   virtual Float64 GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) override;
   virtual void GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) override;

protected:
   virtual bool ExcludeExteriorBeamShearKeys() override { return true; }
   virtual bool UseOverallWidth() override { return true; }

private:
   void GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                    Float64& H1, 
                                    Float64& H2, 
                                    Float64& H3, 
                                    Float64& H4, 
                                    Float64& H5,
                                    Float64& W1, 
                                    Float64& W2, 
                                    Float64& W3, 
                                    Float64& W4, 
                                    Float64& F1, 
                                    Float64& F2, 
                                    Float64& C1,
                                    Float64& J,
                                    Float64& endBlockLength);
};

#endif //__BOXBEAMFACTORY2_H_
