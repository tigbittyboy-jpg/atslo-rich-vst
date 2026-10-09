#include "PluginProcessor.h"
#include "PluginEditor.h"

ATSLORichSynthAudioProcessor::ATSLORichSynthAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "ATSLO_STATE", createParameterLayout()), presets(state) {
    auto raw = [this](const char* id) { return state.getRawParameterValue(id); };
    voiceParams = { raw("blend"), raw("wave"), raw("wave2"), raw("oscMix"), raw("coarse2"), raw("fm"), raw("sub"), raw("noise"),
        raw("octave"), raw("detune"), raw("unison"), raw("spread"),
        raw("attack"), raw("decay"), raw("sustain"), raw("release"), raw("cutoff"), raw("resonance"),
        raw("filterEnv"), raw("velocity"), raw("sampleStart"), raw("sampleEnd") };
    effectParams = { raw("drive"), raw("chorus"), raw("delayMix"), raw("delayTime"), raw("reverb"), raw("width"), raw("master") };
    synth = std::make_unique<atslo::HybridSynth>(voiceParams);
    effects = std::make_unique<atslo::EffectChain>(effectParams);
    presets.loadFactory(0);
}

juce::AudioProcessorValueTreeState::ParameterLayout ATSLORichSynthAudioProcessor::createParameterLayout() {
    using F = juce::AudioParameterFloat; using C = juce::AudioParameterChoice;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&p](auto v) { p.push_back(std::move(v)); };
    add(std::make_unique<F>(juce::ParameterID("blend",1),"Sample Blend",0.f,1.f,0.f));
    add(std::make_unique<C>(juce::ParameterID("wave",1),"Wave",juce::StringArray{"Sine","Saw","Square","Triangle","Noise"},1));
    add(std::make_unique<C>(juce::ParameterID("wave2",1),"Wave 2",juce::StringArray{"Sine","Saw","Square","Triangle","Noise"},0));
    add(std::make_unique<F>(juce::ParameterID("oscMix",1),"Osc Mix",0.f,1.f,.35f));
    add(std::make_unique<F>(juce::ParameterID("coarse2",1),"Osc 2 Semitones",juce::NormalisableRange<float>(-24,24,1),0));
    add(std::make_unique<F>(juce::ParameterID("fm",1),"FM",0.f,1.f,0.f));
    add(std::make_unique<F>(juce::ParameterID("sub",1),"Sub",0.f,1.f,.1f));
    add(std::make_unique<F>(juce::ParameterID("noise",1),"Noise",0.f,1.f,0.f));
    add(std::make_unique<F>(juce::ParameterID("octave",1),"Octave",juce::NormalisableRange<float>(-2,2,1),0));
    add(std::make_unique<F>(juce::ParameterID("detune",1),"Detune",0.f,100.f,18.f));
    add(std::make_unique<F>(juce::ParameterID("unison",1),"Unison",juce::NormalisableRange<float>(1,4,1),2));
    add(std::make_unique<F>(juce::ParameterID("spread",1),"Spread",0.f,1.f,.45f));
    add(std::make_unique<F>(juce::ParameterID("attack",1),"Attack",juce::NormalisableRange<float>(.001f,5.f,.001f,.35f),.01f));
    add(std::make_unique<F>(juce::ParameterID("decay",1),"Decay",juce::NormalisableRange<float>(.01f,5.f,.001f,.4f),.3f));
    add(std::make_unique<F>(juce::ParameterID("sustain",1),"Sustain",0.f,1.f,.75f));
    add(std::make_unique<F>(juce::ParameterID("release",1),"Release",juce::NormalisableRange<float>(.01f,8.f,.001f,.35f),.45f));
    add(std::make_unique<F>(juce::ParameterID("cutoff",1),"Cutoff",juce::NormalisableRange<float>(30.f,20000.f,.01f,.25f),2800.f));
    add(std::make_unique<F>(juce::ParameterID("resonance",1),"Resonance",.5f,8.f,.8f));
    add(std::make_unique<F>(juce::ParameterID("filterEnv",1),"Filter Env",-1.f,1.f,.35f));
    add(std::make_unique<F>(juce::ParameterID("velocity",1),"Velocity",0.f,1.f,.7f));
    add(std::make_unique<F>(juce::ParameterID("sampleStart",1),"Sample Start",0.f,.99f,0.f));
    add(std::make_unique<F>(juce::ParameterID("sampleEnd",1),"Sample End",.01f,1.f,1.f));
    add(std::make_unique<F>(juce::ParameterID("drive",1),"Drive",0.f,1.f,.15f));
    add(std::make_unique<F>(juce::ParameterID("chorus",1),"Chorus",0.f,1.f,.08f));
    add(std::make_unique<F>(juce::ParameterID("delayMix",1),"Delay",0.f,1.f,.12f));
    add(std::make_unique<F>(juce::ParameterID("delayTime",1),"Delay Time",juce::NormalisableRange<float>(.03f,1.5f,.001f,.4f),.375f));
    add(std::make_unique<F>(juce::ParameterID("reverb",1),"Reverb",0.f,1.f,.2f));
    add(std::make_unique<F>(juce::ParameterID("width",1),"Width",0.f,2.f,1.f));
    add(std::make_unique<F>(juce::ParameterID("master",1),"Master",-36.f,6.f,-6.f));
    return { p.begin(), p.end() };
}

void ATSLORichSynthAudioProcessor::prepareToPlay(double sr, int block) {
    synth->prepare(sr, block, getTotalNumOutputChannels());
    effects->prepare({sr, juce::uint32(block), juce::uint32(getTotalNumOutputChannels())});
}
void ATSLORichSynthAudioProcessor::releaseResources() { effects->reset(); }
bool ATSLORichSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::mono() || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
void ATSLORichSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer& m) {
    juce::ScopedNoDenormals noDenormals; b.clear();
    keyboardState.processNextMidiBuffer(m, 0, b.getNumSamples(), true);
    synth->render(b,m); effects->process(b);
    outputLevel.store(juce::Decibels::gainToDecibels(b.getMagnitude(0, b.getNumSamples()) + 1.0e-7f));
}
juce::AudioProcessorEditor* ATSLORichSynthAudioProcessor::createEditor() { return new ATSLORichSynthAudioProcessorEditor(*this); }
void ATSLORichSynthAudioProcessor::setCurrentProgram(int i) { if (presets.loadFactory(i)) currentPreset = i; }
const juce::String ATSLORichSynthAudioProcessor::getProgramName(int i) { return juce::isPositiveAndBelow(i, getNumPrograms()) ? presets.factory()[size_t(i)].name : juce::String(); }
void ATSLORichSynthAudioProcessor::getStateInformation(juce::MemoryBlock& data) { if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml,data); }
void ATSLORichSynthAudioProcessor::setStateInformation(const void* d, int n) { if (auto xml = getXmlFromBinary(d,n)) { auto v=juce::ValueTree::fromXml(*xml); if(v.isValid()) state.replaceState(v); } }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ATSLORichSynthAudioProcessor(); }
