#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

juce::StringArray LocationsProcessor::placeNames()
{
    // Reihenfolge muss zu loc::place() in LocationsDSP.h passen
    return { "Gleicher Raum", "Nebenraum", "Flur", "Treppenhaus", "Obere Etage", "Dachboden", "Keller",
             "Badezimmer", u8 ("Gro\xc3\x9f" "e Halle"), u8 ("Tunnel / H\xc3\xb6hle"),
             u8 ("Drau\xc3\x9f" "en: Feld"), u8 ("Drau\xc3\x9f" "en: Wald"),
             u8 ("Drau\xc3\x9f" "en: Stra\xc3\x9f" "e"), u8 ("Drau\xc3\x9f" "en: Schlucht / Felswand") };
}

juce::StringArray LocationsProcessor::barrierNames()
{
    return { u8 ("T\xc3\xbcr"), "Fenster", "Vorhang", "Autoscheibe" };   // Reihenfolge wie loc::barrier()
}

juce::StringArray LocationsProcessor::weatherNames()
{
    return { "Ruhig", "Wind", "Regen", "Sturm" };                          // Reihenfolge wie loc::weather()
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
    auto rearText = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { const int pct = roundToInt (v * 100.0f);
                            return pct <= 0 ? String ("vorne") : pct >= 100 ? String ("hinten") : String (pct) + " %"; });
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "rear", 1 }, "Quelle vorne-hinten",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.0f, rearText));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "barrier", 1 }, "Verdeckung (Art)", barrierNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "weather", 1 }, "Wetter-Filter (Ort A draussen)", weatherNames(), 0));
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
    pRear = apvts.getRawParameterValue ("rear");
    pBarrier = apvts.getRawParameterValue ("barrier");
    pWeather = apvts.getRawParameterValue ("weather");
}

void LocationsProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    setLatencySamples (0);
}

bool LocationsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void LocationsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numSamples == 0 || numCh == 0)
        return;

    loc::Params p;
    p.placeA = juce::roundToInt (pPlaceA->load());
    p.placeB = juce::roundToInt (pPlaceB->load());
    p.blend = pBlend->load();
    p.distance = pDistance->load();   // Entfernung von Ort A; Ort B ist immer nah
    p.door = pDoor->load();
    p.roomDb = pRoom->load();
    p.pan = pPan->load();
    p.rear = pRear->load();
    p.barrier = juce::roundToInt (pBarrier->load());
    p.weather = juce::roundToInt (pWeather->load());

    float* left = buffer.getWritePointer (0);
    float* right = numCh > 1 ? buffer.getWritePointer (1) : left;
    engine.process (left, right, numSamples, p);

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
