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

#pragma once;

#include <Reporting\ReportingExp.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CStrandLocations

   Encapsulates the construction of the Strand Locations tables.


DESCRIPTION
   Encapsulates the construction of the Strand Locations tables.

LOG
   rdp : 08.17.1999 : Created file
*****************************************************************************/

class REPORTINGCLASS CStrandLocations
{
public:
   //------------------------------------------------------------------------
   // Default constructor
   CStrandLocations();

   //------------------------------------------------------------------------
   // Copy constructor
   CStrandLocations(const CStrandLocations& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CStrandLocations();

   //------------------------------------------------------------------------
   // Assignment operator
   CStrandLocations& operator = (const CStrandLocations& rOther);

   //------------------------------------------------------------------------
   // Builds the stirrup table.
   virtual void Build(rptChapter* pChapter,IBroker* pBroker,const CSegmentKey& segmentKey,
                      IEAFDisplayUnits* pDisplayUnits) const;

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const CStrandLocations& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CStrandLocations& rOther);
};
