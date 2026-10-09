#include "PluginEditor.h"

GoldLook::GoldLook() {
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffe0a928));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff191919));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff9d7720));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff34280f));
}
void GoldLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) {
    auto r=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(7); auto a=start+pos*(end-start);
    g.setColour(juce::Colour(0xff090909)); g.fillEllipse(r); g.setColour(juce::Colour(0xff3d3d3d)); g.drawEllipse(r,2);
    juce::Path arc; arc.addCentredArc(r.getCentreX(),r.getCentreY(),r.getWidth()/2,r.getHeight()/2,0,start,a,true);
    g.setColour(juce::Colour(0xffe0a928)); g.strokePath(arc,{3,juce::PathStrokeType::curved,juce::PathStrokeType::rounded});
    juce::Point<float> c=r.getCentre(), p=c.getPointOnCircumference(r.getWidth()*.34f,a);
    g.drawLine(c.x,c.y,p.x,p.y,2);
}
ATSLORichSynthAudioProcessorEditor::ATSLORichSynthAudioProcessorEditor(ATSLORichSynthAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&look); setResizable(true,true); setResizeLimits(760,520,1400,900); setSize(1040,680);
    brand.setText("ATSLO",juce::dontSendNotification); brand.setFont(juce::FontOptions(30).withStyle("Bold")); brand.setColour(juce::Label::textColourId,juce::Colour(0xffffc13b));
    subtitle.setText("RICH SYNTH",juce::dontSendNotification); subtitle.setFont(juce::FontOptions(16));
    juce::Component* visible[] = { &brand, &subtitle, &presetBox, &previous, &next, &importSample, &savePreset, &loadPreset, &waveBox, &wave2Box, &keyboard, &meter };
    for (auto* c : visible) addAndMakeVisible(c);
    for(const auto& pr:processor.presets.factory()) presetBox.addItem(pr.category+"  /  "+pr.name,presetBox.getNumItems()+1);
    presetBox.setSelectedItemIndex(processor.getCurrentProgram(),juce::dontSendNotification);
    presetBox.onChange=[this]{ processor.setCurrentProgram(presetBox.getSelectedItemIndex()); };
    previous.onClick=[this]{ presetBox.setSelectedItemIndex((presetBox.getSelectedItemIndex()-1+presetBox.getNumItems())%presetBox.getNumItems()); };
    next.onClick=[this]{ presetBox.setSelectedItemIndex((presetBox.getSelectedItemIndex()+1)%presetBox.getNumItems()); };
    waveBox.addItemList({"SINE","SAW","SQUARE","TRIANGLE","NOISE"},1);
    waveAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"wave",waveBox);
    wave2Box.addItemList({"SINE 2","SAW 2","SQUARE 2","TRIANGLE 2","NOISE 2"},1);
    wave2Attachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"wave2",wave2Box);
    const std::pair<const char*,const char*> items[]={{"BLEND","blend"},{"OSC MIX","oscMix"},{"FM","fm"},{"SUB","sub"},{"NOISE","noise"},{"DETUNE","detune"},{"ATTACK","attack"},{"DECAY","decay"},
      {"SUSTAIN","sustain"},{"RELEASE","release"},{"CUTOFF","cutoff"},{"RESONANCE","resonance"},{"FILTER ENV","filterEnv"},
      {"DRIVE","drive"},{"CHORUS","chorus"},{"DELAY","delayMix"},{"REVERB","reverb"},{"WIDTH","width"},{"OUTPUT","master"}};
    for(auto item:items) addKnob(item.first,item.second);
    importSample.onClick=[this]{ chooser=std::make_shared<juce::FileChooser>("Load an original WAV or AIFF",juce::File{},"*.wav;*.aif;*.aiff");
      chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& fc){ if(fc.getResult().existsAsFile()) processor.loadSample(fc.getResult()); }); };
    savePreset.onClick=[this]{ chooser=std::make_shared<juce::FileChooser>("Save ATSLO preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("My ATSLO.atslo"),"*.atslo");
      chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting,[this](const juce::FileChooser& fc){ if(fc.getResult()!=juce::File{}) processor.presets.saveUserPreset(fc.getResult().withFileExtension("atslo")); }); };
    loadPreset.onClick=[this]{ chooser=std::make_shared<juce::FileChooser>("Load ATSLO preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),"*.atslo");
      chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& fc){ if(fc.getResult().existsAsFile()) processor.presets.loadUserPreset(fc.getResult()); }); };
    startTimerHz(24);
}
ATSLORichSynthAudioProcessorEditor::~ATSLORichSynthAudioProcessorEditor(){ setLookAndFeel(nullptr); }
void ATSLORichSynthAudioProcessorEditor::addKnob(juce::String text,const char* id){
    auto s=std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,juce::Slider::TextBoxBelow);
    s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,66,16); auto l=std::make_unique<juce::Label>(); l->setText(text,juce::dontSendNotification); l->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*s); addAndMakeVisible(*l); knobAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,*s)); knobs.push_back(std::move(s)); labels.push_back(std::move(l));
}
void ATSLORichSynthAudioProcessorEditor::paint(juce::Graphics& g){
    g.fillAll(juce::Colour(0xff0b0b0d)); auto r=getLocalBounds().toFloat(); juce::ColourGradient grad(juce::Colour(0xff282014),r.getCentreX(),0,juce::Colour(0xff08090b),r.getCentreX(),r.getBottom(),false); g.setGradientFill(grad); g.fillRect(r);
    g.setColour(juce::Colour(0xff9d7720)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8),10,1.5f);
    g.setColour(juce::Colour(0x25ffc13b)); for(int y=115;y<getHeight()-110;y+=145) g.drawHorizontalLine(y,18,float(getWidth()-18));
}
void ATSLORichSynthAudioProcessorEditor::resized(){
    auto area=getLocalBounds().reduced(22); auto top=area.removeFromTop(72); brand.setBounds(top.removeFromLeft(150)); subtitle.setBounds(top.removeFromLeft(120)); previous.setBounds(top.removeFromLeft(34).reduced(2)); next.setBounds(top.removeFromLeft(34).reduced(2)); presetBox.setBounds(top.removeFromLeft(330).reduced(4,14)); importSample.setBounds(top.removeFromRight(130).reduced(3,14)); loadPreset.setBounds(top.removeFromRight(64).reduced(3,14)); savePreset.setBounds(top.removeFromRight(64).reduced(3,14));
    auto keys=area.removeFromBottom(90); keyboard.setBounds(keys.reduced(2)); auto row=area.removeFromTop(65); waveBox.setBounds(row.removeFromLeft(150).reduced(8,16)); wave2Box.setBounds(row.removeFromLeft(150).reduced(8,16)); meter.setBounds(row.removeFromRight(120));
    const int cols=5, rows=(int(knobs.size())+cols-1)/cols, w=area.getWidth()/cols, h=area.getHeight()/rows;
    for(size_t i=0;i<knobs.size();++i){ int x=int(i)%cols, y=int(i)/cols; auto cell=juce::Rectangle<int>(area.getX()+x*w,area.getY()+y*h,w,h); labels[i]->setBounds(cell.removeFromTop(22)); knobs[i]->setBounds(cell.reduced(8)); }
}
void ATSLORichSynthAudioProcessorEditor::timerCallback(){ meter.setText("OUT  "+juce::String(processor.outputLevel.load(),1)+" dB",juce::dontSendNotification); }
