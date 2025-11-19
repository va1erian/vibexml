#ifndef XMLTREECTRL_H
#define XMLTREECTRL_H

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/xml/xml.h>
#include <wx/menu.h>
#include <map>

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
    wxTreeItemId BuildTree(wxXmlNode* node, const wxTreeItemId& parent, int& lineNumber, const std::map<wxString, int>& tagLineMap);
    wxString FormatNodeLabel(wxXmlNode* node) const;
    wxString GetNodeAttributes(wxXmlNode* node) const;
    wxString GetNodeTextContent(wxXmlNode* node) const;

    wxXmlDocument m_xmlDoc;
    wxTreeItemId m_rootItem;
    std::map<wxTreeItemId, int> m_itemLineMap; // Map tree items to line numbers
    std::map<wxTreeItemId, wxXmlNode*> m_itemNodeMap; // Map tree items to XML nodes
    wxTreeItemId m_contextMenuItem;
    wxString m_filePath; // Store file path for line number calculation
};

#endif // XMLTREECTRL_H

