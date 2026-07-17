#include "YanoSpaceProcessor.h"
#include "PluginEditor.h"

YanoSpaceProcessor::YanoSpaceProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout YanoSpaceProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "amount", "Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    return { params.begin(), params.end() };
}

bool YanoSpaceProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // The M/S width math (and the runtime guard in processBlock) assume
    // exactly 2 channels, same constraint as Montagem Widener.
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void YanoSpaceProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    lastSampleRate = sampleRate;

    airFilterL.prepare({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
    airFilterR.prepare({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
    airFilterL.reset();
    airFilterR.reset();

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    reverb.prepare(spec);
    reverb.reset();

    // One-pole lowpass at 150Hz defines the width crossover -- same formula
    // and same fixed frequency as Montagem Widener, so bass stays mono
    // regardless of how far Amount pushes the width above it.
    sideLowCoefL = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 150.0f / (float) sampleRate);
    sideLowStateL = 0.0f;

    delayBufferSize = (int) (0.05 * sampleRate) + 4;
    delayBuffer.assign((size_t) delayBufferSize, 0.0f);
    delayWritePos = 0;

    amountSmoothed.reset(sampleRate, 0.03);
    amountSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("amount"));

    updateFromAmount(amountSmoothed.getCurrentValue());
}

void YanoSpaceProcessor::updateFromAmount(float amount01)
{
    // Air shelf: gentle high-frequency lift above 9kHz, researched as the
    // Air shelf, reverb size/damping/width and the width-gain curves further
    // below are all tuned against amapiano production references for the
    // shipped build; the exact curves here are simplified for this public
    // version.
    const float airGainDb = juce::jmap(amount01, 0.0f, 1.0f, 0.0f, 3.0f);
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        lastSampleRate, 8000.0f, 0.7f, juce::Decibels::decibelsToGain(airGainDb));
    *airFilterL.coefficients = *coeffs;
    *airFilterR.coefficients = *coeffs;

    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = juce::jmap(amount01, 0.0f, 1.0f, 0.2f, 0.55f);
    rp.damping = juce::jmap(amount01, 0.0f, 1.0f, 0.6f, 0.4f);
    rp.width = juce::jmap(amount01, 0.0f, 1.0f, 0.75f, 1.0f);
    rp.wetLevel = juce::jmap(amount01, 0.0f, 1.0f, 0.0f, 0.18f);
    rp.dryLevel = 1.0f;
    reverb.setParameters(rp);
}

void YanoSpaceProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() < 2)
        return; // width/reverb need a stereo signal; pass mono through untouched

    amountSmoothed.setTargetValue(*apvts.getRawParameterValue("amount"));
    const float amount01 = amountSmoothed.skip(buffer.getNumSamples());
    updateFromAmount(amount01);

    juce::dsp::AudioBlock<float> block(buffer);

    // Air EQ: high-shelf lift, per channel (IIR::Filter is mono).
    auto channelBlockL = block.getSingleChannelBlock(0);
    auto channelBlockR = block.getSingleChannelBlock(1);
    juce::dsp::ProcessContextReplacing<float> airContextL(channelBlockL);
    juce::dsp::ProcessContextReplacing<float> airContextR(channelBlockR);
    airFilterL.process(airContextL);
    airFilterR.process(airContextR);

    // Spacious stereo reverb.
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);

    // Gentle M/S widening on top of the (now reverberant) signal -- see the
    // note above this function about simplified public-version curves.
    const float widthGain = juce::jmap(amount01, 0.0f, 1.0f, 1.0f, 1.4f);
    const float maxDelayMs = 6.0f;
    const float delaySamples = juce::jmap(amount01, 0.0f, 1.0f, 0.0f, maxDelayMs) * 0.001f * (float) lastSampleRate;
    const float haasMix = amount01 * 0.2f;

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float mid = (left[i] + right[i]) * 0.5f;
        const float side = (left[i] - right[i]) * 0.5f;

        sideLowStateL += sideLowCoefL * (side - sideLowStateL);
        const float sideLow = sideLowStateL;
        const float sideHigh = side - sideLow;

        // Haas delay fed from sideHigh only -- never reintroduces L/R
        // difference into the bass, same guard as Widener.
        delayBuffer[(size_t) delayWritePos] = sideHigh;

        float readPos = (float) delayWritePos - delaySamples;
        while (readPos < 0.0f)
            readPos += (float) delayBufferSize;
        const int readIdx0 = (int) readPos % delayBufferSize;
        const int readIdx1 = (readIdx0 + 1) % delayBufferSize;
        const float frac = readPos - std::floor(readPos);
        const float delayedSideHigh = delayBuffer[(size_t) readIdx0] * (1.0f - frac) + delayBuffer[(size_t) readIdx1] * frac;

        delayWritePos = (delayWritePos + 1) % delayBufferSize;

        const float sideOut = sideLow + sideHigh * widthGain + delayedSideHigh * haasMix;

        left[i] = mid + sideOut;
        right[i] = mid - sideOut;
    }

    // Final safety ceiling -- proportional rescale, never a per-sample clamp.
    const float ceiling = juce::Decibels::decibelsToGain(-0.3f);
    float blockPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        blockPeak = juce::jmax(blockPeak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    if (blockPeak > ceiling)
        buffer.applyGain(ceiling / blockPeak);
}

juce::AudioProcessorEditor* YanoSpaceProcessor::createEditor()
{
    return new YanoSpaceEditor(*this);
}

void YanoSpaceProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary(*xml, destData);
}

void YanoSpaceProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
