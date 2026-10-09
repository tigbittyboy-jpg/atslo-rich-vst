#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <iostream>

class ATSLOTests final : public juce::UnitTest {
public:
    ATSLOTests() : UnitTest("ATSLO audio engine") {}
    void runTest() override {
        beginTest("factory library has distinct required presets");
        ATSLORichSynthAudioProcessor processor;
        expectEquals(processor.getNumPrograms(), 30);
        expect(processor.getProgramName(0) != processor.getProgramName(29));

        beginTest("MIDI note renders audio and releases safely");
        processor.setPlayConfigDetails(0, 2, 48000.0, 512);
        processor.prepareToPlay(48000.0, 512);
        juce::AudioBuffer<float> audio(2, 512); juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        processor.processBlock(audio, midi);
        expect(audio.getMagnitude(0, audio.getNumSamples()) > 0.001f);
        midi.clear(); midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        processor.processBlock(audio, midi);
        expect(std::isfinite(audio.getRMSLevel(0, 0, audio.getNumSamples())));

        beginTest("parameter boundaries stay finite");
        for (auto* parameter : processor.getParameters()) {
            parameter->setValue(0.0f); midi.clear(); processor.processBlock(audio, midi);
            expect(std::isfinite(audio.getMagnitude(0, audio.getNumSamples())));
            parameter->setValue(1.0f); midi.clear(); processor.processBlock(audio, midi);
            expect(std::isfinite(audio.getMagnitude(0, audio.getNumSamples())));
        }

        beginTest("state round trip restores a parameter");
        auto* cutoff = processor.state.getParameter("cutoff"); cutoff->setValueNotifyingHost(0.1f);
        juce::MemoryBlock saved; processor.getStateInformation(saved); cutoff->setValueNotifyingHost(0.9f);
        processor.setStateInformation(saved.getData(), int(saved.getSize()));
        expectWithinAbsoluteError(cutoff->getValue(), 0.1f, 0.001f);

        beginTest("offline render can be written as WAV");
        auto temp = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("atslo-smoke.wav");
        temp.deleteFile(); std::unique_ptr<juce::FileOutputStream> stream(temp.createOutputStream());
        juce::WavAudioFormat wav; std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.release(),48000,2,24,{},0));
        expect(writer != nullptr); if(writer) expect(writer->writeFromAudioSampleBuffer(audio,0,audio.getNumSamples()));
        expect(temp.existsAsFile()); temp.deleteFile();

        beginTest("user sample loads and renders through the sample engine");
        juce::AudioBuffer<float> source(1, 2400);
        for (int i = 0; i < source.getNumSamples(); ++i) source.setSample(0, i, std::sin(float(i) * 0.08f) * 0.4f);
        stream = temp.createOutputStream(); writer.reset(wav.createWriterFor(stream.release(),48000,1,24,{},0));
        expect(writer != nullptr); if (writer) writer->writeFromAudioSampleBuffer(source,0,source.getNumSamples()); writer.reset();
        expect(processor.loadSample(temp));
        auto* blend = processor.state.getParameter("blend"); blend->setValueNotifyingHost(1.0f);
        midi.clear(); midi.addEvent(juce::MidiMessage::noteOn(1,60,1.0f),0); processor.processBlock(audio,midi);
        expect(audio.getMagnitude(0,audio.getNumSamples()) > 0.001f); temp.deleteFile();
    }
};

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    ATSLOTests tests; juce::UnitTestRunner runner; runner.runAllTests();
    int failures = 0; for (int i=0;i<runner.getNumResults();++i) failures += runner.getResult(i)->failures;
    std::cout << runner.getNumResults() << " test groups, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
