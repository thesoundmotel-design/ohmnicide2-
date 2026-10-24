#pragma once
#include "PluginProcessor.h"

class MotelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MotelLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};

class MultiEditor : public juce::AudioProcessorEditor
{
public:
    explicit MultiEditor (MultiProcessor&);
    ~MultiEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum Kind { kHz, kDb, kPct };

    MultiProcessor& proc;
    MotelLookAndFeel laf;
    juce::Image logo;

    // 0..5: d0,l0,d1,l1,d2,l2   |   6..11: xo1,xo2,fb,fbf,mix,out
    juce::Slider knobs[12];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtt[12];
    juce::ComboBox models[3];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt[3];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiEditor)
};
