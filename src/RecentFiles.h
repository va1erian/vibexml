#ifndef RECENTFILES_H
#define RECENTFILES_H

#include <wx/wx.h>
#include <wx/filename.h>
#include <vector>

class RecentFiles
{
public:
    RecentFiles();
    ~RecentFiles();

    void AddFile(const wxString& filePath);
    wxString GetFile(size_t index) const;
    const std::vector<wxString>& GetFiles() const { return m_files; }
    void Save();
    void Load();

private:
    static const size_t MAX_RECENT_FILES = 10;
    std::vector<wxString> m_files;
    wxString GetConfigFilePath() const;
};

#endif // RECENTFILES_H

