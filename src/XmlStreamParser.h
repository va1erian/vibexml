#ifndef XMLSTREAMPARSER_H
#define XMLSTREAMPARSER_H

#include <wx/wx.h>
#include <vector>
#include <functional>

// Represents a parsed XML element (lightweight - no DOM)
struct XmlElementInfo
{
    wxString name;           // Element tag name
    wxString attributes;     // Attributes as string (for display)
    int startLine;           // 1-based line number where element starts
    int endLine;             // 1-based line number where element ends (0 if self-closing or unknown)
    size_t startPos;         // Character position of '<'
    size_t endPos;           // Character position after '>' of closing tag
    bool hasChildren;        // Whether this element has child elements
    bool isSelfClosing;      // Whether this is a self-closing tag <foo/>
    wxString textContent;    // Immediate text content (truncated for display)
};

// Callback for progress during scanning
using ScanProgressCallback = std::function<void(int percent, const wxString& status)>;

// Lightweight streaming XML parser
// Scans XML text without building a full DOM tree
class XmlStreamParser
{
public:
    XmlStreamParser();
    
    // Set the source text to parse
    void SetSource(const wxString& text);
    
    // Get top-level elements (immediate children of root, or root itself)
    // This is fast - only scans the opening tag, not the entire element
    std::vector<XmlElementInfo> GetTopLevelElements(ScanProgressCallback progress = nullptr);
    
    // Get children of an element starting at parentStartPos
    // Scans only immediate children, stops at parent's closing tag
    std::vector<XmlElementInfo> GetChildElements(size_t parentStartPos, size_t parentEndPos);
    
    // Parse just the opening tag of an element (fast - doesn't scan content)
    XmlElementInfo ParseElementOpening(size_t pos);
    
    // Find where an element ends (for lazy loading)
    // Returns the position after the closing tag
    size_t FindElementEnd(size_t startPos);
    
    // Get line number at a character position
    int GetLineAtPosition(size_t pos) const;
    
    // Check if parsing was successful
    bool IsValid() const { return m_valid; }
    wxString GetError() const { return m_error; }

private:
    // Skip whitespace starting from pos, return new position
    size_t SkipWhitespace(size_t pos) const;
    
    // Skip XML declaration <?xml ... ?>
    size_t SkipDeclaration(size_t pos) const;
    
    // Skip comment <!-- ... -->
    size_t SkipComment(size_t pos) const;
    
    // Skip CDATA section <![CDATA[ ... ]]>
    size_t SkipCData(size_t pos) const;
    
    // Skip DOCTYPE declaration <!DOCTYPE ... >
    size_t SkipDoctype(size_t pos) const;
    
    // Parse a single element starting at '<', returns info and updates pos
    XmlElementInfo ParseElement(size_t& pos);
    
    // Parse tag name starting after '<'
    wxString ParseTagName(size_t& pos) const;
    
    // Parse attributes until '>' or '/>'
    wxString ParseAttributes(size_t& pos, bool& isSelfClosing) const;
    
    // Extract text content between tags (truncated)
    wxString ExtractTextContent(size_t startPos, size_t endPos) const;
    
    // Check if there are child elements in range
    bool HasChildElements(size_t startPos, size_t endPos) const;
    
    // Build line index for fast line lookups
    void BuildLineIndex();
    
    wxString m_text;
    std::vector<size_t> m_lineStarts;  // Position of each line start
    bool m_valid;
    wxString m_error;
};

#endif // XMLSTREAMPARSER_H

