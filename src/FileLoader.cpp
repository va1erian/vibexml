#include "FileLoader.h"
#include "XmlEditorCtrl.h"
#include <wx/wfstream.h>

// Define events
wxDEFINE_EVENT(wxEVT_FILE_PROGRESS, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_FILE_COMPLETE, wxThreadEvent);

// ============================================================================
// FileLoader::ReaderThread
// ============================================================================

FileLoader::ReaderThread::ReaderThread(wxEvtHandler* handler, const wxString& filePath)
    : wxThread(wxTHREAD_DETACHED),
      m_handler(handler),
      m_filePath(filePath)
{
}

wxThread::ExitCode FileLoader::ReaderThread::Entry()
{
    FileLoadResult result;
    result.success = false;
    
    // Get file info
    wxFileName fileName(m_filePath);
    if (!fileName.FileExists())
    {
        result.errorMessage = "File does not exist";
        wxThreadEvent* event = new wxThreadEvent(wxEVT_FILE_COMPLETE);
        event->SetPayload(result);
        wxQueueEvent(m_handler, event);
        return nullptr;
    }
    
    result.fileSize = fileName.GetSize();
    wxULongLong totalSize = result.fileSize;
    
    // Open file
    wxFile file(m_filePath, wxFile::read);
    if (!file.IsOpened())
    {
        result.errorMessage = "Failed to open file";
        wxThreadEvent* event = new wxThreadEvent(wxEVT_FILE_COMPLETE);
        event->SetPayload(result);
        wxQueueEvent(m_handler, event);
        return nullptr;
    }
    
    // Read in chunks
    const size_t CHUNK_SIZE = 16 * 1024 * 1024;  // 16 MB
    wxMemoryBuffer buffer;
    buffer.SetBufSize(static_cast<size_t>(totalSize.GetValue()) + 1);
    
    wxULongLong bytesRead = 0;
    int lastPercent = -1;
    char* tempBuffer = new char[CHUNK_SIZE];
    
    while (!file.Eof() && !m_cancelled)
    {
        size_t read = file.Read(tempBuffer, CHUNK_SIZE);
        if (read > 0)
        {
            buffer.AppendData(tempBuffer, read);
            bytesRead += read;
            
            int percent = static_cast<int>((bytesRead.GetValue() * 100) / totalSize.GetValue());
            if (percent != lastPercent)
            {
                lastPercent = percent;
                wxThreadEvent* progressEvent = new wxThreadEvent(wxEVT_FILE_PROGRESS);
                progressEvent->SetInt(percent);
                progressEvent->SetString(wxString::Format("Reading file: %s of %s",
                    wxFileName::GetHumanReadableSize(bytesRead),
                    wxFileName::GetHumanReadableSize(totalSize)));
                wxQueueEvent(m_handler, progressEvent);
            }
        }
    }
    
    delete[] tempBuffer;
    file.Close();
    
    if (m_cancelled)
    {
        result.cancelled = true;
        result.errorMessage = "Cancelled";
        wxThreadEvent* event = new wxThreadEvent(wxEVT_FILE_COMPLETE);
        event->SetPayload(result);
        wxQueueEvent(m_handler, event);
        return nullptr;
    }
    
    // Convert to string
    {
        wxThreadEvent* progressEvent = new wxThreadEvent(wxEVT_FILE_PROGRESS);
        progressEvent->SetInt(100);
        progressEvent->SetString("Converting to text...");
        wxQueueEvent(m_handler, progressEvent);
    }
    
    buffer.AppendByte(0);
    result.content = wxString::FromUTF8(static_cast<const char*>(buffer.GetData()), 
                                         buffer.GetDataLen() - 1);
    
    if (result.content.IsEmpty() && buffer.GetDataLen() > 1)
    {
        result.content = wxString(static_cast<const char*>(buffer.GetData()), 
                                   buffer.GetDataLen() - 1);
    }
    
    result.success = true;
    
    wxThreadEvent* event = new wxThreadEvent(wxEVT_FILE_COMPLETE);
    event->SetPayload(result);
    wxQueueEvent(m_handler, event);
    
    return nullptr;
}

// ============================================================================
// FileLoader static methods
// ============================================================================

// Helper class to receive thread events
class FileLoadHandler : public wxEvtHandler
{
public:
    FileLoadHandler(wxProgressDialog* dlg) 
        : m_dialog(dlg), m_done(false), m_cancelled(false), m_thread(nullptr) {}
    
    void SetThread(FileLoader::ReaderThread* thread) { m_thread = thread; }
    
    void OnProgress(wxThreadEvent& event)
    {
        if (m_dialog && !m_cancelled)
        {
            wxString status = event.GetString();
            bool cont;
            
            // Use Pulse for indeterminate operations like text conversion
            if (status.Contains("Converting"))
            {
                cont = m_dialog->Pulse(status);
            }
            else
            {
                cont = m_dialog->Update(event.GetInt(), status);
            }
            
            if (!cont && m_thread)
            {
                m_cancelled = true;
                m_thread->RequestCancel();
            }
        }
    }
    
    void OnComplete(wxThreadEvent& event)
    {
        m_result = event.GetPayload<FileLoadResult>();
        m_done = true;
    }
    
    bool IsDone() const { return m_done; }
    bool WasCancelled() const { return m_cancelled; }
    const FileLoadResult& GetResult() const { return m_result; }
    
private:
    wxProgressDialog* m_dialog;
    FileLoadResult m_result;
    bool m_done;
    bool m_cancelled;
    FileLoader::ReaderThread* m_thread;
    
    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(FileLoadHandler, wxEvtHandler)
    EVT_THREAD(wxID_ANY, FileLoadHandler::OnProgress)
wxEND_EVENT_TABLE()

bool FileLoader::LoadFile(wxWindow* parent, 
                          const wxString& filePath,
                          FileLoadResult& result)
{
    wxFileName fileName(filePath);
    wxString title = "Loading File";
    wxString message = wxString::Format("Loading: %s", fileName.GetFullName());
    
    wxProgressDialog dlg(title, message, 100, parent,
                         wxPD_APP_MODAL | wxPD_SMOOTH | wxPD_AUTO_HIDE |
                         wxPD_CAN_ABORT | wxPD_ELAPSED_TIME);
    dlg.SetSize(450, 160);
    dlg.CentreOnParent();
    
    wxStopWatch stopWatch;
    stopWatch.Start();
    
    // Create handler for thread events
    FileLoadHandler handler(&dlg);
    handler.Bind(wxEVT_FILE_PROGRESS, &FileLoadHandler::OnProgress, &handler);
    handler.Bind(wxEVT_FILE_COMPLETE, &FileLoadHandler::OnComplete, &handler);
    
    // Start reader thread
    ReaderThread* thread = new ReaderThread(&handler, filePath);
    handler.SetThread(thread);
    
    if (thread->Run() != wxTHREAD_NO_ERROR)
    {
        delete thread;
        result.errorMessage = "Failed to start loading thread";
        return false;
    }
    
    // Process events until loading completes or cancelled
    while (!handler.IsDone() && !handler.WasCancelled())
    {
        wxYield();
        wxMilliSleep(10);
    }
    
    // Wait for thread to finish if cancelled
    if (handler.WasCancelled())
    {
        // Give thread time to exit cleanly
        int timeout = 50;  // 500ms max wait
        while (!handler.IsDone() && timeout-- > 0)
        {
            wxYield();
            wxMilliSleep(10);
        }
    }
    
    // Trigger auto-hide by updating to 100%
    dlg.Update(100, "Complete");
    
    result = handler.GetResult();
    result.loadTimeSeconds = stopWatch.Time() / 1000.0;
    
    return result.success;
}

bool FileLoader::LoadIntoEditor(wxWindow* parent,
                                XmlEditorCtrl* editor,
                                const wxString& content)
{
    if (!editor)
        return false;
    
    // For small files, just load directly
    if (content.Length() < 10 * 1024 * 1024)  // < 10 MB
    {
        return editor->LoadFromString(content);
    }
    
    // For large files, use chunked loading with progress
    wxProgressDialog dlg("Loading Editor", "Preparing editor...", 100, parent,
                         wxPD_APP_MODAL | wxPD_SMOOTH | wxPD_AUTO_HIDE | wxPD_CAN_ABORT);
    dlg.SetSize(400, 150);
    dlg.CentreOnParent();
    
    bool cancelled = false;
    bool success = editor->LoadFromStringWithProgress(content,
        [&dlg, &cancelled](int percent, const wxString& status) -> bool {
            bool cont = dlg.Update(percent, status);
            if (!cont)
            {
                cancelled = true;
                return false;
            }
            return true;
        });
    
    // Trigger auto-hide by updating to 100%
    if (!cancelled)
    {
        dlg.Update(100, "Complete");
    }
    
    // If cancelled, clear the editor
    if (cancelled)
    {
        editor->ClearAll();
        return false;
    }
    
    return success;
}

