#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

juce::StringArray LocationsProcessor::placeNames()
{
    // Reihenfolge muss zu loc::place() in LocationsDSP.h passen
    return { "Gleicher Raum", "Nebenraum", "Flur", "Treppenhaus", "Obere Etage", "Dachboden", "Keller",
             "Badezimmer", u8 ("Gro\xc3\x9f" "e Halle"), u8 ("Tunnel / H\xc3\xb6hle"),
             "Kirche", "Museum", "Tiefgarage",
             u8 ("Drau\xc3\x9f" "en: Feld"), u8 ("Drau\xc3\x9f" "en: Wald"),
             u8 ("Drau\xc3\x9f" "en: Stra\xc3\x9f" "e"), u8 ("Drau\xc3\x9f" "en: Schlucht / Felswand") };
}

juce::StringArray LocationsProcessor::barrierNames()
{
    return { u8 ("T\xc3\xbcr"), "Fenster", "Vorhang", "Autoscheibe", "Schrank", "Kofferraum", "Unter Wasser" };   // Reihenfolge wie loc::barrier()
}

juce::StringArray LocationsProcessor::weatherNames()
{
    return { "Ruhig", "Wind / Regen", "Sturm" };                          // Reihenfolge wie loc::weather()
}

juce::AudioProcessorValueTreeState::ParameterLayout LocationsProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    const auto names = placeNames();
    jassert (names.size() == loc::kNumPlaces);

    auto percent = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; });
    auto decibel = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (v, 1) + " dB"; });

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "placeA", 1 }, "Ort A", names, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "placeB", 1 }, "Ort B", names, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "blend", 1 }, "Position (A nach B)",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.0f, percent));
    auto distText = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { const int pct = roundToInt (v * 100.0f);
                            return pct <= 0 ? String ("nah") : pct >= 100 ? String ("weit") : String (pct) + " %"; });
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "distance", 1 }, "Start-Entfernung (Ort A)",
                                                       NormalisableRange<float> (0.0f, 1.0f), 1.0f, distText));
    auto doorText = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { const int pct = roundToInt (v * 100.0f);
                            return pct <= 0 ? String ("zu") : pct >= 100 ? String ("offen") : String (pct) + " %"; });
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "door", 1 }, "Verdeckung (zu-offen)",
                                                       NormalisableRange<float> (0.0f, 1.0f), 1.0f, doorText));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "barrier", 1 }, "Verdeckung (Art)", barrierNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "weather", 1 }, "Wetter-Filter (Ort A draussen)", weatherNames(), 0));
    {
        NormalisableRange<float> r (0.25f, 4.0f);
        r.setSkewForCentre (1.0f);   // Mitte = Original-Echo-Zeit des Ortes
        auto echoText = AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return "x" + String (v, 2); });
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "echoTime", 1 }, "Echo-Zeit (nur Draussen)", r, 1.0f, echoText));
    }
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "room", 1 }, "Raumanteil (Ort A)",
                                                       NormalisableRange<float> (-24.0f, 12.0f), 0.0f, decibel));
    auto panText = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { const int pct = roundToInt (std::abs (v) * 100.0f);
                            return pct == 0 ? String ("Mitte") : String (v < 0 ? "L " : "R ") + String (pct) + " %"; });
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "pan", 1 }, "Quelle L-R (Panorama)",
                                                       NormalisableRange<float> (-1.0f, 1.0f), 0.0f, panText));
    return layout;
}

LocationsProcessor::LocationsProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LOCATIONS", createLayout())
{
    pPlaceA = apvts.getRawParameterValue ("placeA");
    pPlaceB = apvts.getRawParameterValue ("placeB");
    pBlend = apvts.getRawParameterValue ("blend");
    pDistance = apvts.getRawParameterValue ("distance");
    pDoor = apvts.getRawParameterValue ("door");
    pRoom = apvts.getRawParameterValue ("room");
    pPan = apvts.getRawParameterValue ("pan");
    pBarrier = apvts.getRawParameterValue ("barrier");
    pWeather = apvts.getRawParameterValue ("weather");
    pEchoTime = apvts.getRawParameterValue ("echoTime");
}

double LocationsProcessor::getTailLengthSeconds() const
{
    auto duration = [] (int index, float scale)
    {
        const auto& def = loc::place (index);
        double tail = 2.0 * def.rt60;
        if (def.echoMs > 0)
        {
            const double repeats = def.echoFb > 0 ? 1.0 + std::ceil (std::log (1e-5) / std::log (def.echoFb)) : 1.0;
            tail = std::max (tail, def.echoMs * 0.001 * (index >= loc::kFirstOutdoor ? scale : 1.0f) * repeats);
        }
        return tail + def.delayMs * 0.001;
    };
    auto choice = [] (float x) { return (int) loc::finiteClamp (x, 0, loc::kNumPlaces - 1, 0); };
    const float scale = loc::finiteClamp (pEchoTime->load(), 0.25f, 4, 1);
    return std::max (duration (choice (pPlaceA->load()), scale), duration (choice (pPlaceB->load()), 1));
}

void LocationsProcessor::prepareToPlay (double sampleRate, int maxBlock)
{
    engine.prepare (sampleRate);
    monoScratch.assign ((size_t) juce::jmax (maxBlock, 4096), 0.0f);
    setLatencySamples (0);
}

bool LocationsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (in == out) return true;
    return in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo();   // Mono rein, Stereo raus
}

void LocationsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numSamples == 0 || numCh == 0)
        return;

    auto choice = [] (float x, int maximum) { return juce::roundToInt (loc::finiteClamp (x, 0, (float) maximum, 0)); };
    loc::Params p;
    p.placeA = choice (pPlaceA->load(), loc::kNumPlaces - 1);
    p.placeB = choice (pPlaceB->load(), loc::kNumPlaces - 1);
    p.blend = pBlend->load();
    p.distance = pDistance->load();   // Entfernung von Ort A; Ort B ist immer nah
    p.door = pDoor->load();
    p.roomDb = pRoom->load();
    p.pan = pPan->load();
    p.barrier = choice (pBarrier->load(), loc::kNumBarriers - 1);
    p.weather = choice (pWeather->load(), loc::kNumWeathers - 1);
    p.echoTime = pEchoTime->load();

    float* left = buffer.getWritePointer (0);

    if (numCh > 1)
    {
        // Mono rein, Stereo raus: den Eingang auf beide Seiten legen, dann wirkt L-R normal
        if (getTotalNumInputChannels() == 1)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
        engine.process (left, buffer.getWritePointer (1), numSamples, p);
    }
    else
    {
        // reine Mono-Spur: kein Panorama moeglich (das macht der Pan am Kanal), Raum links/rechts zusammenmischen
        p.pan = 0.0f;
        if (monoScratch.empty()) { buffer.clear(); return; }
        for (int offset = 0; offset < numSamples;)
        {
            const int count = std::min ((int) monoScratch.size(), numSamples - offset);
            auto* l = left + offset;
            auto* r = monoScratch.data();
            std::copy_n (l, count, r);
            engine.process (l, r, count, p);
            for (int i = 0; i < count; ++i) l[i] = 0.5f * (l[i] + r[i]);
            offset += count;
        }
    }

    for (int ch = 2; ch < numCh; ++ch)
        buffer.clear (ch, 0, numSamples);
}

void LocationsProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LocationsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* LocationsProcessor::createEditor() { return new LocationsEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LocationsProcessor(); }
