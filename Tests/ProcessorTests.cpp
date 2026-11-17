#include "PluginProcessor.h"
#include "TestSupport.h"
#include <memory>
int main(){juce::ScopedJuceInitialiser_GUI init;
 auto p=std::make_unique<LocationsProcessor>();auto layout=p->getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::mono());CHECK(p->setBusesLayout(layout));p->prepareToPlay(48000,64);
 juce::AudioBuffer<float> b(1,32769);for(int i=0;i<b.getNumSamples();++i)b.setSample(0,i,float(.1*sin(i*.03)));
 loc::Engine ref;ref.prepare(48000);loc::Params params;params.distance=1;std::vector<float> l(b.getReadPointer(0),b.getReadPointer(0)+b.getNumSamples()),r=l;ref.process(l.data(),r.data(),(int)l.size(),params);
 juce::MidiBuffer midi;{NoAllocations guard;p->processBlock(b,midi);}for(int i=0;i<b.getNumSamples();++i)CHECK(b.getSample(0,i)==.5f*(l[(size_t)i]+r[(size_t)i]));
 auto* echo=p->apvts.getParameter("echoTime");echo->setValueNotifyingHost(echo->convertTo0to1(3));juce::MemoryBlock state;p->getStateInformation(state);echo->setValueNotifyingHost(echo->convertTo0to1(1));p->setStateInformation(state.getData(),(int)state.getSize());CHECK(std::abs(p->apvts.getRawParameterValue("echoTime")->load()-3)<1e-5f);
 for(int n:{0,1,7,31,8193}){b.setSize(1,n);b.clear();{NoAllocations guard;p->processBlock(b,midi);}}
 auto* place=p->apvts.getParameter("placeA");place->setValueNotifyingHost(place->convertTo0to1(16));echo->setValueNotifyingHost(echo->convertTo0to1(4));CHECK(p->getTailLengthSeconds()>20);
 puts("Locations processor: oversized mono block matches reference, bounded callbacks, state roundtrip passed");}
