/////////////////////////////////////////////////////////////////////////////
// Name:        toolkit.h
// Author:      Laurent Pugin
// Created:     17/10/2013
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef __VRV_MERGETOOLKIT_H__
#define __VRV_MERGETOOLKIT_H__

#include <string>

#ifdef USE_EMSCRIPTEN
#include "jsonxx.h"
#endif

#include "utf8.h"

//----------------------------------------------------------------------------

#include "doc.h"
#include "rest.h"
#include "toolkit.h"
#include "view.h"


namespace vrv {
    
    
    //----------------------------------------------------------------------------
    // Toolkit
    //----------------------------------------------------------------------------
    
    class MergeToolkit: public Toolkit
    {
    public:
        /**
         * @name Constructors and destructors
         */
        ///@{
        /** If initFont is set to false, Resources::InitFonts will have to be called explicitely */
        MergeToolkit( bool initFont = true );
        ///@}
      
        
        /**
         * Merge the two voices into one
         * Return the result as an MEI string
         */
        std::string Merge(int pageNo = 1);
        
    private:
        void MergeMeasures(Measure* m1, Measure* m2);
        
        void MergeStaffs(Staff* s1, Staff* s2);
        
        void MergeLayers(Layer* l1, Layer* l2);

        void MergeBeams(Beam* b1, Beam* b2);
        
        void MergeNotes(Note* n1, Note* n2);
        
        void MergeRests(Rest* r1, Rest* r2);
        
        void MergeVerses(Verse* v1, Verse* v2, Note* n1, Note* n2, int index);
        
    };
    
}// namespace vrv
#endif
