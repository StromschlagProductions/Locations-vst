#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }

juce::StringArray LocationsProcessor::placeNames()
{
    // Reihenfolge muss zu loc::place() in LocationsDSP.h passen
    return { "Gleicher Raum", "Nebenraum", "Flur", "Treppenhaus", "Obere Etage", "Keller",
             u8 ("Gro\xc3\x9f" "e Halle"), u8 ("Drau\xc3\x9f" "en: Feld"),
             u8 ("Drau\xc3\x9f" "en: Wald"), u8 ("Drau\xc3\x9f" "en: Stra\xc3\x9f" "e") };
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
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "blend", 1 }, "Blende A-B",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.0f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "distance", 1 }, "Entfernung",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.3f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "door", 1 }, u8 ("T\xc3\xbcr (zu-offen)"),
                                                       NormalisableRange<float> (0.0f, 1.0f), 1.0f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "room", 1 }, "Raumanteil",
                                                       NormalisableRange<float> (-24.0f, 12.0f), 0.0f, decibel));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "level", 1 }, "Pegelwirkung",
                                                       NormalisableRange<float> (0.0f, 1.0f), 1.0f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Ausgang",
                                                       NormalisableRange<float> (-24.0f, 12.0f), 0.0f, decibel));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "delay", 1 }, u8 ("Weg-Verz\xc3\xb6gerung"), false));
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
    pLevel = apvts.getRawParameterValue ("level");
    pOut = apvts.getRawParameterValue ("out");
    pDelay = apvts.getRawParameterValue ("delay");
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
    p.distance = pDistance->load();
    p.door = pDoor->load();
    p.roomDb = pRoom->load();
    p.levelAmt = pLevel->load();
    p.outDb = pOut->load();
    p.pathDelay = pDelay->load() > 0.5f;

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
