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

#ifndef INCLUDED_EDITGIRDER_H_
#define INCLUDED_EDITGIRDER_H_

#include <System\Transaction.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <IFace\Project.h>

class txnEditGirder : public txnTransaction
{
public:
   txnEditGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,
                 bool bOldUseSameGirder, bool bNewUseSameGirder,
                 const std::_tstring& strOldGirderName, const std::_tstring& newGirderName,
                 const CGirderData& oldGirderData,const CGirderData& newGirderData,
                 const CShearData& oldShearData,const CShearData& newShearData,
                 const CLongitudinalRebarData& oldRebarData,const CLongitudinalRebarData& newRebarData,
                 Float64 oldLiftingLocation,  Float64 newLiftingLocation,
                 Float64 oldTrailingOverhang, Float64 newTrailingOverhang,
                 Float64 oldLeadingOverhang,  Float64 newLeadingOverhang,
                 pgsTypes::SlabOffsetType oldSlabOffsetType,pgsTypes::SlabOffsetType newSlabOffsetType,
                 Float64 oldSlabOffsetStart,Float64 newSlabOffsetStart,
                 Float64 oldSlabOffsetEnd, Float64 newSlabOffsetEnd
                 );

   ~txnEditGirder();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   SpanIndexType m_SpanIdx;
   GirderIndexType m_GirderIdx;
   bool m_bUseSameGirder[2];
   std::_tstring m_strGirderName[2];
   CGirderData m_GirderData[2];
   CShearData m_ShearData[2];
   CLongitudinalRebarData m_RebarData[2];
   Float64 m_LiftingLocation[2];
   Float64 m_TrailingOverhang[2];
   Float64 m_LeadingOverhang[2];

   pgsTypes::SlabOffsetType m_SlabOffsetType[2];
   Float64 m_SlabOffset[2][2]; // first index is new/old, second index is pgsTypes::MemberEndType
   // if slab offset is whole bridge then m_SlabOffset[i][pgsTypes::metStart] contains the value
};

#endif // INCLUDED_EDITGIRDER_H_