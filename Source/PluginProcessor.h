#pragma once
#include <JuceHeader.h>
#include "DSP/HybridSynth.h"
#include "DSP/Effects.h"
#include "Presets/PresetManager.h"

class ATSLORichSynthAudioProcessor final : public juce::AudioProcessor {
public:
    ATSLORichSynthAudioProcessor();
    ~ATSLORichSynthAudioProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ATSLO RICH SYNTH"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return int(presets.factory().size()); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram(int i) override;
    const juce::String getProgramName(int i) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    bool loadSample(const juce::File& f) { return synth->loadSample(f); }
    juce::AudioProcessorValueTreeState state;
    atslo::PresetManager presets;
    juce::MidiKeyboardState keyboardState;
    std::atomic<float> outputLevel { 0.0f };
private:
    atslo::VoiceParams voiceParams;
    atslo::EffectParams effectParams;
    std::unique_ptr<atslo::HybridSynth> synth;
    std::unique_ptr<atslo::EffectChain> effects;
    int currentPreset = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ATSLORichSynthAudioProcessor)
};
