#include "Effects.h"

namespace atslo {
void EffectChain::prepare(const juce::dsp::ProcessSpec& spec) {
    sampleRate = spec.sampleRate;
    chorus.prepare(spec); reverb.prepare(spec); limiter.prepare(spec);
    chorus.setRate(0.35f); chorus.setDepth(0.25f); chorus.setCentreDelay(8.0f); chorus.setFeedback(0.05f);
    delayBuffer.setSize(int(spec.numChannels), int(sampleRate * 2.1)); delayBuffer.clear();
    gain.reset(sampleRate, 0.03); reset();
}
void EffectChain::reset() { chorus.reset(); reverb.reset(); limiter.reset(); delayBuffer.clear(); delayWrite = 0; }
void EffectChain::process(juce::AudioBuffer<float>& buffer) {
    const float drive = params.drive->load();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(ch, i, std::tanh(buffer.getSample(ch, i) * (1.0f + drive * 8.0f)) / (1.0f + drive));

    juce::dsp::AudioBlock<float> block(buffer);
    chorus.setMix(params.chorus->load());
    chorus.process(juce::dsp::ProcessContextReplacing<float>(block));

    const int delaySamples = juce::jlimit(1, delayBuffer.getNumSamples() - 1, int(params.delayTime->load() * sampleRate));
    const float mix = params.delayMix->load();
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        const int read = (delayWrite - delaySamples + delayBuffer.getNumSamples()) % delayBuffer.getNumSamples();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float dry = buffer.getSample(ch, i), delayed = delayBuffer.getSample(ch, read);
            delayBuffer.setSample(ch, delayWrite, dry + delayed * 0.38f);
            buffer.setSample(ch, i, dry + delayed * mix);
        }
        delayWrite = (delayWrite + 1) % delayBuffer.getNumSamples();
    }

    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.25f + params.reverb->load() * 0.7f; rp.damping = 0.55f;
    rp.wetLevel = params.reverb->load() * 0.42f; rp.dryLevel = 1.0f; rp.width = 1.0f;
    reverb.setParameters(rp); reverb.process(juce::dsp::ProcessContextReplacing<float>(block));
    if (buffer.getNumChannels() == 2) {
        const float width = params.width->load();
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            const float l = buffer.getSample(0, i), r = buffer.getSample(1, i);
            const float mid = 0.5f * (l + r), side = 0.5f * (l - r) * width;
            buffer.setSample(0, i, mid + side); buffer.setSample(1, i, mid - side);
        }
    }
    gain.setTargetValue(juce::Decibels::decibelsToGain(params.master->load()));
    gain.applyGain(buffer, buffer.getNumSamples());
    limiter.setThreshold(-0.5f); limiter.setRelease(80.0f);
    limiter.process(juce::dsp::ProcessContextReplacing<float>(block));
}
}
