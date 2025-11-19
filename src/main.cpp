#include "MainFrame.h"
#include <wx/wx.h>

class XmlViewerApp : public wxApp
{
public:
    virtual bool OnInit() override
    {
        if (!wxApp::OnInit())
            return false;

        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(XmlViewerApp);

