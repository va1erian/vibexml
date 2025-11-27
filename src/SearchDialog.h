#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/timer.h>
#include "XmlEditorCtrl.h"

class VirtualXmlTree;

class SearchDialog : public wxDialog
{
public:
    SearchDialog(wxWindow* parent, XmlEditorCtrl* editor, VirtualXmlTree* treeCtrl);
    virtual ~SearchDialog();

private:
    void OnFind(wxCommandEvent& event);
    void OnFindPrev(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnWindowClose(wxCloseEvent& event);
    void OnTextChanged(wxCommandEvent& event);
    void OnCheckboxChanged(wxCommandEvent& event);
    void OnSearchTimer(wxTimerEvent& event);
    
    void BindControls();
    void UpdateStatus(const SearchResult& result);
    void PerformSearch();
    void ExpandTreeToLine(int lineNumber);

    wxTextCtrl* m_searchTextCtrl;
    wxCheckBox* m_caseSensitiveCheck;
    wxCheckBox* m_wholeWordCheck;
    wxButton* m_findButton;
    wxButton* m_findPrevButton;
    wxStaticText* m_statusLabel;
    XmlEditorCtrl* m_editor;
    VirtualXmlTree* m_treeCtrl;
    
    // Debounce timer for auto-search
    wxTimer m_searchTimer;
    static const int SEARCH_DELAY_MS = 300;  // Delay before auto-search
    static const int MIN_SEARCH_LENGTH = 2;  // Minimum characters to trigger auto-search
};

#endif // SEARCHDIALOG_H
