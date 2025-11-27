#include "SearchDialog.h"
#include <wx/xrc/xmlres.h>

SearchDialog::SearchDialog(wxWindow* parent, XmlEditorCtrl* editor)
    : m_editor(editor),
      m_searchTextCtrl(nullptr),
      m_caseSensitiveCheck(nullptr),
      m_wholeWordCheck(nullptr),
      m_findButton(nullptr),
      m_findPrevButton(nullptr),
      m_statusLabel(nullptr)
{
    // Load the dialog from XRC
    wxXmlResource::Get()->LoadDialog(this, parent, "SearchDialog");
    
    // Get control references
    BindControls();
    
    // Set focus to search text
    if (m_searchTextCtrl)
        m_searchTextCtrl->SetFocus();
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

void SearchDialog::UpdateMatchCount()
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
    
    bool caseSensitive = m_caseSensitiveCheck ? m_caseSensitiveCheck->GetValue() : false;
    bool wholeWord = m_wholeWordCheck ? m_wholeWordCheck->GetValue() : false;
    
    m_editor->SetSearchText(searchText, caseSensitive, wholeWord);
    
    int matchCount = m_editor->GetMatchCount();
    if (matchCount == 0)
    {
        m_statusLabel->SetLabel("No matches found");
        m_statusLabel->SetForegroundColour(wxColour(200, 50, 50));  // Red
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

void SearchDialog::OnFind(wxCommandEvent& event)
{
    if (!m_editor || !m_searchTextCtrl)
        return;

    wxString searchText = m_searchTextCtrl->GetValue();
    if (searchText.IsEmpty())
        return;

    bool caseSensitive = m_caseSensitiveCheck ? m_caseSensitiveCheck->GetValue() : false;
    bool wholeWord = m_wholeWordCheck ? m_wholeWordCheck->GetValue() : false;

    m_editor->SetSearchText(searchText, caseSensitive, wholeWord);
    SearchResult result = m_editor->FindNext();
    UpdateStatus(result);
}

void SearchDialog::OnFindPrev(wxCommandEvent& event)
{
    if (!m_editor || !m_searchTextCtrl)
        return;

    wxString searchText = m_searchTextCtrl->GetValue();
    if (searchText.IsEmpty())
        return;

    bool caseSensitive = m_caseSensitiveCheck ? m_caseSensitiveCheck->GetValue() : false;
    bool wholeWord = m_wholeWordCheck ? m_wholeWordCheck->GetValue() : false;

    m_editor->SetSearchText(searchText, caseSensitive, wholeWord);
    SearchResult result = m_editor->FindPrevious();
    UpdateStatus(result);
}

void SearchDialog::OnClose(wxCommandEvent& event)
{
    // Clear highlights when closing
    if (m_editor)
        m_editor->ClearSearch();
    
    EndModal(wxID_CANCEL);
}

void SearchDialog::OnTextChanged(wxCommandEvent& event)
{
    bool hasText = m_searchTextCtrl && !m_searchTextCtrl->GetValue().IsEmpty();
    
    if (m_findButton)
        m_findButton->Enable(hasText);
    if (m_findPrevButton)
        m_findPrevButton->Enable(hasText);
    
    // Update match count and highlighting as user types
    UpdateMatchCount();
}

void SearchDialog::OnCheckboxChanged(wxCommandEvent& event)
{
    // Re-apply search when options change
    UpdateMatchCount();
}
