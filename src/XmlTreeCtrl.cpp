#include "XmlTreeCtrl.h"
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <sstream>
#include <map>

XmlTreeCtrl::XmlTreeCtrl(wxWindow* parent, wxWindowID id,
                         const wxPoint& pos, const wxSize& size, long style)
    : wxTreeCtrl(parent, id, pos, size, style)
{
    Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &XmlTreeCtrl::OnItemRightClick, this);
    Bind(wxEVT_TREE_ITEM_ACTIVATED, &XmlTreeCtrl::OnItemActivated, this);
    Bind(wxEVT_MOTION, &XmlTreeCtrl::OnMouseMove, this);
}

bool XmlTreeCtrl::LoadXmlFile(const wxString& filePath)
{
    DeleteAllItems();
    m_itemLineMap.clear();
    m_itemNodeMap.clear();
    m_filePath = filePath;

    // Parse XML with TinyXML-2
    m_xmlDoc = std::make_unique<tinyxml2::XMLDocument>();
    
    // Convert wxString to UTF-8 for TinyXML-2
    wxCharBuffer utf8Buffer = filePath.ToUTF8();
    tinyxml2::XMLError error = m_xmlDoc->LoadFile(utf8Buffer.data());
    
    if (error != tinyxml2::XML_SUCCESS)
    {
        return false;
    }

    // Freeze the tree control to prevent UI updates during building
    Freeze();

    // Create root item (hidden)
    m_rootItem = AddRoot("XML Document");

    // Build tree from XML document
    tinyxml2::XMLElement* root = m_xmlDoc->FirstChildElement();
    if (root)
    {
        BuildTree(root, m_rootItem);
        // Don't expand all - let users expand nodes as needed for better performance
        // Only expand the root level for better initial view
        if (m_rootItem.IsOk())
        {
            Expand(m_rootItem);
        }
    }

    // Thaw the tree control to update the UI
    Thaw();

    return true;
}

wxTreeItemId XmlTreeCtrl::BuildTree(tinyxml2::XMLElement* element, const wxTreeItemId& parent)
{
    if (!element)
        return wxTreeItemId();

    wxTreeItemId item;
    wxString label = FormatNodeLabel(element);

    item = AppendItem(parent, label);

    // Store XML node pointer
    m_itemNodeMap[item] = element;

    // Use TinyXML-2's built-in line number information
    int lineNumber = element->GetLineNum();
    if (lineNumber > 0)
    {
        m_itemLineMap[item] = lineNumber;
    }
    else
    {
        // Fallback if line number not available
        m_itemLineMap[item] = 1;
    }

    // Process attributes
    const tinyxml2::XMLAttribute* attr = element->FirstAttribute();
    while (attr)
    {
        wxString attrName = wxString::FromUTF8(attr->Name());
        wxString attrValue = wxString::FromUTF8(attr->Value());
        wxString attrLabel = wxString::Format("@%s = \"%s\"", attrName, attrValue);
        wxTreeItemId attrItem = AppendItem(item, attrLabel);
        // Attributes are on the same line as the element
        m_itemLineMap[attrItem] = m_itemLineMap[item];
        attr = attr->Next();
    }

    // Process child nodes
    tinyxml2::XMLNode* child = element->FirstChild();
    while (child)
    {
        if (child->ToElement())
        {
            // Element node
            BuildTree(child->ToElement(), item);
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
                    // Use line number from text node if available
                    int textLineNumber = child->GetLineNum();
                    if (textLineNumber > 0)
                    {
                        m_itemLineMap[textItem] = textLineNumber;
                    }
                    else
                    {
                        m_itemLineMap[textItem] = m_itemLineMap[item];
                    }
                    m_itemNodeMap[textItem] = child; // Store text node
                }
            }
        }
        child = child->NextSibling();
    }

    return item;
}

wxTreeItemId XmlTreeCtrl::BuildTree(tinyxml2::XMLNode* node, const wxTreeItemId& parent)
{
    if (!node)
        return wxTreeItemId();

    tinyxml2::XMLElement* element = node->ToElement();
    if (element)
    {
        return BuildTree(element, parent);
    }

    return wxTreeItemId();
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

