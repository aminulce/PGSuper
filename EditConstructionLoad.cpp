///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2010  Washington State Department of Transportation
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
#include "EditConstructionLoad.h"
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

txnEditConstructionLoad::txnEditConstructionLoad(Float64 oldLoad,Float64 newLoad)
{
   m_Load[0] = oldLoad;
   m_Load[1] = newLoad;
}

txnEditConstructionLoad::~txnEditConstructionLoad()
{
}

bool txnEditConstructionLoad::Execute()
{
   DoExecute(1);
   return true;
}

void txnEditConstructionLoad::Undo()
{
   DoExecute(0);
}

void txnEditConstructionLoad::DoExecute(int i)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IUserDefinedLoadData, pLoads );
   GET_IFACE2(pBroker,IEvents, pEvents);

   pEvents->HoldEvents(); // don't fire any changed events until all changes are done

   pLoads->SetConstructionLoad(m_Load[i]);

   pEvents->FirePendingEvents();
}

txnTransaction* txnEditConstructionLoad::CreateClone() const
{
   return new txnEditConstructionLoad(m_Load[0],m_Load[1]);
}

std::string txnEditConstructionLoad::Name() const
{
   return "Edit Construction Load";
}

bool txnEditConstructionLoad::IsUndoable()
{
   return true;
}

bool txnEditConstructionLoad::IsRepeatable()
{
   return false;
}
