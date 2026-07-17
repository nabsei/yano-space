#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// One-knob "space" processor for amapiano material -- the third piece of the
// Yano line, after Yano Log and Yano Finish. Where Montagem Widener is pure
// M/S width for phonk, amapiano references lean toward an airier, more
// reverberant sense of space (open Rhodes stabs, log drum sitting in room
// rather than dry/upfront), so this chain is air EQ (high shelf) -> spacious
// stereo reverb -> gentle M/S widening on top, all driven by a single
// Amount macro. Bass stays mono below the width crossover, same technique
// as Widener, so the widening never smears the low end.
class YanoSpaceProcessor : public juce::AudioProcessor
{
public:
    YanoSpaceProcessor();
    ~YanoSpaceProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Yano Space"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; } // covers the larger reverb tail

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateFromAmount(float amount01);

    double lastSampleRate = 44100.0;

    juce::dsp::IIR::Filter<float> airFilterL, airFilterR;
    juce::dsp::Reverb reverb;

    // M/S width crossover + Haas delay state, same technique as Montagem Widener.
    float sideLowCoefL = 0.0f, sideLowStateL = 0.0f;
    int delayBufferSize = 0;
    std::vector<float> delayBuffer;
    int delayWritePos = 0;

    juce::SmoothedValue<float> amountSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YanoSpaceProcessor)
};
