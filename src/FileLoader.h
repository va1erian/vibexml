#ifndef FILELOADER_H
#define FILELOADER_H

#include <wx/wx.h>
#include <wx/thread.h>
#include <wx/progdlg.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <atomic>
#include <functional>

// Progress callback: returns false to cancel
using ProgressCallback = std::function<bool(int percent, const wxString& status)>;

// Result of file loading
struct FileLoadResult
{
    bool success = false;
    bool cancelled = false;
    wxString content;
    wxString errorMessage;
    wxULongLong fileSize;
    double loadTimeSeconds = 0;
};

// Load a file with progress reporting
// Shows a wxProgressDialog and loads the file in a background thread
class FileLoader
{
public:
    // Load file with progress dialog
    // Returns true if successful, false if cancelled or error
    static bool LoadFile(wxWindow* parent, 
                         const wxString& filePath,
                         FileLoadResult& result);
    
    // Load string content into an editor with progress dialog
    // Uses chunked loading to keep UI responsive
    static bool LoadIntoEditor(wxWindow* parent,
                               class XmlEditorCtrl* editor,
                               const wxString& content);

    // Background thread for file reading
    class ReaderThread : public wxThread
    {
    public:
        ReaderThread(wxEvtHandler* handler, const wxString& filePath);
        void RequestCancel() { m_cancelled = true; }
        
    protected:
        virtual ExitCode Entry() override;
        
    private:
        wxEvtHandler* m_handler;
        wxString m_filePath;
        std::atomic<bool> m_cancelled{false};
    };
};

// Events for background file loading
wxDECLARE_EVENT(wxEVT_FILE_PROGRESS, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_FILE_COMPLETE, wxThreadEvent);

#endif // FILELOADER_H
