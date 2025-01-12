///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2023  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSLIB_SHEARZONEDATA_H_
#define INCLUDED_PGSLIB_SHEARZONEDATA_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>


// PROJECT INCLUDES
//
#include "psgLibLib.h"

#include <StrData.h>
#include <Material\Rebar.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
   class sysIStructuredLoad;
   class sysIStructuredSave;
   class CShearData2;
// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CShearZoneData

   Utility class for shear zone description data.

DESCRIPTION
   Utility class for shear description data. This class encapsulates 
   the input data a single shear zone and implements the IStructuredLoad 
   and IStructuredSave persistence interfaces.

LOG
   rdp : 12.03.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS CShearZoneData2
{
   friend CShearData2; // only or friend can see legacy data

public:
   ZoneIndexType  ZoneNum;
   matRebar::Size VertBarSize;
   Float64 BarSpacing;
   Float64 ZoneLength;
   Float64 nVertBars;
   Float64 nHorzInterfaceBars;
   matRebar::Size ConfinementBarSize;

   bool bWasDesigned; // For use by design algorithm only

private:
   // These values are used only for CShearData version < 9
   matRebar::Size  legacy_HorzBarSize;
   Uint32          legacy_nHorzBars;

   // GROUP: LIFECYCLE
public:
   //------------------------------------------------------------------------
   // Default constructor
   CShearZoneData2();

   //------------------------------------------------------------------------
   // Copy constructor
   CShearZoneData2(const CShearZoneData2& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CShearZoneData2();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CShearZoneData2& operator = (const CShearZoneData2& rOther);
   bool operator == (const CShearZoneData2& rOther) const;
   bool operator != (const CShearZoneData2& rOther) const;

   // GROUP: OPERATIONS

	HRESULT Load(sysIStructuredLoad* pStrLoad, bool bConvertToShearDataVersion9, 
                matRebar::Size ConfinementBarSize,Uint32 NumConfinementZones, 
                bool bDoStirrupsEngageDeck);

	HRESULT Save(sysIStructuredSave* pStrSave);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CShearZoneData2& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CShearZoneData2& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//
class PSGLIBCLASS ShearZoneData2Less
{
public:
   bool operator()(const CShearZoneData2& a, const CShearZoneData2& b)
   {
      return a.ZoneNum < b.ZoneNum;
   }
};


#endif // INCLUDED_PGSLIB_SHEARZONEDATA_H_
