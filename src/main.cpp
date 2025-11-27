#include "MainFrame.h"
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

// XRC resources are compiled into the executable by wxrc
extern void InitXrcResources();

class XmlViewerApp : public wxApp
{
public:
    virtual bool OnInit() override
    {
        if (!wxApp::OnInit())
            return false;

        // Initialize XRC handlers and load embedded resources
        wxXmlResource::Get()->InitAllHandlers();
        InitXrcResources();

        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(XmlViewerApp);
