#include "HybridSynth.h"
#include <cmath>

namespace atslo {
HybridVoice::HybridVoice(VoiceParams& vp, const std::shared_ptr<const SampleBank>& s)
    : p(vp), samples(s) { filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass); }

void HybridVoice::prepare(double sr, int block, int channels) {
    filter.prepare({ sr, juce::uint32(block), juce::uint32(channels) });
    filter.reset();
}

bool HybridVoice::canPlaySound(juce::SynthesiserSound* s) { return dynamic_cast<HybridSound*>(s) != nullptr; }

void HybridVoice::updateEnvelopes() {
    juce::ADSR::Parameters e { p.attack->load(), p.decay->load(), p.sustain->load(), p.release->load() };
    ampEnv.setParameters(e);
    filterEnv.setParameters({ juce::jmax(0.002f, e.attack * 0.5f), e.decay, 0.0f, e.release });
}

void HybridVoice::startNote(int note, float velocity, juce::SynthesiserSound*, int wheel) {
    midiNote = note; level = velocity; phase1.fill(0.0); phase2.fill(0.0); subPhase = samplePosition = 0;
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

float HybridVoice::polyBlep(double x, double dt) noexcept {
    if (x < dt) { x /= dt; return float(x + x - x * x - 1.0); }
    if (x > 1.0 - dt) { x = (x - 1.0) / dt; return float(x * x + x + x + 1.0); }
    return 0.0f;
}

float HybridVoice::oscillator(double x, int shape, double dt) noexcept {
    switch (shape) {
        case 0: return std::sin(float(x * juce::MathConstants<double>::twoPi));
        case 1: return float(2.0 * x - 1.0) - polyBlep(x, dt);
        case 2: {
            float value = x < 0.5 ? 1.0f : -1.0f;
            value += polyBlep(x, dt); value -= polyBlep(std::fmod(x + 0.5, 1.0), dt); return value;
        }
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
    const float blend = p.blend->load();
    const float velocityGain = juce::jmap(p.velocity->load(), 1.0f, level);
    const int shape1 = juce::roundToInt(p.wave->load()), shape2 = juce::roundToInt(p.wave2->load());
    const int unison = juce::jlimit(1, 4, juce::roundToInt(p.unison->load()));
    const float stereoSpread = p.spread->load(), oscMix = p.oscMix->load();
    filter.setResonance(p.resonance->load());
    for (int i = 0; i < count; ++i) {
        const float env = ampEnv.getNextSample();
        float synthLeft = 0.0f, synthRight = 0.0f;
        for (int u = 0; u < unison; ++u) {
            const float position = unison == 1 ? 0.0f : juce::jmap(float(u), 0.0f, float(unison - 1), -1.0f, 1.0f);
            const double inc1 = base * std::pow(2.0, cents * position / 1200.0) / sr;
            const double inc2 = base * std::pow(2.0, (p.coarse2->load() * 100.0 - cents * position) / 1200.0) / sr;
            const float second = shape2 == 4 ? random.nextFloat() * 2.0f - 1.0f : oscillator(phase2[size_t(u)], shape2, inc2);
            const double modulated = std::fmod(phase1[size_t(u)] + 1.0 + second * p.fm->load() * 0.14, 1.0);
            const float first = shape1 == 4 ? random.nextFloat() * 2.0f - 1.0f : oscillator(modulated, shape1, inc1);
            const float voice = juce::jmap(oscMix, first, second) / std::sqrt(float(unison));
            const float pan = position * stereoSpread;
            synthLeft += voice * std::sqrt(0.5f * (1.0f - pan)); synthRight += voice * std::sqrt(0.5f * (1.0f + pan));
            phase1[size_t(u)] = std::fmod(phase1[size_t(u)] + inc1, 1.0);
            phase2[size_t(u)] = std::fmod(phase2[size_t(u)] + inc2, 1.0);
        }
        const float sub = std::sin(float(subPhase * juce::MathConstants<double>::twoPi)) * p.sub->load();
        subPhase = std::fmod(subPhase + base * 0.5 / sr, 1.0);
        const float grit = (random.nextFloat() * 2.0f - 1.0f) * p.noise->load();
        synthLeft += sub + grit; synthRight += sub + grit;
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
        const float gain = env * velocityGain * 0.16f;
        const float left = filter.processSample(0, ((1.0f - blend) * synthLeft + blend * sampled) * gain);
        out.addSample(0, start + i, left);
        if (out.getNumChannels() > 1) {
            const float right = filter.processSample(1, ((1.0f - blend) * synthRight + blend * sampled) * gain);
            out.addSample(1, start + i, right);
        }
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
    for (int i = 0; i < synth.getNumVoices(); ++i)
        static_cast<HybridVoice*>(synth.getVoice(i))->prepare(sr, block, channels);
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
