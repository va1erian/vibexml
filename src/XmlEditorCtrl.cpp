#include "XmlEditorCtrl.h"
#include <wx/file.h>

XmlEditorCtrl::XmlEditorCtrl(wxWindow* parent, wxWindowID id,
                             const wxPoint& pos, const wxSize& size, long style)
    : wxStyledTextCtrl(parent, id, pos, size, style),
      m_caseSensitive(false),
      m_wholeWord(false),
      m_lastSearchPos(0)
{
    SetupXmlLexer();
    
    // Enable line numbers
    SetMarginType(0, wxSTC_MARGIN_NUMBER);
    SetMarginWidth(0, 50);

    // Enable folding
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
    
    // Update folding markers for theme
    bool isDark = theme.background.GetLuminance() < 0.5;
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

    SetReadOnly(false);
    SetText(content);
    SetReadOnly(true);

    // Reset search position
    m_lastSearchPos = 0;
    EmptyUndoBuffer();
    SetSavePoint();

    return true;
}

void XmlEditorCtrl::FindNext()
{
    if (m_searchText.IsEmpty())
        return;

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
    }

    if (foundPos != -1)
    {
        int endFoundPos = foundPos + m_searchText.Length();
        SetSelection(foundPos, endFoundPos);
        EnsureCaretVisible();
        m_lastSearchPos = endFoundPos;
    }
    else
    {
        wxBell(); // Beep to indicate no more matches
        m_lastSearchPos = 0; // Reset for next search
    }
}

void XmlEditorCtrl::SetSearchText(const wxString& text, bool caseSensitive, bool wholeWord)
{
    m_searchText = text;
    m_caseSensitive = caseSensitive;
    m_wholeWord = wholeWord;
    m_lastSearchPos = GetCurrentPos();
}

void XmlEditorCtrl::ClearSearch()
{
    m_searchText.Clear();
    m_lastSearchPos = 0;
}

void XmlEditorCtrl::GotoLine(int lineNumber)
{
    if (lineNumber < 1)
        return;

    int lineCount = GetLineCount();
    if (lineNumber > lineCount)
        lineNumber = lineCount;

    // wxStyledTextCtrl::GotoLine uses 0-based line numbers
    wxStyledTextCtrl::GotoLine(lineNumber - 1);
    EnsureCaretVisible();
    SetSelection(GetCurrentPos(), GetCurrentPos());
}
