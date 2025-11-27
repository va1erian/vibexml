#include "EditorSettings.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>

EditorSettings& EditorSettings::Get()
{
    static EditorSettings instance;
    return instance;
}

EditorSettings::EditorSettings()
    : m_font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Consolas"),
      m_theme(EditorTheme::LightTheme()),
      m_themeName("Light")
{
    Load();
}

wxArrayString EditorSettings::GetPresetThemeNames()
{
    wxArrayString names;
    names.Add("Light");
    names.Add("Dark");
    names.Add("Dracula");
    names.Add("Solarized Dark");
    return names;
}

void EditorSettings::ApplyPresetTheme(const wxString& name)
{
    m_themeName = name;
    
    if (name == "Light")
    {
        m_theme = EditorTheme::LightTheme();
    }
    else if (name == "Dark")
    {
        m_theme = EditorTheme::DarkTheme();
    }
    else if (name == "Dracula")
    {
        m_theme = EditorTheme::DraculaTheme();
    }
    else if (name == "Solarized Dark")
    {
        m_theme = EditorTheme::SolarizedDarkTheme();
    }
    else
    {
        // Default to light theme if unknown
        m_theme = EditorTheme::LightTheme();
        m_themeName = "Light";
    }
}

void EditorSettings::SaveColour(wxConfigBase* config, const wxString& key, const wxColour& colour)
{
    config->Write(key, colour.GetAsString(wxC2S_HTML_SYNTAX));
}

wxColour EditorSettings::LoadColour(wxConfigBase* config, const wxString& key, const wxColour& defaultColour)
{
    wxString colourStr;
    if (config->Read(key, &colourStr))
    {
        wxColour colour(colourStr);
        if (colour.IsOk())
            return colour;
    }
    return defaultColour;
}

void EditorSettings::Load()
{
    // Get config file path in user's app data folder
    wxString configPath = wxStandardPaths::Get().GetUserConfigDir();
    wxFileName configFile(configPath, "vibexml.ini");
    
    wxFileConfig config("VibeXML", wxEmptyString, configFile.GetFullPath());
    
    // Load font settings
    wxString fontFace;
    int fontSize = 10;
    bool fontBold = false;
    
    config.Read("Font/Face", &fontFace, "Consolas");
    config.Read("Font/Size", &fontSize, 10);
    config.Read("Font/Bold", &fontBold, false);
    
    m_font = wxFont(fontSize, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                    fontBold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL, false, fontFace);
    
    // Load theme name
    wxString themeName;
    config.Read("Theme/Name", &themeName, "Light");
    
    // Check if it's a preset theme
    wxArrayString presets = GetPresetThemeNames();
    if (presets.Index(themeName) != wxNOT_FOUND)
    {
        ApplyPresetTheme(themeName);
    }
    else
    {
        // Custom theme - load all colors
        m_themeName = "Custom";
        EditorTheme defaultTheme = EditorTheme::LightTheme();
        
        m_theme.name = "Custom";
        m_theme.background = LoadColour(&config, "Theme/Background", defaultTheme.background);
        m_theme.foreground = LoadColour(&config, "Theme/Foreground", defaultTheme.foreground);
        m_theme.lineNumberFg = LoadColour(&config, "Theme/LineNumberFg", defaultTheme.lineNumberFg);
        m_theme.lineNumberBg = LoadColour(&config, "Theme/LineNumberBg", defaultTheme.lineNumberBg);
        m_theme.caretLineBackground = LoadColour(&config, "Theme/CaretLineBg", defaultTheme.caretLineBackground);
        m_theme.selectionBackground = LoadColour(&config, "Theme/SelectionBg", defaultTheme.selectionBackground);
        
        m_theme.tagColor = LoadColour(&config, "Theme/TagColor", defaultTheme.tagColor);
        m_theme.attributeNameColor = LoadColour(&config, "Theme/AttrNameColor", defaultTheme.attributeNameColor);
        m_theme.attributeValueColor = LoadColour(&config, "Theme/AttrValueColor", defaultTheme.attributeValueColor);
        m_theme.commentColor = LoadColour(&config, "Theme/CommentColor", defaultTheme.commentColor);
        m_theme.textContentColor = LoadColour(&config, "Theme/TextContentColor", defaultTheme.textContentColor);
        m_theme.entityColor = LoadColour(&config, "Theme/EntityColor", defaultTheme.entityColor);
        m_theme.cdataColor = LoadColour(&config, "Theme/CDataColor", defaultTheme.cdataColor);
    }
}

void EditorSettings::Save()
{
    // Get config file path in user's app data folder
    wxString configPath = wxStandardPaths::Get().GetUserConfigDir();
    wxFileName configFile(configPath, "vibexml.ini");
    
    wxFileConfig config("VibeXML", wxEmptyString, configFile.GetFullPath());
    
    // Save font settings
    config.Write("Font/Face", m_font.GetFaceName());
    config.Write("Font/Size", m_font.GetPointSize());
    config.Write("Font/Bold", m_font.GetWeight() == wxFONTWEIGHT_BOLD);
    
    // Save theme
    config.Write("Theme/Name", m_themeName);
    
    // Always save custom colors (even for presets, in case user modified them)
    SaveColour(&config, "Theme/Background", m_theme.background);
    SaveColour(&config, "Theme/Foreground", m_theme.foreground);
    SaveColour(&config, "Theme/LineNumberFg", m_theme.lineNumberFg);
    SaveColour(&config, "Theme/LineNumberBg", m_theme.lineNumberBg);
    SaveColour(&config, "Theme/CaretLineBg", m_theme.caretLineBackground);
    SaveColour(&config, "Theme/SelectionBg", m_theme.selectionBackground);
    
    SaveColour(&config, "Theme/TagColor", m_theme.tagColor);
    SaveColour(&config, "Theme/AttrNameColor", m_theme.attributeNameColor);
    SaveColour(&config, "Theme/AttrValueColor", m_theme.attributeValueColor);
    SaveColour(&config, "Theme/CommentColor", m_theme.commentColor);
    SaveColour(&config, "Theme/TextContentColor", m_theme.textContentColor);
    SaveColour(&config, "Theme/EntityColor", m_theme.entityColor);
    SaveColour(&config, "Theme/CDataColor", m_theme.cdataColor);
    
    config.Flush();
}

