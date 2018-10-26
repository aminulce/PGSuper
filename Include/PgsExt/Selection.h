///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2017  Washington State Department of Transportation
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

#include <PgsExt\PgsExtExp.h>

class PGSEXTCLASS CSelection
{
public:
   enum Type { None, Pier, Span, Girder, Segment, ClosureJoint, TemporarySupport, Deck, Alignment } Type;
   SpanIndexType SpanIdx;
   PierIndexType PierIdx;
   GroupIndexType GroupIdx;
   GirderIndexType GirderIdx;
   SegmentIndexType SegmentIdx;
   SupportIDType tsID; // ID of the selected temporary support

   CSelection();
   CSelection(const CSelection& other);
   ~CSelection();

   CSelection& operator=(const CSelection& other);

   bool operator==(const CSelection& other) const;
   bool operator!=(const CSelection& other) const;
};
