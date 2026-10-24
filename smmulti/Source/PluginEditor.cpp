#include "PluginEditor.h"
#include <BinaryData.h>

static const juce::Colour kYellow (0xfff7e03c), kRed (0xffe60f26), kDarkRed (0xff3a0d12), kOrange (0xffff8c1a);

static const char* kKnobIds[12]    = { "d0", "l0", "d1", "l1", "d2", "l2", "xo1", "xo2", "fb", "fbf", "mix", "out" };
static const char* kKnobLabels[12] = { "DRIVE", "LEVEL", "DRIVE", "LEVEL", "DRIVE", "LEVEL",
                                       "LOW XOVER", "HIGH XOVER", "FEEDBACK", "FB FREQ", "MIX", "OUTPUT" };
static const int  kKnobKinds[12]   = { 2, 1, 2, 1, 2, 1, 0, 0, 2, 0, 2, 1 };   // 0 = Hz, 1 = dB, 2 = %
static const char* kModelIds[3]    = { "m0", "m1", "m2" };
static const char* kBandNames[3]   = { "LOW", "MID", "HIGH" };

MotelLookAndFeel::MotelLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1a0f0f));
    setColour (juce::PopupMenu::textColourId, kYellow);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kRed);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a0f0f));
    setColour (juce::ComboBox::textColourId, kYellow);
    setColour (juce::ComboBox::outlineColourId, kRed);
    setColour (juce::ComboBox::arrowColourId, kYellow);
}

juce::Font MotelLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (16.0f, juce::Font::bold));
}

void MotelLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float sw = juce::jmax (4.0f, r * 0.09f);
    const float tr = r - sw * 0.5f;
    const juce::PathStrokeType stroke (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (c.x, c.y, tr, tr, 0.0f, startAngle, endAngle, true);
    g.setColour (kDarkRed);
    g.strokePath (track, stroke);

    juce::Path value;
    value.addCentredArc (c.x, c.y, tr, tr, 0.0f, startAngle, angle, true);
    g.setColour (kRed);
    g.strokePath (value, stroke);

    const float kr = r * 0.74f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff343434), c.x - kr * 0.5f, c.y - kr * 0.6f,
                                             juce::Colour (0xff050505), c.x + kr, c.y + kr, true));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (kYellow);
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 2.0f);

    juce::Path pointer;
    const float pw = juce::jmax (3.0f, kr * 0.09f);
    pointer.addRoundedRectangle (-pw * 0.5f, -kr + kr * 0.12f, pw, kr * 0.42f, pw * 0.5f);
    g.setColour (kYellow);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

MultiEditor::MultiEditor (MultiProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    for (int b = 0; b < 3; ++b)
    {
        if (auto* cp = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (kModelIds[b])))
            models[b].addItemList (cp->choices, 1);
        models[b].setJustificationType (juce::Justification::centred);
        addAndMakeVisible (models[b]);
        modelAtt[b] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, kModelIds[b], models[b]);
    }

    for (int i = 0; i < 12; ++i)
    {
        auto& s = knobs[i];
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 20);
        s.setColour (juce::Slider::textBoxTextColourId, kYellow);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (s);
        knobAtt[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kKnobIds[i], s);

        // texto del valor (se asigna después del attachment)
        switch (kKnobKinds[i])
        {
            case kHz:  s.textFromValueFunction = [] (double v)
                       { return v >= 1000.0 ? juce::String (v / 1000.0, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; }; break;
            case kDb:  s.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " dB"; }; break;
            default:   s.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; }; break;
        }
        s.updateText();
    }

    setSize (820, 560);
}

MultiEditor::~MultiEditor() { setLookAndFeel (nullptr); }

void MultiEditor::resized()
{
    for (int b = 0; b < 3; ++b)
    {
        const int x = 320 + b * 165;
        models[b].setBounds (x + 10, 47, 135, 28);
        knobs[b * 2].setBounds     (x, 88, 155, 110);    // drive
        knobs[b * 2 + 1].setBounds (x, 220, 155, 110);   // level
    }
    for (int i = 0; i < 6; ++i)
        knobs[6 + i].setBounds (15 + i * 131, 384, 131, 160);
}

void MultiEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c1414), 0.0f, 0.0f,
                                             juce::Colour (0xff0a0707), 0.0f, (float) getHeight(), false));
    g.fillAll();

    if (logo.isValid())
        g.drawImage (logo, juce::Rectangle<float> (15.0f, 15.0f, 290.0f, 330.0f), juce::RectanglePlacement::centred);

    const juce::Colour accents[3] = { kRed, kOrange, kYellow };
    for (int b = 0; b < 3; ++b)
    {
        const float x = (float) (320 + b * 165);
        g.setColour (juce::Colour (0x33000000));
        g.fillRoundedRectangle (x, 15.0f, 155.0f, 330.0f, 10.0f);
        g.setColour (kDarkRed);
        g.drawRoundedRectangle (x, 15.0f, 155.0f, 330.0f, 10.0f, 1.5f);

        g.setColour (accents[b]);
        g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
        g.drawText (kBandNames[b], (int) x, 20, 155, 22, juce::Justification::centred);
        g.fillRect (x + 40.0f, 42.0f, 75.0f, 2.0f);

        g.setColour (kYellow);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("DRIVE", (int) x, 74, 155, 16, juce::Justification::centred);
        g.drawText ("LEVEL", (int) x, 204, 155, 16, juce::Justification::centred);
    }

    g.setColour (juce::Colour (0x33000000));
    g.fillRoundedRectangle (15.0f, 360.0f, 790.0f, 190.0f, 10.0f);
    g.setColour (kDarkRed);
    g.drawRoundedRectangle (15.0f, 360.0f, 790.0f, 190.0f, 10.0f, 1.5f);

    g.setColour (kYellow);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    for (int i = 0; i < 6; ++i)
        g.drawText (kKnobLabels[6 + i], 15 + i * 131, 366, 131, 18, juce::Justification::centred);
}
