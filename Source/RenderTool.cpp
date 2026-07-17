#include "YanoSpaceProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>

// Dev-only render tool: pushes a real WAV file through the actual
// processBlock() so the result can be auditioned, same code path the
// shipped VST3/AU/Standalone uses.
int main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::cout << "usage: RenderTool input.wav output.wav amount(0-1)\n";
        return 1;
    }

    const juce::File inFile(argv[1]);
    const juce::File outFile(argv[2]);
    const float amount = (float) std::atof(argv[3]);

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inFile));
    if (reader == nullptr)
    {
        std::cout << "could not open " << inFile.getFullPathName() << "\n";
        return 1;
    }

    const int numChannels = juce::jmax(2, (int) reader->numChannels);
    const int numSamples = (int) reader->lengthInSamples;
    const double sampleRate = reader->sampleRate;

    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    buffer.clear();
    reader->read(&buffer, 0, numSamples, 0, true, true);
    if (reader->numChannels == 1)
        buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);

    YanoSpaceProcessor processor;
    processor.setPlayConfigDetails(numChannels, numChannels, sampleRate, 512);
    processor.prepareToPlay(sampleRate, 512);
    processor.apvts.getParameter("amount")->setValueNotifyingHost(amount);

    const int blockSize = 512;
    juce::MidiBuffer midi;
    for (int start = 0; start < numSamples; start += blockSize)
    {
        const int thisBlock = juce::jmin(blockSize, numSamples - start);
        juce::AudioBuffer<float> block(buffer.getArrayOfWritePointers(), numChannels, start, thisBlock);
        processor.processBlock(block, midi);
    }

    outFile.deleteFile();
    std::unique_ptr<juce::FileOutputStream> outStream(outFile.createOutputStream());
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outStream.get(), sampleRate, (unsigned int) numChannels, 16, {}, 0));
    if (writer == nullptr)
    {
        std::cout << "could not create writer\n";
        return 1;
    }
    outStream.release();
    writer->writeFromAudioSampleBuffer(buffer, 0, numSamples);

    std::cout << "wrote " << outFile.getFullPathName() << "\n";
    return 0;
}
