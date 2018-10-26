///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_IFACE_DISPLAYUNITS_H_
#define INCLUDED_IFACE_DISPLAYUNITS_H_

/*****************************************************************************
COPYRIGHT
   Copyright � 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#if !defined INCLUDED_UNITMGT_INDIRECTMEASURE_H_
#include <UnitMgt\IndirectMeasure.h>
#endif

#include "PGSuperTypes.h"

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IDisplayUnits

   Interface for getting the display units.

DESCRIPTION
   Interface for getting the display units.
*****************************************************************************/
// {8438FE92-68ED-11d2-883C-006097C68A9C}
DEFINE_GUID(IID_IDisplayUnits, 
0x8438fe92, 0x68ed, 0x11d2, 0x88, 0x3c, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IDisplayUnits : IUnknown
{
	virtual pgsTypes::UnitMode               GetUnitDisplayMode() = 0;
	virtual const unitStationFormat&         GetStationFormat() = 0;
   virtual const unitmgtScalar&             GetScalarFormat() = 0;
   virtual const unitmgtLengthData&         GetComponentDimUnit() = 0;
   virtual const unitmgtLengthData&         GetXSectionDimUnit() = 0;
   virtual const unitmgtLengthData&         GetSpanLengthUnit() = 0;
   virtual const unitmgtLengthData&         GetDisplacementUnit() = 0;
   virtual const unitmgtLengthData&         GetAlignmentLengthUnit() = 0;
   virtual const unitmgtLength2Data&        GetAreaUnit() = 0;
   virtual const unitmgtLength4Data&        GetMomentOfInertiaUnit() = 0;
   virtual const unitmgtLength3Data&        GetSectModulusUnit() = 0;
   virtual const unitmgtPressureData&       GetStressUnit() = 0;
   virtual const unitmgtPressureData&       GetModEUnit() = 0;
   virtual const unitmgtForceData&          GetGeneralForceUnit() = 0;
   virtual const unitmgtForceData&          GetShearUnit() = 0;
   virtual const unitmgtMomentData&         GetMomentUnit() = 0;
   virtual const unitmgtAngleData&          GetAngleUnit() = 0;
   virtual const unitmgtAngleData&          GetRadAngleUnit() = 0;  // Radians always
   virtual const unitmgtDensityData&        GetDensityUnit() = 0;
   virtual const unitmgtMassPerLengthData&  GetMassPerLengthUnit() = 0;
   virtual const unitmgtForcePerLengthData& GetForcePerLengthUnit() = 0;
   virtual const unitmgtMomentPerAngleData& GetMomentPerAngleUnit() = 0;
   virtual const unitmgtTimeData&           GetShortTimeUnit() = 0;
   virtual const unitmgtTimeData&           GetLongTimeUnit() = 0;
   virtual const unitmgtAreaPerLengthData&  GetAvOverSUnit()=0;
   virtual const unitmgtForceLength2Data&   GetStiffnessUnit()=0;
   virtual const unitmgtSqrtPressureData&   GetTensionCoefficientUnit() = 0;
   virtual const unitmgtPerLengthData&      GetPerLengthUnit() = 0;
   virtual const unitmgtPressureData&       GetSidewalkPressureUnit() = 0;
   virtual const unitmgtPressureData&       GetOverlayWeightUnit() = 0;
};

#define IS_US_UNITS(p) p->GetUnitDisplayMode() == pgsTypes::umUS ? true : false
#define IS_SI_UNITS(p) !IS_US_UNITS(p)

#endif // INCLUDED_IFACE_DISPLAYUNITS_H_