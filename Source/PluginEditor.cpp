#include "PluginEditor.h"

namespace
{
const juce::Colour kBg (0xff15191f), kPanel (0xff1f2630), kAccent (0xffe8a23a), kText (0xffe6e9ee), kDim (0xff8a93a0);
juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
}

LocationsEditor::LocationsEditor (LocationsProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    look.setColour (juce::Slider::thumbColourId, kAccent);
    look.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    look.setColour (juce::Slider::rotarySliderOutlineColourId, kPanel.brighter (0.25f));
    look.setColour (juce::Slider::trackColourId, kAccent.withAlpha (0.6f));
    look.setColour (juce::Slider::backgroundColourId, kPanel.brighter (0.25f));
    look.setColour (juce::Slider::textBoxTextColourId, kText);
    look.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    look.setColour (juce::ComboBox::backgroundColourId, kPanel);
    look.setColour (juce::ComboBox::textColourId, kText);
    look.setColour (juce::ComboBox::outlineColourId, kPanel.brighter (0.4f));
    look.setColour (juce::PopupMenu::backgroundColourId, kPanel);
    look.setColour (juce::PopupMenu::textColourId, kText);
    look.setColour (juce::PopupMenu::highlightedBackgroundColourId, kAccent);
    look.setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    look.setColour (juce::ToggleButton::textColourId, kText);
    look.setColour (juce::ToggleButton::tickColourId, kAccent);
    look.setColour (juce::Label::textColourId, kText);
    setLookAndFeel (&look);

    const auto names = LocationsProcessor::placeNames();
    for (auto* box : { &placeA, &placeB })
    {
        box->addItemList (names, 1);
        addAndMakeVisible (*box);
    }
    attA = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, "placeA", placeA);
    attB = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, "placeB", placeB);

    labelA.setText ("ORT A", juce::dontSendNotification);
    labelB.setText ("ORT B", juce::dontSendNotification);
    labelB.setJustificationType (juce::Justification::centredRight);
    labelBlend.setText (u8 ("\xe2\x97\x80  A      Blende      B  \xe2\x96\xb6"), juce::dontSendNotification);
    labelBlend.setJustificationType (juce::Justification::centred);
    for (auto* l : { &labelA, &labelB, &labelBlend })
    {
        l->setColour (juce::Label::textColourId, kDim);
        addAndMakeVisible (*l);
    }

    blend.setSliderStyle (juce::Slider::LinearHorizontal);
    blend.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible (blend);
    attBlend = std::make_unique<APVTS::SliderAttachment> (processor.apvts, "blend", blend);

    setupKnob (distance, "distance", "Entfernung");
    setupKnob (door, "door", u8 ("T\xc3\xbcr  zu \xe2\x86\x94 offen"));
    setupKnob (room, "room", "Raumanteil");
    setupKnob (level, "level", "Pegelwirkung");
    setupKnob (out, "out", "Ausgang");

    delay.setButtonText (u8 ("Weg-Verz\xc3\xb6gerung"));
    addAndMakeVisible (delay);
    attDelay = std::make_unique<APVTS::ButtonAttachment> (processor.apvts, "delay", delay);

    setSize (660, 380);
}

LocationsEditor::~LocationsEditor() { setLookAndFeel (nullptr); }

void LocationsEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    addAndMakeVisible (k.slider);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<APVTS::SliderAttachment> (processor.apvts, paramId, k.slider);
}

void LocationsEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (kPanel);
    g.fillRoundedRectangle (12.0f, 52.0f, (float) getWidth() - 24.0f, 122.0f, 8.0f);
    g.fillRoundedRectangle (12.0f, 184.0f, (float) getWidth() - 24.0f, 184.0f, 8.0f);
    g.setColour (kAccent);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("LOCATIONS", 20, 12, 300, 30, juce::Justification::centredLeft);
    g.setColour (kDim);
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Stromschlag Audio", getWidth() - 220, 12, 200, 30, juce::Justification::centredRight);
}

void LocationsEditor::resized()
{
    const int w = getWidth();
    labelA.setBounds (24, 58, 200, 18);
    labelB.setBounds (w - 224, 58, 200, 18);
    placeA.setBounds (24, 78, 190, 28);
    placeB.setBounds (w - 214, 78, 190, 28);
    labelBlend.setBounds (224, 58, w - 448, 18);
    blend.setBounds (24, 116, w - 48, 50);

    juce::Rectangle<int> row (24, 192, w - 48, 136);
    const int kw = row.getWidth() / 5;
    for (auto* k : { &distance, &door, &room, &level, &out })
    {
        auto cell = row.removeFromLeft (kw);
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (8, 2));
    }
    delay.setBounds (24, 334, 220, 26);
}
