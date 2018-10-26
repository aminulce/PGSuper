///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2014  Washington State Department of Transportation
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

#include <System\Transaction.h>
#include <PgsExt\PrecastSegmentData.h>
#include <IFace\Project.h>

struct txnEditPrecastSegmentData
{
   bool operator<(const txnEditPrecastSegmentData& rOther) const { return m_SegmentKey < rOther.m_SegmentKey; }
   CSegmentKey m_SegmentKey;
   CPrecastSegmentData m_SegmentData;
   EventIndexType  m_ConstructionEventIdx;
   EventIndexType  m_ErectionEventIdx;
};

class txnEditPrecastSegment : public txnTransaction
{
public:
   txnEditPrecastSegment(const CSegmentKey& segmentKey,const txnEditPrecastSegmentData& newData);

   ~txnEditPrecastSegment();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);

   void SetSegmentData(const CSegmentKey& segmentKey,const txnEditPrecastSegmentData& data);

   CSegmentKey m_SegmentKey;
   txnEditPrecastSegmentData m_NewSegmentData;
   std::set<txnEditPrecastSegmentData> m_OldSegmentData;
};
