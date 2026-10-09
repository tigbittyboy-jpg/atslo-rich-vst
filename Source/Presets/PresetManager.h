#pragma once
#include <JuceHeader.h>

namespace atslo {
struct FactoryPreset { juce::String name, category; std::map<juce::String, float> values; };

class PresetManager {
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState&);
    const std::vector<FactoryPreset>& factory() const { return presets; }
    bool loadFactory(int index);
    bool saveUserPreset(const juce::File&) const;
    bool loadUserPreset(const juce::File&);
private:
    juce::AudioProcessorValueTreeState& state;
    std::vector<FactoryPreset> presets;
};
}
