#include "PresetManager.h"

namespace atslo {
static FactoryPreset make(juce::String name, juce::String category, int wave, float cutoff, float attack,
                          float release, float detune, float blend, float drive, float reverb) {
    return { std::move(name), std::move(category), {{"wave", float(wave)}, {"cutoff", cutoff}, {"attack", attack},
        {"release", release}, {"detune", detune}, {"blend", blend}, {"drive", drive}, {"reverb", reverb}} };
}
PresetManager::PresetManager(juce::AudioProcessorValueTreeState& s) : state(s) {
    presets = {
        make("Empire Stab","RICH BRASS",1,1800,.006,.18,19,0,.42,.18), make("Gold Horns","RICH BRASS",1,1250,.02,.3,31,0,.3,.24),
        make("Marching Rich","RICH BRASS",2,950,.008,.12,12,0,.5,.12), make("Cinematic Hit","RICH BRASS",1,720,.004,.7,42,0,.62,.45),
        make("Brass Stack","RICH BRASS",1,2400,.015,.24,48,0,.36,.16),
        make("Midnight Piano","DARK KEYS",3,1700,.005,1.1,3,0,.08,.35), make("Funeral Keys","DARK KEYS",2,760,.012,1.5,7,0,.18,.52),
        make("Haunted Ivory","DARK KEYS",3,2900,.004,1.8,14,0,.1,.6), make("Dust Keys","DARK KEYS",4,1100,.02,.9,0,0,.16,.3),
        make("Cold Room","DARK KEYS",0,2200,.12,2.3,5,0,.05,.7),
        make("Trap Cathedral","TRAP BELLS",0,5200,.002,2.2,13,0,.12,.55), make("Black Chime","TRAP BELLS",3,6800,.001,1.6,36,0,.18,.48),
        make("Music Box 404","TRAP BELLS",2,4100,.001,.8,5,0,.08,.28), make("Steel Prayer","TRAP BELLS",0,8500,.002,3.0,51,0,.24,.68),
        make("Night Pluck","TRAP BELLS",1,3600,.001,.35,22,0,.2,.22),
        make("Zone Six Saw","STREET LEADS",1,7200,.008,.16,38,0,.36,.12), make("Siren Lead","STREET LEADS",2,4800,.015,.3,9,0,.2,.25),
        make("Chrome Glide","STREET LEADS",3,6300,.004,.22,24,0,.46,.1), make("Razor Melody","STREET LEADS",1,9200,.002,.12,55,0,.6,.08),
        make("Alley Whistle","STREET LEADS",0,6400,.025,.55,7,0,.1,.38),
        make("Suspense Strings","CINEMATIC",1,1600,.65,2.4,27,0,.12,.62), make("Concrete Choir","CINEMATIC",3,1250,1.1,3.2,16,0,.08,.76),
        make("Dark Horizon","CINEMATIC",1,780,1.8,4.0,45,0,.2,.8), make("Crime Scene","CINEMATIC",4,940,.45,2.8,0,0,.25,.7),
        make("Orchestra Smoke","CINEMATIC",2,2100,.85,3.4,33,0,.15,.72),
        make("Deep 808 Synth","SYNTH BASS",0,420,.002,.7,0,0,.18,.02), make("Distorted Block","SYNTH BASS",2,680,.003,.22,5,0,.78,.04),
        make("Sub Pressure","SYNTH BASS",0,240,.01,1.2,0,0,.05,.01), make("Sliding Saw Bass","SYNTH BASS",1,540,.004,.45,18,0,.52,.03),
        make("Basement Brass","SYNTH BASS",2,880,.008,.35,29,0,.62,.09)
    };
}
bool PresetManager::loadFactory(int index) {
    if (!juce::isPositiveAndBelow(index, int(presets.size()))) return false;
    for (const auto& [id, value] : presets[size_t(index)].values)
        if (auto* parameter = state.getParameter(id)) parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    state.state.setProperty("presetName", presets[size_t(index)].name, nullptr); return true;
}
bool PresetManager::saveUserPreset(const juce::File& file) const {
    if (auto xml = state.copyState().createXml()) return xml->writeTo(file, {});
    return false;
}
bool PresetManager::loadUserPreset(const juce::File& file) {
    if (auto xml = juce::parseXML(file)) { auto v = juce::ValueTree::fromXml(*xml); if (v.isValid()) { state.replaceState(v); return true; } }
    return false;
}
}
