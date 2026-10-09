#include "HybridSynth.h"
#include <cmath>

namespace atslo {
HybridVoice::HybridVoice(VoiceParams& vp, const std::shared_ptr<const SampleBank>& s)
    : p(vp), samples(s) { filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass); }

bool HybridVoice::canPlaySound(juce::SynthesiserSound* s) { return dynamic_cast<HybridSound*>(s) != nullptr; }

void HybridVoice::updateEnvelopes() {
    juce::ADSR::Parameters e { p.attack->load(), p.decay->load(), p.sustain->load(), p.release->load() };
    ampEnv.setParameters(e);
    filterEnv.setParameters({ juce::jmax(0.002f, e.attack * 0.5f), e.decay, 0.0f, e.release });
}

void HybridVoice::startNote(int note, float velocity, juce::SynthesiserSound*, int wheel) {
    midiNote = note; level = velocity; phase1 = phase2 = samplePosition = 0;
    pitchWheelMoved(wheel); hz = juce::MidiMessage::getMidiNoteInHertz(note);
    updateEnvelopes(); ampEnv.noteOn(); filterEnv.noteOn();
    activeSample.reset();
    const int v = juce::roundToInt(velocity * 127.0f);
    const auto bank = std::atomic_load(&samples);
    for (const auto& sample : *bank)
        if (note >= sample->lowNote && note <= sample->highNote && v >= sample->lowVelocity && v <= sample->highVelocity) {
            activeSample = sample; break;
        }
}

void HybridVoice::stopNote(float, bool tail) {
    ampEnv.noteOff(); filterEnv.noteOff();
    if (!tail) { ampEnv.reset(); clearCurrentNote(); }
}

void HybridVoice::pitchWheelMoved(int value) { bend = std::pow(2.0f, ((value - 8192) / 8192.0f) * 2.0f / 12.0f); }
void HybridVoice::controllerMoved(int cc, int value) { if (cc == 1) modWheel = value / 127.0f; }

float HybridVoice::oscillator(double x, int shape) const noexcept {
    switch (shape) {
        case 0: return std::sin(float(x * juce::MathConstants<double>::twoPi));
        case 1: return float(2.0 * x - 1.0); // band-limited by modest internal low-pass
        case 2: return x < 0.5 ? 1.0f : -1.0f;
        case 3: return float(1.0 - 4.0 * std::abs(x - 0.5));
        default: return 0.0f;
    }
}

void HybridVoice::renderNextBlock(juce::AudioBuffer<float>& out, int start, int count) {
    if (!isVoiceActive()) return;
    const double sr = getSampleRate();
    const auto oct = juce::roundToInt(p.octave->load());
    const double base = hz * bend * std::pow(2.0, oct);
    const double cents = p.detune->load();
    const double d1 = base * std::pow(2.0, -cents / 2400.0) / sr;
    const double d2 = base * std::pow(2.0, cents / 2400.0) / sr;
    const float blend = p.blend->load();
    const float velocityGain = juce::jmap(p.velocity->load(), 1.0f, level);
    const int shape = juce::roundToInt(p.wave->load());
    filter.setResonance(p.resonance->load());
    for (int i = 0; i < count; ++i) {
        const float env = ampEnv.getNextSample();
        const float wobble = 1.0f + 0.004f * modWheel * std::sin(float(phase1 * juce::MathConstants<double>::twoPi));
        float synthSample = shape == 4 ? random.nextFloat() * 2.0f - 1.0f
                                      : 0.5f * (oscillator(phase1, shape) + oscillator(phase2, shape));
        phase1 = std::fmod(phase1 + d1 * wobble, 1.0); phase2 = std::fmod(phase2 + d2 * wobble, 1.0);
        float sampled = 0.0f;
        if (activeSample && activeSample->audio.getNumSamples() > 1) {
            const auto& s = *activeSample;
            const double ratio = std::pow(2.0, (midiNote - s.rootNote) / 12.0) * s.sourceRate / sr;
            const int maxEnd = juce::jmax(1, s.audio.getNumSamples() - 1);
            const int begin = juce::roundToInt(p.sampleStart->load() * maxEnd);
            const int end = juce::jmax(begin + 1, juce::roundToInt(p.sampleEnd->load() * maxEnd));
            const int pos = juce::jlimit(0, maxEnd, begin + int(samplePosition));
            const int next = juce::jmin(pos + 1, maxEnd);
            const float frac = float(samplePosition - int(samplePosition));
            sampled = juce::jmap(frac, s.audio.getSample(0, pos), s.audio.getSample(0, next));
            samplePosition += ratio;
            if (begin + samplePosition >= end) {
                if (s.looping) samplePosition = juce::jmax(0, s.loopStart - begin);
                else activeSample.reset();
            }
        }
        const float cutoff = juce::jlimit(30.0f, float(sr * 0.45), p.cutoff->load() * std::pow(2.0f, filterEnv.getNextSample() * p.filterEnv->load() * 4.0f));
        filter.setCutoffFrequency(cutoff);
        const float mixed = filter.processSample(0, ((1.0f - blend) * synthSample + blend * sampled) * env * velocityGain);
        for (int ch = 0; ch < out.getNumChannels(); ++ch) out.addSample(ch, start + i, mixed * 0.22f);
        if (!ampEnv.isActive()) { clearCurrentNote(); break; }
    }
}

HybridSynth::HybridSynth(VoiceParams& p) : params(p) {
    formats.registerBasicFormats();
    synth.addSound(new HybridSound());
    for (int i = 0; i < 16; ++i) synth.addVoice(new HybridVoice(params, samples));
}
void HybridSynth::prepare(double sr, int block, int channels) {
    synth.setCurrentPlaybackSampleRate(sr);
    juce::ignoreUnused(block, channels);
}
void HybridSynth::render(juce::AudioBuffer<float>& b, juce::MidiBuffer& m) { synth.renderNextBlock(b, m, 0, b.getNumSamples()); }
bool HybridSynth::loadSample(const juce::File& file, int root, int low, int high) {
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader || reader->lengthInSamples <= 0 || reader->lengthInSamples > 60 * reader->sampleRate) return false;
    auto data = std::make_shared<SampleData>();
    data->audio.setSize(juce::jmin(2, int(reader->numChannels)), int(reader->lengthInSamples));
    reader->read(&data->audio, 0, data->audio.getNumSamples(), 0, true, true);
    data->sourceRate = reader->sampleRate; data->rootNote = root; data->lowNote = low; data->highNote = high;
    data->loopEnd = data->audio.getNumSamples();
    auto updated = std::make_shared<HybridVoice::SampleBank>(*std::atomic_load(&samples));
    updated->push_back(std::move(data));
    std::atomic_store(&samples, std::shared_ptr<const HybridVoice::SampleBank>(std::move(updated)));
    return true;
}
void HybridSynth::clearSamples() { std::atomic_store(&samples, std::make_shared<const HybridVoice::SampleBank>()); }
int HybridSynth::getNumActiveVoices() const {
    int n = 0;
    for (int i = 0; i < synth.getNumVoices(); ++i) n += synth.getVoice(i)->isVoiceActive() ? 1 : 0;
    return n;
}
}
