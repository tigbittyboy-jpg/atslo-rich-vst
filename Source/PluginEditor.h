#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class GoldLook final : public juce::LookAndFeel_V4 {
public:
    GoldLook();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};

class ATSLORichSynthAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit ATSLORichSynthAudioProcessorEditor(ATSLORichSynthAudioProcessor&);
    ~ATSLORichSynthAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void addKnob(juce::String, const char* id);
    ATSLORichSynthAudioProcessor& processor;
    GoldLook look;
    juce::ComboBox presetBox, waveBox, wave2Box;
    juce::TextButton previous{"<"}, next{">"}, importSample{"IMPORT SAMPLE"}, savePreset{"SAVE"}, loadPreset{"LOAD"};
    juce::Label brand, subtitle, meter;
    juce::MidiKeyboardComponent keyboard {processor.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard};
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> wave2Attachment;
    std::shared_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ATSLORichSynthAudioProcessorEditor)
};
