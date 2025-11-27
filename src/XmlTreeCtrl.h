#ifndef XMLTREECTRL_H
#define XMLTREECTRL_H

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/menu.h>
#include <tinyxml2.h>
#include <unordered_map>
#include <memory>

// Custom hash function for wxTreeItemId
struct TreeItemIdHash {
    size_t operator()(const wxTreeItemId& id) const {
        return std::hash<void*>()(id.GetID());
    }
};

// Custom equality function for wxTreeItemId
struct TreeItemIdEqual {
    bool operator()(const wxTreeItemId& lhs, const wxTreeItemId& rhs) const {
        return lhs.GetID() == rhs.GetID();
    }
};

class XmlTreeCtrl : public wxTreeCtrl
{
public:
    XmlTreeCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);

    bool LoadXmlFile(const wxString& filePath);
    int GetLineNumber(const wxTreeItemId& item) const;

private:
    void OnItemRightClick(wxTreeEvent& event);
    void OnItemActivated(wxTreeEvent& event);
    void OnItemHover(wxTreeEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnShowTextContent(wxCommandEvent& event);
    void OnItemExpanding(wxTreeEvent& event);
    
    // Create a single tree item for an element (without children)
    wxTreeItemId CreateElementItem(tinyxml2::XMLElement* element, const wxTreeItemId& parent);
    
    // Populate the direct children of an item (lazy loading)
    void PopulateChildren(const wxTreeItemId& item);
    
    // Check if an element has child elements (for showing expand button)
    bool HasChildElements(tinyxml2::XMLElement* element) const;
    
    wxString FormatNodeLabel(tinyxml2::XMLElement* element) const;
    wxString GetNodeAttributes(tinyxml2::XMLElement* element) const;
    wxString GetNodeTextContent(tinyxml2::XMLElement* element) const;

    std::unique_ptr<tinyxml2::XMLDocument> m_xmlDoc;
    wxTreeItemId m_rootItem;
    
    // Use unordered_map for O(1) lookup instead of std::map's O(log n)
    std::unordered_map<wxTreeItemId, int, TreeItemIdHash, TreeItemIdEqual> m_itemLineMap;
    std::unordered_map<wxTreeItemId, tinyxml2::XMLNode*, TreeItemIdHash, TreeItemIdEqual> m_itemNodeMap;
    std::unordered_map<wxTreeItemId, bool, TreeItemIdHash, TreeItemIdEqual> m_itemPopulated; // Track if children are loaded
    
    wxTreeItemId m_contextMenuItem;
    wxString m_filePath;
};

#endif // XMLTREECTRL_H
