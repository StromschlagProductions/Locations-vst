#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
const char* kThemeProp = "theme";   // 0 = dunkel, 1 = hell (wird mit dem Projekt gespeichert)
}

// Farben angelehnt an die Control-Room-App (Schiefer-Gruen + Orange)
LocationsEditor::Theme LocationsEditor::darkTheme()
{
    return { juce::Colour (0xff182326), juce::Colour (0xff233539), juce::Colour (0xfff5781a),
             juce::Colour (0xffe8ecec), juce::Colour (0xff8fa1a5), juce::Colour (0xff35494e), true };
}
LocationsEditor::Theme LocationsEditor::lightTheme()
{
    return { juce::Colour (0xffd5dbda), juce::Colour (0xffe8eae9), juce::Colour (0xfff5781a),
             juce::Colour (0xff233539), juce::Colour (0xff5d6e72), juce::Colour (0xffb5bfbe), false };
}

LocationsEditor::LocationsEditor (LocationsProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    addAndMakeVisible (content);
    setResizable (true, true);
    setResizeLimits (kBaseW / 2, kBaseH / 2, kBaseW * 3, kBaseH * 3);   // 50 % bis 300 %
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) kBaseW / (double) kBaseH);

    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);
    theme = (int) processor.apvts.state.getProperty (kThemeProp, 0) == 1 ? lightTheme() : darkTheme();
    setLookAndFeel (&look);

    const auto names = LocationsProcessor::placeNames();
    for (auto* box : { &placeA, &placeB })
    {
        box->addItemList (names, 1);
        content.addAndMakeVisible (*box);
    }
    attA = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, "placeA", placeA);
    attB = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, "placeB", placeB);

    labelA.setText ("ORT A (START)", juce::dontSendNotification);
    labelB.setText ("ORT B (ZIEL)", juce::dontSendNotification);
    labelB.setJustificationType (juce::Justification::centredRight);
    labelBlend.setText (u8 ("\xe2\x97\x80  bei A (weit)     POSITION     bei B (nah)  \xe2\x96\xb6"), juce::dontSendNotification);
    labelBlend.setJustificationType (juce::Justification::centred);
    for (auto* l : { &labelA, &labelB, &labelBlend })
        content.addAndMakeVisible (*l);

    blend.setSliderStyle (juce::Slider::LinearHorizontal);
    blend.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    content.addAndMakeVisible (blend);
    attBlend = std::make_unique<APVTS::SliderAttachment> (processor.apvts, "blend", blend);

    setupKnob (distance, "distance", "Start-Entfernung");
    setupKnob (door, "door", u8 ("T\xc3\xbcr  zu \xe2\x86\x94 offen"));
    setupKnob (room, "room", "Raumanteil");
    setupKnob (level, "level", "Pegelwirkung");
    setupKnob (out, "out", "Ausgang");

    delay.setButtonText (u8 ("Weg-Verz\xc3\xb6gerung"));
    content.addAndMakeVisible (delay);
    attDelay = std::make_unique<APVTS::ButtonAttachment> (processor.apvts, "delay", delay);

    themeButton.onClick = [this]
    {
        theme = theme.dark ? lightTheme() : darkTheme();
        processor.apvts.state.setProperty (kThemeProp, theme.dark ? 0 : 1, nullptr);
        applyTheme();
    };
    content.addAndMakeVisible (themeButton);

    applyTheme();
    setSize (kBaseW, kBaseH);
}

LocationsEditor::~LocationsEditor() { setLookAndFeel (nullptr); }

void LocationsEditor::applyTheme()
{
    look.setColour (juce::Slider::thumbColourId, theme.accent);
    look.setColour (juce::Slider::rotarySliderFillColourId, theme.accent);
    look.setColour (juce::Slider::rotarySliderOutlineColourId, theme.line);
    look.setColour (juce::Slider::trackColourId, theme.accent.withAlpha (0.75f));
    look.setColour (juce::Slider::backgroundColourId, theme.line);
    look.setColour (juce::Slider::textBoxTextColourId, theme.text);
    look.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    look.setColour (juce::ComboBox::backgroundColourId, theme.bg);
    look.setColour (juce::ComboBox::textColourId, theme.text);
    look.setColour (juce::ComboBox::arrowColourId, theme.text);
    look.setColour (juce::ComboBox::outlineColourId, theme.line);
    look.setColour (juce::PopupMenu::backgroundColourId, theme.panel);
    look.setColour (juce::PopupMenu::textColourId, theme.text);
    look.setColour (juce::PopupMenu::highlightedBackgroundColourId, theme.accent);
    look.setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    look.setColour (juce::ToggleButton::textColourId, theme.text);
    look.setColour (juce::ToggleButton::tickColourId, theme.accent);
    look.setColour (juce::ToggleButton::tickDisabledColourId, theme.dim);
    look.setColour (juce::Label::textColourId, theme.text);
    look.setColour (juce::TextButton::buttonColourId, theme.dark ? theme.panel : theme.bg);
    look.setColour (juce::TextButton::textColourOffId, theme.text);

    for (auto* l : { &labelA, &labelB, &labelBlend })
        l->setColour (juce::Label::textColourId, theme.dim);
    themeButton.setButtonText (theme.dark ? "Look: Dunkel" : "Look: Hell");
    sendLookAndFeelChange();
    repaint();
}

void LocationsEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    content.addAndMakeVisible (k.slider);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    content.addAndMakeVisible (k.label);
    k.attachment = std::make_unique<APVTS::SliderAttachment> (processor.apvts, paramId, k.slider);
}

void LocationsEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme.bg);
    g.addTransform (juce::AffineTransform::scale (uiScale));
    const int W = kBaseW;

    // Kopfleiste: folgt dem Look (dunkel = dunkle Leiste + helles Logo, hell = helle Leiste + dunkles Logo)
    g.setColour (theme.panel);
    g.fillRect (0, 0, W, 64);
    g.setColour (theme.line);
    g.fillRect (0, 63, W, 1);
    g.setColour (theme.accent);
    g.fillRect (0, 0, 5, 64);

    if (logo.isValid())
    {
        g.setColour (theme.text);   // Logo ist eine Tinten-Maske und wird in der Textfarbe gezeichnet
        g.drawImage (logo, juce::Rectangle<float> (14.0f, 4.0f, 56.0f, 56.0f),
                     juce::RectanglePlacement::centred, true);
    }

    g.setColour (theme.text);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("LOCATIONS", 80, 8, 260, 30, juce::Justification::centredLeft);
    g.setColour (theme.dim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (u8 ("STROMSCHLAG AUDIO  \xc2\xb7  KOKOPELLI STUDIO"), 80, 36, 300, 16, juce::Justification::centredLeft);

    // Karten
    g.setColour (theme.panel);
    g.fillRoundedRectangle (12.0f, 76.0f, (float) W - 24.0f, 122.0f, 8.0f);
    g.fillRoundedRectangle (12.0f, 208.0f, (float) W - 24.0f, 200.0f, 8.0f);
    g.setColour (theme.line);
    g.drawRoundedRectangle (12.5f, 76.5f, (float) W - 25.0f, 121.0f, 8.0f, 1.0f);
    g.drawRoundedRectangle (12.5f, 208.5f, (float) W - 25.0f, 199.0f, 8.0f, 1.0f);
}

void LocationsEditor::resized()
{
    uiScale = std::min ((float) getWidth() / kBaseW, (float) getHeight() / kBaseH);
    content.setTransform (juce::AffineTransform::scale (uiScale));
    content.setBounds (0, 0, kBaseW, kBaseH);
    repaint();

    const int w = kBaseW;
    themeButton.setBounds (w - 128, 18, 112, 28);

    labelA.setBounds (24, 82, 200, 18);
    labelB.setBounds (w - 224, 82, 200, 18);
    placeA.setBounds (24, 102, 190, 28);
    placeB.setBounds (w - 214, 102, 190, 28);
    labelBlend.setBounds (224, 82, w - 448, 18);
    blend.setBounds (24, 140, w - 48, 50);

    juce::Rectangle<int> row (24, 216, w - 48, 150);
    const int kw = row.getWidth() / 5;
    for (auto* k : { &distance, &door, &room, &level, &out })
    {
        auto cell = row.removeFromLeft (kw);
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (8, 2));
    }
    delay.setBounds (24, 372, 220, 26);
}
