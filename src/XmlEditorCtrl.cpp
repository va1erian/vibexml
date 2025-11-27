#include "XmlEditorCtrl.h"
#include <wx/file.h>

XmlEditorCtrl::XmlEditorCtrl(wxWindow* parent, wxWindowID id,
                             const wxPoint& pos, const wxSize& size, long style)
    : wxStyledTextCtrl(parent, id, pos, size, style),
      m_caseSensitive(false),
      m_wholeWord(false),
      m_lastSearchPos(0),
      m_matchCount(0),
      m_currentMatchIndex(0),
      m_highlightedLine(-1)
{
    SetupXmlLexer();
    SetupIndicators();
    SetupMarkers();
    
    // Enable line numbers
    SetMarginType(0, wxSTC_MARGIN_NUMBER);
    SetMarginWidth(0, 50);

    // Enable folding margin
    SetMarginType(1, wxSTC_MARGIN_SYMBOL);
    SetMarginWidth(1, 16);
    SetMarginMask(1, wxSTC_MASK_FOLDERS);
    SetMarginSensitive(1, true);
    SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);

    // Set folding markers
    MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS);
    MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUSCONNECTED);
    MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER);
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED);
    MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_VLINE);
    MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_LCORNER);

    // Set properties for better performance with large files
    SetBufferedDraw(true);
    SetLayoutCache(wxSTC_CACHE_PAGE);
    SetScrollWidthTracking(true);

    // Set tab width
    SetTabWidth(4);
    SetUseTabs(false);
    SetIndent(4);

    // Read-only by default (viewer mode)
    SetReadOnly(true);
    
    // Apply settings from configuration
    ApplySettings();
}

void XmlEditorCtrl::SetupXmlLexer()
{
    SetLexer(wxSTC_LEX_XML);
}

void XmlEditorCtrl::SetupIndicators()
{
    // Indicator for all search matches (yellow/orange highlight)
    IndicatorSetStyle(INDICATOR_SEARCH, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetForeground(INDICATOR_SEARCH, wxColour(255, 200, 0));  // Yellow/orange
    IndicatorSetAlpha(INDICATOR_SEARCH, 100);
    IndicatorSetOutlineAlpha(INDICATOR_SEARCH, 200);
    IndicatorSetUnder(INDICATOR_SEARCH, true);
    
    // Indicator for current match (brighter, more prominent)
    IndicatorSetStyle(INDICATOR_CURRENT, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetForeground(INDICATOR_CURRENT, wxColour(255, 128, 0));  // Orange
    IndicatorSetAlpha(INDICATOR_CURRENT, 180);
    IndicatorSetOutlineAlpha(INDICATOR_CURRENT, 255);
    IndicatorSetUnder(INDICATOR_CURRENT, true);
}

void XmlEditorCtrl::SetupMarkers()
{
    // Marker for highlighting the current line when navigating from tree
    MarkerDefine(MARKER_HIGHLIGHT, wxSTC_MARK_BACKGROUND);
    MarkerSetBackground(MARKER_HIGHLIGHT, wxColour(100, 200, 255));  // Light blue
    MarkerSetAlpha(MARKER_HIGHLIGHT, 80);
}

void XmlEditorCtrl::ApplySettings()
{
    EditorSettings& settings = EditorSettings::Get();
    wxFont font = settings.GetFont();
    EditorTheme theme = settings.GetTheme();
    
    // Apply font to default style first
    StyleSetFont(wxSTC_STYLE_DEFAULT, font);
    StyleSetForeground(wxSTC_STYLE_DEFAULT, theme.foreground);
    StyleSetBackground(wxSTC_STYLE_DEFAULT, theme.background);
    
    // Clear all styles and inherit from default
    StyleClearAll();
    
    // Line numbers
    StyleSetForeground(wxSTC_STYLE_LINENUMBER, theme.lineNumberFg);
    StyleSetBackground(wxSTC_STYLE_LINENUMBER, theme.lineNumberBg);
    StyleSetFont(wxSTC_STYLE_LINENUMBER, font);
    
    // XML tags
    StyleSetForeground(wxSTC_H_TAG, theme.tagColor);
    StyleSetBold(wxSTC_H_TAG, true);
    StyleSetForeground(wxSTC_H_TAGEND, theme.tagColor);
    StyleSetBold(wxSTC_H_TAGEND, true);
    StyleSetForeground(wxSTC_H_TAGUNKNOWN, theme.tagColor);
    
    // XML attribute names
    StyleSetForeground(wxSTC_H_ATTRIBUTE, theme.attributeNameColor);
    StyleSetBold(wxSTC_H_ATTRIBUTE, true);
    StyleSetForeground(wxSTC_H_ATTRIBUTEUNKNOWN, theme.attributeNameColor);

    // XML attribute values (strings)
    StyleSetForeground(wxSTC_H_DOUBLESTRING, theme.attributeValueColor);
    StyleSetForeground(wxSTC_H_SINGLESTRING, theme.attributeValueColor);
    StyleSetForeground(wxSTC_H_VALUE, theme.attributeValueColor);

    // XML comments
    StyleSetForeground(wxSTC_H_COMMENT, theme.commentColor);
    StyleSetItalic(wxSTC_H_COMMENT, true);

    // XML numbers
    StyleSetForeground(wxSTC_H_NUMBER, theme.attributeValueColor);

    // XML entities
    StyleSetForeground(wxSTC_H_ENTITY, theme.entityColor);

    // CDATA
    StyleSetForeground(wxSTC_H_CDATA, theme.cdataColor);
    
    // XML declaration
    StyleSetForeground(wxSTC_H_XMLSTART, theme.tagColor);
    StyleSetForeground(wxSTC_H_XMLEND, theme.tagColor);
    
    // Default text content
    StyleSetForeground(wxSTC_H_DEFAULT, theme.textContentColor);
    
    // Set all backgrounds to theme background
    for (int i = 0; i <= wxSTC_H_SGML_ENTITY; i++)
    {
        StyleSetBackground(i, theme.background);
    }
    
    // Caret and selection
    SetCaretForeground(theme.foreground);
    SetCaretLineVisible(true);
    SetCaretLineBackground(theme.caretLineBackground);
    SetSelBackground(true, theme.selectionBackground);
    
    // Update indicator colors based on theme
    bool isDark = theme.background.GetLuminance() < 0.5;
    if (isDark)
    {
        // Brighter colors for dark themes
        IndicatorSetForeground(INDICATOR_SEARCH, wxColour(200, 150, 50));
        IndicatorSetForeground(INDICATOR_CURRENT, wxColour(255, 180, 0));
        // Line highlight for dark theme
        MarkerSetBackground(MARKER_HIGHLIGHT, wxColour(80, 120, 180));
    }
    else
    {
        // Standard colors for light themes
        IndicatorSetForeground(INDICATOR_SEARCH, wxColour(255, 200, 0));
        IndicatorSetForeground(INDICATOR_CURRENT, wxColour(255, 128, 0));
        // Line highlight for light theme
        MarkerSetBackground(MARKER_HIGHLIGHT, wxColour(180, 220, 255));
    }
    
    // Update folding markers for theme
    wxColour markerFg = isDark ? wxColour(200, 200, 200) : wxColour(80, 80, 80);
    wxColour markerBg = theme.background;
    
    MarkerSetForeground(wxSTC_MARKNUM_FOLDER, markerFg);
    MarkerSetBackground(wxSTC_MARKNUM_FOLDER, markerBg);
    MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPEN, markerFg);
    MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPEN, markerBg);
    MarkerSetForeground(wxSTC_MARKNUM_FOLDEREND, markerFg);
    MarkerSetBackground(wxSTC_MARKNUM_FOLDEREND, markerBg);
    MarkerSetForeground(wxSTC_MARKNUM_FOLDERMIDTAIL, markerFg);
    MarkerSetBackground(wxSTC_MARKNUM_FOLDERMIDTAIL, markerBg);
    MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPENMID, markerFg);
    MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPENMID, markerBg);
    MarkerSetForeground(wxSTC_MARKNUM_FOLDERSUB, markerFg);
    MarkerSetBackground(wxSTC_MARKNUM_FOLDERSUB, markerBg);
    MarkerSetForeground(wxSTC_MARKNUM_FOLDERTAIL, markerFg);
    MarkerSetBackground(wxSTC_MARKNUM_FOLDERTAIL, markerBg);
    
    // Set fold margin colors
    SetFoldMarginColour(true, theme.lineNumberBg);
    SetFoldMarginHiColour(true, theme.lineNumberBg);
    
    Refresh();
}

void XmlEditorCtrl::SetupStyles()
{
    // This is now handled by ApplySettings()
    ApplySettings();
}

bool XmlEditorCtrl::LoadFile(const wxString& filePath)
{
    wxFile file(filePath);
    if (!file.IsOpened())
        return false;

    wxString content;
    if (!file.ReadAll(&content))
        return false;

    return LoadFromString(content);
}

bool XmlEditorCtrl::LoadFromString(const wxString& content)
{
    SetReadOnly(false);
    
    // For very large content, disable styling temporarily for faster loading
    bool largeFile = content.Length() > 10 * 1024 * 1024;  // 10 MB
    
    if (largeFile)
    {
        // Disable lexer temporarily for faster text insertion
        SetLexer(wxSTC_LEX_NULL);
    }
    
    SetText(content);
    
    if (largeFile)
    {
        // Re-enable XML lexer
        SetLexer(wxSTC_LEX_XML);
        // Only colorize visible portion initially
        Colourise(0, GetEndStyled());
    }
    
    SetReadOnly(true);

    // Reset search and line highlight
    ClearSearch();
    ClearLineHighlight();
    EmptyUndoBuffer();
    SetSavePoint();
    
    // Go to start
    GotoPos(0);

    return true;
}

void XmlEditorCtrl::ClearHighlights()
{
    // Clear all search indicators
    SetIndicatorCurrent(INDICATOR_SEARCH);
    IndicatorClearRange(0, GetLength());
    SetIndicatorCurrent(INDICATOR_CURRENT);
    IndicatorClearRange(0, GetLength());
}

void XmlEditorCtrl::ClearLineHighlight()
{
    if (m_highlightedLine >= 0)
    {
        MarkerDelete(m_highlightedLine, MARKER_HIGHLIGHT);
        m_highlightedLine = -1;
    }
}

void XmlEditorCtrl::HighlightLine(int lineNumber)
{
    // Clear previous highlight
    ClearLineHighlight();
    
    // Add new highlight (lineNumber is 0-based here)
    if (lineNumber >= 0 && lineNumber < GetLineCount())
    {
        MarkerAdd(lineNumber, MARKER_HIGHLIGHT);
        m_highlightedLine = lineNumber;
    }
}

void XmlEditorCtrl::HighlightAllMatches()
{
    ClearHighlights();
    
    if (m_searchText.IsEmpty())
        return;
    
    int flags = 0;
    if (m_caseSensitive)
        flags |= wxSTC_FIND_MATCHCASE;
    if (m_wholeWord)
        flags |= wxSTC_FIND_WHOLEWORD;
    
    int searchLen = m_searchText.Length();
    int pos = 0;
    int docLength = GetLength();
    
    SetIndicatorCurrent(INDICATOR_SEARCH);
    
    // Find and highlight all matches
    while (pos < docLength)
    {
        int foundPos = FindText(pos, docLength, m_searchText, flags);
        if (foundPos == -1)
            break;
        
        // Highlight this match
        IndicatorFillRange(foundPos, searchLen);
        pos = foundPos + searchLen;
    }
}

int XmlEditorCtrl::CountMatches()
{
    if (m_searchText.IsEmpty())
        return 0;
    
    int flags = 0;
    if (m_caseSensitive)
        flags |= wxSTC_FIND_MATCHCASE;
    if (m_wholeWord)
        flags |= wxSTC_FIND_WHOLEWORD;
    
    int count = 0;
    int pos = 0;
    int docLength = GetLength();
    int searchLen = m_searchText.Length();
    
    while (pos < docLength)
    {
        int foundPos = FindText(pos, docLength, m_searchText, flags);
        if (foundPos == -1)
            break;
        
        count++;
        pos = foundPos + searchLen;
    }
    
    return count;
}

int XmlEditorCtrl::GetMatchIndexAtPosition(int pos)
{
    if (m_searchText.IsEmpty())
        return 0;
    
    int flags = 0;
    if (m_caseSensitive)
        flags |= wxSTC_FIND_MATCHCASE;
    if (m_wholeWord)
        flags |= wxSTC_FIND_WHOLEWORD;
    
    int index = 0;
    int searchPos = 0;
    int docLength = GetLength();
    int searchLen = m_searchText.Length();
    
    while (searchPos < docLength)
    {
        int foundPos = FindText(searchPos, docLength, m_searchText, flags);
        if (foundPos == -1)
            break;
        
        index++;
        if (foundPos >= pos)
            return index;
        
        searchPos = foundPos + searchLen;
    }
    
    return index;
}

SearchResult XmlEditorCtrl::FindNext()
{
    SearchResult result;
    
    // Clear line highlight when searching
    ClearLineHighlight();
    
    if (m_searchText.IsEmpty())
        return result;

    int flags = 0;
    if (m_caseSensitive)
        flags |= wxSTC_FIND_MATCHCASE;
    if (m_wholeWord)
        flags |= wxSTC_FIND_WHOLEWORD;

    int startPos = m_lastSearchPos;
    int endPos = GetLength();

    // Search from current position to end
    int foundPos = FindText(startPos, endPos, m_searchText, flags);

    // If not found, wrap around and search from beginning
    if (foundPos == -1 && startPos > 0)
    {
        foundPos = FindText(0, startPos, m_searchText, flags);
        if (foundPos != -1)
        {
            result.wrapped = true;
        }
    }

    if (foundPos != -1)
    {
        result.found = true;
        int searchLen = m_searchText.Length();
        int endFoundPos = foundPos + searchLen;
        
        // Clear previous current match indicator and set new one
        SetIndicatorCurrent(INDICATOR_CURRENT);
        IndicatorClearRange(0, GetLength());
        IndicatorFillRange(foundPos, searchLen);
        
        SetSelection(foundPos, endFoundPos);
        EnsureCaretVisible();
        m_lastSearchPos = endFoundPos;
        
        // Update match info
        m_currentMatchIndex = GetMatchIndexAtPosition(foundPos);
        result.matchIndex = m_currentMatchIndex;
        result.totalMatches = m_matchCount;
    }
    else
    {
        wxBell();
        m_lastSearchPos = 0;
        m_currentMatchIndex = 0;
    }
    
    return result;
}

SearchResult XmlEditorCtrl::FindPrevious()
{
    SearchResult result;
    
    // Clear line highlight when searching
    ClearLineHighlight();
    
    if (m_searchText.IsEmpty())
        return result;

    int flags = 0;
    if (m_caseSensitive)
        flags |= wxSTC_FIND_MATCHCASE;
    if (m_wholeWord)
        flags |= wxSTC_FIND_WHOLEWORD;

    // Use selection start to determine "current" position
    // This way if we're on a match, we look for matches BEFORE this match starts
    int selStart = GetSelectionStart();
    int searchLen = m_searchText.Length();
    int docLength = GetLength();
    
    // Search backwards by finding all matches and getting the one before current
    int lastMatch = -1;
    int pos = 0;
    
    // Find all matches that START before selStart
    while (pos < selStart)
    {
        int foundPos = FindText(pos, docLength, m_searchText, flags);
        if (foundPos == -1 || foundPos >= selStart)
            break;
        
        lastMatch = foundPos;
        pos = foundPos + searchLen;
    }
    
    // If no match found before current position, wrap to end of document
    if (lastMatch == -1)
    {
        // Find the last match in the entire document
        pos = 0;
        while (pos < docLength)
        {
            int foundPos = FindText(pos, docLength, m_searchText, flags);
            if (foundPos == -1)
                break;
            
            lastMatch = foundPos;
            pos = foundPos + searchLen;
        }
        
        // Only set wrapped if we found something and it's not the same position
        if (lastMatch != -1 && lastMatch >= selStart)
        {
            result.wrapped = true;
        }
    }
    
    if (lastMatch != -1)
    {
        result.found = true;
        int endFoundPos = lastMatch + searchLen;
        
        // Clear previous current match indicator and set new one
        SetIndicatorCurrent(INDICATOR_CURRENT);
        IndicatorClearRange(0, docLength);
        IndicatorFillRange(lastMatch, searchLen);
        
        SetSelection(lastMatch, endFoundPos);
        EnsureCaretVisible();
        m_lastSearchPos = lastMatch;
        
        // Update match info
        m_currentMatchIndex = GetMatchIndexAtPosition(lastMatch);
        result.matchIndex = m_currentMatchIndex;
        result.totalMatches = m_matchCount;
    }
    else
    {
        wxBell();
        m_currentMatchIndex = 0;
    }
    
    return result;
}

void XmlEditorCtrl::SetSearchText(const wxString& text, bool caseSensitive, bool wholeWord)
{
    bool searchChanged = (text != m_searchText) || 
                         (caseSensitive != m_caseSensitive) || 
                         (wholeWord != m_wholeWord);
    
    m_searchText = text;
    m_caseSensitive = caseSensitive;
    m_wholeWord = wholeWord;
    m_lastSearchPos = GetCurrentPos();
    
    if (searchChanged)
    {
        // Count and highlight all matches
        m_matchCount = CountMatches();
        HighlightAllMatches();
        m_currentMatchIndex = 0;
    }
}

void XmlEditorCtrl::ClearSearch()
{
    m_searchText.Clear();
    m_lastSearchPos = 0;
    m_matchCount = 0;
    m_currentMatchIndex = 0;
    ClearHighlights();
}

bool XmlEditorCtrl::FormatXml(int indentSpaces)
{
    // Get the current XML content
    wxString content = GetText();
    if (content.IsEmpty())
        return false;
    
    // Parse the XML
    tinyxml2::XMLDocument doc;
    wxCharBuffer utf8Content = content.ToUTF8();
    tinyxml2::XMLError error = doc.Parse(utf8Content.data());
    
    if (error != tinyxml2::XML_SUCCESS)
    {
        wxMessageBox(
            wxString::Format("Failed to parse XML: %s\n\nLine: %d", 
                             doc.ErrorStr(), doc.ErrorLineNum()),
            "XML Format Error", 
            wxOK | wxICON_ERROR);
        return false;
    }
    
    // Use TinyXML-2's printer to format the document
    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);
    
    // Get the formatted output
    wxString formattedXml = wxString::FromUTF8(printer.CStr());
    
    // Store current position to try to restore it
    int currentLine = LineFromPosition(GetCurrentPos());
    
    // Replace the content
    SetReadOnly(false);
    SetText(formattedXml);
    SetReadOnly(true);
    
    // Try to go back to approximately the same position
    if (currentLine < GetLineCount())
    {
        GotoLine(currentLine + 1, false);
    }
    
    // Mark as modified (though we're read-only, this helps track state)
    SetSavePoint();
    
    return true;
}

void XmlEditorCtrl::GotoLine(int lineNumber, bool highlight)
{
    if (lineNumber < 1)
        return;

    int lineCount = GetLineCount();
    if (lineNumber > lineCount)
        lineNumber = lineCount;

    // Convert to 0-based line number
    int line0 = lineNumber - 1;
    
    // wxStyledTextCtrl::GotoLine uses 0-based line numbers
    wxStyledTextCtrl::GotoLine(line0);
    
    // Select the entire line content for visibility
    int lineStart = PositionFromLine(line0);
    int lineEnd = GetLineEndPosition(line0);
    SetSelection(lineStart, lineEnd);
    
    // Ensure line is visible (unfold if needed)
    EnsureVisible(line0);
    EnsureCaretVisible();
    
    // Highlight the line if requested
    if (highlight)
    {
        HighlightLine(line0);
    }
}
