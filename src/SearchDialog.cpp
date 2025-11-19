#include "SearchDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>

SearchDialog::SearchDialog(wxWindow* parent, XmlEditorCtrl* editor)
    : wxDialog(parent, wxID_ANY, "Find", wxDefaultPosition, wxSize(400, 200)),
      m_editor(editor)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Search text
    wxStaticText* label = new wxStaticText(this, wxID_ANY, "Find:");
    mainSizer->Add(label, 0, wxALL, 5);

    m_searchTextCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
    mainSizer->Add(m_searchTextCtrl, 0, wxEXPAND | wxALL, 5);

    // Options
    m_caseSensitiveCheck = new wxCheckBox(this, wxID_ANY, "Case sensitive");
    mainSizer->Add(m_caseSensitiveCheck, 0, wxALL, 5);

    m_wholeWordCheck = new wxCheckBox(this, wxID_ANY, "Whole words only");
    mainSizer->Add(m_wholeWordCheck, 0, wxALL, 5);

    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_findButton = new wxButton(this, wxID_OK, "Find");
    m_findNextButton = new wxButton(this, wxID_ANY, "Find Next");
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

    buttonSizer->Add(m_findButton, 0, wxALL, 5);
    buttonSizer->Add(m_findNextButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    SetSizer(mainSizer);
    Layout();

    // Bind events
    m_findButton->Bind(wxEVT_BUTTON, &SearchDialog::OnFind, this);
    m_findNextButton->Bind(wxEVT_BUTTON, &SearchDialog::OnFindNext, this);
    cancelButton->Bind(wxEVT_BUTTON, &SearchDialog::OnCancel, this);
    m_searchTextCtrl->Bind(wxEVT_TEXT, &SearchDialog::OnTextChanged, this);

    // Set focus to search text
    m_searchTextCtrl->SetFocus();
}

void SearchDialog::OnFind(wxCommandEvent& event)
{
    if (!m_editor)
        return;

    wxString searchText = m_searchTextCtrl->GetValue();
    if (searchText.IsEmpty())
        return;

    bool caseSensitive = m_caseSensitiveCheck->GetValue();
    bool wholeWord = m_wholeWordCheck->GetValue();

    m_editor->SetSearchText(searchText, caseSensitive, wholeWord);
    m_editor->FindNext();
}

void SearchDialog::OnFindNext(wxCommandEvent& event)
{
    if (!m_editor)
        return;

    wxString searchText = m_searchTextCtrl->GetValue();
    if (searchText.IsEmpty())
        return;

    bool caseSensitive = m_caseSensitiveCheck->GetValue();
    bool wholeWord = m_wholeWordCheck->GetValue();

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
    bool hasText = !m_searchTextCtrl->GetValue().IsEmpty();
    m_findButton->Enable(hasText);
    m_findNextButton->Enable(hasText);
}

