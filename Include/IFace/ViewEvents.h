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

#pragma once

/////////////////////////////////////////////////////////////////////////////////////////
// View event callback interfaces are defined in this header file. Extension agents
// should implement these interfaces, and attached them to their corrosponding views,
// to receive event notifications.
//
// These are prototype interfaces. That means they are subject to change. It is unclear
// as to exactly which events extenions need to receive and how they are to be broadcast.
//
// Be warned that these interfaces may change in the future and your extension agents
// are likely to break.
//

/////////////////////////////////////////////////////////
// Callbacks for the Bridge Plan View
interface IBridgePlanViewEventCallback
{
   // called when a context menu is created in the background of the view
   virtual void OnBackgroundContextMenu(CEAFMenu* pMenu) = 0;

   // called when a context menu is created for a pier
   virtual void OnPierContextMenu(PierIndexType pierIdx,CEAFMenu* pMenu) = 0;

   // called when a context menu is created for a span
   virtual void OnSpanContextMenu(SpanIndexType spanIdx,CEAFMenu* pMenu) = 0;

   // called when a context menu is created for a girder
   virtual void OnGirderContextMenu(SpanIndexType spanIdx,GirderIndexType gdrIdx,CEAFMenu* pMenu) = 0;

   // called when a context menu is created for the deck
   virtual void OnDeckContextMenu(CEAFMenu* pMenu) = 0;

   // called when a context menu is created for the alignment
   virtual void OnAlignmentContextMenu(CEAFMenu* pMenu) = 0;

   // called when a context menu is created for the section cut object
   virtual void OnSectionCutContextMenu(CEAFMenu* pMenu) = 0;
};


/////////////////////////////////////////////////////////
// Callbacks for the Bridge Section View
interface IBridgeSectionViewEventCallback
{
   virtual void OnBackgroundContextMenu(CEAFMenu* pMenu) = 0;
   virtual void OnGirderContextMenu(SpanIndexType spanIdx,GirderIndexType gdrIdx,CEAFMenu* pMenu) = 0;
   virtual void OnDeckContextMenu(CEAFMenu* pMenu) = 0;
};


/////////////////////////////////////////////////////////
// Callbacks for the Girder Elevation View
interface IGirderElevationViewEventCallback
{
   virtual void OnBackgroundContextMenu(CEAFMenu* pMenu) = 0;
};

/////////////////////////////////////////////////////////
// Callbacks for the Girder Section View
interface IGirderSectionViewEventCallback
{
   virtual void OnBackgroundContextMenu(CEAFMenu* pMenu) = 0;
};

/////////////////////////////////////////////////////////
// IRegisterViewEvents
// Use this interface to register callbacks for view events

// {EB057BFE-3A37-48af-8F19-80465DBA2A14}
DEFINE_GUID(IID_IRegisterViewEvents, 
0xeb057bfe, 0x3a37, 0x48af, 0x8f, 0x19, 0x80, 0x46, 0x5d, 0xba, 0x2a, 0x14);
struct __declspec(uuid("{EB057BFE-3A37-48af-8F19-80465DBA2A14}")) IRegisterViewEvents;
interface IRegisterViewEvents : IUnknown
{
   virtual Uint32 RegisterBridgePlanViewCallback(IBridgePlanViewEventCallback* pCallback) = 0;
   virtual Uint32 RegisterBridgeSectionViewCallback(IBridgeSectionViewEventCallback* pCallback) = 0;
   virtual Uint32 RegisterGirderElevationViewCallback(IGirderElevationViewEventCallback* pCallback) = 0;
   virtual Uint32 RegisterGirderSectionViewCallback(IGirderSectionViewEventCallback* pCallback) = 0;
};