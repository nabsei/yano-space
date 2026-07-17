#include "PluginEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    YanoSpaceProcessor processor;
    processor.prepareToPlay(44100.0, 512);
    processor.apvts.getParameter("amount")->setValueNotifyingHost(0.6f);

    YanoSpaceEditor editor(processor);
    editor.setVisible(true);
    editor.resized();

    auto image = editor.createComponentSnapshot(editor.getLocalBounds());
    juce::File outFile("/Users/nabil/.claude/jobs/b49db9b3/tmp/yano_space_ui_snapshot.png");
    outFile.getParentDirectory().createDirectory();
    outFile.deleteFile();
    juce::FileOutputStream stream(outFile);
    juce::PNGImageFormat png;
    png.writeImageToStream(image, stream);

    return 0;
}
