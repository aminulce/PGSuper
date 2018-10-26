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

// Usage notes:
// Add the following members to your dialog
// CColumnFixityComboBox m_cbColumnFixity
// pgsTypes::ColumnFixityType m_ColumnFixity
//
// In DoDataExchange, add the following
//   DDX_Control(pDX,IDC_COLUMN_FIXITY,m_cbColumnFixity);
//   DDX_CBItemData(pDX, IDC_COLUMN_FIXITY, m_ColumnFixity);

#define COLUMN_FIXITY_FIXED  0x0001
#define COLUMN_FIXITY_PINNED 0x0002
class PGSEXTCLASS CColumnFixityComboBox : public CComboBox
{
public:
   CColumnFixityComboBox(UINT fixity = COLUMN_FIXITY_FIXED | COLUMN_FIXITY_PINNED);

   void SetFixityTypes(UINT fixity);

   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);


private:
   void UpdateFixity();
   int AddFixity(pgsTypes::ColumnFixityType fixityType);

protected:
   UINT m_Fixity;
   virtual void PreSubclassWindow();
};
