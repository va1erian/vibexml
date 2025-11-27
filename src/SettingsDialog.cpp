#include "SettingsDialog.h"
#include <wx/xrc/xmlres.h>
#include <wx/statline.h>

SettingsDialog::SettingsDialog(wxWindow* parent)
    : m_fontPicker(nullptr),
      m_themeChoice(nullptr),
      m_preview(nullptr),
      m_previewPanel(nullptr),
      m_bgColorPicker(nullptr),
      m_fgColorPicker(nullptr),
      m_tagColorPicker(nullptr),
      m_attrNameColorPicker(nullptr),
      m_attrValueColorPicker(nullptr),
      m_commentColorPicker(nullptr),
      m_textContentColorPicker(nullptr),
      m_lineNumFgPicker(nullptr),
      m_lineNumBgPicker(nullptr)
{
    // Load the dialog from XRC
    wxXmlResource::Get()->LoadDialog(this, parent, "SettingsDialog");
    
    // Load current settings first
    LoadCurrentSettings();
    
    // Get control references and bind events
    BindControls();
    
    // Create the preview control (not in XRC since it's wxStyledTextCtrl)
    CreatePreviewControl();
    
    // Update color pickers to reflect current theme
    UpdateColorPickers();
    
    // Update preview
    UpdatePreview();
    
    CenterOnParent();
}

void SettingsDialog::LoadCurrentSettings()
{
    EditorSettings& settings = EditorSettings::Get();
    m_font = settings.GetFont();
    m_theme = settings.GetTheme();
    m_themeName = settings.GetThemeName();
}

void SettingsDialog::BindControls()
{
    // Get control references from XRC
    m_themeChoice = XRCCTRL(*this, "ThemeChoice", wxChoice);
    m_fontPicker = XRCCTRL(*this, "FontPicker", wxFontPickerCtrl);
    m_previewPanel = XRCCTRL(*this, "PreviewPanel", wxPanel);
    
    // Color pickers
    m_bgColorPicker = XRCCTRL(*this, "BgColorPicker", wxColourPickerCtrl);
    m_fgColorPicker = XRCCTRL(*this, "FgColorPicker", wxColourPickerCtrl);
    m_lineNumFgPicker = XRCCTRL(*this, "LineNumFgPicker", wxColourPickerCtrl);
    m_lineNumBgPicker = XRCCTRL(*this, "LineNumBgPicker", wxColourPickerCtrl);
    m_tagColorPicker = XRCCTRL(*this, "TagColorPicker", wxColourPickerCtrl);
    m_attrNameColorPicker = XRCCTRL(*this, "AttrNameColorPicker", wxColourPickerCtrl);
    m_attrValueColorPicker = XRCCTRL(*this, "AttrValueColorPicker", wxColourPickerCtrl);
    m_commentColorPicker = XRCCTRL(*this, "CommentColorPicker", wxColourPickerCtrl);
    m_textContentColorPicker = XRCCTRL(*this, "TextContentColorPicker", wxColourPickerCtrl);
    
    // Set current theme selection
    if (m_themeChoice)
    {
        int themeIndex = m_themeChoice->FindString(m_themeName);
        if (themeIndex == wxNOT_FOUND)
            themeIndex = m_themeChoice->FindString("Custom");
        if (themeIndex != wxNOT_FOUND)
            m_themeChoice->SetSelection(themeIndex);
        
        m_themeChoice->Bind(wxEVT_CHOICE, &SettingsDialog::OnThemeChanged, this);
    }
    
    // Set current font
    if (m_fontPicker)
    {
        m_fontPicker->SetSelectedFont(m_font);
        m_fontPicker->Bind(wxEVT_FONTPICKER_CHANGED, &SettingsDialog::OnFontChanged, this);
    }
    
    // Bind color picker events
    if (m_bgColorPicker)
        m_bgColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnColourChanged, this);
    if (m_fgColorPicker)
        m_fgColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnColourChanged, this);
    if (m_lineNumFgPicker)
        m_lineNumFgPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnColourChanged, this);
    if (m_lineNumBgPicker)
        m_lineNumBgPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnColourChanged, this);
    if (m_tagColorPicker)
        m_tagColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnColourChanged, this);
    if (m_attrNameColorPicker)
        m_attrNameColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnColourChanged, this);
    if (m_attrValueColorPicker)
        m_attrValueColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnColourChanged, this);
    if (m_commentColorPicker)
        m_commentColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnColourChanged, this);
    if (m_textContentColorPicker)
        m_textContentColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &SettingsDialog::OnColourChanged, this);
    
    // Bind button events
    wxButton* applyBtn = XRCCTRL(*this, "ApplyButton", wxButton);
    if (applyBtn)
        applyBtn->Bind(wxEVT_BUTTON, &SettingsDialog::OnApply, this);
    
    wxButton* okBtn = XRCCTRL(*this, "wxID_OK", wxButton);
    if (okBtn)
        okBtn->Bind(wxEVT_BUTTON, &SettingsDialog::OnOK, this);
}

void SettingsDialog::UpdateColorPickers()
{
    if (m_bgColorPicker)
        m_bgColorPicker->SetColour(m_theme.background);
    if (m_fgColorPicker)
        m_fgColorPicker->SetColour(m_theme.foreground);
    if (m_lineNumFgPicker)
        m_lineNumFgPicker->SetColour(m_theme.lineNumberFg);
    if (m_lineNumBgPicker)
        m_lineNumBgPicker->SetColour(m_theme.lineNumberBg);
    if (m_tagColorPicker)
        m_tagColorPicker->SetColour(m_theme.tagColor);
    if (m_attrNameColorPicker)
        m_attrNameColorPicker->SetColour(m_theme.attributeNameColor);
    if (m_attrValueColorPicker)
        m_attrValueColorPicker->SetColour(m_theme.attributeValueColor);
    if (m_commentColorPicker)
        m_commentColorPicker->SetColour(m_theme.commentColor);
    if (m_textContentColorPicker)
        m_textContentColorPicker->SetColour(m_theme.textContentColor);
}

void SettingsDialog::CreatePreviewControl()
{
    if (!m_previewPanel)
        return;
    
    // Create the styled text control inside the preview panel
    m_preview = new wxStyledTextCtrl(m_previewPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    m_preview->SetLexer(wxSTC_LEX_XML);
    m_preview->SetReadOnly(false);
    
    // Sample XML for preview
    wxString sampleXml = 
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!-- This is a comment -->\n"
        "<root attribute=\"value\" count=\"42\">\n"
        "    <child>Text content here</child>\n"
        "    <item id=\"1\">&amp;entity;</item>\n"
        "</root>";
    m_preview->SetText(sampleXml);
    m_preview->SetReadOnly(true);
    
    // Set up margins
    m_preview->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    m_preview->SetMarginWidth(0, 40);
    
    // Add to panel sizer
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_preview, 1, wxEXPAND);
    m_previewPanel->SetSizer(sizer);
}

void SettingsDialog::OnThemeChanged(wxCommandEvent& event)
{
    if (!m_themeChoice)
        return;
    
    wxString selectedTheme = m_themeChoice->GetStringSelection();
    
    if (selectedTheme != "Custom")
    {
        // Apply preset theme
        if (selectedTheme == "Light")
            m_theme = EditorTheme::LightTheme();
        else if (selectedTheme == "Dark")
            m_theme = EditorTheme::DarkTheme();
        else if (selectedTheme == "Dracula")
            m_theme = EditorTheme::DraculaTheme();
        else if (selectedTheme == "Solarized Dark")
            m_theme = EditorTheme::SolarizedDarkTheme();
        
        m_themeName = selectedTheme;
        
        // Update color pickers to reflect new theme
        UpdateColorPickers();
    }
    else
    {
        m_themeName = "Custom";
    }
    
    UpdatePreview();
}

void SettingsDialog::OnFontChanged(wxFontPickerEvent& event)
{
    m_font = event.GetFont();
    UpdatePreview();
}

void SettingsDialog::OnColourChanged(wxColourPickerEvent& event)
{
    // When any color is manually changed, switch to Custom theme
    m_themeName = "Custom";
    
    if (m_themeChoice)
    {
        int customIndex = m_themeChoice->FindString("Custom");
        if (customIndex != wxNOT_FOUND)
            m_themeChoice->SetSelection(customIndex);
    }
    
    // Update theme colors from pickers
    if (m_bgColorPicker)
        m_theme.background = m_bgColorPicker->GetColour();
    if (m_fgColorPicker)
        m_theme.foreground = m_fgColorPicker->GetColour();
    if (m_lineNumFgPicker)
        m_theme.lineNumberFg = m_lineNumFgPicker->GetColour();
    if (m_lineNumBgPicker)
        m_theme.lineNumberBg = m_lineNumBgPicker->GetColour();
    if (m_tagColorPicker)
        m_theme.tagColor = m_tagColorPicker->GetColour();
    if (m_attrNameColorPicker)
        m_theme.attributeNameColor = m_attrNameColorPicker->GetColour();
    if (m_attrValueColorPicker)
        m_theme.attributeValueColor = m_attrValueColorPicker->GetColour();
    if (m_commentColorPicker)
        m_theme.commentColor = m_commentColorPicker->GetColour();
    if (m_textContentColorPicker)
        m_theme.textContentColor = m_textContentColorPicker->GetColour();
    
    UpdatePreview();
}

void SettingsDialog::UpdatePreview()
{
    if (!m_preview)
        return;
    
    // Apply font
    m_preview->StyleSetFont(wxSTC_STYLE_DEFAULT, m_font);
    m_preview->StyleClearAll();
    
    // Apply colors
    m_preview->StyleSetBackground(wxSTC_STYLE_DEFAULT, m_theme.background);
    m_preview->StyleSetForeground(wxSTC_STYLE_DEFAULT, m_theme.foreground);
    
    // Line numbers
    m_preview->StyleSetForeground(wxSTC_STYLE_LINENUMBER, m_theme.lineNumberFg);
    m_preview->StyleSetBackground(wxSTC_STYLE_LINENUMBER, m_theme.lineNumberBg);
    
    // XML tags
    m_preview->StyleSetForeground(wxSTC_H_TAG, m_theme.tagColor);
    m_preview->StyleSetBold(wxSTC_H_TAG, true);
    m_preview->StyleSetForeground(wxSTC_H_TAGEND, m_theme.tagColor);
    m_preview->StyleSetBold(wxSTC_H_TAGEND, true);
    m_preview->StyleSetForeground(wxSTC_H_TAGUNKNOWN, m_theme.tagColor);
    
    // Attributes
    m_preview->StyleSetForeground(wxSTC_H_ATTRIBUTE, m_theme.attributeNameColor);
    m_preview->StyleSetForeground(wxSTC_H_ATTRIBUTEUNKNOWN, m_theme.attributeNameColor);
    
    // Attribute values (strings)
    m_preview->StyleSetForeground(wxSTC_H_DOUBLESTRING, m_theme.attributeValueColor);
    m_preview->StyleSetForeground(wxSTC_H_SINGLESTRING, m_theme.attributeValueColor);
    m_preview->StyleSetForeground(wxSTC_H_VALUE, m_theme.attributeValueColor);
    
    // Comments
    m_preview->StyleSetForeground(wxSTC_H_COMMENT, m_theme.commentColor);
    m_preview->StyleSetItalic(wxSTC_H_COMMENT, true);
    
    // Entities
    m_preview->StyleSetForeground(wxSTC_H_ENTITY, m_theme.entityColor);
    
    // CDATA
    m_preview->StyleSetForeground(wxSTC_H_CDATA, m_theme.cdataColor);
    
    // XML declaration
    m_preview->StyleSetForeground(wxSTC_H_XMLSTART, m_theme.tagColor);
    m_preview->StyleSetForeground(wxSTC_H_XMLEND, m_theme.tagColor);
    
    // Default/other text
    m_preview->StyleSetForeground(wxSTC_H_DEFAULT, m_theme.textContentColor);
    m_preview->StyleSetBackground(wxSTC_H_DEFAULT, m_theme.background);
    
    // Set all backgrounds to theme background
    for (int i = 0; i <= wxSTC_H_SGML_ENTITY; i++)
    {
        m_preview->StyleSetBackground(i, m_theme.background);
    }
    
    // Set caret and selection colors
    m_preview->SetCaretForeground(m_theme.foreground);
    m_preview->SetSelBackground(true, m_theme.selectionBackground);
    
    m_preview->Refresh();
}

void SettingsDialog::OnApply(wxCommandEvent& event)
{
    // Save settings
    EditorSettings& settings = EditorSettings::Get();
    settings.SetFont(m_font);
    settings.SetTheme(m_theme);
    settings.SetThemeName(m_themeName);
    settings.Save();
}

void SettingsDialog::OnOK(wxCommandEvent& event)
{
    // Apply and close
    OnApply(event);
    EndModal(wxID_OK);
}
