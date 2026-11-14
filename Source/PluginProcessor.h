#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "LocationsDSP.h"

class LocationsProcessor : public juce::AudioProcessor
{
public:
    LocationsProcessor();
    ~LocationsProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void reset() override { engine.reset(); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Locations"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::StringArray placeNames();
    static juce::StringArray barrierNames();
    static juce::StringArray weatherNames();
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    loc::Engine engine;
    std::vector<float> monoScratch;   // zweiter Kanal fuer Mono-Spuren (vorab angelegt)
    std::atomic<float>* pPlaceA = nullptr; std::atomic<float>* pPlaceB = nullptr;
    std::atomic<float>* pBlend = nullptr;  std::atomic<float>* pDistance = nullptr;
    std::atomic<float>* pDoor = nullptr;   std::atomic<float>* pRoom = nullptr;
    std::atomic<float>* pPan = nullptr;
    std::atomic<float>* pBarrier = nullptr; std::atomic<float>* pWeather = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocationsProcessor)
};
