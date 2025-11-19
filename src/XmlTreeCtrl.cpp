#include "XmlTreeCtrl.h"
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/textfile.h>
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

    if (!m_xmlDoc.Load(filePath))
    {
        return false;
    }

    // Build line number map by parsing the file
    std::map<wxString, int> tagLineMap;
    wxTextFile textFile;
    if (textFile.Open(filePath))
    {
        wxString line;
        for (size_t i = 0; i < textFile.GetLineCount(); ++i)
        {
            line = textFile[i];
            // Find opening tags
            size_t pos = 0;
            while ((pos = line.find('<', pos)) != wxString::npos)
            {
                if (pos + 1 < line.Length() && line[pos + 1] != '/' && line[pos + 1] != '!' && line[pos + 1] != '?')
                {
                    size_t tagEnd = line.find_first_of(" >", pos + 1);
                    if (tagEnd != wxString::npos)
                    {
                        wxString tag = line.SubString(pos + 1, tagEnd - 1);
                        tag.Trim(true).Trim(false);
                        if (!tag.IsEmpty() && tagLineMap.find(tag) == tagLineMap.end())
                        {
                            tagLineMap[tag] = static_cast<int>(i + 1);
                        }
                    }
                }
                pos++;
            }
        }
        textFile.Close();
    }

    // Create root item (hidden)
    m_rootItem = AddRoot("XML Document");

    // Build tree from XML document
    wxXmlNode* root = m_xmlDoc.GetRoot();
    if (root)
    {
        int lineNumber = 1;
        BuildTree(root, m_rootItem, lineNumber, tagLineMap);
        ExpandAll();
    }

    return true;
}

wxTreeItemId XmlTreeCtrl::BuildTree(wxXmlNode* node, const wxTreeItemId& parent, int& lineNumber, const std::map<wxString, int>& tagLineMap)
{
    if (!node)
        return wxTreeItemId();

    wxTreeItemId item;
    wxString label = FormatNodeLabel(node);

    item = AppendItem(parent, label);

    // Store XML node pointer
    m_itemNodeMap[item] = node;

    // Try to find line number from tag map, otherwise use approximation
    wxString tagName = node->GetName();
    auto it = tagLineMap.find(tagName);
    if (it != tagLineMap.end())
    {
        m_itemLineMap[item] = it->second;
        lineNumber = it->second; // Update current line number
    }
    else
    {
        m_itemLineMap[item] = lineNumber;
    }

    // Process attributes
    if (node->GetAttributes())
    {
        wxXmlAttribute* attr = node->GetAttributes();
        while (attr)
        {
            wxString attrLabel = wxString::Format("@%s = \"%s\"", attr->GetName(), attr->GetValue());
            wxTreeItemId attrItem = AppendItem(item, attrLabel);
            m_itemLineMap[attrItem] = m_itemLineMap[item]; // Attributes are on the same line
            attr = attr->GetNext();
        }
    }

    // Process child nodes
    wxXmlNode* child = node->GetChildren();
    while (child)
    {
        if (child->GetType() == wxXML_ELEMENT_NODE)
        {
            lineNumber++;
            BuildTree(child, item, lineNumber, tagLineMap);
        }
        else if (child->GetType() == wxXML_TEXT_NODE)
        {
            wxString text = child->GetContent();
            text.Trim(true).Trim(false);
            if (!text.IsEmpty())
            {
                wxString textLabel = wxString::Format("[Text: %s]", text.Left(50));
                if (text.Length() > 50)
                    textLabel += "...";
                wxTreeItemId textItem = AppendItem(item, textLabel);
                m_itemLineMap[textItem] = m_itemLineMap[item];
                m_itemNodeMap[textItem] = child; // Store text node
            }
        }
        child = child->GetNext();
    }

    return item;
}

wxString XmlTreeCtrl::FormatNodeLabel(wxXmlNode* node) const
{
    if (!node)
        return wxEmptyString;

    wxString label = node->GetName();

    // Add attribute count if any
    int attrCount = 0;
    wxXmlAttribute* attr = node->GetAttributes();
    while (attr)
    {
        attrCount++;
        attr = attr->GetNext();
    }

    if (attrCount > 0)
    {
        label += wxString::Format(" (%d attr)", attrCount);
    }

    return label;
}

wxString XmlTreeCtrl::GetNodeAttributes(wxXmlNode* node) const
{
    if (!node || !node->GetAttributes())
        return wxEmptyString;

    wxString attributes;
    wxXmlAttribute* attr = node->GetAttributes();
    bool first = true;

    while (attr)
    {
        if (!first)
            attributes += ", ";
        attributes += wxString::Format("%s=\"%s\"", attr->GetName(), attr->GetValue());
        first = false;
        attr = attr->GetNext();
    }

    return attributes;
}

wxString XmlTreeCtrl::GetNodeTextContent(wxXmlNode* node) const
{
    if (!node)
        return wxEmptyString;

    wxString content;
    wxXmlNode* child = node->GetChildren();

    while (child)
    {
        if (child->GetType() == wxXML_TEXT_NODE)
        {
            content += child->GetContent();
        }
        child = child->GetNext();
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
            wxXmlNode* node = nodeIt->second;
            if (node->GetType() == wxXML_ELEMENT_NODE)
            {
                wxString attributes = GetNodeAttributes(node);
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

    wxXmlNode* node = nodeIt->second;
    if (!node)
        return;

    wxString textContent;
    
    // If it's a text node, get its content directly
    if (node->GetType() == wxXML_TEXT_NODE)
    {
        textContent = node->GetContent();
    }
    else
    {
        // For element nodes, get all text content from children
        textContent = GetNodeTextContent(node);
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

