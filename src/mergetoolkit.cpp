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
    
    const char *MERGE_UTF_16_BE_BOM = "\xFE\xFF";
    const char *MERGE_UTF_16_LE_BOM = "\xFF\xFE";
    
    //----------------------------------------------------------------------------
    // Toolkit
    //----------------------------------------------------------------------------
    
    MergeToolkit::MergeToolkit( bool initFont )
    {
        
        m_scale = DEFAULT_SCALE;
        m_format = mei_file;
        
        // default page size
        m_pageHeight = DEFAULT_PAGE_HEIGHT;
        m_pageWidth = DEFAULT_PAGE_WIDTH;
        m_border = DEFAULT_PAGE_LEFT_MAR;
        m_spacingStaff = DEFAULT_SPACING_STAFF;
        m_spacingSystem = DEFAULT_SPACING_SYSTEM;
        
        m_noLayout = false;
        m_ignoreLayout = false;
        m_adjustPageHeight = false;
        m_noJustification = false;
        m_showBoundingBoxes = false;
        
        m_cString = NULL;
        
        if ( initFont ) {
            Resources::InitFonts();
        }
    }
    
    
    MergeToolkit::~MergeToolkit()
    {
        if (m_cString) {
            free( m_cString );
        }
    }
    
    bool MergeToolkit::SetResourcePath( const std::string &path )
    {
        Resources::SetPath( path );
        return Resources::InitFonts();
    };
    
    bool MergeToolkit::SetBorder( int border )
    {
        // We use left margin values because for now we cannot specify different values for each margin
        if (border < MIN_PAGE_LEFT_MAR || border > MAX_PAGE_LEFT_MAR) {
            LogError( "Border out of bounds; default is %d, minimun is %d, and maximum is %d", DEFAULT_PAGE_LEFT_MAR, MIN_PAGE_LEFT_MAR, MAX_PAGE_LEFT_MAR );
            return false;
        }
        m_border = border;
        return true;
    }
    
    bool MergeToolkit::SetScale( int scale )
    {
        if (scale < MIN_SCALE || scale > MAX_SCALE) {
            LogError( "Scale out of bounds; default is %d, minimun is %d, and maximum is %d", DEFAULT_SCALE, MIN_SCALE, MAX_SCALE );
            return false;
        }
        m_scale = scale;
        return true;
    }
    
    bool MergeToolkit::SetPageHeight( int h )
    {
        if (h < MIN_PAGE_HEIGHT || h > MAX_PAGE_HEIGHT) {
            LogError( "Page height out of bounds; default is %d, minimun is %d, and maximum is %d", DEFAULT_PAGE_HEIGHT, MIN_PAGE_HEIGHT, MAX_PAGE_HEIGHT );
            return false;
        }
        m_pageHeight = h;
        return true;
    }
    
    bool MergeToolkit::SetPageWidth( int w )
    {
        if (w < MIN_PAGE_WIDTH || w > MAX_PAGE_WIDTH) {
            LogError( "Page width out of bounds; default is %d, minimun is %d, and maximum is %d", DEFAULT_PAGE_WIDTH, MIN_PAGE_WIDTH, MAX_PAGE_WIDTH );
            return false;
        }
        m_pageWidth = w;
        return true;
    };
    
    bool MergeToolkit::SetSpacingStaff( int spacingStaff )
    {
        if (spacingStaff < MIN_SPACING_STAFF || spacingStaff > MAX_SPACING_STAFF) {
            LogError( "Spacing staff out of bounds; default is %d, minimun is %d, and maximum is %d", DEFAULT_SPACING_STAFF, MIN_SPACING_STAFF, MAX_SPACING_STAFF );
            return false;
        }
        m_spacingStaff = spacingStaff;
        return true;
    }
    
    
    bool MergeToolkit::SetSpacingSystem( int spacingSystem )
    {
        if (spacingSystem < MIN_SPACING_SYSTEM || spacingSystem > MAX_SPACING_SYSTEM) {
            LogError( "Spacing system out of bounds; default is %d, minimun is %d, and maximum is %d", DEFAULT_SPACING_SYSTEM, MIN_SPACING_SYSTEM, MAX_SPACING_SYSTEM );
            return false;
        }
        m_spacingSystem = spacingSystem;
        return true;
    }
    
    
    bool MergeToolkit::SetFormat( std::string const &informat )
    {
        if (informat == "pae")
            m_format = pae_file;
        else if(informat == "darms")
            m_format = darms_file;
        else if(informat == "mei")
            m_format = mei_file;
        else {
            LogError("Input format can only be: pae mei or darms");
            return false;
        }
        return true;
    };
    
    
    bool MergeToolkit::SetFont( std::string const &font )
    {
        return Resources::SetFont(font);
    };
    
    bool MergeToolkit::LoadFile( const std::string &filename )
    {
        if ( IsUTF16( filename ) ) {
            return LoadUTF16File( filename );
        }
        
        std::ifstream in( filename.c_str() );
        if (!in.is_open()) {
            return false;
        }
        
        in.seekg(0, std::ios::end);
        std::streamsize fileSize = (std::streamsize)in.tellg();
        in.clear();
        in.seekg(0, std::ios::beg);
        
        // read the file into the string:
        std::string content( fileSize, 0 );
        in.read(&content[0], fileSize);
        
        return LoadString( content );
    }
    
    bool MergeToolkit::IsUTF16( const std::string &filename )
    {
        std::ifstream fin(filename.c_str(), std::ios::in | std::ios::binary);
        if (!fin.is_open()) {
            return false;
        }
        
        char data[2];
        memset( data, 0, 2 );
        fin.read( data, 2 );
        fin.close();
        
        if (memcmp(data, MERGE_UTF_16_LE_BOM, 2) == 0) return true;
        if (memcmp(data, MERGE_UTF_16_BE_BOM, 2) == 0) return true;
        
        return false;
    }
    
    bool MergeToolkit::LoadUTF16File( const std::string &filename )
    {
        /// Loading a UTF-16 file with basic conversion ot UTF-8
        /// This is called after checking if the file has a UTF-16 BOM
        
        LogWarning("The file seems to be UTF-16 - trying to convert to UTF-8");
        
        std::ifstream fin(filename.c_str(), std::ios::in | std::ios::binary);
        if (!fin.is_open()) {
            return false;
        }
        
        fin.seekg(0, std::ios::end);
        std::streamsize wfileSize = (std::streamsize)fin.tellg();
        fin.clear();
        fin.seekg(0, std::wios::beg);
        
        std::vector<unsigned short> utf16line;
        utf16line.reserve(wfileSize / 2 + 1);
        
        unsigned short buffer;
        while(fin.read((char *)&buffer, sizeof(unsigned short)))
        {
            utf16line.push_back(buffer);
        }
        //LogDebug("%d %d", wfileSize, utf8line.size());
        
        std::string utf8line;
        utf8::utf16to8(utf16line.begin(), utf16line.end(), back_inserter(utf8line));
        
        return LoadString( utf8line );
    }
    
    bool MergeToolkit::LoadString( const std::string &data )
    {
        FileInputStream *input = NULL;
        if (m_format == pae_file) {
            input = new PaeInput( &m_doc, "" );
        } else if (m_format == darms_file) {
            input = new DarmsInput( &m_doc, "" );
        } else if (m_format == mei_file) {
            input = new MeiInput( &m_doc, "" );
        }
        else {
            LogError( "Unknown format" );
            return false;
        }
        
        // something went wrong
        if ( !input ) {
            LogError( "Unknown error" );
            return false;
        }
        
        // ignore layout?
        if ( m_ignoreLayout || m_noLayout ) {
            input->IgnoreLayoutInformation();
        }
        
        // rdg xpath query?
        if ( m_rdgXPathQuery.length() > 0 ) {
            input->SetRdgXPathQuery( m_rdgXPathQuery );
        }
        
        // load the file
        if ( !input->ImportString( data )) {
            LogError( "Error importing data" );
            delete input;
            return false;
        }
        
        m_doc.SetPageHeight( this->GetPageHeight() );
        m_doc.SetPageWidth( this->GetPageWidth() );
        m_doc.SetPageRightMar( this->GetBorder() );
        m_doc.SetPageLeftMar( this->GetBorder() );
        m_doc.SetPageTopMar( this->GetBorder() );
        m_doc.SetSpacingStaff( this->GetSpacingStaff() );
        m_doc.SetSpacingSystem( this->GetSpacingSystem() );
        
        m_doc.PrepareDrawing();
        
        if (input->HasMeasureWithinEditoMarkup() && !m_noLayout) {
            LogWarning( "Only continous layout is possible with <measure> within editorial markup, switching to --no-layout" );
            this->SetNoLayout( true );
        }
        
        // do the layout? this depends on the options and of the
        // file. PAE and DARMS of no layout information. MEI files
        // can have, but this might have been ignored because of the
        // --ignore-layout option. We won't do it if --no-layout option
        // was set, though.
        if (!input->HasLayoutInformation() && !m_noLayout) {
            //LogElapsedTimeStart();
            m_doc.CastOff();
            //LogElapsedTimeEnd("layout");
        }
        
        // disable justification if no layout or no justification
        if (m_noLayout || m_noJustification) {
            m_doc.SetJustificationX(false);
        }
        
        delete input;
        m_view.SetDoc( &m_doc );
        
        return true;
    }
    
    
    std::string MergeToolkit::GetMEI( int pageNo, bool scoreBased )
    {
        // Page number is one-based - correction to 0-based first
        pageNo--;
        
        MeiOutput meioutput( &m_doc, "" );
        meioutput.SetScoreBasedMEI( scoreBased );
        return meioutput.GetOutput( pageNo );
    }
    
    
    std::string MergeToolkit::Merge( int pageNo)
    {
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
                                }
                                else {
                                    std::cerr << "false";
                                }
                            }
                        }
                        
                    }
                }

            }
        }
        return "Finished";
    }
    
    bool MergeToolkit::SaveFile( const std::string &filename )
    {
        MeiOutput meioutput( &m_doc, filename.c_str());
        if (!meioutput.ExportFile()) {
            LogError( "Unknown error" );
            return false;
        }
        return true;
    }
    
    bool MergeToolkit::ParseOptions( const std::string &json_options ) {
#ifdef USE_EMSCRIPTEN
        
        jsonxx::Object json;
        
        // Read JSON options
        if (!json.parse(json_options)) {
            LogError( "Can not parse JSON string." );
            return false;
        }
        
        if (json.has<jsonxx::String>("inputFormat"))
            SetFormat( json.get<jsonxx::String>("inputFormat") );
        
        if (json.has<jsonxx::Number>("scale"))
            SetScale( json.get<jsonxx::Number>("scale") );
        
        if (json.has<jsonxx::Number>("border"))
            SetBorder( json.get<jsonxx::Number>("border") );
        
        if (json.has<jsonxx::String>("font"))
            SetFont(json.get<jsonxx::String>("font"));
        
        if (json.has<jsonxx::Number>("pageWidth"))
            SetPageWidth( json.get<jsonxx::Number>("pageWidth") );
        
        if (json.has<jsonxx::Number>("pageHeight"))
            SetPageHeight( json.get<jsonxx::Number>("pageHeight") );
        
        if (json.has<jsonxx::Number>("spacingStaff"))
            SetSpacingStaff( json.get<jsonxx::Number>("spacingStaff") );
        
        if (json.has<jsonxx::Number>("spacingSystem"))
            SetSpacingSystem( json.get<jsonxx::Number>("spacingSystem") );
        
        if (json.has<jsonxx::String>("rdgXPathQuery"))
            SetRdgXPathQuery( json.get<jsonxx::String>("rdgXPathQuery") );
        
        // Parse the various flags
        // Note: it seems that there is a bug with jsonxx and emscripten
        // Boolean value false do not parse properly. We have to use Number instead
        
        if (json.has<jsonxx::Number>("noLayout"))
            SetNoLayout(json.get<jsonxx::Number>("noLayout"));
        
        if (json.has<jsonxx::Number>("ignoreLayout"))
            SetIgnoreLayout(json.get<jsonxx::Number>("ignoreLayout"));
        
        if (json.has<jsonxx::Number>("adjustPageHeight"))
            SetAdjustPageHeight(json.get<jsonxx::Number>("adjustPageHeight"));
        
        if (json.has<jsonxx::Number>("noJustification"))
            SetNoJustification(json.get<jsonxx::Number>("noJustification"));
        
        if (json.has<jsonxx::Number>("showBoundingBoxes"))
            SetShowBoundingBoxes(json.get<jsonxx::Number>("showBoundingBoxes"));
        
        return true;
        
#else
        // The non js version of the app should not use this function.
        return false;
#endif
    }
    
    
    std::string MergeToolkit::GetElementAttr( const std::string &xmlId )
    {
#ifdef USE_EMSCRIPTEN
        jsonxx::Object o;
        
        if ( !m_doc.GetDrawingPage() ) return o.json();
        Object *element = m_doc.GetDrawingPage()->FindChildByUuid(xmlId);
        if (!element) {
            LogMessage("Element with id '%s' could not be found", xmlId.c_str() );
            return o.json();
        }
        
        // Fill the attribute array (pair of string) by looking by attributes for all available MEI modules
        ArrayOfStrAttr attributes;
        element->GetAttributes(&attributes);
        
        // Fill the JSON object
        ArrayOfStrAttr::iterator iter;
        for (iter = attributes.begin(); iter != attributes.end(); iter++) {
            o << (*iter).first << (*iter).second;
            //LogMessage("Element %s - %s", (*iter).first.c_str(), (*iter).second.c_str() );
        }
        return o.json();
        
#else
        // The non js version of the app should not use this function.
        return "";
#endif
    }
    
    bool MergeToolkit::Edit( const std::string &json_editorAction ) {
#ifdef USE_EMSCRIPTEN
        
        jsonxx::Object json;
        
        // Read JSON actions
        if (!json.parse(json_editorAction)) {
            LogError( "Can not parse JSON string." );
            return false;
        }
        
        if (json.has<jsonxx::String>("action") && json.has<jsonxx::Object>("param")) {
            if ( json.get<jsonxx::String>("action") == "drag" ) {
                std::string elementId;
                int x, y;
                if (this->ParseDragAction( json.get<jsonxx::Object>("param"), &elementId, &x, &y )) {
                    return this->Drag( elementId, x, y );
                }
            }
            else if ( json.get<jsonxx::String>("action") == "insert" ) {
                LogMessage("insert...");
                std::string elementType, startid, endid;
                if (this->ParseInsertAction( json.get<jsonxx::Object>("param"), &elementType, &startid, &endid )) {
                    return this->Insert( elementType, startid, endid );
                }
                else {
                    LogMessage("Insert!!!! %s %s %s", elementType.c_str(), startid.c_str(), endid.c_str() );
                }
            }
            else if ( json.get<jsonxx::String>("action") == "set" ) {
                std::string elementId, attrType, attrValue;
                if (this->ParseSetAction( json.get<jsonxx::Object>("param"), &elementId, &attrType, &attrValue )) {
                    return this->Set( elementId, attrType, attrValue );
                }
            }
        }
        LogError( "Does not understand action." );
        return false;
        
#else
        // The non js version of the app should not use this function.
        return false;
#endif
    }
    
    
    std::string MergeToolkit::GetLogString() {
#ifdef USE_EMSCRIPTEN
        std::string str;
        std::vector<std::string>::iterator iter;
        for (iter = logBuffer.begin(); iter != logBuffer.end(); iter++) {
            str += (*iter);
        }
        return str;
#else
        // The non js version of the app should not use this function.
        return "";
#endif
    }
    
    std::string MergeToolkit::GetVersion() {
        return vrv::GetVersion();
    }
    
    
    void  MergeToolkit::ResetLogBuffer() {
#ifdef USE_EMSCRIPTEN
        vrv::logBuffer.clear();
#endif
    }
    
    std::string MergeToolkit::RenderToSvg( int pageNo, bool xml_declaration )
    {
        // Page number is one-based - correction to 0-based first
        pageNo--;
        
        // Get the current system for the SVG clipping size
        m_view.SetPage( pageNo );
        
        // Adjusting page width and height according to the options
        int width = m_pageWidth;
        if ( m_noLayout ) {
            width = m_doc.GetAdjustedDrawingPageWidth();
        }
        
        int height = m_pageHeight;
        if ( m_adjustPageHeight || m_noLayout ) {
            height = m_doc.GetAdjustedDrawingPageHeight();
        }
        
        // Create the SVG object, h & w come from the system
        // We will need to set the size of the page after having drawn it depending on the options
        SvgDeviceContext svg( width, height );
        
        // set scale and border from user options
        svg.SetUserScale((double)m_scale / 100, (double)m_scale / 100);
        
        // debug BB?
        svg.SetDrawBoundingBoxes(m_showBoundingBoxes);
        
        // render the page
        m_view.DrawCurrentPage( &svg, false );
        
        std::string out_str = svg.GetStringSVG( xml_declaration );
        return out_str;
    }
    
    void MergeToolkit::RedoLayout()
    {
        m_doc.SetPageHeight( this->GetPageHeight() );
        m_doc.SetPageWidth( this->GetPageWidth() );
        m_doc.SetPageRightMar( this->GetBorder() );
        m_doc.SetPageLeftMar( this->GetBorder() );
        m_doc.SetPageTopMar( this->GetBorder() );
        m_doc.SetSpacingStaff( this->GetSpacingStaff() );
        m_doc.SetSpacingSystem( this->GetSpacingSystem() );
        
        m_doc.UnCastOff();
        m_doc.CastOff();
    }
    
    bool MergeToolkit::RenderToSvgFile( const std::string &filename, int pageNo )
    {
        std::string output = RenderToSvg( pageNo, true );
        
        std::ofstream outfile;
        outfile.open ( filename.c_str() );
        
        if ( !outfile.is_open() ) {
            // add message?
            return false;
        }
        
        outfile << output;
        outfile.close();
        return true;
    }
    
    
    int MergeToolkit::GetPageCount() {
        return m_doc.GetPageCount();
    }
    
    int MergeToolkit::GetPageWithElement( const std::string &xmlId ) {
        Object *element = m_doc.FindChildByUuid(xmlId);
        if (!element) {
            return 0;
        }
        Page *page = dynamic_cast<Page*>( element->GetFirstParent( &typeid(Page) ) );
        if (!page) {
            return 0;
        }
        return page->GetIdx() + 1;
    }
    
    void MergeToolkit::SetCString( const std::string &data )
    {
        if (m_cString) {
            free(m_cString);
            m_cString = NULL;
        }
        
        m_cString = (char *)malloc(strlen(data.c_str()) + 1);
        
        // something went wrong
        if (!m_cString) {
            return;
        }
        strcpy(m_cString, data.c_str());
    }
    
    const char *MergeToolkit::GetCString( )
    {
        if (m_cString) {
            return m_cString;
        }
        else {
            return "[unspecified]";
        }
    }
    
    bool MergeToolkit::Drag( std::string elementId, int x, int y )
    {
        if ( !m_doc.GetDrawingPage() ) return false;
        Object *element = m_doc.GetDrawingPage()->FindChildByUuid(elementId);
        if ( dynamic_cast<Note*>(element) ) {
            Note *note = dynamic_cast<Note*>(element);
            Layer *layer = dynamic_cast<Layer*>(note->GetFirstParent(&typeid(Layer)));
            if ( !layer ) return false;
            int oct;
            data_PITCHNAME pname = (data_PITCHNAME)m_view.CalculatePitchCode( layer, m_view.ToLogicalY(y), note->GetDrawingX(), &oct  );
            note->SetPname(pname);
            note->SetOct(oct);
            return true;
        }
        return false;
    }
    
    bool MergeToolkit::Insert( std::string elementType, std::string startid, std::string endid )
    {
        LogMessage("Insert!");
        if ( !m_doc.GetDrawingPage() ) return false;
        Object *start = m_doc.GetDrawingPage()->FindChildByUuid(startid);
        Object *end = m_doc.GetDrawingPage()->FindChildByUuid(endid);
        // Check that start and end element exists
        if ( !start || !end ) {
            LogMessage("Elements start and end ids '%s' and '%s' could not be found", startid.c_str(), endid.c_str() );
            return false;
        }
        // Check that it is a LayerElement
        if ( !dynamic_cast<LayerElement*>(start) ) {
            LogMessage("Element '%s' is not supported as start element", start->GetClassName().c_str() );
            return false;
        }
        if ( !dynamic_cast<LayerElement*>(end) ) {
            LogMessage("Element '%s' is not supported as end element", start->GetClassName().c_str() );
            return false;
        }
        
        Measure *measure = dynamic_cast<Measure*>(start->GetFirstParent(&typeid(Measure)));
        assert( measure );
        if (elementType == "slur" ) {
            Slur *slur = new Slur();
            slur->SetStartid( startid );
            slur->SetEndid( endid );
            measure->AddMeasureElement(slur);
            m_doc.PrepareDrawing();
            return true;
        }
        return false;
    }
    
    bool MergeToolkit::Set( std::string elementId, std::string attrType, std::string attrValue )
    {
        if ( !m_doc.GetDrawingPage() ) return false;
        Object *element = m_doc.GetDrawingPage()->FindChildByUuid(elementId);
        if ( Att::SetCmn(element, attrType, attrValue )) return true;
        if ( Att::SetCritapp(element, attrType, attrValue )) return true;
        if ( Att::SetMensural(element, attrType, attrValue )) return true;
        if ( Att::SetPagebased(element, attrType, attrValue )) return true;
        if ( Att::SetShared(element, attrType, attrValue )) return true;
        return false;
    }
    
#ifdef USE_EMSCRIPTEN
    bool MergeToolkit::ParseDragAction( jsonxx::Object param, std::string *elementId, int *x, int *y )
    {
        if (!param.has<jsonxx::String>("elementId")) return false;
        (*elementId) = param.get<jsonxx::String>("elementId");
        if (!param.has<jsonxx::Number>("x")) return false;
        (*x) = param.get<jsonxx::Number>("x");
        if (!param.has<jsonxx::Number>("y")) return false;
        (*y) = param.get<jsonxx::Number>("y");
        return true;
    }
    
    bool MergeToolkit::ParseInsertAction( jsonxx::Object param, std::string *elementType, std::string *startid, std::string *endid )
    {
        if (!param.has<jsonxx::String>("elementType")) return false;
        (*elementType) = param.get<jsonxx::String>("elementType");
        if (!param.has<jsonxx::String>("startid")) return false;
        (*startid) = param.get<jsonxx::String>("startid");
        if (!param.has<jsonxx::String>("endid")) return false;
        (*endid) = param.get<jsonxx::String>("endid");
        return true;
    }
    
    bool MergeToolkit::ParseSetAction( jsonxx::Object param, std::string *elementId, std::string *attrType, std::string *attrValue )
    {
        if (!param.has<jsonxx::String>("elementId")) return false;
        (*elementId) = param.get<jsonxx::String>("elementId");
        if (!param.has<jsonxx::String>("attrType")) return false;
        (*attrType) = param.get<jsonxx::String>("attrType");
        if (!param.has<jsonxx::String>("attrValue")) return false;
        (*attrValue) = param.get<jsonxx::String>("attrValue");
        return true;
    }
#endif
    
    
} //namespace vrv
