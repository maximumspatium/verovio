/////////////////////////////////////////////////////////////////////////////
// Name:        toolkit.cpp
// Author:      Laurent Pugin
// Created:     17/10/2013
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "mergetoolkit.h"

//----------------------------------------------------------------------------

#include <assert.h>

//----------------------------------------------------------------------------

#include "editorial.h"
#include "iodarms.h"
#include "iomei.h"
#include "iopae.h"
#include "layer.h"
#include "page.h"
#include "measure.h"
#include "note.h"
#include "rest.h"
#include "slur.h"
#include "staff.h"
#include "svgdevicecontext.h"
#include "style.h"
#include "system.h"
#include "verse.h"
#include "vrv.h"

namespace vrv {
    
    
    MergeToolkit::MergeToolkit( bool initFont ):Toolkit(initFont)
    {
        
    }
    
    
    std::string MergeToolkit::Merge( int pageNo)
    {
        pageNo--;
        Page* current_page = dynamic_cast<Page*>((&m_doc)->GetChild(pageNo));
        ArrayOfObjects systems = current_page->m_children;
        for (int i = 0; i < systems.size(); i++) {
            System* current_system = dynamic_cast<System*>(systems.at(i));
            ArrayOfObjects measures = current_system->m_children;
            for(int j = 0; j < measures.size(); j++) {
                // for now assume two voices/staffs
                Measure* current_measure = dynamic_cast<Measure*>(measures.at(j));
                ArrayOfObjects staffs = current_measure->m_children;
                Staff* staff_1 = dynamic_cast<Staff*>(staffs.at(0));
                Staff* staff_2 = dynamic_cast<Staff*>(staffs.at(1));
                // assume 1 layer
                Layer* layer_1 = dynamic_cast<Layer*>(staff_1->m_children.front());
                Layer* layer_2 = dynamic_cast<Layer*>(staff_2->m_children.front());
                ArrayOfObjects notes_1 = layer_1->m_children;
                ArrayOfObjects notes_2 = layer_2->m_children;
                if (notes_1.size() != notes_2.size()) {
                    std::cerr << "false1";
                }
                else {
                    for (int k = 0; k < notes_1.size(); k++) {
                        Note* note_1 = dynamic_cast<Note*>(notes_1.at(k));
                        Note* note_2 = dynamic_cast<Note*>(notes_2.at(k));
                        if (note_1 == NULL && note_2 == NULL) {
                            Rest* rest_1 = dynamic_cast<Rest*>(notes_1.at(k));
                            Rest* rest_2 = dynamic_cast<Rest*>(notes_2.at(k));
                            if (rest_1 == NULL || rest_2 == NULL) {
                                
                            }
                            else {
                                if (rest_1->GetActualDur() == rest_2->GetActualDur()) {
                                
                                }
                                else {
                                    std::cerr << "false";
                                }
                            }
                        }
                        else {
                            if (note_1 == NULL || note_2 == NULL) {
                                std::cerr << "false";
                            }
                            else {
                                if (note_1->GetActualDur() == note_2->GetActualDur() &&
                                    note_1->GetDiatonicPitch() == note_2->GetDiatonicPitch()) {
                                    if (note_1->m_children.size() > 0 && note_1->m_children.size() > 0) {
                                    Verse* verse_1 = dynamic_cast<Verse*>(note_1->m_children.front());
                                    Verse* verse_2 = dynamic_cast<Verse*>(note_2->m_children.front());
                                        if (verse_1 != NULL && verse_2 != NULL) {
                                            note_1->m_children.clear();
                                            note_2->m_children.clear();
                                            verse_1->m_parent = NULL;
                                            verse_2->m_parent = NULL;
                                            App* app = new App(EDITORIAL_NOTE);
                                            Lem* lem = new Lem();
                                            lem->AddLayerElement(verse_1);
                                            Rdg* rdg = new Rdg();
                                            rdg->AddLayerElement(verse_2);
                                            app->AddLemOrRdg(lem);
                                            app->AddLemOrRdg(rdg);
                                            note_1->AddEditorialElement(app);
                                        }
                                    }
                                }
                                else {
                                    std::cerr << "false";
                                }
                            }
                        }
                    }
                    
                }
                //delete staff_2;
            }
        }
        
        MeiOutput meioutput( &m_doc, "" );
        meioutput.SetScoreBasedMEI( true);
        return meioutput.GetOutput( pageNo );
    }
 } //namespace vrv
