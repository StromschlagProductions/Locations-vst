#pragma once
#include "PluginProcessor.h"

class LocationsEditor : public juce::AudioProcessorEditor
{
public:
    explicit LocationsEditor (LocationsProcessor&);
    ~LocationsEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    struct Knob
    {
        juce::Slider slider; juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };
    struct Theme { juce::Colour bg, panel, accent, text, dim, line; bool dark; };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& text);
    void applyTheme();
    static Theme darkTheme();
    static Theme lightTheme();

    static constexpr int kBaseW = 680, kBaseH = 420;

    LocationsProcessor& processor;
    juce::LookAndFeel_V4 look;
    juce::Component content;      // feste 680x420-Flaeche, wird skaliert
    float uiScale = 1.0f;
    Theme theme;
    bool sizeReady = false;   // erst nach dem Wiederherstellen der Groesse Fenstergroesse speichern
    juce::Image logo;
    juce::ComboBox placeA, placeB;
    juce::Label labelA, labelB, labelBlend;
    juce::Slider blend;
    Knob distance, door, pan, room;
    juce::ComboBox barrierBox, weatherBox;
    juce::Label weatherLabel;
    std::unique_ptr<APVTS::ComboBoxAttachment> attBarrier, attWeather;
    juce::TextButton themeButton;
    juce::TextButton helpButton;
    juce::TextEditor helpText;
    juce::TooltipWindow tooltipWindow { this, 400 };
    std::unique_ptr<APVTS::ComboBoxAttachment> attA, attB;
    std::unique_ptr<APVTS::SliderAttachment> attBlend;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocationsEditor)
};
