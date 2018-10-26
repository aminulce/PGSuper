///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2011  Washington State Department of Transportation
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

#include <WbflTypes.h>
#include <Lrfd\LiveLoadDistributionFactorBase.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>

#define EBD_ROADWAY        0

#define EBD_GENERAL        0
#define EBD_FRAMING        1
#define EBD_RAILING        2
#define EBD_DECK           3
#define EBD_DECKREBAR      4
#define EBD_ENVIRONMENT    5

#define EGD_GENERAL        0
#define EGD_PRESTRESSING   1
#define EGD_DEBONDING      2
#define EGD_CONCRETE       EGD_GENERAL
#define EGD_STIRRUPS       3
#define EGD_TRANSPORTATION 4
#define EGD_CONDITION      5

/*****************************************************************************
INTERFACE
   IEditByUI

   Interface used to invoke elements of the user interface so that the user
   can edit input data.

DESCRIPTION
   Interface used to invoke elements of the user interface so that the user
   can edit input data.

   Methods on this interface should only be called by core agents (not extension agents)

   Future versions of PGSuper will permit Agents to supply their own user interface
   and this will likely eliminate the need for this interface. This interface
   may be removed in the future.
*****************************************************************************/
// {E1CF3EAA-3E85-450a-9A67-D68FF321DC16}
DEFINE_GUID(IID_IEditByUI, 
0xe1cf3eaa, 0x3e85, 0x450a, 0x9a, 0x67, 0xd6, 0x8f, 0xf3, 0x21, 0xdc, 0x16);
interface IEditByUI : IUnknown
{
   virtual void EditBridgeDescription(int nPage) = 0;
   virtual void EditAlignmentDescription(int nPage) = 0;
   virtual bool EditGirderDescription(SpanIndexType span,GirderIndexType girder, int nPage) = 0;
   virtual bool EditSpanDescription(SpanIndexType spanIdx, int nPage) = 0;
   virtual bool EditPierDescription(PierIndexType pierIdx, int nPage) = 0;
   virtual void EditLiveLoads() = 0;
   virtual void EditLiveLoadDistributionFactors(pgsTypes::DistributionFactorMethod method,LldfRangeOfApplicabilityAction roaAction) = 0;
   virtual bool EditPointLoad(CollectionIndexType loadIdx) = 0;
   virtual bool EditDistributedLoad(CollectionIndexType loadIdx) = 0;
   virtual bool EditMomentLoad(CollectionIndexType loadIdx) = 0;

   virtual UINT GetStdToolBarID() = 0;
   virtual UINT GetLibToolBarID() = 0;
   virtual UINT GetHelpToolBarID() = 0;
};

// Extends the load editing capabilities... presents user with load editing UI as needed
// {9E1D97F8-8315-4c77-AF6F-909310E114E6}
DEFINE_GUID(IID_IEditByUIEx, 
0x9e1d97f8, 0x8315, 0x4c77, 0xaf, 0x6f, 0x90, 0x93, 0x10, 0xe1, 0x14, 0xe6);
interface IEditByUIEx : IEditByUI
{
   virtual void AddPointLoad(const CPointLoadData& loadData) = 0;
   virtual void DeletePointLoad(CollectionIndexType loadIdx) = 0;
   virtual void AddDistributedLoad(const CDistributedLoadData& loadData) = 0;
   virtual void DeleteDistributedLoad(CollectionIndexType loadIdx) = 0;
   virtual void AddMomentLoad(const CMomentLoadData& loadData) = 0;
   virtual void DeleteMomentLoad(CollectionIndexType loadIdx) = 0;
};
