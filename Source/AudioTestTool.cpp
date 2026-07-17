#include "YanoSpaceProcessor.h"
#include <iostream>

// Dev-only headless sanity check: pushes real audio through the actual
// processBlock() at Amount=0 and Amount=1, verifies no NaN/Inf, the output
// never exceeds the safety ceiling, stereo width actually increases with
// Amount, and the reverb tail is audibly longer at Amount=1 than Amount=0.
namespace
{
    float measureRms(const juce::AudioBuffer<float>& buf, int channel, int startSample, int numSamples)
    {
        double sumSq = 0.0;
        auto* d = buf.getReadPointer(channel);
        for (int i = startSample; i < startSample + numSamples; ++i)
            sumSq += (double) d[i] * d[i];
        return (float) std::sqrt(sumSq / juce::jmax(1, numSamples));
    }

    bool runAmountTest(float amount01)
    {
        const double sr = 44100.0;
        const int blockSize = 512;
        const int totalSamples = (int)(sr * 1.5);

        YanoSpaceProcessor processor;
        processor.setPlayConfigDetails(2, 2, sr, blockSize);
        processor.prepareToPlay(sr, blockSize);
        processor.apvts.getParameter("amount")->setValueNotifyingHost(amount01);

        // Slightly different L/R content (real-ish stereo source, not a mono
        // sum) so there is an actual side signal for width to act on.
        juce::AudioBuffer<float> full(2, totalSamples);
        double phaseL = 0.0, phaseR = 0.0;
        for (int i = 0; i < totalSamples; ++i)
        {
            full.setSample(0, i, 0.5f * (float) std::sin(phaseL));
            full.setSample(1, i, 0.5f * (float) std::sin(phaseR));
            phaseL += juce::MathConstants<double>::twoPi * 220.0 / sr;
            phaseR += juce::MathConstants<double>::twoPi * 225.0 / sr; // slight detune -> real side content
        }

        bool hasNanOrInf = false;
        float peak = 0.0f;
        juce::MidiBuffer midi;

        for (int start = 0; start < totalSamples; start += blockSize)
        {
            const int n = juce::jmin(blockSize, totalSamples - start);
            juce::AudioBuffer<float> block(full.getArrayOfWritePointers(), 2, start, n);
            processor.processBlock(block, midi);

            for (int ch = 0; ch < 2; ++ch)
            {
                auto* d = block.getReadPointer(ch);
                for (int i = 0; i < n; ++i)
                {
                    if (!std::isfinite(d[i])) hasNanOrInf = true;
                    peak = juce::jmax(peak, std::abs(d[i]));
                }
            }
        }

        float peakDb = juce::Decibels::gainToDecibels(peak);
        std::cout << "amount=" << amount01 << "  hasNanOrInf=" << (hasNanOrInf ? "YES (BAD)" : "no")
                   << "  peak=" << peakDb << "dB\n";

        bool ceilingOk = peakDb <= -0.29;
        std::cout << (ceilingOk ? "[PASS] " : "[FAIL] ") << "output never exceeds the -0.3dB safety ceiling\n";

        return !hasNanOrInf && ceilingOk;
    }

    // Renders a burst followed by silence; returns (settled side RMS during
    // the burst, tail RMS measured 100ms after the burst stops).
    struct Metrics { float sideRms; float tailRms; };

    Metrics renderMetrics(float amount01)
    {
        const double sr = 44100.0;
        const int blockSize = 512;
        const int burstSamples = (int)(sr * 0.5);
        const int silenceSamples = (int)(sr * 1.0);
        const int totalSamples = burstSamples + silenceSamples;

        YanoSpaceProcessor processor;
        processor.setPlayConfigDetails(2, 2, sr, blockSize);
        processor.prepareToPlay(sr, blockSize);
        processor.apvts.getParameter("amount")->setValueNotifyingHost(amount01);

        juce::AudioBuffer<float> full(2, totalSamples);
        full.clear();
        double phaseL = 0.0, phaseR = 0.0;
        for (int i = 0; i < burstSamples; ++i)
        {
            full.setSample(0, i, 0.5f * (float) std::sin(phaseL));
            full.setSample(1, i, 0.5f * (float) std::sin(phaseR));
            phaseL += juce::MathConstants<double>::twoPi * 220.0 / sr;
            phaseR += juce::MathConstants<double>::twoPi * 225.0 / sr;
        }

        juce::MidiBuffer midi;
        for (int start = 0; start < totalSamples; start += blockSize)
        {
            const int n = juce::jmin(blockSize, totalSamples - start);
            juce::AudioBuffer<float> block(full.getArrayOfWritePointers(), 2, start, n);
            processor.processBlock(block, midi);
        }

        // Side RMS near the end of the burst (settled).
        const int sideStart = burstSamples - (int)(sr * 0.1);
        double sumSq = 0.0;
        for (int i = sideStart; i < burstSamples; ++i)
        {
            const float side = (full.getSample(0, i) - full.getSample(1, i)) * 0.5f;
            sumSq += (double) side * side;
        }
        const float sideRms = (float) std::sqrt(sumSq / (double)(burstSamples - sideStart));

        // Tail RMS measured 100ms after the burst stops (reverb decay window).
        const int tailStart = burstSamples + (int)(sr * 0.1);
        const int tailLen = (int)(sr * 0.1);
        const float tailRmsL = measureRms(full, 0, tailStart, tailLen);
        const float tailRmsR = measureRms(full, 1, tailStart, tailLen);
        const float tailRms = 0.5f * (tailRmsL + tailRmsR);

        return { sideRms, tailRms };
    }
}

int main()
{
    bool ok = true;
    ok &= runAmountTest(0.0f);
    ok &= runAmountTest(0.5f);
    ok &= runAmountTest(1.0f);

    Metrics low = renderMetrics(0.0f);
    Metrics high = renderMetrics(1.0f);

    std::cout << "side RMS at amount=0: " << juce::Decibels::gainToDecibels(low.sideRms)
               << "dB, at amount=1: " << juce::Decibels::gainToDecibels(high.sideRms) << "dB\n";
    bool widerOk = high.sideRms > low.sideRms;
    std::cout << (widerOk ? "[PASS] " : "[FAIL] ") << "higher Amount widens the stereo image (side RMS increases)\n";
    ok &= widerOk;

    std::cout << "reverb tail RMS at amount=0: " << juce::Decibels::gainToDecibels(low.tailRms)
               << "dB, at amount=1: " << juce::Decibels::gainToDecibels(high.tailRms) << "dB\n";
    bool tailOk = high.tailRms > low.tailRms * 2.0f; // clearly audible difference, not just noise floor
    std::cout << (tailOk ? "[PASS] " : "[FAIL] ") << "higher Amount produces an audibly longer reverb tail\n";
    ok &= tailOk;

    std::cout << (ok ? "\nALL TESTS PASSED\n" : "\nSOME TESTS FAILED\n");
    return ok ? 0 : 1;
}
