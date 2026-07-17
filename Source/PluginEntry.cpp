#include "YanoSpaceProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new YanoSpaceProcessor();
}
