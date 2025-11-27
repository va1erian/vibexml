#include "XmlStreamParser.h"

XmlStreamParser::XmlStreamParser()
    : m_valid(true)
{
}

void XmlStreamParser::SetSource(const wxString& text)
{
    m_text = text;
    m_valid = true;
    m_error.Clear();
    BuildLineIndex();
}

void XmlStreamParser::BuildLineIndex()
{
    m_lineStarts.clear();
    m_lineStarts.push_back(0);  // Line 1 starts at position 0
    
    for (size_t i = 0; i < m_text.length(); ++i)
    {
        if (m_text[i] == '\n')
        {
            m_lineStarts.push_back(i + 1);
        }
    }
}

int XmlStreamParser::GetLineAtPosition(size_t pos) const
{
    // Binary search for the line containing pos
    if (m_lineStarts.empty())
        return 1;
    
    auto it = std::upper_bound(m_lineStarts.begin(), m_lineStarts.end(), pos);
    return static_cast<int>(it - m_lineStarts.begin());
}

size_t XmlStreamParser::SkipWhitespace(size_t pos) const
{
    while (pos < m_text.length() && 
           (m_text[pos] == ' ' || m_text[pos] == '\t' || 
            m_text[pos] == '\n' || m_text[pos] == '\r'))
    {
        ++pos;
    }
    return pos;
}

size_t XmlStreamParser::SkipDeclaration(size_t pos) const
{
    // Skip <?xml ... ?>
    if (pos + 1 < m_text.length() && m_text[pos] == '<' && m_text[pos + 1] == '?')
    {
        pos += 2;
        while (pos + 1 < m_text.length())
        {
            if (m_text[pos] == '?' && m_text[pos + 1] == '>')
            {
                return pos + 2;
            }
            ++pos;
        }
    }
    return pos;
}

size_t XmlStreamParser::SkipComment(size_t pos) const
{
    // Skip <!-- ... -->
    if (pos + 3 < m_text.length() && 
        m_text[pos] == '<' && m_text[pos + 1] == '!' && 
        m_text[pos + 2] == '-' && m_text[pos + 3] == '-')
    {
        pos += 4;
        while (pos + 2 < m_text.length())
        {
            if (m_text[pos] == '-' && m_text[pos + 1] == '-' && m_text[pos + 2] == '>')
            {
                return pos + 3;
            }
            ++pos;
        }
    }
    return pos;
}

size_t XmlStreamParser::SkipCData(size_t pos) const
{
    // Skip <![CDATA[ ... ]]>
    if (pos + 8 < m_text.length() && m_text.Mid(pos, 9) == "<![CDATA[")
    {
        pos += 9;
        while (pos + 2 < m_text.length())
        {
            if (m_text[pos] == ']' && m_text[pos + 1] == ']' && m_text[pos + 2] == '>')
            {
                return pos + 3;
            }
            ++pos;
        }
    }
    return pos;
}

size_t XmlStreamParser::SkipDoctype(size_t pos) const
{
    // Skip <!DOCTYPE ... > (simplified - doesn't handle internal subset perfectly)
    if (pos + 8 < m_text.length() && m_text.Mid(pos, 9).Upper() == "<!DOCTYPE")
    {
        int depth = 1;
        pos += 9;
        while (pos < m_text.length() && depth > 0)
        {
            if (m_text[pos] == '<')
                ++depth;
            else if (m_text[pos] == '>')
                --depth;
            ++pos;
        }
    }
    return pos;
}

wxString XmlStreamParser::ParseTagName(size_t& pos) const
{
    wxString name;
    while (pos < m_text.length())
    {
        wxChar c = m_text[pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || 
            c == '>' || c == '/' || c == '=')
        {
            break;
        }
        name += c;
        ++pos;
    }
    return name;
}

wxString XmlStreamParser::ParseAttributes(size_t& pos, bool& isSelfClosing) const
{
    wxString attrs;
    isSelfClosing = false;
    
    // Skip initial whitespace
    pos = SkipWhitespace(pos);
    
    size_t attrStart = pos;
    bool inQuote = false;
    wxChar quoteChar = 0;
    
    while (pos < m_text.length())
    {
        wxChar c = m_text[pos];
        
        if (inQuote)
        {
            if (c == quoteChar)
                inQuote = false;
        }
        else
        {
            if (c == '"' || c == '\'')
            {
                inQuote = true;
                quoteChar = c;
            }
            else if (c == '/')
            {
                if (pos + 1 < m_text.length() && m_text[pos + 1] == '>')
                {
                    attrs = m_text.Mid(attrStart, pos - attrStart).Trim();
                    isSelfClosing = true;
                    pos += 2;  // Skip />
                    return attrs;
                }
            }
            else if (c == '>')
            {
                attrs = m_text.Mid(attrStart, pos - attrStart).Trim();
                ++pos;  // Skip >
                return attrs;
            }
        }
        ++pos;
    }
    
    return attrs;
}

XmlElementInfo XmlStreamParser::ParseElement(size_t& pos)
{
    XmlElementInfo info;
    info.startPos = pos;
    info.startLine = GetLineAtPosition(pos);
    info.endLine = 0;
    info.endPos = 0;
    info.hasChildren = false;
    info.isSelfClosing = false;
    
    // Skip '<'
    ++pos;
    
    // Parse tag name
    info.name = ParseTagName(pos);
    
    // Parse attributes
    info.attributes = ParseAttributes(pos, info.isSelfClosing);
    
    if (info.isSelfClosing)
    {
        info.endPos = pos;
        info.endLine = GetLineAtPosition(pos - 1);
        return info;
    }
    
    // Find the matching closing tag
    size_t contentStart = pos;
    int depth = 1;
    bool foundChildren = false;
    
    while (pos < m_text.length() && depth > 0)
    {
        if (m_text[pos] == '<')
        {
            if (pos + 1 < m_text.length())
            {
                if (m_text[pos + 1] == '/')
                {
                    // Closing tag
                    size_t closeStart = pos;
                    pos += 2;
                    wxString closeName = ParseTagName(pos);
                    
                    if (closeName == info.name)
                    {
                        --depth;
                        if (depth == 0)
                        {
                            // Skip to end of closing tag
                            while (pos < m_text.length() && m_text[pos] != '>')
                                ++pos;
                            if (pos < m_text.length())
                                ++pos;
                            
                            info.endPos = pos;
                            info.endLine = GetLineAtPosition(pos - 1);
                            
                            // Extract text content if no children
                            if (!foundChildren)
                            {
                                info.textContent = ExtractTextContent(contentStart, closeStart);
                            }
                            break;
                        }
                    }
                }
                else if (m_text[pos + 1] == '!')
                {
                    // Comment or CDATA
                    size_t newPos = SkipComment(pos);
                    if (newPos == pos)
                        newPos = SkipCData(pos);
                    if (newPos > pos)
                    {
                        pos = newPos;
                        continue;
                    }
                    ++pos;
                }
                else if (m_text[pos + 1] == '?')
                {
                    // Processing instruction
                    pos = SkipDeclaration(pos);
                    continue;
                }
                else if (m_text[pos + 1] != ' ' && m_text[pos + 1] != '\t' && 
                         m_text[pos + 1] != '\n' && m_text[pos + 1] != '\r')
                {
                    // Opening tag of child element
                    foundChildren = true;
                    ++depth;
                    ++pos;
                    
                    // Skip past this child element's opening tag
                    ParseTagName(pos);
                    bool childSelfClosing = false;
                    ParseAttributes(pos, childSelfClosing);
                    if (childSelfClosing)
                        --depth;
                    continue;
                }
                else
                {
                    ++pos;
                }
            }
            else
            {
                ++pos;
            }
        }
        else
        {
            ++pos;
        }
    }
    
    info.hasChildren = foundChildren;
    return info;
}

wxString XmlStreamParser::ExtractTextContent(size_t startPos, size_t endPos) const
{
    if (startPos >= endPos || startPos >= m_text.length())
        return wxString();
    
    wxString content = m_text.Mid(startPos, endPos - startPos);
    
    // Trim whitespace
    content.Trim(true).Trim(false);
    
    // Truncate for display
    const size_t maxLen = 100;
    if (content.length() > maxLen)
    {
        content = content.Left(maxLen) + "...";
    }
    
    // Replace newlines with spaces for display
    content.Replace("\n", " ");
    content.Replace("\r", "");
    content.Replace("\t", " ");
    
    // Collapse multiple spaces
    while (content.Replace("  ", " ") > 0) {}
    
    return content;
}

bool XmlStreamParser::HasChildElements(size_t startPos, size_t endPos) const
{
    size_t pos = startPos;
    
    while (pos < endPos && pos < m_text.length())
    {
        if (m_text[pos] == '<')
        {
            if (pos + 1 < endPos)
            {
                wxChar next = m_text[pos + 1];
                // Skip comments, CDATA, closing tags
                if (next != '!' && next != '?' && next != '/')
                {
                    // This is a child element
                    return true;
                }
            }
        }
        ++pos;
    }
    return false;
}

XmlElementInfo XmlStreamParser::ParseElementOpening(size_t pos)
{
    XmlElementInfo info;
    info.startPos = pos;
    info.startLine = GetLineAtPosition(pos);
    info.endLine = 0;
    info.endPos = m_text.length();  // Assume extends to end (will be refined when expanded)
    info.hasChildren = true;        // Assume has children (will be verified when expanded)
    info.isSelfClosing = false;
    
    // Skip '<'
    ++pos;
    
    // Parse tag name
    info.name = ParseTagName(pos);
    
    // Parse attributes
    info.attributes = ParseAttributes(pos, info.isSelfClosing);
    
    if (info.isSelfClosing)
    {
        info.endPos = pos;
        info.endLine = GetLineAtPosition(pos - 1);
        info.hasChildren = false;
    }
    
    // Store content start position for later child parsing
    info.endPos = pos;  // Temporarily store where content starts
    
    return info;
}

std::vector<XmlElementInfo> XmlStreamParser::GetTopLevelElements(ScanProgressCallback progress)
{
    std::vector<XmlElementInfo> elements;
    size_t pos = 0;
    
    // Skip BOM if present
    if (m_text.length() >= 3)
    {
        wxString bom = m_text.Left(3);
        if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)
        {
            pos = 3;
        }
    }
    
    // Skip XML declaration and DOCTYPE
    while (pos < m_text.length())
    {
        pos = SkipWhitespace(pos);
        if (pos >= m_text.length())
            break;
        
        if (m_text[pos] == '<')
        {
            size_t newPos = SkipDeclaration(pos);
            if (newPos > pos) { pos = newPos; continue; }
            
            newPos = SkipComment(pos);
            if (newPos > pos) { pos = newPos; continue; }
            
            newPos = SkipDoctype(pos);
            if (newPos > pos) { pos = newPos; continue; }
            
            // This should be the root element
            break;
        }
        else
        {
            ++pos;
        }
    }
    
    // Parse root element (just the opening tag - fast!)
    if (pos < m_text.length() && m_text[pos] == '<')
    {
        if (progress)
        {
            progress(50, "Found root element...");
        }
        
        XmlElementInfo root = ParseElementOpening(pos);
        
        // For root element, set endPos to end of content
        // (children will be parsed lazily)
        size_t contentStart = root.endPos;
        root.endPos = m_text.length();
        
        // Quick check: does root have children?
        // Scan forward a bit to see if there's a child element
        size_t scanPos = contentStart;
        root.hasChildren = false;
        
        // Only scan a reasonable amount to detect children
        size_t scanLimit = std::min(contentStart + 10000, m_text.length());
        while (scanPos < scanLimit)
        {
            scanPos = SkipWhitespace(scanPos);
            if (scanPos >= scanLimit)
                break;
            
            if (m_text[scanPos] == '<')
            {
                if (scanPos + 1 < m_text.length())
                {
                    wxChar next = m_text[scanPos + 1];
                    if (next == '/')
                    {
                        // Closing tag - no children
                        break;
                    }
                    else if (next != '!' && next != '?')
                    {
                        // Found a child element
                        root.hasChildren = true;
                        break;
                    }
                    else
                    {
                        // Comment or PI, skip
                        size_t newPos = SkipComment(scanPos);
                        if (newPos == scanPos)
                            newPos = SkipDeclaration(scanPos);
                        if (newPos > scanPos)
                        {
                            scanPos = newPos;
                            continue;
                        }
                    }
                }
                ++scanPos;
            }
            else
            {
                ++scanPos;
            }
        }
        
        // If we didn't find anything in scan limit, assume there are children
        if (scanPos >= scanLimit && !root.hasChildren)
        {
            root.hasChildren = true;  // Assume children for large files
        }
        
        elements.push_back(root);
        
        if (progress)
        {
            progress(100, "Done");
        }
    }
    
    return elements;
}

std::vector<XmlElementInfo> XmlStreamParser::GetChildElements(size_t parentStartPos, size_t parentEndPos)
{
    std::vector<XmlElementInfo> children;
    
    // Find where content starts (after opening tag)
    size_t pos = parentStartPos;
    wxString parentName;
    
    // Skip past the opening tag and get parent name
    if (pos < m_text.length() && m_text[pos] == '<')
    {
        ++pos;
        // Get tag name
        size_t nameStart = pos;
        while (pos < m_text.length() && 
               m_text[pos] != ' ' && m_text[pos] != '\t' && 
               m_text[pos] != '\n' && m_text[pos] != '\r' &&
               m_text[pos] != '>' && m_text[pos] != '/')
        {
            ++pos;
        }
        parentName = m_text.Mid(nameStart, pos - nameStart);
        
        // Skip attributes to find >
        bool inQuote = false;
        wxChar quoteChar = 0;
        while (pos < m_text.length())
        {
            if (inQuote)
            {
                if (m_text[pos] == quoteChar)
                    inQuote = false;
            }
            else
            {
                if (m_text[pos] == '"' || m_text[pos] == '\'')
                {
                    inQuote = true;
                    quoteChar = m_text[pos];
                }
                else if (m_text[pos] == '/')
                {
                    // Self-closing, no children
                    return children;
                }
                else if (m_text[pos] == '>')
                {
                    ++pos;
                    break;
                }
            }
            ++pos;
        }
    }
    
    // Now scan for child elements until we hit parent's closing tag
    while (pos < m_text.length())
    {
        pos = SkipWhitespace(pos);
        if (pos >= m_text.length())
            break;
        
        if (m_text[pos] == '<')
        {
            if (pos + 1 < m_text.length())
            {
                wxChar next = m_text[pos + 1];
                
                if (next == '/')
                {
                    // Check if this is the parent's closing tag
                    size_t checkPos = pos + 2;
                    wxString closeName;
                    while (checkPos < m_text.length() && 
                           m_text[checkPos] != ' ' && m_text[checkPos] != '\t' &&
                           m_text[checkPos] != '\n' && m_text[checkPos] != '\r' &&
                           m_text[checkPos] != '>')
                    {
                        closeName += m_text[checkPos];
                        ++checkPos;
                    }
                    
                    if (closeName == parentName)
                    {
                        // This is parent's closing tag - we're done
                        break;
                    }
                    else
                    {
                        // This is some other closing tag (shouldn't happen in well-formed XML)
                        // Skip past it
                        while (pos < m_text.length() && m_text[pos] != '>')
                            ++pos;
                        if (pos < m_text.length())
                            ++pos;
                    }
                }
                else if (next == '!')
                {
                    // Comment or CDATA
                    size_t newPos = SkipComment(pos);
                    if (newPos > pos) { pos = newPos; continue; }
                    newPos = SkipCData(pos);
                    if (newPos > pos) { pos = newPos; continue; }
                    ++pos;
                }
                else if (next == '?')
                {
                    // Processing instruction
                    pos = SkipDeclaration(pos);
                    continue;
                }
                else
                {
                    // Child element - parse just the opening tag
                    XmlElementInfo child = ParseElementOpening(pos);
                    size_t contentStart = child.endPos;
                    
                    // Now we need to find where this child ends
                    if (!child.isSelfClosing)
                    {
                        // Scan to find the matching closing tag
                        int depth = 1;
                        size_t scanPos = contentStart;
                        bool foundChildren = false;
                        
                        while (scanPos < m_text.length() && depth > 0)
                        {
                            if (m_text[scanPos] == '<')
                            {
                                if (scanPos + 1 < m_text.length())
                                {
                                    if (m_text[scanPos + 1] == '/')
                                    {
                                        // Closing tag
                                        size_t closeNameStart = scanPos + 2;
                                        size_t closeNameEnd = closeNameStart;
                                        while (closeNameEnd < m_text.length() && 
                                               m_text[closeNameEnd] != ' ' && m_text[closeNameEnd] != '\t' &&
                                               m_text[closeNameEnd] != '\n' && m_text[closeNameEnd] != '\r' &&
                                               m_text[closeNameEnd] != '>')
                                        {
                                            ++closeNameEnd;
                                        }
                                        wxString closeName = m_text.Mid(closeNameStart, closeNameEnd - closeNameStart);
                                        
                                        if (closeName == child.name)
                                        {
                                            --depth;
                                            if (depth == 0)
                                            {
                                                // Skip to end of closing tag
                                                while (scanPos < m_text.length() && m_text[scanPos] != '>')
                                                    ++scanPos;
                                                if (scanPos < m_text.length())
                                                    ++scanPos;
                                                
                                                child.endPos = scanPos;
                                                child.endLine = GetLineAtPosition(scanPos - 1);
                                                break;
                                            }
                                        }
                                        // Skip past closing tag
                                        scanPos = closeNameEnd;
                                        while (scanPos < m_text.length() && m_text[scanPos] != '>')
                                            ++scanPos;
                                        if (scanPos < m_text.length())
                                            ++scanPos;
                                        continue;
                                    }
                                    else if (m_text[scanPos + 1] == '!' || m_text[scanPos + 1] == '?')
                                    {
                                        // Comment, CDATA, or PI - skip
                                        size_t newPos = SkipComment(scanPos);
                                        if (newPos == scanPos)
                                            newPos = SkipCData(scanPos);
                                        if (newPos == scanPos)
                                            newPos = SkipDeclaration(scanPos);
                                        if (newPos > scanPos)
                                        {
                                            scanPos = newPos;
                                            continue;
                                        }
                                        ++scanPos;
                                    }
                                    else
                                    {
                                        // Nested element opening tag
                                        foundChildren = true;
                                        ++scanPos;
                                        
                                        // Parse this nested element's opening
                                        wxString nestedName;
                                        while (scanPos < m_text.length() && 
                                               m_text[scanPos] != ' ' && m_text[scanPos] != '\t' &&
                                               m_text[scanPos] != '\n' && m_text[scanPos] != '\r' &&
                                               m_text[scanPos] != '>' && m_text[scanPos] != '/')
                                        {
                                            nestedName += m_text[scanPos];
                                            ++scanPos;
                                        }
                                        
                                        // Check if self-closing
                                        bool inQuote = false;
                                        wxChar quoteChar = 0;
                                        bool isSelfClosing = false;
                                        while (scanPos < m_text.length())
                                        {
                                            if (inQuote)
                                            {
                                                if (m_text[scanPos] == quoteChar)
                                                    inQuote = false;
                                            }
                                            else
                                            {
                                                if (m_text[scanPos] == '"' || m_text[scanPos] == '\'')
                                                {
                                                    inQuote = true;
                                                    quoteChar = m_text[scanPos];
                                                }
                                                else if (m_text[scanPos] == '/')
                                                {
                                                    if (scanPos + 1 < m_text.length() && m_text[scanPos + 1] == '>')
                                                    {
                                                        isSelfClosing = true;
                                                        scanPos += 2;
                                                        break;
                                                    }
                                                }
                                                else if (m_text[scanPos] == '>')
                                                {
                                                    ++scanPos;
                                                    break;
                                                }
                                            }
                                            ++scanPos;
                                        }
                                        
                                        if (!isSelfClosing && nestedName == child.name)
                                        {
                                            ++depth;
                                        }
                                        continue;
                                    }
                                }
                                else
                                {
                                    ++scanPos;
                                }
                            }
                            else
                            {
                                ++scanPos;
                            }
                        }
                        
                        child.hasChildren = foundChildren;
                        
                        // Extract text content if no children
                        if (!foundChildren)
                        {
                            child.textContent = ExtractTextContent(contentStart, child.endPos - child.name.length() - 3);
                        }
                    }
                    
                    children.push_back(child);
                    pos = child.endPos;
                }
            }
            else
            {
                ++pos;
            }
        }
        else
        {
            ++pos;
        }
    }
    
    return children;
}

size_t XmlStreamParser::FindElementEnd(size_t startPos)
{
    size_t pos = startPos;
    XmlElementInfo info = ParseElement(pos);
    return info.endPos;
}

