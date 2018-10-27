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

#pragma once

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;

// Reports the stresses in the girder segments due to pretensioning
class REPORTINGCLASS CPretensionStressTable
{
public:
   CPretensionStressTable();
   CPretensionStressTable(const CPretensionStressTable& rOther);
   virtual ~CPretensionStressTable();

   CPretensionStressTable& operator = (const CPretensionStressTable& rOther);
   virtual rptRcTable* Build(IBroker* pBroker,const CSegmentKey& segmentKey,
                             bool bDesign,IEAFDisplayUnits* pDisplayUnits) const;

protected:
   void MakeCopy(const CPretensionStressTable& rOther);
   void MakeAssignment(const CPretensionStressTable& rOther);
};
