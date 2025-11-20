#ifndef XMLTREECTRL_H
#define XMLTREECTRL_H

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/menu.h>
#include <tinyxml2.h>
#include <map>
#include <memory>

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
    wxTreeItemId BuildTree(tinyxml2::XMLElement* element, const wxTreeItemId& parent);
    wxTreeItemId BuildTree(tinyxml2::XMLNode* node, const wxTreeItemId& parent);
    wxString FormatNodeLabel(tinyxml2::XMLElement* element) const;
    wxString GetNodeAttributes(tinyxml2::XMLElement* element) const;
    wxString GetNodeTextContent(tinyxml2::XMLElement* element) const;

    std::unique_ptr<tinyxml2::XMLDocument> m_xmlDoc;
    wxTreeItemId m_rootItem;
    std::map<wxTreeItemId, int> m_itemLineMap; // Map tree items to line numbers
    std::map<wxTreeItemId, tinyxml2::XMLNode*> m_itemNodeMap; // Map tree items to XML nodes
    wxTreeItemId m_contextMenuItem;
    wxString m_filePath; // Store file path for line number calculation
};

#endif // XMLTREECTRL_H

