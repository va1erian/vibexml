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
    SetupStyles();

    // Set monospace font
    wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    StyleSetFont(wxSTC_STYLE_DEFAULT, font);

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
}

void XmlEditorCtrl::SetupXmlLexer()
{
    SetLexer(wxSTC_LEX_XML);
}

void XmlEditorCtrl::SetupStyles()
{
    // Default style
    StyleSetForeground(wxSTC_STYLE_DEFAULT, wxColour(0, 0, 0));
    StyleSetBackground(wxSTC_STYLE_DEFAULT, wxColour(255, 255, 255));
    StyleSetFont(wxSTC_STYLE_DEFAULT, wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    // XML tags
    StyleSetForeground(wxSTC_H_TAG, wxColour(128, 0, 0));
    StyleSetBold(wxSTC_H_TAG, true);

    // XML tag end
    StyleSetForeground(wxSTC_H_TAGEND, wxColour(128, 0, 0));
    StyleSetBold(wxSTC_H_TAGEND, true);

    // XML attribute names
    StyleSetForeground(wxSTC_H_ATTRIBUTE, wxColour(255, 0, 0));
    StyleSetBold(wxSTC_H_ATTRIBUTE, true);

    // XML attribute values
    StyleSetForeground(wxSTC_H_ATTRIBUTEUNKNOWN, wxColour(0, 0, 255));

    // XML text content (use default style for text)
    // Note: wxSTC_H_TEXT may not be available in all wxWidgets versions

    // XML comments
    StyleSetForeground(wxSTC_H_COMMENT, wxColour(0, 128, 0));
    StyleSetItalic(wxSTC_H_COMMENT, true);

    // XML numbers
    StyleSetForeground(wxSTC_H_NUMBER, wxColour(0, 0, 255));

    // XML strings
    StyleSetForeground(wxSTC_H_DOUBLESTRING, wxColour(0, 0, 255));
    StyleSetForeground(wxSTC_H_SINGLESTRING, wxColour(0, 0, 255));

    // XML entities
    StyleSetForeground(wxSTC_H_ENTITY, wxColour(128, 0, 128));

    // Line number margin
    StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColour(128, 128, 128));
    StyleSetBackground(wxSTC_STYLE_LINENUMBER, wxColour(240, 240, 240));
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

