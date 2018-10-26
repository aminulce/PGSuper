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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\GirderMaterial.h>
#include <Units\SysUnits.h>
#include <StdIo.h>

#include <Lrfd\StrandPool.h>
#include <Material\Concrete.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CGirderMaterial
****************************************************************************/



////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CGirderMaterial::CGirderMaterial()
{
}  

CGirderMaterial::CGirderMaterial(const CGirderMaterial& rOther)
{
   MakeCopy(rOther);
}

CGirderMaterial::~CGirderMaterial()
{
}

//======================== OPERATORS  =======================================
CGirderMaterial& CGirderMaterial::operator= (const CGirderMaterial& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CGirderMaterial::operator==(const CGirderMaterial& rOther) const
{
   if ( Concrete != rOther.Concrete )
   {
      return false;
   }

   return true;
}

bool CGirderMaterial::operator!=(const CGirderMaterial& rOther) const
{
   return !operator==(rOther);
}

void CGirderMaterial::MakeCopy(const CGirderMaterial& rOther)
{
   Concrete        = rOther.Concrete;
}


void CGirderMaterial::MakeAssignment(const CGirderMaterial& rOther)
{
   MakeCopy( rOther );
}

HRESULT CGirderMaterial::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   return Concrete.Save(pStrSave,pProgress);
}

HRESULT CGirderMaterial::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   return Concrete.Load(pStrLoad,pProgress);
}
