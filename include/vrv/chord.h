/////////////////////////////////////////////////////////////////////////////
// Name:        chord.h
// Author:      Andrew Horwitz
// Created:     2015
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef __VRV_CHORD_H__
#define __VRV_CHORD_H__

#include <typeinfo>

//----------------------------------------------------------------------------

#include "atts_shared.h"
#include "drawinglistinterface.h"
#include "durationinterface.h"
#include "layerelement.h"
#include "object.h"

namespace vrv {
    
    
//----------------------------------------------------------------------------
// Chord
//----------------------------------------------------------------------------

/**
 * This class represents a collection of notes in the same layer with the
 *    same onset time.
 * A chord is contained in a layer.
 * It contains notes.
 */
struct yExtremes {
    float yMin;
    float yMax;
};
    
class Chord: public LayerElement, public DrawingListInterface, public ObjectListInterface, public DurationInterface,
    public AttCommon,
    public AttStemmed,
    public AttColoration
{
public:
    /**
     * @name Constructors, destructors, reset and class name methods
     * Reset method reset all attribute classes
     */
    ///@{
    Chord( );
    virtual ~Chord();
    virtual void Reset();
    virtual std::string GetClassName( ) { return "Chord"; };
    
    /**
     * Add an element (only note supported) to a chord.
     */
    void AddElement(LayerElement *element);

    void FilterList();
    
    yExtremes GetYExtremes(float initial);

    ///@}
};

} // namespace vrv

#endif