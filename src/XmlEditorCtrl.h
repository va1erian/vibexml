#ifndef XMLEDITORCTRL_H
#define XMLEDITORCTRL_H

#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <tinyxml2.h>
#include "EditorSettings.h"

// Search result information
struct SearchResult
{
    bool found;
    bool wrapped;       // True if search wrapped around
    int matchIndex;     // Current match (1-based)
    int totalMatches;   // Total number of matches
    
    SearchResult() : found(false), wrapped(false), matchIndex(0), totalMatches(0) {}
};

class XmlEditorCtrl : public wxStyledTextCtrl
{
public:
    XmlEditorCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = 0);

    bool LoadFile(const wxString& filePath);
    bool LoadFromString(const wxString& content);
    SearchResult FindNext();
    SearchResult FindPrevious();
    void SetSearchText(const wxString& text, bool caseSensitive, bool wholeWord);
    void ClearSearch();
    void GotoLine(int lineNumber, bool highlight = false);
    
    // Format/beautify XML
    bool FormatXml(int indentSpaces = 4);
    
    // Apply current settings from EditorSettings
    void ApplySettings();
    
    // Get current search info
    int GetMatchCount() const { return m_matchCount; }
    int GetCurrentMatchIndex() const { return m_currentMatchIndex; }

private:
    void SetupXmlLexer();
    void SetupStyles();
    void SetupIndicators();
    void SetupMarkers();
    void HighlightAllMatches();
    void ClearHighlights();
    void ClearLineHighlight();
    void HighlightLine(int lineNumber);
    int CountMatches();
    int GetMatchIndexAtPosition(int pos);

    wxString m_searchText;
    bool m_caseSensitive;
    bool m_wholeWord;
    int m_lastSearchPos;
    int m_matchCount;
    int m_currentMatchIndex;
    int m_highlightedLine;  // Currently highlighted line (-1 if none)
    
    // Indicator numbers for search highlights
    static const int INDICATOR_SEARCH = 8;
    static const int INDICATOR_CURRENT = 9;
    
    // Marker for line highlight
    static const int MARKER_HIGHLIGHT = 1;
};

#endif // XMLEDITORCTRL_H
