///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2012  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "EditLoadFactors.h"
#include <IFace\Project.h>

txnEditLoadFactors::txnEditLoadFactors(const CLoadFactors& oldLoadFactors,const CLoadFactors& newLoadFactors)
{
   m_LoadFactors[0] = oldLoadFactors;
   m_LoadFactors[1] = newLoadFactors;
}

txnEditLoadFactors::~txnEditLoadFactors()
{
}

bool txnEditLoadFactors::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditLoadFactors::Undo()
{
   DoExecute(0);
}

txnTransaction* txnEditLoadFactors::CreateClone() const
{
   return new txnEditLoadFactors(m_LoadFactors[0],m_LoadFactors[1]);
}

std::_tstring txnEditLoadFactors::Name() const
{
   std::_tostringstream os;
   os << "Edit Load Factors" << std::endl;
   return os.str();
}

bool txnEditLoadFactors::IsUndoable()
{
   return true;
}

bool txnEditLoadFactors::IsRepeatable()
{
   return false;
}

void txnEditLoadFactors::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEvents, pEvents);
   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   GET_IFACE2(pBroker,ILoadFactors,pLoadFactors);

   pLoadFactors->SetLoadFactors(m_LoadFactors[i]);

   pEvents->FirePendingEvents();
}
