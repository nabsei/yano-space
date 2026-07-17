#pragma once
#include "YanoSpaceLookAndFeel.h"
#include "YanoSpaceProcessor.h"

class YanoSpaceEditor : public juce::AudioProcessorEditor
{
public:
    explicit YanoSpaceEditor(YanoSpaceProcessor&);
    ~YanoSpaceEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    YanoSpaceProcessor& processorRef;
    YanoSpaceLookAndFeel lookAndFeel;

    juce::Slider amountSlider;
    juce::Label amountValueLabel;
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label footerLabel;
    juce::Label brandLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;

    void updateValueLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(YanoSpaceEditor)
};
