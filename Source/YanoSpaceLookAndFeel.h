#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Same Bumpin Audio catalogue palette as Delta Zero/Delta Blind, Montagem,
// and the rest of the Yano line -- shared visual identity across the whole
// catalogue, not the original warm terracotta/gold.
class YanoSpaceLookAndFeel : public juce::LookAndFeel_V4
{
public:
    inline static const juce::Colour bg        { 0xff0a0912 };
    inline static const juce::Colour bgLighter { 0xff1c1c24 };
    inline static const juce::Colour cyan      { 0xff29c2d6 };
    inline static const juce::Colour magenta   { 0xffe0479c };
    inline static const juce::Colour cream     { 0xffffffff };
    inline static const juce::Colour textDim   { 0xffa0a0aa };

    YanoSpaceLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, cream);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(10.0f);
        const auto centre = bounds.getCentre();
        const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const auto knobColour = cyan.interpolatedWith(magenta, sliderPosProportional);

        {
            juce::Path track;
            track.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f,
                                 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(bgLighter);
            g.strokePath(track, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        {
            juce::Path valueArc;
            valueArc.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f,
                                    0.0f, rotaryStartAngle, angle, true);
            g.setColour(knobColour);
            g.strokePath(valueArc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setColour(knobColour.withAlpha(0.12f));
        g.fillEllipse(bounds.reduced(radius * 0.18f));

        auto face = bounds.reduced(radius * 0.34f);
        g.setColour(bgLighter);
        g.fillEllipse(face);
        g.setColour(knobColour.withAlpha(0.7f));
        g.drawEllipse(face, 1.5f);

        {
            juce::Path pointer;
            const float pointerLength = face.getWidth() * 0.38f;
            const float pointerThickness = 3.5f;
            pointer.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength, pointerThickness * 0.5f);
            pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
            g.setColour(cream);
            g.fillPath(pointer);
        }
    }

    juce::Font getLabelFont(juce::Label&) override
    {
        return juce::Font(juce::FontOptions(14.0f, juce::Font::bold));
    }
};
