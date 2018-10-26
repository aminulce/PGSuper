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

/*****************************************************************************
INTERFACE
   IDocumentType

   Interface used to learn the document type that is currently open.

DESCRIPTION
   Interface used to learn the document type that is currently open. PGSuper and
   PGSplice agents may need to know if the current document is a PGSuper or a PGSplice
   document.
*****************************************************************************/
// {BEA4D31A-3C91-4d6e-AB8B-F363106E3AB3}
DEFINE_GUID(IID_IDocumentType, 
0xbea4d31a, 0x3c91, 0x4d6e, 0xab, 0x8b, 0xf3, 0x63, 0x10, 0x6e, 0x3a, 0xb3);
interface IDocumentType : IUnknown
{
   virtual bool IsPGSuperDocument() = 0;
   virtual bool IsPGSpliceDocument() = 0;
};
