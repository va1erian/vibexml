#ifndef XMLEDITORCTRL_H
#define XMLEDITORCTRL_H

#include <wx/wx.h>
#include <wx/stc/stc.h>

class XmlEditorCtrl : public wxStyledTextCtrl
{
public:
    XmlEditorCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = 0);

    bool LoadFile(const wxString& filePath);
    void FindNext();
    void SetSearchText(const wxString& text, bool caseSensitive, bool wholeWord);
    void ClearSearch();
    void GotoLine(int lineNumber);

private:
    void SetupXmlLexer();
    void SetupStyles();

    wxString m_searchText;
    bool m_caseSensitive;
    bool m_wholeWord;
    int m_lastSearchPos;
};

#endif // XMLEDITORCTRL_H

