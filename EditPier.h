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

#ifndef INCLUDED_EDITPIER_H_
#define INCLUDED_EDITPIER_H_

#include <System\Transaction.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\PierData.h>
#include <PgsExt\GirderSpacing.h>
#include <IFace\Project.h>

struct txnEditPierData
{
   txnEditPierData();
   txnEditPierData(const CPierData* pPier);
   Float64 Station;
   std::_tstring Orientation;
   Float64 BearingOffset[2];
   Float64 EndDistance[2];
   Float64 SupportWidth[2];
   ConnectionLibraryEntry::EndDistanceMeasurementType EndDistanceMeasurementType[2];
   ConnectionLibraryEntry::BearingOffsetMeasurementType BearingOffsetMeasurementType[2];
   GirderIndexType nGirders[2];
   
   pgsTypes::PierConnectionType ConnectionType;
   CGirderSpacing GirderSpacing[2];

   Float64 DiaphragmHeight[2];
   Float64 DiaphragmWidth[2];
   ConnectionLibraryEntry::DiaphragmLoadType DiaphragmLoadType[2];
   Float64 DiaphragmLoadLocation[2];

   // data for the entire bridge
   pgsTypes::SupportedBeamSpacing GirderSpacingType;
   pgsTypes::MeasurementLocation GirderMeasurementLocation;
   bool UseSameNumberOfGirdersInAllSpans;

   pgsTypes::SlabOffsetType SlabOffsetType;
   Float64 SlabOffset[2];
};

class txnEditPier : public txnTransaction
{
public:
   txnEditPier(PierIndexType pierIdx,
               const txnEditPierData& oldPierData,
               const txnEditPierData& newPierData,
               pgsTypes::MovePierOption moveOption);

   ~txnEditPier();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);

   PierIndexType m_PierIdx;
   txnEditPierData m_PierData[2];
   pgsTypes::MovePierOption m_MoveOption;

   std::_tstring m_strPierType; // abutment or pier
};

#endif // INCLUDED_EDITPIER_H_