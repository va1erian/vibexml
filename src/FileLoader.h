#ifndef FILELOADER_H
#define FILELOADER_H

#include <wx/wx.h>
#include <wx/thread.h>
#include <wx/progdlg.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <atomic>
#include <functional>

// Event sent when loading progress updates
wxDECLARE_EVENT(wxEVT_FILE_LOAD_PROGRESS, wxThreadEvent);
// Event sent when loading completes
wxDECLARE_EVENT(wxEVT_FILE_LOAD_COMPLETE, wxThreadEvent);
// Event sent when loading fails
wxDECLARE_EVENT(wxEVT_FILE_LOAD_ERROR, wxThreadEvent);

// Result of file loading
struct FileLoadResult
{
    bool success;
    wxString content;
    wxString errorMessage;
    wxULongLong fileSize;
    double loadTimeSeconds;
};

// Worker thread for loading large files
class FileLoaderThread : public wxThread
{
public:
    FileLoaderThread(wxEvtHandler* handler, const wxString& filePath);
    
    void RequestCancel() { m_cancelled = true; }
    bool IsCancelled() const { return m_cancelled; }

protected:
    virtual ExitCode Entry() override;

private:
    wxEvtHandler* m_handler;
    wxString m_filePath;
    std::atomic<bool> m_cancelled;
};

// Dialog that shows loading progress
class FileLoadProgressDialog : public wxDialog
{
public:
    FileLoadProgressDialog(wxWindow* parent, const wxString& filePath);
    ~FileLoadProgressDialog();
    
    // Start loading and show dialog (returns when done or cancelled)
    bool Load();
    
    // Get the loaded content (only valid after successful Load())
    const wxString& GetContent() const { return m_result.content; }
    const FileLoadResult& GetResult() const { return m_result; }

private:
    void OnProgress(wxThreadEvent& event);
    void OnComplete(wxThreadEvent& event);
    void OnError(wxThreadEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    
    void UpdateProgress(int percent, const wxString& message);
    
    wxString m_filePath;
    wxGauge* m_gauge;
    wxStaticText* m_statusText;
    wxStaticText* m_detailText;
    wxButton* m_cancelButton;
    
    FileLoaderThread* m_thread;
    FileLoadResult m_result;
    bool m_loading;
    wxStopWatch m_stopWatch;
};

#endif // FILELOADER_H

