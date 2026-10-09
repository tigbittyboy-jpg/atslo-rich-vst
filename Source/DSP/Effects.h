#pragma once
#include <JuceHeader.h>

namespace atslo {
struct EffectParams {
    std::atomic<float>* drive{}; std::atomic<float>* chorus{};
    std::atomic<float>* delayMix{}; std::atomic<float>* delayTime{};
    std::atomic<float>* reverb{}; std::atomic<float>* width{};
    std::atomic<float>* master{};
};

class EffectChain {
public:
    explicit EffectChain(EffectParams& p) : params(p) {}
    void prepare(const juce::dsp::ProcessSpec&);
    void reset();
    void process(juce::AudioBuffer<float>&);
private:
    EffectParams& params;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> delayBuffer;
    int delayWrite = 0; double sampleRate = 44100;
    juce::dsp::Limiter<float> limiter;
    juce::SmoothedValue<float> gain;
};
}
