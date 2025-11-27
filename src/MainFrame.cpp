#include "MainFrame.h"
#include "SearchDialog.h"
#include "SettingsDialog.h"
#include "EditorSettings.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "XML Viewer", wxDefaultPosition, wxSize(1200, 800)),
      m_recentFilesMenu(nullptr),
      m_currentFilePath(wxEmptyString)
{
    CreateMenuBar();
    CreateStatusBar();

    // Create splitter window
    m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_LIVE_UPDATE);

    // Create tree control (left panel)
    m_treeCtrl = new XmlTreeCtrl(m_splitter, wxID_ANY);
    m_treeCtrl->SetMinSize(wxSize(250, -1));

    // Create editor control (right panel)
    m_editorCtrl = new XmlEditorCtrl(m_splitter, wxID_ANY);

    // Connect tree selection event
    m_treeCtrl->Bind(wxEVT_TREE_SEL_CHANGED, &MainFrame::OnTreeItemSelected, this);

    // Split the window
    m_splitter->SplitVertically(m_treeCtrl, m_editorCtrl, 300);
    m_splitter->SetMinimumPaneSize(200);

    // Initialize recent files manager
    m_recentFiles = new RecentFiles();
    UpdateRecentFilesMenu();

    // Set status bar text
    GetStatusBar()->SetStatusText("Ready");
}

MainFrame::~MainFrame()
{
    delete m_recentFiles;
}

void MainFrame::CreateMenuBar()
{
    wxMenuBar* menuBar = new wxMenuBar();

    // File menu
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(ID_Open, "&Open...\tCtrl+O", "Open an XML file");
    m_recentFilesMenu = new wxMenu();
    fileMenu->AppendSubMenu(m_recentFilesMenu, "Recent &Files");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_Exit, "E&xit\tAlt+F4", "Exit the application");

    // Edit menu
    wxMenu* editMenu = new wxMenu();
    editMenu->Append(ID_Find, "&Find...\tCtrl+F", "Find text in the document");
    editMenu->Append(ID_FindNext, "Find &Next\tF3", "Find next occurrence");
    editMenu->Append(ID_FindPrevious, "Find &Previous\tShift+F3", "Find previous occurrence");
    editMenu->AppendSeparator();
    editMenu->Append(ID_FormatXml, "Format &XML\tCtrl+Shift+F", "Auto-indent and beautify XML");
    editMenu->AppendSeparator();
    editMenu->Append(ID_Settings, "&Settings...\tCtrl+,", "Configure editor appearance");

    // View menu
    wxMenu* viewMenu = new wxMenu();
    viewMenu->AppendCheckItem(ID_ToggleTree, "&Tree Panel\tCtrl+T", "Toggle tree panel visibility");
    viewMenu->Check(ID_ToggleTree, true);

    menuBar->Append(fileMenu, "&File");
    menuBar->Append(editMenu, "&Edit");
    menuBar->Append(viewMenu, "&View");

    SetMenuBar(menuBar);

    // Bind events
    Bind(wxEVT_MENU, &MainFrame::OnOpenFile, this, ID_Open);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, ID_Exit);
    Bind(wxEVT_MENU, &MainFrame::OnFind, this, ID_Find);
    Bind(wxEVT_MENU, &MainFrame::OnFindNext, this, ID_FindNext);
    Bind(wxEVT_MENU, &MainFrame::OnFindPrevious, this, ID_FindPrevious);
    Bind(wxEVT_MENU, &MainFrame::OnFormatXml, this, ID_FormatXml);
    Bind(wxEVT_MENU, &MainFrame::OnToggleTreePanel, this, ID_ToggleTree);
    Bind(wxEVT_MENU, &MainFrame::OnSettings, this, ID_Settings);
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
}

void MainFrame::CreateStatusBar()
{
    wxFrame::CreateStatusBar(2);  // Two fields: main status and search status
    
    int widths[] = { -1, 200 };  // Main field stretches, search status fixed width
    GetStatusBar()->SetStatusWidths(2, widths);
    
    SetStatusText("Ready", 0);
}

void MainFrame::UpdateSearchStatus(const SearchResult& result)
{
    wxStatusBar* statusBar = GetStatusBar();
    if (!statusBar)
        return;
    
    if (!result.found)
    {
        statusBar->SetStatusText("No matches", 1);
    }
    else if (result.wrapped)
    {
        statusBar->SetStatusText(wxString::Format("Match %d/%d (wrapped)", 
                                                   result.matchIndex, result.totalMatches), 1);
    }
    else
    {
        statusBar->SetStatusText(wxString::Format("Match %d/%d", 
                                                   result.matchIndex, result.totalMatches), 1);
    }
}

void MainFrame::OnOpenFile(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, "Open XML file", "", "",
                                "XML files (*.xml)|*.xml|All files (*.*)|*.*",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString filePath = openFileDialog.GetPath();
    LoadXmlFile(filePath);
}

void MainFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void MainFrame::OnFind(wxCommandEvent& event)
{
    SearchDialog dlg(this, m_editorCtrl);
    dlg.ShowModal();
    
    // Update status bar with current match info after dialog closes
    if (m_editorCtrl && m_editorCtrl->GetMatchCount() > 0)
    {
        SearchResult result;
        result.found = true;
        result.wrapped = false;
        result.matchIndex = m_editorCtrl->GetCurrentMatchIndex();
        result.totalMatches = m_editorCtrl->GetMatchCount();
        UpdateSearchStatus(result);
    }
    else
    {
        GetStatusBar()->SetStatusText("", 1);
    }
}

void MainFrame::OnFindNext(wxCommandEvent& event)
{
    if (m_editorCtrl)
    {
        SearchResult result = m_editorCtrl->FindNext();
        UpdateSearchStatus(result);
    }
}

void MainFrame::OnFindPrevious(wxCommandEvent& event)
{
    if (m_editorCtrl)
    {
        SearchResult result = m_editorCtrl->FindPrevious();
        UpdateSearchStatus(result);
    }
}

void MainFrame::OnFormatXml(wxCommandEvent& event)
{
    if (m_editorCtrl)
    {
        if (m_editorCtrl->FormatXml())
        {
            // Reload tree with the newly formatted content
            if (m_treeCtrl)
            {
                wxString formattedContent = m_editorCtrl->GetText();
                if (m_treeCtrl->LoadXmlFromString(formattedContent))
                {
                    SetStatusText("XML formatted successfully", 0);
                }
                else
                {
                    SetStatusText("XML formatted - tree reload failed", 0);
                }
            }
            else
            {
                SetStatusText("XML formatted successfully", 0);
            }
        }
    }
}

void MainFrame::OnToggleTreePanel(wxCommandEvent& event)
{
    bool isChecked = GetMenuBar()->IsChecked(ID_ToggleTree);
    m_treeCtrl->Show(isChecked);
    m_splitter->Layout();
}

void MainFrame::OnSettings(wxCommandEvent& event)
{
    SettingsDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK)
    {
        // Apply updated settings to the editor
        if (m_editorCtrl)
        {
            m_editorCtrl->ApplySettings();
        }
    }
}

void MainFrame::OnRecentFile(wxCommandEvent& event)
{
    int index = event.GetId() - ID_RecentFileBase;
    wxString filePath = m_recentFiles->GetFile(index);
    if (!filePath.IsEmpty())
    {
        LoadXmlFile(filePath);
    }
}

void MainFrame::OnTreeItemSelected(wxTreeEvent& event)
{
    wxTreeItemId itemId = event.GetItem();
    int lineNumber = m_treeCtrl->GetLineNumber(itemId);
    if (lineNumber > 0 && m_editorCtrl)
    {
        // Navigate to line and highlight it
        m_editorCtrl->GotoLine(lineNumber, true);
        
        // Update status bar with line info
        SetStatusText(wxString::Format("Line %d", lineNumber), 1);
    }
}

void MainFrame::OnClose(wxCloseEvent& event)
{
    // Clear tree to speed up shutdown cleanup
    if (m_treeCtrl)
    {
        m_treeCtrl->DeleteAllItems();
    }
    
    // Save recent files (destructor will also try to save, but this ensures it happens)
    if (m_recentFiles)
    {
        m_recentFiles->Save();
    }
    
    event.Skip();
}

void MainFrame::LoadXmlFile(const wxString& filePath)
{
    if (!wxFileExists(filePath))
    {
        wxMessageBox("File does not exist: " + filePath, "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Load XML content into editor
    if (!m_editorCtrl->LoadFile(filePath))
    {
        wxMessageBox("Failed to load file: " + filePath, "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Parse XML and build tree
    if (!m_treeCtrl->LoadXmlFile(filePath))
    {
        wxMessageBox("Failed to parse XML file: " + filePath, "Error", wxOK | wxICON_ERROR);
        return;
    }

    m_currentFilePath = filePath;
    m_recentFiles->AddFile(filePath);
    UpdateRecentFilesMenu();

    // Update status bar
    wxFileName fileName(filePath);
    SetStatusText("Loaded: " + fileName.GetFullName(), 0);
    SetStatusText("", 1);  // Clear search status
}

void MainFrame::UpdateRecentFilesMenu()
{
    if (!m_recentFilesMenu)
        return;

    // Remove all existing items from the menu
    while (m_recentFilesMenu->GetMenuItemCount() > 0)
    {
        wxMenuItem* item = m_recentFilesMenu->FindItemByPosition(0);
        if (item)
        {
            m_recentFilesMenu->Destroy(item);
        }
        else
        {
            break;
        }
    }

    const auto& files = m_recentFiles->GetFiles();
    if (files.empty())
    {
        m_recentFilesMenu->Append(wxID_ANY, "No recent files")->Enable(false);
        return;
    }

    for (size_t i = 0; i < files.size(); ++i)
    {
        wxFileName fileName(files[i]);
        wxString label = wxString::Format("&%zu %s", i + 1, fileName.GetFullName());
        int id = ID_RecentFileBase + static_cast<int>(i);
        m_recentFilesMenu->Append(id, label);
        Bind(wxEVT_MENU, &MainFrame::OnRecentFile, this, id);
    }
}
