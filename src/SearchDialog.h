#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <wx/wx.h>
#include <wx/dialog.h>
#include "XmlEditorCtrl.h"

class SearchDialog : public wxDialog
{
public:
    SearchDialog(wxWindow* parent, XmlEditorCtrl* editor);

private:
    void OnFind(wxCommandEvent& event);
    void OnFindPrev(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnTextChanged(wxCommandEvent& event);
    void OnCheckboxChanged(wxCommandEvent& event);
    
    void BindControls();
    void UpdateStatus(const SearchResult& result);
    void UpdateMatchCount();

    wxTextCtrl* m_searchTextCtrl;
    wxCheckBox* m_caseSensitiveCheck;
    wxCheckBox* m_wholeWordCheck;
    wxButton* m_findButton;
    wxButton* m_findPrevButton;
    wxStaticText* m_statusLabel;
    XmlEditorCtrl* m_editor;
};

#endif // SEARCHDIALOG_H
