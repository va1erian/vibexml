#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/menu.h>
#include "XmlTreeCtrl.h"
#include "XmlEditorCtrl.h"
#include "RecentFiles.h"

class MainFrame : public wxFrame
{
public:
    MainFrame();
    ~MainFrame();

private:
    void OnOpenFile(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnFind(wxCommandEvent& event);
    void OnFindNext(wxCommandEvent& event);
    void OnToggleTreePanel(wxCommandEvent& event);
    void OnRecentFile(wxCommandEvent& event);
    void OnTreeItemSelected(wxTreeEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnSettings(wxCommandEvent& event);

    void LoadXmlFile(const wxString& filePath);
    void UpdateRecentFilesMenu();
    void CreateMenuBar();
    void CreateStatusBar();

    wxSplitterWindow* m_splitter;
    XmlTreeCtrl* m_treeCtrl;
    XmlEditorCtrl* m_editorCtrl;
    RecentFiles* m_recentFiles;
    wxMenu* m_recentFilesMenu;
    wxString m_currentFilePath;

    enum
    {
        ID_Open = wxID_OPEN,
        ID_Exit = wxID_EXIT,
        ID_Find = wxID_FIND,
        ID_FindNext = 1001,
        ID_ToggleTree = 1000,
        ID_RecentFileBase = 2000,
        ID_Settings = 3000
    };
};

#endif // MAINFRAME_H
