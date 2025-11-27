#ifndef VIRTUALXMLTREE_H
#define VIRTUALXMLTREE_H

#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/thread.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include "XmlStreamParser.h"

// Custom events
wxDECLARE_EVENT(wxEVT_VIRTUALTREE_SELECTION_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_CHILDREN_LOADED, wxThreadEvent);

// Forward declarations
class VirtualXmlTreeModel;
class VirtualXmlTree;

// Node in the virtual tree (lightweight - no wxWidgets overhead)
// Declared first because other classes reference it
struct VirtualTreeNode
{
    wxString label;              // Display text
    int lineNumber;              // Line in source
    size_t startPos;             // Position in content
    size_t endPos;               // End position
    bool hasChildren;            // Can be expanded
    bool childrenLoaded;         // Children have been fetched
    bool isMorePlaceholder;      // True if this is a "load more" placeholder
    size_t moreStartIndex;       // For placeholder: index to start loading from
    size_t moreTotalCount;       // For placeholder: total count of items
    VirtualTreeNode* parent;     // Parent node (null for root)
    std::vector<std::unique_ptr<VirtualTreeNode>> children;
    
    VirtualTreeNode() : lineNumber(0), startPos(0), endPos(0), 
                        hasChildren(false), childrenLoaded(false),
                        isMorePlaceholder(false), moreStartIndex(0), moreTotalCount(0),
                        parent(nullptr) {}
};

// Result of async child loading
struct ChildLoadResult
{
    VirtualTreeNode* parentNode;
    std::vector<XmlElementInfo> children;
};

// Worker thread for loading children
class ChildLoaderThread : public wxThread
{
public:
    ChildLoaderThread(VirtualXmlTree* tree, VirtualTreeNode* node,
                      const wxString& content, size_t startPos, size_t endPos);
    
protected:
    virtual ExitCode Entry() override;
    
private:
    VirtualXmlTree* m_tree;
    VirtualTreeNode* m_node;
    wxString m_content;
    size_t m_startPos;
    size_t m_endPos;
};

// Virtual tree control for XML - only renders visible items
class VirtualXmlTree : public wxDataViewCtrl
{
public:
    VirtualXmlTree(wxWindow* parent, wxWindowID id = wxID_ANY,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize);
    
    // Load XML content
    bool LoadFromString(const wxString& content);
    
    // Load with preloaded data from background thread
    bool LoadWithPreloadedData(const wxString& content,
                               const std::vector<XmlElementInfo>& rootElements,
                               const std::vector<XmlElementInfo>& firstLevelChildren);
    
    // Get line number for selected item
    int GetSelectedLineNumber() const;
    
    // Expand tree to show the node containing a specific line
    void ExpandToLine(int lineNumber);
    
    // Get the model
    VirtualXmlTreeModel* GetModel() const { return m_model; }

    // Called when children are loaded in background
    void OnChildrenLoaded(wxThreadEvent& event);

private:
    void OnSelectionChanged(wxDataViewEvent& event);
    void OnItemExpanding(wxDataViewEvent& event);
    void OnItemActivated(wxDataViewEvent& event);
    void OnContextMenu(wxDataViewEvent& event);
    
    void StartChildLoading(VirtualTreeNode* node);
    void LoadMoreChildren(VirtualTreeNode* placeholderNode);
    void AddLoadingPlaceholder(VirtualTreeNode* node);
    void RemoveLoadingPlaceholder(VirtualTreeNode* node);
    void UpdateBackgroundCursor();
    
    VirtualXmlTreeModel* m_model;
    wxString m_content;
    std::shared_ptr<XmlStreamParser> m_parser;
    
    // Track nodes being loaded to prevent duplicate loads
    std::unordered_set<VirtualTreeNode*> m_loadingNodes;
    wxCriticalSection m_loadingLock;
    
    // Background cursor management
    wxCursor m_originalCursor;
    bool m_showingBusyCursor;
};

// The virtual model - provides data on demand
class VirtualXmlTreeModel : public wxDataViewModel
{
public:
    VirtualXmlTreeModel();
    virtual ~VirtualXmlTreeModel();
    
    // Set content and parser (shared ownership)
    void SetContent(const wxString& content, std::shared_ptr<XmlStreamParser> parser);
    
    // Set root nodes
    void SetRootNodes(std::vector<std::unique_ptr<VirtualTreeNode>> roots);
    
    // Add children to a node
    void AddChildrenToNode(VirtualTreeNode* node, const std::vector<XmlElementInfo>& children);
    
    // Notify view about preloaded children in a node
    void NotifyChildrenLoaded(VirtualTreeNode* node);
    
    // Get node from item
    VirtualTreeNode* GetNode(const wxDataViewItem& item) const;
    
    // Find the node closest to a given line number
    VirtualTreeNode* FindNodeByLine(int lineNumber) const;
    
    // wxDataViewModel interface
    virtual unsigned int GetColumnCount() const override { return 1; }
    virtual wxString GetColumnType(unsigned int col) const override { return "string"; }
    virtual void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
    virtual bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override { return false; }
    virtual wxDataViewItem GetParent(const wxDataViewItem& item) const override;
    virtual bool IsContainer(const wxDataViewItem& item) const override;
    virtual bool HasContainerColumns(const wxDataViewItem& item) const override { return false; }
    virtual unsigned int GetChildren(const wxDataViewItem& parent, wxDataViewItemArray& array) const override;
    
    // Get root node
    VirtualTreeNode* GetRootNode() const { return m_rootNode.get(); }
    
private:
    std::unique_ptr<VirtualTreeNode> m_rootNode;
    wxString m_content;
    std::shared_ptr<XmlStreamParser> m_parser;  // Shared with VirtualXmlTree
};

#endif // VIRTUALXMLTREE_H
