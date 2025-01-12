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

#ifndef INCLUDED_EDITENVIRONMENTTXN_H_
#define INCLUDED_EDITENVIRONMENTTXN_H_

#include <System\Transaction.h>
#include <IFace\Project.h>

class txnEditEnvironment : public txnTransaction
{
public:
   txnEditEnvironment(enumExposureCondition oldExposureCondition, enumExposureCondition newExposureCondition,
                      Float64 oldRelHumidity, Float64 newRelHumidity );

   ~txnEditEnvironment();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void Execute(int i);

   enumExposureCondition m_ExposureCondition[2];
   Float64 m_RelHumidity[2];
};

#endif // INCLUDED_EDITENVIRONMENTTXN_H_