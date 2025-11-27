#include "MainFrame.h"
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

// When XRC is embedded, wxrc generates this initialization function
#ifdef XRC_EMBEDDED
extern void InitXrcResources();
#endif

class XmlViewerApp : public wxApp
{
public:
    virtual bool OnInit() override
    {
        if (!wxApp::OnInit())
            return false;

        // Initialize XRC resource handler
        wxXmlResource::Get()->InitAllHandlers();
        
        // Load XRC resources
        if (!LoadXrcResources())
        {
            wxMessageBox("Failed to load UI resources. The application may not function correctly.",
                         "Warning", wxOK | wxICON_WARNING);
        }

        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
    
private:
    bool LoadXrcResources()
    {
#ifdef XRC_EMBEDDED
        // XRC resources are compiled into the executable
        // Call the generated initialization function
        InitXrcResources();
        return true;
#else
        // Load XRC from external file
        return LoadXrcFromFile();
#endif
    }
    
    bool LoadXrcFromFile()
    {
        // Try multiple locations for the XRC file
        wxArrayString searchPaths;
        
        // 1. Same directory as executable
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeDir(exePath);
        searchPaths.Add(exeDir.GetPath() + wxFILE_SEP_PATH + "dialogs.xrc");
        searchPaths.Add(exeDir.GetPath() + wxFILE_SEP_PATH + "resources" + wxFILE_SEP_PATH + "dialogs.xrc");
        
#ifdef __WXMAC__
        // macOS: Check inside the app bundle Resources folder
        wxString resourcesPath = wxStandardPaths::Get().GetResourcesDir();
        searchPaths.Add(resourcesPath + wxFILE_SEP_PATH + "dialogs.xrc");
#endif
        
        // 2. Parent directory (for development builds)
        wxFileName parentDir = exeDir;
        parentDir.RemoveLastDir();
        searchPaths.Add(parentDir.GetPath() + wxFILE_SEP_PATH + "resources" + wxFILE_SEP_PATH + "dialogs.xrc");
        
        // 3. Two directories up (common for VS builds: project/Release/XmlViewer.exe)
        wxFileName grandParentDir = parentDir;
        grandParentDir.RemoveLastDir();
        searchPaths.Add(grandParentDir.GetPath() + wxFILE_SEP_PATH + "resources" + wxFILE_SEP_PATH + "dialogs.xrc");
        
        // 4. Current working directory
        searchPaths.Add("resources" + wxString(wxFILE_SEP_PATH) + "dialogs.xrc");
        searchPaths.Add("dialogs.xrc");
        
        // Try each path
        for (const wxString& path : searchPaths)
        {
            if (wxFileExists(path))
            {
                return wxXmlResource::Get()->Load(path);
            }
        }
        
        // Log searched paths for debugging
        wxString searchedPaths;
        for (const wxString& path : searchPaths)
        {
            searchedPaths += "\n  - " + path;
        }
        wxLogDebug("XRC file not found. Searched paths:%s", searchedPaths);
        
        return false;
    }
};

wxIMPLEMENT_APP(XmlViewerApp);
