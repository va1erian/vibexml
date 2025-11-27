#include "FileLoader.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

// Define the custom events
wxDEFINE_EVENT(wxEVT_FILE_LOAD_PROGRESS, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_FILE_LOAD_COMPLETE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_FILE_LOAD_ERROR, wxThreadEvent);

// ============================================================================
// FileLoaderThread
// ============================================================================

FileLoaderThread::FileLoaderThread(wxEvtHandler* handler, const wxString& filePath)
    : wxThread(wxTHREAD_DETACHED),
      m_handler(handler),
      m_filePath(filePath),
      m_cancelled(false)
{
}

wxThread::ExitCode FileLoaderThread::Entry()
{
    FileLoadResult result;
    result.success = false;
    
    // Get file size
    wxFileName fileName(m_filePath);
    if (!fileName.FileExists())
    {
        result.errorMessage = "File does not exist";
        wxThreadEvent* errorEvent = new wxThreadEvent(wxEVT_FILE_LOAD_ERROR);
        errorEvent->SetPayload(result);
        wxQueueEvent(m_handler, errorEvent);
        return nullptr;
    }
    
    result.fileSize = fileName.GetSize();
    wxULongLong totalSize = result.fileSize;
    
    // Open file
    wxFile file(m_filePath, wxFile::read);
    if (!file.IsOpened())
    {
        result.errorMessage = "Failed to open file";
        wxThreadEvent* errorEvent = new wxThreadEvent(wxEVT_FILE_LOAD_ERROR);
        errorEvent->SetPayload(result);
        wxQueueEvent(m_handler, errorEvent);
        return nullptr;
    }
    
    // Read in chunks for progress reporting
    // Use larger chunks for better performance
    const size_t chunkSize = 16 * 1024 * 1024;  // 16 MB chunks
    wxMemoryBuffer buffer;
    buffer.SetBufSize(static_cast<size_t>(totalSize.GetValue()) + 1);
    
    wxULongLong bytesRead = 0;
    int lastPercent = -1;
    
    char* tempBuffer = new char[chunkSize];
    
    while (!file.Eof() && !m_cancelled)
    {
        size_t read = file.Read(tempBuffer, chunkSize);
        if (read > 0)
        {
            buffer.AppendData(tempBuffer, read);
            bytesRead += read;
            
            // Calculate progress
            int percent = static_cast<int>((bytesRead.GetValue() * 100) / totalSize.GetValue());
            
            // Only send progress event if percentage changed (reduces overhead)
            if (percent != lastPercent)
            {
                lastPercent = percent;
                wxThreadEvent* progressEvent = new wxThreadEvent(wxEVT_FILE_LOAD_PROGRESS);
                progressEvent->SetInt(percent);
                progressEvent->SetString(wxString::Format("Read %s of %s",
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
        result.errorMessage = "Loading cancelled";
        wxThreadEvent* errorEvent = new wxThreadEvent(wxEVT_FILE_LOAD_ERROR);
        errorEvent->SetPayload(result);
        wxQueueEvent(m_handler, errorEvent);
        return nullptr;
    }
    
    // Send progress for conversion phase
    wxThreadEvent* progressEvent = new wxThreadEvent(wxEVT_FILE_LOAD_PROGRESS);
    progressEvent->SetInt(100);
    progressEvent->SetString("Converting to text...");
    wxQueueEvent(m_handler, progressEvent);
    
    // Convert buffer to wxString
    // Null-terminate the buffer
    buffer.AppendByte(0);
    
    // Try UTF-8 first
    result.content = wxString::FromUTF8(static_cast<const char*>(buffer.GetData()), buffer.GetDataLen() - 1);
    
    // If that failed (empty string for non-empty file), try local encoding
    if (result.content.IsEmpty() && buffer.GetDataLen() > 1)
    {
        result.content = wxString(static_cast<const char*>(buffer.GetData()), buffer.GetDataLen() - 1);
    }
    
    result.success = true;
    
    // Send completion event
    wxThreadEvent* completeEvent = new wxThreadEvent(wxEVT_FILE_LOAD_COMPLETE);
    completeEvent->SetPayload(result);
    wxQueueEvent(m_handler, completeEvent);
    
    return nullptr;
}

// ============================================================================
// FileLoadProgressDialog
// ============================================================================

FileLoadProgressDialog::FileLoadProgressDialog(wxWindow* parent, const wxString& filePath)
    : wxDialog(parent, wxID_ANY, "Loading File", wxDefaultPosition, wxSize(450, 180),
               wxDEFAULT_DIALOG_STYLE),
      m_filePath(filePath),
      m_thread(nullptr),
      m_loading(false)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // File name
    wxFileName fileName(filePath);
    wxStaticText* fileLabel = new wxStaticText(this, wxID_ANY, 
        wxString::Format("Loading: %s", fileName.GetFullName()));
    fileLabel->SetFont(fileLabel->GetFont().Bold());
    mainSizer->Add(fileLabel, 0, wxALL | wxEXPAND, 10);
    
    // File size info
    wxULongLong fileSize = fileName.GetSize();
    wxStaticText* sizeLabel = new wxStaticText(this, wxID_ANY,
        wxString::Format("Size: %s", wxFileName::GetHumanReadableSize(fileSize)));
    mainSizer->Add(sizeLabel, 0, wxLEFT | wxRIGHT, 10);
    
    // Progress gauge
    m_gauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 25));
    mainSizer->Add(m_gauge, 0, wxALL | wxEXPAND, 10);
    
    // Status text
    m_statusText = new wxStaticText(this, wxID_ANY, "Preparing...");
    mainSizer->Add(m_statusText, 0, wxLEFT | wxRIGHT, 10);
    
    // Detail text
    m_detailText = new wxStaticText(this, wxID_ANY, "");
    m_detailText->SetForegroundColour(wxColour(100, 100, 100));
    mainSizer->Add(m_detailText, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Cancel button
    m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    mainSizer->Add(m_cancelButton, 0, wxALIGN_CENTER | wxBOTTOM, 10);
    
    SetSizer(mainSizer);
    CenterOnParent();
    
    // Bind events
    Bind(wxEVT_FILE_LOAD_PROGRESS, &FileLoadProgressDialog::OnProgress, this);
    Bind(wxEVT_FILE_LOAD_COMPLETE, &FileLoadProgressDialog::OnComplete, this);
    Bind(wxEVT_FILE_LOAD_ERROR, &FileLoadProgressDialog::OnError, this);
    Bind(wxEVT_BUTTON, &FileLoadProgressDialog::OnCancel, this, wxID_CANCEL);
    Bind(wxEVT_CLOSE_WINDOW, &FileLoadProgressDialog::OnClose, this);
}

FileLoadProgressDialog::~FileLoadProgressDialog()
{
    // Thread is detached and will clean itself up
}

bool FileLoadProgressDialog::Load()
{
    m_loading = true;
    m_result.success = false;
    m_stopWatch.Start();
    
    // Create and start the loader thread
    m_thread = new FileLoaderThread(this, m_filePath);
    if (m_thread->Run() != wxTHREAD_NO_ERROR)
    {
        delete m_thread;
        m_thread = nullptr;
        m_loading = false;
        wxMessageBox("Failed to start loading thread", "Error", wxOK | wxICON_ERROR);
        return false;
    }
    
    // Show dialog modally - it will be closed when loading completes or is cancelled
    ShowModal();
    
    return m_result.success;
}

void FileLoadProgressDialog::UpdateProgress(int percent, const wxString& message)
{
    m_gauge->SetValue(percent);
    m_statusText->SetLabel(message);
    
    // Show elapsed time
    long elapsed = m_stopWatch.Time();
    m_detailText->SetLabel(wxString::Format("Elapsed: %.1f seconds", elapsed / 1000.0));
}

void FileLoadProgressDialog::OnProgress(wxThreadEvent& event)
{
    int percent = event.GetInt();
    wxString message = event.GetString();
    UpdateProgress(percent, message);
}

void FileLoadProgressDialog::OnComplete(wxThreadEvent& event)
{
    m_result = event.GetPayload<FileLoadResult>();
    m_result.loadTimeSeconds = m_stopWatch.Time() / 1000.0;
    m_loading = false;
    m_thread = nullptr;
    
    EndModal(wxID_OK);
}

void FileLoadProgressDialog::OnError(wxThreadEvent& event)
{
    m_result = event.GetPayload<FileLoadResult>();
    m_loading = false;
    m_thread = nullptr;
    
    EndModal(wxID_CANCEL);
}

void FileLoadProgressDialog::OnCancel(wxCommandEvent& event)
{
    if (m_thread && m_loading)
    {
        m_statusText->SetLabel("Cancelling...");
        m_cancelButton->Enable(false);
        m_thread->RequestCancel();
    }
    else
    {
        EndModal(wxID_CANCEL);
    }
}

void FileLoadProgressDialog::OnClose(wxCloseEvent& event)
{
    if (m_thread && m_loading)
    {
        m_thread->RequestCancel();
        // Wait a bit for the thread to notice
        wxMilliSleep(100);
    }
    event.Skip();
}

