#include "XmlTreeCtrl.h"
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <sstream>

XmlTreeCtrl::XmlTreeCtrl(wxWindow* parent, wxWindowID id,
                         const wxPoint& pos, const wxSize& size, long style)
    : wxTreeCtrl(parent, id, pos, size, style)
{
    Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &XmlTreeCtrl::OnItemRightClick, this);
    Bind(wxEVT_TREE_ITEM_ACTIVATED, &XmlTreeCtrl::OnItemActivated, this);
    Bind(wxEVT_MOTION, &XmlTreeCtrl::OnMouseMove, this);
    // Bind the expanding event for lazy loading
    Bind(wxEVT_TREE_ITEM_EXPANDING, &XmlTreeCtrl::OnItemExpanding, this);
}

bool XmlTreeCtrl::LoadXmlFile(const wxString& filePath)
{
    // Freeze to prevent UI updates
    Freeze();
    
    DeleteAllItems();
    m_itemLineMap.clear();
    m_itemNodeMap.clear();
    m_itemPopulated.clear();
    m_filePath = filePath;

    // Parse XML with TinyXML-2
    m_xmlDoc = std::make_unique<tinyxml2::XMLDocument>();
    
    // Convert wxString to UTF-8 for TinyXML-2
    wxCharBuffer utf8Buffer = filePath.ToUTF8();
    tinyxml2::XMLError error = m_xmlDoc->LoadFile(utf8Buffer.data());
    
    if (error != tinyxml2::XML_SUCCESS)
    {
        Thaw();
        return false;
    }

    // Create root item (hidden)
    m_rootItem = AddRoot("XML Document");
    m_itemPopulated[m_rootItem] = true; // Mark as populated since we'll add children now

    // Only add top-level elements (not their children)
    tinyxml2::XMLElement* root = m_xmlDoc->FirstChildElement();
    if (root)
    {
        // Create the root XML element item
        wxTreeItemId rootElementItem = CreateElementItem(root, m_rootItem);
        
        // Also process any sibling root elements (multiple root elements in some XML-like files)
        tinyxml2::XMLElement* sibling = root->NextSiblingElement();
        while (sibling)
        {
            CreateElementItem(sibling, m_rootItem);
            sibling = sibling->NextSiblingElement();
        }
        
        // Expand the hidden root to show the first level
        Expand(m_rootItem);
    }

    Thaw();
    return true;
}

wxTreeItemId XmlTreeCtrl::CreateElementItem(tinyxml2::XMLElement* element, const wxTreeItemId& parent)
{
    if (!element)
        return wxTreeItemId();

    wxString label = FormatNodeLabel(element);
    wxTreeItemId item = AppendItem(parent, label);

    // Store XML node pointer
    m_itemNodeMap[item] = element;

    // Store line number
    int lineNumber = element->GetLineNum();
    m_itemLineMap[item] = (lineNumber > 0) ? lineNumber : 1;

    // Mark as not yet populated
    m_itemPopulated[item] = false;

    // Check if this element has children that need to be loaded later
    // If so, set the item to show an expand button
    if (HasChildElements(element))
    {
        SetItemHasChildren(item, true);
    }

    return item;
}

bool XmlTreeCtrl::HasChildElements(tinyxml2::XMLElement* element) const
{
    if (!element)
        return false;

    // Check for attributes (we display these as children)
    if (element->FirstAttribute())
        return true;

    // Check for child nodes (elements or non-empty text)
    tinyxml2::XMLNode* child = element->FirstChild();
    while (child)
    {
        if (child->ToElement())
            return true;
        
        if (child->ToText())
        {
            const char* text = child->ToText()->Value();
            if (text)
            {
                wxString textStr = wxString::FromUTF8(text);
                textStr.Trim(true).Trim(false);
                if (!textStr.IsEmpty())
                    return true;
            }
        }
        child = child->NextSibling();
    }

    return false;
}

void XmlTreeCtrl::OnItemExpanding(wxTreeEvent& event)
{
    wxTreeItemId item = event.GetItem();
    
    // Check if already populated
    auto it = m_itemPopulated.find(item);
    if (it != m_itemPopulated.end() && it->second)
    {
        // Already populated, nothing to do
        return;
    }

    // Populate children now (lazy loading)
    PopulateChildren(item);
}

void XmlTreeCtrl::PopulateChildren(const wxTreeItemId& item)
{
    if (!item.IsOk())
        return;

    // Get the XML node for this item
    auto nodeIt = m_itemNodeMap.find(item);
    if (nodeIt == m_itemNodeMap.end() || !nodeIt->second)
        return;

    tinyxml2::XMLElement* element = nodeIt->second->ToElement();
    if (!element)
        return;

    // Mark as populated first to avoid re-entry
    m_itemPopulated[item] = true;

    // Freeze for batch updates
    Freeze();

    // Add attributes as child items
    const tinyxml2::XMLAttribute* attr = element->FirstAttribute();
    while (attr)
    {
        wxString attrName = wxString::FromUTF8(attr->Name());
        wxString attrValue = wxString::FromUTF8(attr->Value());
        wxString attrLabel = wxString::Format("@%s = \"%s\"", attrName, attrValue);
        wxTreeItemId attrItem = AppendItem(item, attrLabel);
        // Attributes are on the same line as the element
        m_itemLineMap[attrItem] = m_itemLineMap[item];
        m_itemPopulated[attrItem] = true; // Attributes have no children
        attr = attr->Next();
    }

    // Add child elements and text nodes
    tinyxml2::XMLNode* child = element->FirstChild();
    while (child)
    {
        if (child->ToElement())
        {
            // Create child element item (lazy - won't load its children yet)
            CreateElementItem(child->ToElement(), item);
        }
        else if (child->ToText())
        {
            // Text node
            const char* textContent = child->ToText()->Value();
            if (textContent)
            {
                wxString text = wxString::FromUTF8(textContent);
                text.Trim(true).Trim(false);
                if (!text.IsEmpty())
                {
                    wxString textLabel = wxString::Format("[Text: %s]", text.Left(50));
                    if (text.Length() > 50)
                        textLabel += "...";
                    wxTreeItemId textItem = AppendItem(item, textLabel);
                    
                    int textLineNumber = child->GetLineNum();
                    m_itemLineMap[textItem] = (textLineNumber > 0) ? textLineNumber : m_itemLineMap[item];
                    m_itemNodeMap[textItem] = child;
                    m_itemPopulated[textItem] = true; // Text nodes have no children
                }
            }
        }
        child = child->NextSibling();
    }

    Thaw();
}

wxString XmlTreeCtrl::FormatNodeLabel(tinyxml2::XMLElement* element) const
{
    if (!element)
        return wxEmptyString;

    wxString label = wxString::FromUTF8(element->Name());

    // Add attribute count if any
    int attrCount = 0;
    const tinyxml2::XMLAttribute* attr = element->FirstAttribute();
    while (attr)
    {
        attrCount++;
        attr = attr->Next();
    }

    if (attrCount > 0)
    {
        label += wxString::Format(" (%d attr)", attrCount);
    }

    return label;
}

wxString XmlTreeCtrl::GetNodeAttributes(tinyxml2::XMLElement* element) const
{
    if (!element)
        return wxEmptyString;

    wxString attributes;
    const tinyxml2::XMLAttribute* attr = element->FirstAttribute();
    bool first = true;

    while (attr)
    {
        if (!first)
            attributes += ", ";
        wxString attrName = wxString::FromUTF8(attr->Name());
        wxString attrValue = wxString::FromUTF8(attr->Value());
        attributes += wxString::Format("%s=\"%s\"", attrName, attrValue);
        first = false;
        attr = attr->Next();
    }

    return attributes;
}

wxString XmlTreeCtrl::GetNodeTextContent(tinyxml2::XMLElement* element) const
{
    if (!element)
        return wxEmptyString;

    wxString content;
    tinyxml2::XMLNode* child = element->FirstChild();

    while (child)
    {
        if (child->ToText())
        {
            const char* textValue = child->ToText()->Value();
            if (textValue)
            {
                content += wxString::FromUTF8(textValue);
            }
        }
        child = child->NextSibling();
    }

    return content.Trim(true).Trim(false);
}

int XmlTreeCtrl::GetLineNumber(const wxTreeItemId& item) const
{
    auto it = m_itemLineMap.find(item);
    if (it != m_itemLineMap.end())
    {
        return it->second;
    }
    return -1;
}

void XmlTreeCtrl::OnItemRightClick(wxTreeEvent& event)
{
    m_contextMenuItem = event.GetItem();
    if (!m_contextMenuItem.IsOk())
        return;

    wxMenu menu;
    menu.Append(wxID_ANY, "Show Text Content");
    menu.Bind(wxEVT_MENU, &XmlTreeCtrl::OnShowTextContent, this);
    PopupMenu(&menu);
}

void XmlTreeCtrl::OnItemActivated(wxTreeEvent& event)
{
    // This is handled by the parent frame's OnTreeItemSelected
    event.Skip();
}

void XmlTreeCtrl::OnItemHover(wxTreeEvent& event)
{
    // Could show attributes in tooltip or status bar
    event.Skip();
}

void XmlTreeCtrl::OnMouseMove(wxMouseEvent& event)
{
    int flags = 0;
    wxTreeItemId item = HitTest(event.GetPosition(), flags);
    
    if (item.IsOk() && (flags & wxTREE_HITTEST_ONITEMLABEL || flags & wxTREE_HITTEST_ONITEMICON))
    {
        // Get the XML node for this item
        auto nodeIt = m_itemNodeMap.find(item);
        if (nodeIt != m_itemNodeMap.end() && nodeIt->second)
        {
            tinyxml2::XMLNode* node = nodeIt->second;
            tinyxml2::XMLElement* element = node->ToElement();
            if (element)
            {
                wxString attributes = GetNodeAttributes(element);
                if (!attributes.IsEmpty())
                {
                    SetToolTip(attributes);
                }
                else
                {
                    UnsetToolTip();
                }
            }
            else
            {
                UnsetToolTip();
            }
        }
        else
        {
            UnsetToolTip();
        }
    }
    else
    {
        UnsetToolTip();
    }
    
    event.Skip();
}

void XmlTreeCtrl::OnShowTextContent(wxCommandEvent& event)
{
    if (!m_contextMenuItem.IsOk())
        return;

    // Get the XML node for this tree item
    auto nodeIt = m_itemNodeMap.find(m_contextMenuItem);
    if (nodeIt == m_itemNodeMap.end())
        return;

    tinyxml2::XMLNode* node = nodeIt->second;
    if (!node)
        return;

    wxString textContent;
    
    // If it's a text node, get its content directly
    tinyxml2::XMLText* textNode = node->ToText();
    if (textNode)
    {
        const char* textValue = textNode->Value();
        if (textValue)
        {
            textContent = wxString::FromUTF8(textValue);
        }
    }
    else
    {
        // For element nodes, get all text content from children
        tinyxml2::XMLElement* element = node->ToElement();
        if (element)
        {
            textContent = GetNodeTextContent(element);
        }
    }

    if (textContent.IsEmpty())
    {
        wxMessageBox("No text content found for this element.", "Text Content", wxOK | wxICON_INFORMATION);
    }
    else
    {
        // Show in a scrollable dialog for long content
        wxDialog dlg(this, wxID_ANY, "Text Content", wxDefaultPosition, wxSize(500, 400));
        wxTextCtrl* textCtrl = new wxTextCtrl(&dlg, wxID_ANY, textContent,
                                              wxDefaultPosition, wxDefaultSize,
                                              wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(textCtrl, 1, wxEXPAND | wxALL, 10);
        wxButton* closeBtn = new wxButton(&dlg, wxID_OK, "Close");
        sizer->Add(closeBtn, 0, wxALIGN_CENTER | wxALL, 10);
        dlg.SetSizer(sizer);
        dlg.ShowModal();
    }
}
