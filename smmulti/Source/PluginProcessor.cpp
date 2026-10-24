#include "PluginProcessor.h"
#include "PluginEditor.h"

MultiProcessor::MultiProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MultiProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    StringArray modelNames;
    for (int i = 0; i < Models::count; ++i) modelNames.add (Models::name (i));

    auto hz = [] (const char* id, const char* name, float lo, float hi, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                    NormalisableRange<float> (lo, hi, 1.0f, 0.4f), def,
                    AudioParameterFloatAttributes().withLabel ("Hz"));
    };
    auto pct = [] (const char* id, const char* name, float hi, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                    NormalisableRange<float> (0.0f, hi, 0.1f), def,
                    AudioParameterFloatAttributes().withLabel ("%"));
    };
    auto db = [] (const char* id, const char* name)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                    NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withLabel ("dB"));
    };

    layout.add (hz ("xo1", "Low/Mid Crossover", 40.0f, 2000.0f, 200.0f));
    layout.add (hz ("xo2", "Mid/High Crossover", 500.0f, 12000.0f, 2500.0f));

    const char* mid[3] = { "m0", "m1", "m2" };
    const char* did[3] = { "d0", "d1", "d2" };
    const char* lid[3] = { "l0", "l1", "l2" };
    const char* bn[3]  = { "Low", "Mid", "High" };
    const int defModel[3] = { 0, 2, 1 };
    for (int b = 0; b < 3; ++b)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { mid[b], 1 }, String (bn[b]) + " Model", modelNames, defModel[b]));
        layout.add (pct (did[b], (String (bn[b]) + " Drive").toRawUTF8(), 100.0f, 35.0f));
        layout.add (db  (lid[b], (String (bn[b]) + " Level").toRawUTF8()));
    }

    layout.add (pct ("fb", "Feedback", 95.0f, 0.0f));
    layout.add (hz  ("fbf", "Feedback Freq", 40.0f, 2000.0f, 220.0f));
    layout.add (pct ("mix", "Mix", 100.0f, 100.0f));
    layout.add (db  ("out", "Output"));
    return layout;
}

MultiEngine::Params MultiProcessor::readParams() const
{
    MultiEngine::Params p;
    p.xo1 = raw ("xo1");
    p.xo2 = raw ("xo2");
    const char* mid[3] = { "m0", "m1", "m2" };
    const char* did[3] = { "d0", "d1", "d2" };
    const char* lid[3] = { "l0", "l1", "l2" };
    for (int b = 0; b < 3; ++b)
    {
        p.model[b]   = juce::jlimit (0, Models::count - 1, (int) std::lround (raw (mid[b])));
        p.drive[b]   = raw (did[b]) * 0.01f;
        p.levelDb[b] = raw (lid[b]);
    }
    p.fb     = raw ("fb") * 0.01f;
    p.fbFreq = raw ("fbf");
    p.mix    = raw ("mix") * 0.01f;
    p.outDb  = raw ("out");
    return p;
}

bool MultiProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == l.getMainInputChannelSet();
}

void MultiProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    engine.setParams (readParams());
}

void MultiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    engine.setParams (readParams());
    engine.process (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
}

juce::AudioProcessorEditor* MultiProcessor::createEditor() { return new MultiEditor (*this); }

void MultiProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void MultiProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MultiProcessor(); }
