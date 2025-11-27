#include "SearchDialog.h"
#include <wx/xrc/xmlres.h>

SearchDialog::SearchDialog(wxWindow* parent, XmlEditorCtrl* editor)
    : m_editor(editor),
      m_searchTextCtrl(nullptr),
      m_caseSensitiveCheck(nullptr),
      m_wholeWordCheck(nullptr),
      m_findButton(nullptr),
      m_findNextButton(nullptr)
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
    m_findNextButton = XRCCTRL(*this, "FindNextButton", wxButton);
    
    // Bind events
    if (m_findButton)
        m_findButton->Bind(wxEVT_BUTTON, &SearchDialog::OnFind, this);
    
    if (m_findNextButton)
        m_findNextButton->Bind(wxEVT_BUTTON, &SearchDialog::OnFindNext, this);
    
    // Bind cancel button
    wxButton* cancelButton = XRCCTRL(*this, "wxID_CANCEL", wxButton);
    if (cancelButton)
        cancelButton->Bind(wxEVT_BUTTON, &SearchDialog::OnCancel, this);
    
    if (m_searchTextCtrl)
        m_searchTextCtrl->Bind(wxEVT_TEXT, &SearchDialog::OnTextChanged, this);
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
    m_editor->FindNext();
}

void SearchDialog::OnFindNext(wxCommandEvent& event)
{
    if (!m_editor || !m_searchTextCtrl)
        return;

    wxString searchText = m_searchTextCtrl->GetValue();
    if (searchText.IsEmpty())
        return;

    bool caseSensitive = m_caseSensitiveCheck ? m_caseSensitiveCheck->GetValue() : false;
    bool wholeWord = m_wholeWordCheck ? m_wholeWordCheck->GetValue() : false;

    m_editor->SetSearchText(searchText, caseSensitive, wholeWord);
    m_editor->FindNext();
}

void SearchDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void SearchDialog::OnTextChanged(wxCommandEvent& event)
{
    // Enable/disable buttons based on text
    bool hasText = m_searchTextCtrl && !m_searchTextCtrl->GetValue().IsEmpty();
    
    if (m_findButton)
        m_findButton->Enable(hasText);
    if (m_findNextButton)
        m_findNextButton->Enable(hasText);
}
