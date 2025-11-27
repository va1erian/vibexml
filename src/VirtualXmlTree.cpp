#include "VirtualXmlTree.h"
#include <wx/clipbrd.h>

// Define custom events
wxDEFINE_EVENT(wxEVT_VIRTUALTREE_SELECTION_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CHILDREN_LOADED, wxThreadEvent);

// ============================================================================
// ChildLoaderThread
// ============================================================================

ChildLoaderThread::ChildLoaderThread(VirtualXmlTree* tree, VirtualTreeNode* node,
                                      const wxString& content, size_t startPos, size_t endPos)
    : wxThread(wxTHREAD_DETACHED),
      m_tree(tree),
      m_node(node),
      m_content(content),
      m_startPos(startPos),
      m_endPos(endPos)
{
}

wxThread::ExitCode ChildLoaderThread::Entry()
{
    // Create a parser for this thread
    XmlStreamParser parser;
    parser.SetSource(m_content);
    
    // Parse children
    std::vector<XmlElementInfo> children = parser.GetChildElements(m_startPos, m_endPos);
    
    // Send result back to main thread
    ChildLoadResult result;
    result.parentNode = m_node;
    result.children = std::move(children);
    
    wxThreadEvent* event = new wxThreadEvent(wxEVT_CHILDREN_LOADED);
    event->SetPayload(result);
    wxQueueEvent(m_tree, event);
    
    return nullptr;
}

// ============================================================================
// VirtualXmlTreeModel
// ============================================================================

VirtualXmlTreeModel::VirtualXmlTreeModel()
    : m_parser(nullptr)
{
    m_rootNode = std::make_unique<VirtualTreeNode>();
    m_rootNode->label = "Root";
    m_rootNode->childrenLoaded = true;
}

VirtualXmlTreeModel::~VirtualXmlTreeModel()
{
}

void VirtualXmlTreeModel::SetContent(const wxString& content, XmlStreamParser* parser)
{
    m_content = content;
    m_parser = parser;
}

void VirtualXmlTreeModel::SetRootNodes(std::vector<std::unique_ptr<VirtualTreeNode>> roots)
{
    // Clear existing children
    m_rootNode->children.clear();
    
    // First notify that we're cleared
    Cleared();
    
    // Now add the new roots
    m_rootNode->children = std::move(roots);
    for (auto& child : m_rootNode->children)
    {
        child->parent = m_rootNode.get();
    }
    
    // Notify view about each new item
    wxDataViewItem parentItem(nullptr);  // Top-level parent is null
    for (const auto& child : m_rootNode->children)
    {
        wxDataViewItem item(child.get());
        ItemAdded(parentItem, item);
    }
}

void VirtualXmlTreeModel::AddChildrenToNode(VirtualTreeNode* node, const std::vector<XmlElementInfo>& children)
{
    if (!node || node->childrenLoaded)
        return;
    
    node->childrenLoaded = true;
    
    wxDataViewItem parentItem(node);
    
    // Limit children to prevent memory issues
    const size_t MAX_CHILDREN = 10000;
    size_t count = std::min(children.size(), MAX_CHILDREN);
    
    for (size_t i = 0; i < count; ++i)
    {
        const auto& info = children[i];
        auto child = std::make_unique<VirtualTreeNode>();
        
        // Build label
        wxString label = "<" + info.name;
        if (!info.attributes.IsEmpty())
        {
            wxString attrs = info.attributes;
            if (attrs.length() > 40)
                attrs = attrs.Left(37) + "...";
            label += " " + attrs;
        }
        if (info.isSelfClosing)
        {
            label += "/>";
        }
        else
        {
            label += ">";
            if (!info.hasChildren && !info.textContent.IsEmpty())
            {
                wxString text = info.textContent;
                if (text.length() > 30)
                    text = text.Left(27) + "...";
                label += " " + text;
            }
        }
        
        child->label = label;
        child->lineNumber = info.startLine;
        child->startPos = info.startPos;
        child->endPos = info.endPos;
        child->hasChildren = info.hasChildren && !info.isSelfClosing;
        child->childrenLoaded = false;
        child->parent = node;
        
        VirtualTreeNode* childPtr = child.get();
        node->children.push_back(std::move(child));
        
        // Notify about each child
        ItemAdded(parentItem, wxDataViewItem(childPtr));
    }
    
    // Add indicator if we truncated
    if (children.size() > MAX_CHILDREN)
    {
        auto moreNode = std::make_unique<VirtualTreeNode>();
        moreNode->label = wxString::Format("... and %zu more items", 
                                            children.size() - MAX_CHILDREN);
        moreNode->lineNumber = 0;
        moreNode->hasChildren = false;
        moreNode->childrenLoaded = true;
        moreNode->parent = node;
        
        VirtualTreeNode* morePtr = moreNode.get();
        node->children.push_back(std::move(moreNode));
        ItemAdded(parentItem, wxDataViewItem(morePtr));
    }
}

void VirtualXmlTreeModel::NotifyChildrenLoaded(VirtualTreeNode* node)
{
    if (!node)
        return;
    
    wxDataViewItem parentItem(node);
    for (const auto& child : node->children)
    {
        ItemAdded(parentItem, wxDataViewItem(child.get()));
    }
}

VirtualTreeNode* VirtualXmlTreeModel::GetNode(const wxDataViewItem& item) const
{
    if (!item.IsOk())
        return m_rootNode.get();
    return static_cast<VirtualTreeNode*>(item.GetID());
}

void VirtualXmlTreeModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const
{
    VirtualTreeNode* node = GetNode(item);
    if (node)
    {
        variant = node->label;
    }
}

wxDataViewItem VirtualXmlTreeModel::GetParent(const wxDataViewItem& item) const
{
    VirtualTreeNode* node = GetNode(item);
    if (!node || node == m_rootNode.get())
        return wxDataViewItem(nullptr);
    
    if (node->parent == m_rootNode.get())
        return wxDataViewItem(nullptr);  // Top-level items have no visible parent
    
    return wxDataViewItem(node->parent);
}

bool VirtualXmlTreeModel::IsContainer(const wxDataViewItem& item) const
{
    VirtualTreeNode* node = GetNode(item);
    if (!node)
        return true;  // Root is always a container
    return node->hasChildren;
}

unsigned int VirtualXmlTreeModel::GetChildren(const wxDataViewItem& parent, wxDataViewItemArray& array) const
{
    VirtualTreeNode* node = GetNode(parent);
    if (!node)
        node = m_rootNode.get();
    
    for (const auto& child : node->children)
    {
        array.Add(wxDataViewItem(child.get()));
    }
    
    return node->children.size();
}

// ============================================================================
// VirtualXmlTree
// ============================================================================

VirtualXmlTree::VirtualXmlTree(wxWindow* parent, wxWindowID id,
                               const wxPoint& pos, const wxSize& size)
    : wxDataViewCtrl(parent, id, pos, size, 
                     wxDV_SINGLE | wxDV_ROW_LINES | wxDV_NO_HEADER),
      m_showingBusyCursor(false)
{
    m_model = new VirtualXmlTreeModel();
    AssociateModel(m_model);
    m_model->DecRef();  // Control now owns reference
    
    // Add a single column
    AppendTextColumn("Element", 0, wxDATAVIEW_CELL_INERT, 
                     wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT,
                     wxDATAVIEW_COL_RESIZABLE);
    
    // Save original cursor
    m_originalCursor = GetCursor();
    
    // Bind events
    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &VirtualXmlTree::OnSelectionChanged, this);
    Bind(wxEVT_DATAVIEW_ITEM_EXPANDING, &VirtualXmlTree::OnItemExpanding, this);
    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &VirtualXmlTree::OnContextMenu, this);
    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &VirtualXmlTree::OnItemActivated, this);
    Bind(wxEVT_CHILDREN_LOADED, &VirtualXmlTree::OnChildrenLoaded, this);
}

bool VirtualXmlTree::LoadFromString(const wxString& content)
{
    m_content = content;
    m_parser = std::make_unique<XmlStreamParser>();
    m_parser->SetSource(m_content);
    
    if (!m_parser->IsValid())
        return false;
    
    m_model->SetContent(m_content, m_parser.get());
    
    // Get top-level elements
    std::vector<XmlElementInfo> topElements = m_parser->GetTopLevelElements();
    
    std::vector<std::unique_ptr<VirtualTreeNode>> roots;
    for (const auto& info : topElements)
    {
        auto node = std::make_unique<VirtualTreeNode>();
        
        wxString label = "<" + info.name;
        if (!info.attributes.IsEmpty())
        {
            wxString attrs = info.attributes;
            if (attrs.length() > 40)
                attrs = attrs.Left(37) + "...";
            label += " " + attrs;
        }
        label += ">";
        
        node->label = label;
        node->lineNumber = info.startLine;
        node->startPos = info.startPos;
        node->endPos = m_content.length();  // Root extends to end
        node->hasChildren = !info.isSelfClosing;
        node->childrenLoaded = false;
        
        roots.push_back(std::move(node));
    }
    
    m_model->SetRootNodes(std::move(roots));
    
    return true;
}

bool VirtualXmlTree::LoadWithPreloadedData(const wxString& content,
                                            const std::vector<XmlElementInfo>& rootElements,
                                            const std::vector<XmlElementInfo>& firstLevelChildren)
{
    m_content = content;
    m_parser = std::make_unique<XmlStreamParser>();
    m_parser->SetSource(m_content);
    
    m_model->SetContent(m_content, m_parser.get());
    
    // Create root nodes
    std::vector<std::unique_ptr<VirtualTreeNode>> roots;
    for (const auto& info : rootElements)
    {
        auto node = std::make_unique<VirtualTreeNode>();
        
        wxString label = "<" + info.name;
        if (!info.attributes.IsEmpty())
        {
            wxString attrs = info.attributes;
            if (attrs.length() > 40)
                attrs = attrs.Left(37) + "...";
            label += " " + attrs;
        }
        label += ">";
        
        node->label = label;
        node->lineNumber = info.startLine;
        node->startPos = info.startPos;
        node->endPos = m_content.length();
        node->hasChildren = !firstLevelChildren.empty();
        node->childrenLoaded = true;  // We have the children
        
        // Add preloaded children
        for (const auto& childInfo : firstLevelChildren)
        {
            auto child = std::make_unique<VirtualTreeNode>();
            
            wxString childLabel = "<" + childInfo.name;
            if (!childInfo.attributes.IsEmpty())
            {
                wxString attrs = childInfo.attributes;
                if (attrs.length() > 40)
                    attrs = attrs.Left(37) + "...";
                childLabel += " " + attrs;
            }
            if (childInfo.isSelfClosing)
            {
                childLabel += "/>";
            }
            else
            {
                childLabel += ">";
                if (!childInfo.hasChildren && !childInfo.textContent.IsEmpty())
                {
                    wxString text = childInfo.textContent;
                    if (text.length() > 30)
                        text = text.Left(27) + "...";
                    childLabel += " " + text;
                }
            }
            
            child->label = childLabel;
            child->lineNumber = childInfo.startLine;
            child->startPos = childInfo.startPos;
            child->endPos = childInfo.endPos;
            child->hasChildren = childInfo.hasChildren && !childInfo.isSelfClosing;
            child->childrenLoaded = false;
            child->parent = node.get();
            
            node->children.push_back(std::move(child));
        }
        
        roots.push_back(std::move(node));
    }
    
    m_model->SetRootNodes(std::move(roots));
    
    // Notify about preloaded children and expand root
    VirtualTreeNode* modelRoot = m_model->GetRootNode();
    if (modelRoot && !modelRoot->children.empty())
    {
        VirtualTreeNode* xmlRoot = modelRoot->children[0].get();
        
        // Notify about the preloaded children of the XML root element
        if (!xmlRoot->children.empty())
        {
            m_model->NotifyChildrenLoaded(xmlRoot);
        }
        
        // Expand the XML root element
        wxDataViewItem rootItem(xmlRoot);
        Expand(rootItem);
    }
    
    return true;
}

int VirtualXmlTree::GetSelectedLineNumber() const
{
    wxDataViewItem item = GetSelection();
    if (!item.IsOk())
        return 0;
    
    VirtualTreeNode* node = m_model->GetNode(item);
    return node ? node->lineNumber : 0;
}

void VirtualXmlTree::OnSelectionChanged(wxDataViewEvent& event)
{
    // Forward to parent as a custom event
    wxCommandEvent evt(wxEVT_VIRTUALTREE_SELECTION_CHANGED, GetId());
    evt.SetEventObject(this);
    ProcessWindowEvent(evt);
}

void VirtualXmlTree::OnItemExpanding(wxDataViewEvent& event)
{
    wxDataViewItem item = event.GetItem();
    VirtualTreeNode* node = m_model->GetNode(item);
    
    if (!node || node->childrenLoaded)
        return;
    
    // Check if already loading
    {
        wxCriticalSectionLocker lock(m_loadingLock);
        if (m_loadingNodes.find(node) != m_loadingNodes.end())
            return;  // Already loading
        m_loadingNodes.insert(node);
    }
    
    // Add a "Loading..." placeholder
    AddLoadingPlaceholder(node);
    
    // Update cursor to show background work
    UpdateBackgroundCursor();
    
    // Start background loading
    StartChildLoading(node);
}

void VirtualXmlTree::StartChildLoading(VirtualTreeNode* node)
{
    if (!node || node->startPos >= m_content.length())
        return;
    
    // Create and start worker thread
    ChildLoaderThread* thread = new ChildLoaderThread(
        this, node, m_content, node->startPos, node->endPos);
    
    if (thread->Run() != wxTHREAD_NO_ERROR)
    {
        delete thread;
        
        // Remove from loading set
        wxCriticalSectionLocker lock(m_loadingLock);
        m_loadingNodes.erase(node);
    }
}

void VirtualXmlTree::OnChildrenLoaded(wxThreadEvent& event)
{
    ChildLoadResult result = event.GetPayload<ChildLoadResult>();
    VirtualTreeNode* node = result.parentNode;
    
    // Remove from loading set
    {
        wxCriticalSectionLocker lock(m_loadingLock);
        m_loadingNodes.erase(node);
    }
    
    // Update cursor
    UpdateBackgroundCursor();
    
    // Check if node is still valid (not deleted)
    if (!node)
        return;
    
    wxDataViewItem parentItem(node);
    
    // Remove the loading placeholder
    RemoveLoadingPlaceholder(node);
    
    // Add children to the model
    m_model->AddChildrenToNode(node, result.children);
    
    // If no children found, update the node
    if (result.children.empty())
    {
        node->hasChildren = false;
        // Notify view that this is no longer a container
        m_model->ItemChanged(parentItem);
    }
    else
    {
        // Re-expand the node to ensure it stays open after children are added
        // Use CallAfter to ensure the model has finished updating
        CallAfter([this, parentItem]() {
            if (parentItem.IsOk())
            {
                Expand(parentItem);
            }
        });
    }
}

void VirtualXmlTree::OnItemActivated(wxDataViewEvent& event)
{
    wxDataViewItem item = event.GetItem();
    if (!item.IsOk())
        return;
    
    // Toggle expand/collapse on double-click
    if (IsExpanded(item))
    {
        Collapse(item);
    }
    else
    {
        Expand(item);
    }
}

void VirtualXmlTree::OnContextMenu(wxDataViewEvent& event)
{
    wxDataViewItem item = event.GetItem();
    if (!item.IsOk())
        return;
    
    VirtualTreeNode* node = m_model->GetNode(item);
    if (!node)
        return;
    
    wxMenu menu;
    menu.Append(wxID_COPY, "Copy Element Name");
    menu.Append(1001, "Show Full Content");
    
    menu.Bind(wxEVT_MENU, [this, node](wxCommandEvent&) {
        // Extract element name
        wxString label = node->label;
        size_t start = label.find('<');
        size_t end = label.find_first_of(" />", start);
        if (start != wxString::npos && end != wxString::npos)
        {
            wxString name = label.SubString(start + 1, end - 1);
            if (wxTheClipboard->Open())
            {
                wxTheClipboard->SetData(new wxTextDataObject(name));
                wxTheClipboard->Close();
            }
        }
    }, wxID_COPY);
    
    menu.Bind(wxEVT_MENU, [this, node](wxCommandEvent&) {
        if (node->endPos > node->startPos && node->endPos <= m_content.length())
        {
            wxString content = m_content.Mid(node->startPos, node->endPos - node->startPos);
            
            wxDialog dlg(this, wxID_ANY, "Element Content",
                         wxDefaultPosition, wxSize(600, 400),
                         wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
            
            wxTextCtrl* text = new wxTextCtrl(&dlg, wxID_ANY, content,
                                              wxDefaultPosition, wxDefaultSize,
                                              wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL);
            text->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
            
            wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
            sizer->Add(text, 1, wxEXPAND | wxALL, 5);
            
            wxButton* closeBtn = new wxButton(&dlg, wxID_CLOSE, "Close");
            sizer->Add(closeBtn, 0, wxALIGN_CENTER | wxBOTTOM, 10);
            closeBtn->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) { dlg.EndModal(wxID_OK); });
            
            dlg.SetSizer(sizer);
            dlg.ShowModal();
        }
    }, 1001);
    
    PopupMenu(&menu);
}

void VirtualXmlTree::AddLoadingPlaceholder(VirtualTreeNode* node)
{
    if (!node)
        return;
    
    // Create a "Loading..." placeholder node
    auto placeholder = std::make_unique<VirtualTreeNode>();
    placeholder->label = "Loading...";
    placeholder->lineNumber = 0;
    placeholder->startPos = 0;
    placeholder->endPos = 0;
    placeholder->hasChildren = false;
    placeholder->childrenLoaded = true;
    placeholder->parent = node;
    
    VirtualTreeNode* placeholderPtr = placeholder.get();
    node->children.push_back(std::move(placeholder));
    
    // Notify view about the placeholder
    wxDataViewItem parentItem(node);
    wxDataViewItem placeholderItem(placeholderPtr);
    m_model->ItemAdded(parentItem, placeholderItem);
}

void VirtualXmlTree::RemoveLoadingPlaceholder(VirtualTreeNode* node)
{
    if (!node || node->children.empty())
        return;
    
    // Find and remove the "Loading..." placeholder
    for (auto it = node->children.begin(); it != node->children.end(); ++it)
    {
        if ((*it)->label == "Loading...")
        {
            VirtualTreeNode* placeholderPtr = it->get();
            wxDataViewItem parentItem(node);
            wxDataViewItem placeholderItem(placeholderPtr);
            
            // Notify view that placeholder is being removed
            m_model->ItemDeleted(parentItem, placeholderItem);
            
            // Remove from children
            node->children.erase(it);
            break;
        }
    }
}

void VirtualXmlTree::UpdateBackgroundCursor()
{
    wxCriticalSectionLocker lock(m_loadingLock);
    
    bool shouldShowBusy = !m_loadingNodes.empty();
    
    if (shouldShowBusy && !m_showingBusyCursor)
    {
        // Show "working in background" cursor (arrow with hourglass)
        SetCursor(wxCursor(wxCURSOR_ARROWWAIT));
        m_showingBusyCursor = true;
    }
    else if (!shouldShowBusy && m_showingBusyCursor)
    {
        // Restore normal cursor
        SetCursor(m_originalCursor);
        m_showingBusyCursor = false;
    }
}

