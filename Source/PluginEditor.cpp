#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
const char* kThemeProp = "theme";   // 0 = dunkel, 1 = hell (wird mit dem Projekt gespeichert)
const char* kUiWProp = "uiW";       // Fenstergroesse (wird mit dem Projekt gespeichert)
const char* kUiHProp = "uiH";
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

// Inhalt der Onboard-Hilfe, in der Reihenfolge der Oberflaeche
static juce::AttributedString buildHelp (juce::Colour text, juce::Colour accent, juce::Colour dim)
{
    juce::AttributedString s;
    s.setWordWrap (juce::AttributedString::byWord);
    s.setJustification (juce::Justification::topLeft);

    auto head = [&] (const char* t)
    { s.append (u8 (t) + "\n", juce::Font (juce::FontOptions (14.0f, juce::Font::bold)), accent); };
    auto body = [&] (const char* t)
    { s.append (u8 (t) + "\n\n", juce::Font (juce::FontOptions (13.0f)), text); };
    auto note = [&] (const char* t)
    { s.append (u8 (t) + "\n\n", juce::Font (juce::FontOptions (12.0f)), dim); };

    s.append (u8 ("LOCATIONS – Hilfe") + u8 (" \xc2\xb7 v") + juce::String (PLUGIN_VERSION_TEXT) + "\n\n",
              juce::Font (juce::FontOptions (17.0f, juce::Font::bold)), accent);

    head ("ORT A / ORT B");
    body ("Ort A ist der Startort der Bewegung, Ort B das Ziel. Meistens ist Ort B „Gleicher Raum“, also der Raum der Szene. Ort B ist immer nah.");

    head ("POSITION (A nach B)");
    body ("Der wichtigste Regler: 0 % = Quelle bei Ort A, 100 % = bei Ort B. Automatisiere ihn für jede Bewegung, zum Beispiel Treppe herunter, Suchtrupp kommt näher oder jemand geht weg.");

    head ("Kasten ORT A (wirkt nur am Startort)");
    body ("Entfernung: wie weit weg Ort A ist, von nah bis weit. Beispiel Suchtrupp: weit = tief im Wald, 40 % = ein paar Bäume entfernt.");
    body ("Echo-Zeit: verlängert oder verkürzt das Echo, wenn Ort A draußen ist (Wald, Straße, Schlucht). Mitte = Original, rechts länger, zum Beispiel für weite Täler oder Alpen; angezeigt in Millisekunden. Bei allen anderen Orten ist der Regler grau und ohne Wirkung. Der Hall ist draußen nur noch knapp hörbar, das Echo trägt den Raumeindruck.");
    body ("Raumanteil: mehr oder weniger Hall bzw. Echo von Ort A, in dB. Feinregler, normalerweise auf 0 dB lassen.");
    body ("Wetter-Filter: Ruhig, Wind / Regen, Sturm. Ändert nur den Klang, es kommen keine Wind- oder Regengeräusche dazu. Wirkt nur, wenn Ort A draußen ist, stärker mit der Entfernung, und verschwindet bei Ort B.");

    head ("Kasten QUELLE (L ↔ R)");
    body ("Platziert die Quelle links oder rechts: Stimme, Schritte, Geräusch auf der Spur. Hall und Echo bleiben in der Mitte, anders als beim Pan am Kanal. Gilt über die ganze Bewegung.");
    note ("Funktioniert auf Stereo-Spuren und Stereo-Bussen. Auf einer Mono-Spur ohne Wirkung: dort den Pan am Kanal benutzen oder die Spur auf Stereo stellen (die Mono-Datei bleibt dabei unverändert).");

    head ("Kasten VERDECKUNG (ganz rechts)");
    body ("Etwas, das sich zwischen Quelle und Hörer stellt und den Klang dämpft. Art: Tür, Fenster, Vorhang, Autoscheibe, Schrank, Kofferraum oder Unter Wasser.");
    body ("zu ↔ offen: zu = dumpf und leise mit dem Klang des Hindernisses, offen = keine Wirkung. Die Verdeckung hängt nicht an der Position, sie wirkt also auch bei Ort B (100 %). So funktionieren Kofferraum oder Autoscheibe, auch wenn die Quelle nah ist. Für „frei im Raum“ ganz auf offen stellen.");
    body ("Solange die Verdeckung wirkt, ist ihr Kasten orange umrandet und zeigt „aktiv“.");
    note ("Fenster gekippt = etwa in der Mitte. Autoscheibe: jemand spricht von außen ins Auto oder umgekehrt.");

    head ("Allgemein");
    body ("Doppelklick auf einen Drehregler oder Strg-Klick setzt ihn auf den Standardwert zurück.");
    body ("Look: Dunkel / Hell umschalten, die Wahl wird mit dem Projekt gespeichert. Das Fenster ist am rechten unteren Rand skalierbar (50–300 %).");

    head ("Idee und Konzept");
    body ("Lars Eickstädt, Stromschlag Audio");
    head ("Beta-Testing");
    body ("Markus Hildebrandt (SOUNDJOB)");
    note ("Gebaut mit JUCE.");

    return s;
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

    placeA.onChange = [this] { updateEchoState(); };
    setupKnob (echo, "echoTime", "Echo-Zeit");
    echo.slider.textFromValueFunction = [this] (double v)
    {
        // Anzeige in ms: Echo-Zeit von Ort A mal Faktor
        const int a = juce::roundToInt (processor.apvts.getRawParameterValue ("placeA")->load());
        if (a < loc::kFirstOutdoor || loc::place (a).echoMs <= 0.0f) return juce::String ("--");
        return juce::String (juce::roundToInt (loc::place (a).echoMs * (float) v)) + " ms";
    };
    echo.slider.valueFromTextFunction = [] (const juce::String& t) { return (double) t.getFloatValue(); };
    updateEchoState();

    setupKnob (distance, "distance", "Entfernung");
    setupKnob (door, "door", u8 ("zu \xe2\x86\x94 offen"));
    door.slider.onValueChange = [this] { repaint(); };   // Kasten VERDECKUNG zeigt an, ob sie wirkt
    setupKnob (pan, "pan", u8 ("L \xe2\x86\x94 R"));
    setupKnob (room, "room", "Raumanteil");

    barrierBox.addItemList (LocationsProcessor::barrierNames(), 1);
    weatherBox.addItemList (LocationsProcessor::weatherNames(), 1);
    content.addAndMakeVisible (barrierBox);
    content.addAndMakeVisible (weatherBox);
    attBarrier = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, "barrier", barrierBox);
    attWeather = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, "weather", weatherBox);
    weatherLabel.setText ("Wetter-Filter", juce::dontSendNotification);
    weatherLabel.setJustificationType (juce::Justification::centredLeft);
    content.addAndMakeVisible (weatherLabel);


    themeButton.onClick = [this]
    {
        theme = theme.dark ? lightTheme() : darkTheme();
        processor.apvts.state.setProperty (kThemeProp, theme.dark ? 0 : 1, nullptr);
        applyTheme();
    };
    content.addAndMakeVisible (themeButton);

    helpButton.setButtonText ("?");
    helpButton.setTooltip (u8 ("Hilfe: was machen die Regler?"));
    helpButton.onClick = [this] { helpScroll.setVisible (! helpScroll.isVisible()); };
    content.addAndMakeVisible (helpButton);

    helpScroll.setViewedComponent (&helpText, false);
    helpScroll.setScrollBarsShown (true, false);
    helpScroll.setVisible (false);
    content.addChildComponent (helpScroll);   // startet ausgeblendet

    // Tooltips
    placeA.setTooltip (u8 ("Startort der Bewegung"));
    placeB.setTooltip (u8 ("Zielort der Bewegung, meist Gleicher Raum"));
    echo.slider.setTooltip (u8 ("Echo-Zeit von Ort A, wenn er drau\xc3\x9f""en ist (Wald, Stra\xc3\x9f""e, Schlucht): Mitte = Original, rechts l\xc3\xa4nger (weite T\xc3\xa4ler, Alpen). Bei anderen Orten ohne Wirkung."));
    blend.setTooltip (u8 ("Position: 0 % = bei Ort A (weit), 100 % = bei Ort B (nah). Hauptregler fuer die Bewegung."));
    distance.slider.setTooltip (u8 ("Wie weit weg Ort A ist (nah = 0 %, weit = 100 %). Ort B ist immer nah."));
    door.slider.setTooltip (u8 ("zu = dumpf und leise (Klang je nach Art der Verdeckung), offen = keine Wirkung. Wirkt auch bei Ort B (100 %) \xe2\x80\x93 f\xc3\xbcr frei im Raum ganz auf offen stellen."));
    barrierBox.setTooltip (u8 ("Was zwischen Quelle und H\xc3\xb6rer ist: T\xc3\xbcr, Fenster, Vorhang, Autoscheibe, Schrank, Kofferraum oder Unter Wasser"));
    weatherBox.setTooltip (u8 ("Ändert nur den Klang, erzeugt keine Wind- oder Regengeräusche: Höhen werden geschluckt, Böen lassen Rufe schwanken. Wirkt nur, wenn Ort A draußen ist, und stärker mit der Entfernung."));
    weatherLabel.setTooltip (weatherBox.getTooltip());
    pan.slider.setTooltip (u8 ("Stimme bzw. Ger\xc3\xa4usch links/rechts platzieren. Hall und Echo bleiben in der Mitte. Wirkt bei Ort A, auf dem Weg und bei Ort B."));
    room.slider.setTooltip (u8 ("Mehr oder weniger Hall/Echo von Ort A (Feinregler, dB)"));

    applyTheme();
    // zuletzt gespeicherte Fenstergroesse wiederherstellen (sonst 100 %)
    const int savedW = (int) processor.apvts.state.getProperty (kUiWProp, kBaseW);
    const int savedH = (int) processor.apvts.state.getProperty (kUiHProp, kBaseH);
    juce::ignoreUnused (savedH);   // Hoehe aus dem Seitenverhaeltnis ableiten (altes 1.x-Fenster war niedriger)
    const int restoredW = juce::jlimit (kBaseW / 2, kBaseW * 3, savedW);
    setSize (restoredW, juce::roundToInt ((float) restoredW * (float) kBaseH / (float) kBaseW));
    sizeReady = true;
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
    helpText.setAttributed (buildHelp (theme.text, theme.accent, theme.dim), theme.panel);
    helpScroll.fill = theme.panel;
    helpScroll.edge = theme.line;
    sendLookAndFeelChange();
    repaint();
}

void LocationsEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& text)
{
    // nur senkrecht ziehen (wie bei den meisten Plugins): hoch = mehr, runter = weniger; seitliche Handbewegung zaehlt nicht mit
    k.slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    k.slider.setMouseDragSensitivity (180);   // Pixel fuer den ganzen Regelweg (Standard waere 250)
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
    g.drawText ("LOCATIONS", 80, 8, 150, 30, juce::Justification::centredLeft);
    g.setColour (theme.accent);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("AUDIO DRAMA OFF-STAGE SIMULATOR", 228, 15, 330, 22, juce::Justification::centredLeft);
    g.setColour (theme.dim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (u8 ("STROMSCHLAG AUDIO  \xc2\xb7  v" PLUGIN_VERSION_TEXT), 80, 36, 300, 16, juce::Justification::centredLeft);

    // Kleines Symbol: halboffene Tuer, die Schallwellen kommen aus der Oeffnung (einfarbig, folgt dem Look)
    {
        const float ox = 0.0f, oy = 0.0f;   // Symbol wird in eigenen Koordinaten gebaut und dann verschoben/vergroessert
        juce::Path door, leaf, waves;
        door.addRectangle (ox + 1.0f, oy + 1.0f, 11.5f, 24.0f);
        leaf.startNewSubPath (ox + 1.0f, oy + 1.0f);
        leaf.lineTo (ox + 6.5f, oy + 4.5f);
        leaf.lineTo (ox + 6.5f, oy + 21.5f);
        leaf.lineTo (ox + 1.0f, oy + 25.0f);
        leaf.closeSubPath();
        const float cx = ox + 6.5f, cy = oy + 13.0f, half = 0.62f;
        for (float r : { 4.0f, 8.5f, 13.0f })
            waves.addArc (cx - r, cy - r, 2.0f * r, 2.0f * r,
                          juce::MathConstants<float>::halfPi - half,
                          juce::MathConstants<float>::halfPi + half, true);
        juce::GlyphArrangement ga;
        ga.addLineOfText (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)),
                          "AUDIO DRAMA OFF-STAGE SIMULATOR", 0.0f, 0.0f);
        const float subEnd  = 228.0f + ga.getBoundingBox (0, -1, true).getWidth();   // Ende des Untertitels
        const float helpX   = (float) (kBaseW - 170);                                 // linke Kante des ?-Knopfs
        const float mid     = 0.5f * (subEnd + helpX);                                // Mitte dazwischen
        const auto place = juce::AffineTransform::scale (1.3f).translated (mid - 11.9f, 15.0f);   // 11,9 = Mitte des Symbols
        door.applyTransform (place);
        leaf.applyTransform (place);
        waves.applyTransform (place);
        // dezent: mischt Textfarbe zum Leistenhintergrund, damit Titel und Logo kraeftiger bleiben
        const auto ink  = theme.panel.interpolatedWith (theme.text, 0.30f);
        const auto fill = theme.panel.interpolatedWith (theme.text, 0.10f);
        g.setColour (fill);
        g.fillPath (leaf);
        g.setColour (ink);
        g.strokePath (door, juce::PathStrokeType (1.8f));
        g.strokePath (leaf, juce::PathStrokeType (1.4f));
        g.strokePath (waves, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Karten
    g.setColour (theme.panel);
    g.fillRoundedRectangle (12.0f, 76.0f, (float) W - 24.0f, 122.0f, 8.0f);
    g.setColour (theme.line);
    g.drawRoundedRectangle (12.5f, 76.5f, (float) W - 25.0f, 121.0f, 8.0f, 1.0f);

    // Untere Kaesten: ORT A | VERDECKUNG | QUELLE
    struct Box { float x, w; const char* title; const char* note; };
    const Box boxes[] = { { 12.0f, 336.0f, "ORT A", "nur am Startort" },
                          { 356.0f, 124.0f, "QUELLE", "" },
                          { 488.0f, (float) W - 500.0f, "VERDECKUNG", "" } };
    const bool barrierActive = door.slider.getValue() < 0.995;   // Verdeckung wirkt (nicht ganz offen)
    for (const auto& b : boxes)
    {
        const bool hot = barrierActive && juce::String (b.title) == "VERDECKUNG";
        g.setColour (theme.panel);
        g.fillRoundedRectangle (b.x, 208.0f, b.w, 200.0f, 8.0f);
        g.setColour (hot ? theme.accent : theme.line);
        g.drawRoundedRectangle (b.x + 0.5f, 208.5f, b.w - 1.0f, 199.0f, 8.0f, hot ? 2.0f : 1.0f);
        g.setColour (theme.accent);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (b.title, (int) b.x + 12, 214, (int) b.w - 24, 16, juce::Justification::centredLeft);
        g.setColour (hot ? theme.accent : theme.dim);
        g.setFont (juce::FontOptions (10.5f, hot ? juce::Font::bold : juce::Font::plain));
        g.drawText (hot ? juce::String ("aktiv") : u8 (b.note), (int) b.x + 12, 214, (int) b.w - 24, 16, juce::Justification::centredRight);
    }
}

void LocationsEditor::resized()
{
    if (sizeReady)
    {
        processor.apvts.state.setProperty (kUiWProp, getWidth(), nullptr);
        processor.apvts.state.setProperty (kUiHProp, getHeight(), nullptr);
    }
    uiScale = std::min ((float) getWidth() / kBaseW, (float) getHeight() / kBaseH);
    content.setTransform (juce::AffineTransform::scale (uiScale));
    content.setBounds (0, 0, kBaseW, kBaseH);
    repaint();

    const int w = kBaseW;
    // Look links, Hilfe ganz rechts (gleicher Gesamtplatz, nur getauscht)
    themeButton.setBounds (w - 170, 18, 112, 28);
    helpButton.setBounds (w - 48, 18, 32, 28);
    helpScroll.setBounds (12, 76, w - 24, 332);   // deckt die Kartenflaeche (y 76 bis 408) genau ab
    helpText.setWidth (helpScroll.getMaximumVisibleWidth());

    labelA.setBounds (24, 82, 200, 18);
    labelB.setBounds (w - 224, 82, 200, 18);
    placeA.setBounds (24, 102, 190, 28);
    placeB.setBounds (w - 214, 102, 190, 28);
    labelBlend.setBounds (224, 82, w - 448, 18);
    blend.setBounds (24, 140, w - 48, 50);

    auto placeKnob = [] (Knob& k, int x, int y, int cw)
    {
        k.label.setBounds (x, y, cw, 18);
        k.slider.setBounds (x + 10, y + 20, cw - 20, 100);
    };
    // ORT A: Entfernung, Raumanteil, darunter Wetter-Filter
    placeKnob (distance, 16, 236, 108);
    placeKnob (room, 124, 236, 108);
    placeKnob (echo, 232, 236, 108);
    weatherLabel.setBounds (24, 368, 100, 26);
    weatherBox.setBounds (124, 366, 200, 28);
    // QUELLE (Mitte): L-R
    placeKnob (pan, 356, 236, 124);
    // VERDECKUNG (rechts aussen, wirkt bis zum Schluss): Regler, Art-Auswahl darunter (auf Hoehe des Wetter-Filters)
    const int vx = 494, vw = w - 12 - 6 - vx;
    placeKnob (door, vx, 236, vw);
    barrierBox.setBounds (vx + 8, 366, vw - 16, 28);
    helpScroll.toFront (false);
}

// Echo-Zeit ist nur aktiv (orange), wenn Ort A ein Draussen-Ort mit Echo ist (Wald, Strasse, Schlucht)
void LocationsEditor::updateEchoState()
{
    const int a = juce::roundToInt (processor.apvts.getRawParameterValue ("placeA")->load());
    const bool active = a >= loc::kFirstOutdoor && loc::place (a).echoMs > 0.0f;
    echo.slider.setEnabled (active);
    echo.slider.setAlpha (active ? 1.0f : 0.35f);
    echo.label.setAlpha (active ? 1.0f : 0.45f);
    echo.slider.updateText();
}
