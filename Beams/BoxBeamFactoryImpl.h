///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2010  Washington State Department of Transportation
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

// CBoxBeamFactoryImpl.h : A partial implementation for box beams

#ifndef __BOXBEAMFACTORYIMPL_H_
#define __BOXBEAMFACTORYIMPL_H_

#include "resource.h"       // main symbols
#include "IFace\BeamFactory.h"

#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFactoryImpl
class ATL_NO_VTABLE CBoxBeamFactoryImpl:
   public IBeamFactory
{
public:
	CBoxBeamFactoryImpl()
	{
	}

public:
   // IBeamFactory
   virtual void CreateGirderProfile(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape);
   virtual void LayoutGirderLine(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr);
   virtual void LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr);
   virtual void CreateDistFactorEngineer(IBroker* pBroker,long agentID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng);
   virtual void CreatePsLossEngineer(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng);
   virtual std::vector<std::string> GetDimensionNames();
   virtual std::vector<const unitLength*> GetDimensionUnits(bool bSIUnits);
   virtual std::vector<double> GetDefaultDimensions();
   virtual bool IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual Float64 GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx);
   virtual CLSID GetFamilyCLSID();
   virtual std::string GetGirderFamilyName();
   virtual std::string GetPublisher();
   virtual HINSTANCE GetResourceInstance();
   virtual pgsTypes::SupportedDeckTypes GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs);
   virtual pgsTypes::SupportedBeamSpacings GetSupportedBeamSpacings();
   virtual long GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions);
   virtual Float64 GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType);
   virtual std::string GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType);
   virtual std::string GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType);
   virtual std::string GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType);
   virtual std::string GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType);
   virtual std::string GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType);
   virtual std::string GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType);

protected:
   std::vector<std::string> m_DimNames;
   std::vector<double> m_DefaultDims;
   std::vector<const unitLength*> m_DimUnits[2];

   double GetDimension(const IBeamFactory::Dimensions& dimensions,const std::string& name);

   virtual bool ExcludeExteriorBeamShearKeys() = 0;
   virtual bool UseOverallWidth() = 0;
};

#endif //__BOXBEAMFACTORYIMPL_H_
