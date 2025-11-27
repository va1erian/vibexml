#include "SearchDialog.h"
#include "VirtualXmlTree.h"
#include <wx/xrc/xmlres.h>

SearchDialog::SearchDialog(wxWindow* parent, XmlEditorCtrl* editor, VirtualXmlTree* treeCtrl)
    : m_editor(editor),
      m_treeCtrl(treeCtrl),
      m_searchTextCtrl(nullptr),
      m_caseSensitiveCheck(nullptr),
      m_wholeWordCheck(nullptr),
      m_findButton(nullptr),
      m_findPrevButton(nullptr),
      m_statusLabel(nullptr),
      m_searchTimer(this)
{
    // Load the dialog from XRC
    wxXmlResource::Get()->LoadDialog(this, parent, "SearchDialog");
    
    // Get control references
    BindControls();
    
    // Set focus to search text
    if (m_searchTextCtrl)
        m_searchTextCtrl->SetFocus();
    
    // Bind timer event
    Bind(wxEVT_TIMER, &SearchDialog::OnSearchTimer, this);
    
    // Bind window close event (X button)
    Bind(wxEVT_CLOSE_WINDOW, &SearchDialog::OnWindowClose, this);
}

SearchDialog::~SearchDialog()
{
    m_searchTimer.Stop();
}

void SearchDialog::BindControls()
{
    // Get control references from XRC
    m_searchTextCtrl = XRCCTRL(*this, "SearchTextCtrl", wxTextCtrl);
    m_caseSensitiveCheck = XRCCTRL(*this, "CaseSensitiveCheck", wxCheckBox);
    m_wholeWordCheck = XRCCTRL(*this, "WholeWordCheck", wxCheckBox);
    m_findButton = XRCCTRL(*this, "wxID_OK", wxButton);
    m_findPrevButton = XRCCTRL(*this, "FindPrevButton", wxButton);
    m_statusLabel = XRCCTRL(*this, "SearchStatusLabel", wxStaticText);
    
    // Bind events
    if (m_findButton)
        m_findButton->Bind(wxEVT_BUTTON, &SearchDialog::OnFind, this);
    
    if (m_findPrevButton)
        m_findPrevButton->Bind(wxEVT_BUTTON, &SearchDialog::OnFindPrev, this);
    
    // Bind close button
    wxButton* closeButton = XRCCTRL(*this, "wxID_CANCEL", wxButton);
    if (closeButton)
        closeButton->Bind(wxEVT_BUTTON, &SearchDialog::OnClose, this);
    
    if (m_searchTextCtrl)
        m_searchTextCtrl->Bind(wxEVT_TEXT, &SearchDialog::OnTextChanged, this);
    
    // Bind checkbox events to update highlighting when options change
    if (m_caseSensitiveCheck)
        m_caseSensitiveCheck->Bind(wxEVT_CHECKBOX, &SearchDialog::OnCheckboxChanged, this);
    if (m_wholeWordCheck)
        m_wholeWordCheck->Bind(wxEVT_CHECKBOX, &SearchDialog::OnCheckboxChanged, this);
    
    // Initially disable buttons
    if (m_findButton)
        m_findButton->Enable(false);
    if (m_findPrevButton)
        m_findPrevButton->Enable(false);
}

void SearchDialog::PerformSearch()
{
    if (!m_editor || !m_searchTextCtrl || !m_statusLabel)
        return;
    
    wxString searchText = m_searchTextCtrl->GetValue();
    if (searchText.IsEmpty())
    {
        m_statusLabel->SetLabel("");
        m_editor->ClearSearch();
        return;
    }
    
    // Show busy cursor during search
    wxBusyCursor wait;
    
    bool caseSensitive = m_caseSensitiveCheck ? m_caseSensitiveCheck->GetValue() : false;
    bool wholeWord = m_wholeWordCheck ? m_wholeWordCheck->GetValue() : false;
    
    m_editor->SetSearchText(searchText, caseSensitive, wholeWord);
    
    int matchCount = m_editor->GetMatchCount();
    if (matchCount == 0)
    {
        m_statusLabel->SetLabel("No matches found");
        m_statusLabel->SetForegroundColour(wxColour(200, 50, 50));  // Red
    }
    else if (matchCount < 0)
    {
        // Negative means "at least this many" (search was limited)
        m_statusLabel->SetLabel(wxString::Format("%d+ matches found", -matchCount));
        m_statusLabel->SetForegroundColour(wxColour(100, 100, 100));  // Gray
    }
    else if (matchCount == 1)
    {
        m_statusLabel->SetLabel("1 match found");
        m_statusLabel->SetForegroundColour(wxColour(100, 100, 100));  // Gray
    }
    else
    {
        m_statusLabel->SetLabel(wxString::Format("%d matches found", matchCount));
        m_statusLabel->SetForegroundColour(wxColour(100, 100, 100));  // Gray
    }
    
    m_statusLabel->Refresh();
}

void SearchDialog::UpdateStatus(const SearchResult& result)
{
    if (!m_statusLabel)
        return;
    
    if (!result.found)
    {
        m_statusLabel->SetLabel("No matches found");
        m_statusLabel->SetForegroundColour(wxColour(200, 50, 50));  // Red
    }
    else if (result.wrapped)
    {
        // Show wrap-around notification
        m_statusLabel->SetLabel(wxString::Format("Match %d of %d (wrapped around)", 
                                                  result.matchIndex, result.totalMatches));
        m_statusLabel->SetForegroundColour(wxColour(180, 120, 0));  // Orange/amber
    }
    else
    {
        m_statusLabel->SetLabel(wxString::Format("Match %d of %d", 
                                                  result.matchIndex, result.totalMatches));
        m_statusLabel->SetForegroundColour(wxColour(100, 100, 100));  // Gray
    }
    
    m_statusLabel->Refresh();
}

void SearchDialog::ExpandTreeToLine(int lineNumber)
{
    if (!m_treeCtrl || lineNumber <= 0)
        return;
    
    // Try to expand to the line in the tree
    m_treeCtrl->ExpandToLine(lineNumber);
}

void SearchDialog::OnFind(wxCommandEvent& event)
{
    if (!m_editor || !m_searchTextCtrl)
        return;

    wxString searchText = m_searchTextCtrl->GetValue();
    if (searchText.IsEmpty())
        return;
    
    // Show busy cursor during search
    wxBusyCursor wait;

    bool caseSensitive = m_caseSensitiveCheck ? m_caseSensitiveCheck->GetValue() : false;
    bool wholeWord = m_wholeWordCheck ? m_wholeWordCheck->GetValue() : false;

    m_editor->SetSearchText(searchText, caseSensitive, wholeWord);
    SearchResult result = m_editor->FindNext();
    UpdateStatus(result);
    
    // Expand tree to the found match
    if (result.found)
    {
        int currentLine = m_editor->LineFromPosition(m_editor->GetCurrentPos()) + 1;
        ExpandTreeToLine(currentLine);
    }
}

void SearchDialog::OnFindPrev(wxCommandEvent& event)
{
    if (!m_editor || !m_searchTextCtrl)
        return;

    wxString searchText = m_searchTextCtrl->GetValue();
    if (searchText.IsEmpty())
        return;
    
    // Show busy cursor during search
    wxBusyCursor wait;

    bool caseSensitive = m_caseSensitiveCheck ? m_caseSensitiveCheck->GetValue() : false;
    bool wholeWord = m_wholeWordCheck ? m_wholeWordCheck->GetValue() : false;

    m_editor->SetSearchText(searchText, caseSensitive, wholeWord);
    SearchResult result = m_editor->FindPrevious();
    UpdateStatus(result);
    
    // Expand tree to the found match
    if (result.found)
    {
        int currentLine = m_editor->LineFromPosition(m_editor->GetCurrentPos()) + 1;
        ExpandTreeToLine(currentLine);
    }
}

void SearchDialog::OnClose(wxCommandEvent& event)
{
    // Stop timer
    m_searchTimer.Stop();
    
    // Clear highlights when closing
    if (m_editor)
        m_editor->ClearSearch();
    
    // Hide instead of destroying (for modeless dialog)
    Hide();
}

void SearchDialog::OnWindowClose(wxCloseEvent& event)
{
    // Stop timer
    m_searchTimer.Stop();
    
    // Clear highlights when closing
    if (m_editor)
        m_editor->ClearSearch();
    
    // Hide instead of destroying (for modeless dialog)
    Hide();
}

void SearchDialog::OnTextChanged(wxCommandEvent& event)
{
    wxString searchText = m_searchTextCtrl ? m_searchTextCtrl->GetValue() : "";
    bool hasText = !searchText.IsEmpty();
    
    if (m_findButton)
        m_findButton->Enable(hasText);
    if (m_findPrevButton)
        m_findPrevButton->Enable(hasText);
    
    // Clear search if text is empty
    if (searchText.IsEmpty())
    {
        m_searchTimer.Stop();
        if (m_statusLabel)
        {
            m_statusLabel->SetLabel("");
            m_statusLabel->Refresh();
        }
        if (m_editor)
            m_editor->ClearSearch();
        return;
    }
    
    // Only auto-search if we have enough characters (debounce short inputs)
    if (searchText.Length() >= MIN_SEARCH_LENGTH)
    {
        // Restart the timer (debounce)
        m_searchTimer.Stop();
        m_searchTimer.StartOnce(SEARCH_DELAY_MS);
        
        // Show "Searching..." status while waiting
        if (m_statusLabel)
        {
            m_statusLabel->SetLabel("Searching...");
            m_statusLabel->SetForegroundColour(wxColour(100, 100, 100));
            m_statusLabel->Refresh();
        }
    }
    else
    {
        // Too few characters - show hint
        m_searchTimer.Stop();
        if (m_statusLabel)
        {
            m_statusLabel->SetLabel("Type more to search...");
            m_statusLabel->SetForegroundColour(wxColour(150, 150, 150));
            m_statusLabel->Refresh();
        }
    }
}

void SearchDialog::OnCheckboxChanged(wxCommandEvent& event)
{
    // Re-apply search when options change (with delay)
    wxString searchText = m_searchTextCtrl ? m_searchTextCtrl->GetValue() : "";
    if (searchText.Length() >= MIN_SEARCH_LENGTH)
    {
        m_searchTimer.Stop();
        m_searchTimer.StartOnce(SEARCH_DELAY_MS);
    }
}

void SearchDialog::OnSearchTimer(wxTimerEvent& event)
{
    // Timer fired - perform the actual search
    PerformSearch();
}
