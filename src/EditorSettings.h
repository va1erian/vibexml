#ifndef EDITORSETTINGS_H
#define EDITORSETTINGS_H

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/fileconf.h>

// Represents a complete color theme for the XML editor
struct EditorTheme
{
    wxString name;
    
    // Editor background and default text
    wxColour background;
    wxColour foreground;
    wxColour lineNumberFg;
    wxColour lineNumberBg;
    wxColour caretLineBackground;
    wxColour selectionBackground;
    
    // XML syntax colors
    wxColour tagColor;
    wxColour attributeNameColor;
    wxColour attributeValueColor;
    wxColour commentColor;
    wxColour textContentColor;
    wxColour entityColor;
    wxColour cdataColor;
    
    // Create default light theme
    static EditorTheme LightTheme()
    {
        EditorTheme theme;
        theme.name = "Light";
        theme.background = wxColour(255, 255, 255);
        theme.foreground = wxColour(0, 0, 0);
        theme.lineNumberFg = wxColour(128, 128, 128);
        theme.lineNumberBg = wxColour(240, 240, 240);
        theme.caretLineBackground = wxColour(255, 255, 224);
        theme.selectionBackground = wxColour(173, 214, 255);
        
        theme.tagColor = wxColour(128, 0, 0);
        theme.attributeNameColor = wxColour(255, 0, 0);
        theme.attributeValueColor = wxColour(0, 0, 255);
        theme.commentColor = wxColour(0, 128, 0);
        theme.textContentColor = wxColour(0, 0, 0);
        theme.entityColor = wxColour(128, 0, 128);
        theme.cdataColor = wxColour(128, 128, 0);
        
        return theme;
    }
    
    // Create dark theme (Monokai-inspired)
    static EditorTheme DarkTheme()
    {
        EditorTheme theme;
        theme.name = "Dark";
        theme.background = wxColour(30, 30, 30);
        theme.foreground = wxColour(212, 212, 212);
        theme.lineNumberFg = wxColour(133, 133, 133);
        theme.lineNumberBg = wxColour(30, 30, 30);
        theme.caretLineBackground = wxColour(45, 45, 45);
        theme.selectionBackground = wxColour(66, 66, 66);
        
        theme.tagColor = wxColour(86, 156, 214);          // Blue for tags
        theme.attributeNameColor = wxColour(156, 220, 254); // Light blue for attr names
        theme.attributeValueColor = wxColour(206, 145, 120); // Orange/brown for attr values
        theme.commentColor = wxColour(106, 153, 85);       // Green for comments
        theme.textContentColor = wxColour(212, 212, 212);  // Light gray for text
        theme.entityColor = wxColour(220, 220, 170);       // Yellow for entities
        theme.cdataColor = wxColour(181, 206, 168);        // Light green for CDATA
        
        return theme;
    }
    
    // Create Dracula theme
    static EditorTheme DraculaTheme()
    {
        EditorTheme theme;
        theme.name = "Dracula";
        theme.background = wxColour(40, 42, 54);
        theme.foreground = wxColour(248, 248, 242);
        theme.lineNumberFg = wxColour(98, 114, 164);
        theme.lineNumberBg = wxColour(40, 42, 54);
        theme.caretLineBackground = wxColour(68, 71, 90);
        theme.selectionBackground = wxColour(68, 71, 90);
        
        theme.tagColor = wxColour(255, 121, 198);          // Pink for tags
        theme.attributeNameColor = wxColour(80, 250, 123); // Green for attr names
        theme.attributeValueColor = wxColour(241, 250, 140); // Yellow for attr values
        theme.commentColor = wxColour(98, 114, 164);       // Purple-gray for comments
        theme.textContentColor = wxColour(248, 248, 242);  // White for text
        theme.entityColor = wxColour(189, 147, 249);       // Purple for entities
        theme.cdataColor = wxColour(139, 233, 253);        // Cyan for CDATA
        
        return theme;
    }
    
    // Create Solarized Dark theme
    static EditorTheme SolarizedDarkTheme()
    {
        EditorTheme theme;
        theme.name = "Solarized Dark";
        theme.background = wxColour(0, 43, 54);
        theme.foreground = wxColour(131, 148, 150);
        theme.lineNumberFg = wxColour(88, 110, 117);
        theme.lineNumberBg = wxColour(0, 43, 54);
        theme.caretLineBackground = wxColour(7, 54, 66);
        theme.selectionBackground = wxColour(7, 54, 66);
        
        theme.tagColor = wxColour(38, 139, 210);           // Blue for tags
        theme.attributeNameColor = wxColour(181, 137, 0);  // Yellow for attr names
        theme.attributeValueColor = wxColour(42, 161, 152); // Cyan for attr values
        theme.commentColor = wxColour(88, 110, 117);       // Base01 for comments
        theme.textContentColor = wxColour(131, 148, 150);  // Base0 for text
        theme.entityColor = wxColour(108, 113, 196);       // Violet for entities
        theme.cdataColor = wxColour(133, 153, 0);          // Green for CDATA
        
        return theme;
    }
};

class EditorSettings
{
public:
    static EditorSettings& Get();
    
    void Load();
    void Save();
    
    // Font settings
    wxFont GetFont() const { return m_font; }
    void SetFont(const wxFont& font) { m_font = font; }
    
    // Theme settings
    EditorTheme GetTheme() const { return m_theme; }
    void SetTheme(const EditorTheme& theme) { m_theme = theme; }
    
    wxString GetThemeName() const { return m_themeName; }
    void SetThemeName(const wxString& name) { m_themeName = name; }
    
    // Convenience method to apply a preset theme by name
    void ApplyPresetTheme(const wxString& name);
    
    // Get list of available preset themes
    static wxArrayString GetPresetThemeNames();

private:
    EditorSettings();
    ~EditorSettings() = default;
    
    // Prevent copying
    EditorSettings(const EditorSettings&) = delete;
    EditorSettings& operator=(const EditorSettings&) = delete;
    
    wxFont m_font;
    EditorTheme m_theme;
    wxString m_themeName;
    
    // Helper to save/load colors
    void SaveColour(wxConfigBase* config, const wxString& key, const wxColour& colour);
    wxColour LoadColour(wxConfigBase* config, const wxString& key, const wxColour& defaultColour);
};

#endif // EDITORSETTINGS_H

