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
    void setupKnob (Knob&, const juce::String& paramId, const juce::String& text);

    LocationsProcessor& processor;
    juce::LookAndFeel_V4 look;
    juce::ComboBox placeA, placeB;
    juce::Label labelA, labelB, labelBlend;
    juce::Slider blend;
    Knob distance, door, room, level, out;
    juce::ToggleButton delay;
    std::unique_ptr<APVTS::ComboBoxAttachment> attA, attB;
    std::unique_ptr<APVTS::SliderAttachment> attBlend;
    std::unique_ptr<APVTS::ButtonAttachment> attDelay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocationsEditor)
};
