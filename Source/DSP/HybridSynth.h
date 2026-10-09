#pragma once
#include <JuceHeader.h>

namespace atslo {

struct SampleData {
    juce::AudioBuffer<float> audio;
    double sourceRate = 44100.0;
    int rootNote = 60, lowNote = 0, highNote = 127;
    int lowVelocity = 0, highVelocity = 127;
    int loopStart = 0, loopEnd = 0;
    bool looping = false;
};

struct VoiceParams {
    std::atomic<float>* blend{}; std::atomic<float>* wave{}; std::atomic<float>* wave2{};
    std::atomic<float>* oscMix{}; std::atomic<float>* coarse2{}; std::atomic<float>* fm{};
    std::atomic<float>* sub{}; std::atomic<float>* noise{};
    std::atomic<float>* octave{}; std::atomic<float>* detune{};
    std::atomic<float>* unison{}; std::atomic<float>* spread{};
    std::atomic<float>* attack{}; std::atomic<float>* decay{};
    std::atomic<float>* sustain{}; std::atomic<float>* release{};
    std::atomic<float>* cutoff{}; std::atomic<float>* resonance{};
    std::atomic<float>* filterEnv{}; std::atomic<float>* velocity{};
    std::atomic<float>* sampleStart{}; std::atomic<float>* sampleEnd{};
};

class HybridSound final : public juce::SynthesiserSound {
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class HybridVoice final : public juce::SynthesiserVoice {
public:
    using SampleBank = std::vector<std::shared_ptr<SampleData>>;
    HybridVoice(VoiceParams&, const std::shared_ptr<const SampleBank>&);
    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int, float, juce::SynthesiserSound*, int) override;
    void stopNote(float, bool) override;
    void pitchWheelMoved(int) override;
    void controllerMoved(int, int) override;
    void renderNextBlock(juce::AudioBuffer<float>&, int, int) override;
    void prepare(double sampleRate, int blockSize, int channels);
private:
    static float oscillator(double phase, int shape, double increment) noexcept;
    static float polyBlep(double phase, double increment) noexcept;
    void updateEnvelopes();
    VoiceParams& p;
    const std::shared_ptr<const SampleBank>& samples;
    juce::ADSR ampEnv, filterEnv;
    juce::dsp::StateVariableTPTFilter<float> filter;
    std::array<double, 4> phase1{}, phase2{};
    double subPhase = 0, samplePosition = 0, hz = 440;
    float level = 0, bend = 1, modWheel = 0;
    int midiNote = 60;
    std::shared_ptr<SampleData> activeSample;
    juce::Random random;
};

class HybridSynth {
public:
    explicit HybridSynth(VoiceParams&);
    void prepare(double sampleRate, int blockSize, int channels);
    void render(juce::AudioBuffer<float>&, juce::MidiBuffer&);
    bool loadSample(const juce::File&, int rootNote = 60, int low = 0, int high = 127);
    void clearSamples();
    int getNumActiveVoices() const;
private:
    VoiceParams& params;
    juce::Synthesiser synth;
    juce::AudioFormatManager formats;
    std::shared_ptr<const HybridVoice::SampleBank> samples = std::make_shared<const HybridVoice::SampleBank>();
};
}
