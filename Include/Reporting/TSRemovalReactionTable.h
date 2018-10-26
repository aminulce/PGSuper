///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2013  Washington State Department of Transportation
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
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include "ReportNotes.h"

interface IEAFDisplayUnits;
interface IRatingSpecification;


/*****************************************************************************
CLASS 
   CTSRemovalReactionTable

   Encapsulates the construction of the temporary support removal forces table.


DESCRIPTION
   Encapsulates the construction of the temporary support removal forces table.


COPYRIGHT
   Copyright � 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.18.2012 : Created file
*****************************************************************************/

class REPORTINGCLASS CTSRemovalReactionTable
{
public:
   // This class serves double duty. It can report pier reactions or girder bearing reactions.
   // The two are identical except for the title and the interfaces they use to get responses
   enum TableType { PierReactionsTable, BearingReactionsTable};

   CTSRemovalReactionTable();
   CTSRemovalReactionTable(const CTSRemovalReactionTable& rOther);
   virtual ~CTSRemovalReactionTable();

   CTSRemovalReactionTable& operator = (const CTSRemovalReactionTable& rOther);

   void Build(rptChapter* pChapter,IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,TableType tableType,IEAFDisplayUnits* pDisplayUnits) const;

protected:
   void MakeCopy(const CTSRemovalReactionTable& rOther);
   virtual void MakeAssignment(const CTSRemovalReactionTable& rOther);
};
