#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/clrpicker.h>
#include <wx/fontpicker.h>
#include <wx/stc/stc.h>
#include "EditorSettings.h"

class SettingsDialog : public wxDialog
{
public:
    SettingsDialog(wxWindow* parent);
    
    // Get the modified settings (after dialog closes)
    EditorTheme GetTheme() const { return m_theme; }
    wxFont GetFont() const { return m_font; }
    wxString GetThemeName() const { return m_themeName; }

private:
    void BindControls();
    void CreatePreviewControl();
    void UpdatePreview();
    void LoadCurrentSettings();
    void UpdateColorPickers();
    
    // Event handlers
    void OnThemeChanged(wxCommandEvent& event);
    void OnFontChanged(wxFontPickerEvent& event);
    void OnColourChanged(wxColourPickerEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    
    // Controls (obtained from XRC)
    wxFontPickerCtrl* m_fontPicker;
    wxChoice* m_themeChoice;
    wxStyledTextCtrl* m_preview;
    wxPanel* m_previewPanel;
    
    // Color pickers
    wxColourPickerCtrl* m_bgColorPicker;
    wxColourPickerCtrl* m_fgColorPicker;
    wxColourPickerCtrl* m_tagColorPicker;
    wxColourPickerCtrl* m_attrNameColorPicker;
    wxColourPickerCtrl* m_attrValueColorPicker;
    wxColourPickerCtrl* m_commentColorPicker;
    wxColourPickerCtrl* m_textContentColorPicker;
    wxColourPickerCtrl* m_lineNumFgPicker;
    wxColourPickerCtrl* m_lineNumBgPicker;
    
    // Current settings being edited
    EditorTheme m_theme;
    wxFont m_font;
    wxString m_themeName;
};

#endif // SETTINGSDIALOG_H
