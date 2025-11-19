#include "RecentFiles.h"
#include <wx/stdpaths.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <algorithm>

RecentFiles::RecentFiles()
{
    Load();
}

RecentFiles::~RecentFiles()
{
    Save();
}

void RecentFiles::AddFile(const wxString& filePath)
{
    // Remove if already exists
    auto it = std::find(m_files.begin(), m_files.end(), filePath);
    if (it != m_files.end())
    {
        m_files.erase(it);
    }

    // Add to front
    m_files.insert(m_files.begin(), filePath);

    // Limit to max files
    if (m_files.size() > MAX_RECENT_FILES)
    {
        m_files.resize(MAX_RECENT_FILES);
    }
}

wxString RecentFiles::GetFile(size_t index) const
{
    if (index >= m_files.size())
        return wxEmptyString;

    // Validate file still exists
    wxString filePath = m_files[index];
    if (wxFileExists(filePath))
    {
        return filePath;
    }

    return wxEmptyString;
}

wxString RecentFiles::GetConfigFilePath() const
{
    wxStandardPaths& stdPaths = wxStandardPaths::Get();
    wxString configDir = stdPaths.GetUserConfigDir();
    wxString appName = "XmlViewer";
    
    #ifdef __WXMSW__
        configDir += "\\" + appName;
    #else
        configDir += "/" + appName;
    #endif

    // Create directory if it doesn't exist
    if (!wxDirExists(configDir))
    {
        wxMkdir(configDir, wxS_DIR_DEFAULT);
    }

    wxString separator = wxFileName::GetPathSeparator();
    wxString configFile = configDir + separator + "recentfiles.ini";
    return configFile;
}

void RecentFiles::Save()
{
    wxString configFile = GetConfigFilePath();
    wxFileConfig config("XmlViewer", wxEmptyString, configFile, wxEmptyString,
                        wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_SUBDIR);

    config.SetPath("/RecentFiles");
    config.DeleteGroup("RecentFiles");

    for (size_t i = 0; i < m_files.size(); ++i)
    {
        wxString key = wxString::Format("File%zu", i);
        config.Write(key, m_files[i]);
    }

    config.Flush();
}

void RecentFiles::Load()
{
    m_files.clear();

    wxString configFile = GetConfigFilePath();
    if (!wxFileExists(configFile))
        return;

    wxFileConfig config("XmlViewer", wxEmptyString, configFile, wxEmptyString,
                        wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_SUBDIR);

    config.SetPath("/RecentFiles");

    // Read files by index
    for (size_t i = 0; i < MAX_RECENT_FILES; ++i)
    {
        wxString key = wxString::Format("File%zu", i);
        wxString value;
        if (config.Read(key, &value) && wxFileExists(value))
        {
            m_files.push_back(value);
        }
    }
}

