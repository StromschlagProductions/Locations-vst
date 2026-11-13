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

    LocationsProcessor& processor;
    juce::LookAndFeel_V4 look;
    Theme theme;
    juce::Image logo;
    juce::ComboBox placeA, placeB;
    juce::Label labelA, labelB, labelBlend;
    juce::Slider blend;
    Knob distance, door, room, level, out;
    juce::ToggleButton delay;
    juce::TextButton themeButton;
    std::unique_ptr<APVTS::ComboBoxAttachment> attA, attB;
    std::unique_ptr<APVTS::SliderAttachment> attBlend;
    std::unique_ptr<APVTS::ButtonAttachment> attDelay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocationsEditor)
};
