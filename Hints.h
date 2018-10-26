///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2015  Washington State Department of Transportation
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

#ifndef INCLUDED_HINTS_H_
#define INCLUDED_HINTS_H_

#include <IFace\Project.h> // has girder change hints
#include <EAF\EAFHints.h>

// Hint Dialog states
#define UIHINT_ENABLE_ALL                 0x0000
#define UIHINT_DISABLE_ALL                0xFFFF
#define UIHINT_SAME_NUMBER_OF_GIRDERS     0x0001
#define UIHINT_SAME_GIRDER_SPACING        0x0002
#define UIHINT_SAME_GIRDER_NAME           0x0004
#define UIHINT_SINGLE_SLAB_OFFSET         0x0008
#define UIHINT_FAVORITES_MENU             0x0010

// This file contains all the hints sets to the views
// in the OnUpdate method.

// Doc/View only hints
#define HINT_GIRDERVIEWSETTINGSCHANGED    1
#define HINT_BRIDGEVIEWSETTINGSCHANGED    2
#define HINT_GIRDERVIEWSECTIONCUTCHANGED  3
#define HINT_BRIDGEVIEWSECTIONCUTCHANGED  4
#define HINT_SELECTIONCHANGED             5
#define HINT_GIRDERLABELFORMATCHANGED     8

// the above hints should not cause results to be updated
#define MAX_DISPLAY_HINT                 HINT_GIRDERLABELFORMATCHANGED

// Bridge-wide hints that are results of Agent events
#define MIN_RESULTS_HINT              100

#define HINT_UNITSCHANGED             MIN_RESULTS_HINT + 1
#define HINT_ENVCHANGED               MIN_RESULTS_HINT + 2
#define HINT_BRIDGECHANGED            MIN_RESULTS_HINT + 3
#define HINT_SPECCHANGED              MIN_RESULTS_HINT + 4
#define HINT_LOADMODIFIERSCHANGED     MIN_RESULTS_HINT + 5
#define HINT_PROJECTPROPERTIESCHANGED MIN_RESULTS_HINT + 6
#define HINT_GIRDERFAMILYCHANGED      MIN_RESULTS_HINT + 7
#define HINT_LIVELOADCHANGED          MIN_RESULTS_HINT + 8
#define HINT_ANALYSISTYPECHANGED      MIN_RESULTS_HINT + 9
#define HINT_RATINGSPECCHANGED        MIN_RESULTS_HINT + 10

#define MAX_RESULTS_HINT              HINT_RATINGSPECCHANGED

#define HINT_LIBRARYCHANGED           201 // Changes made to non-referenced entries only
#define HINT_GIRDERCHANGED            202 // Girder changes are treated individualy


class CGirderHint : public CObject
{
public:
   Uint32 lHint; // one of the GCH_xxx hints in IFace\Project.h
   SpanIndexType spanIdx;
   GirderIndexType gdrIdx;

   ~CGirderHint()
   {
      ; // A place to break to check for memory leaks
   }
};

class CBridgeHint : public CObject
{
public:
   // Used when a span is added or removed... 
   PierIndexType PierIdx; // Reference pier where the span is added or removed
   pgsTypes::PierFaceType PierFace; // Pier face where the span is added or removed
   bool bAdded;
};

#endif // INCLUDED_HINTS_H_