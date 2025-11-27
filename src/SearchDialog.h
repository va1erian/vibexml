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
    void OnFindNext(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnTextChanged(wxCommandEvent& event);
    
    void BindControls();

    wxTextCtrl* m_searchTextCtrl;
    wxCheckBox* m_caseSensitiveCheck;
    wxCheckBox* m_wholeWordCheck;
    wxButton* m_findButton;
    wxButton* m_findNextButton;
    XmlEditorCtrl* m_editor;
};

#endif // SEARCHDIALOG_H
