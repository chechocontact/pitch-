#include "SynthHost.h"

SynthHost::SynthHost()
{
    formats.addFormat (new juce::VST3PluginFormat());
}

SynthHost::~SynthHost()
{
    unload();
}

void SynthHost::prepare (double sr, int maxBlock)
{
    sampleRate = sr;
    blockSize = maxBlock;

    if (auto* s = active.load())
    {
        s->instance->setRateAndBufferSizeDetails (sr, maxBlock);
        s->instance->prepareToPlay (sr, maxBlock);
        s->scratchAudio.setSize (juce::jmax (2, s->instance->getTotalNumOutputChannels()), maxBlock, false, true, true);
    }
}

void SynthHost::release()
{
    if (auto* s = active.load())
        s->instance->releaseResources();
}

bool SynthHost::setUpSlot (Slot& s, juce::String& error)
{
    if (s.instance == nullptr)
    {
        error = "The plugin could not be created.";
        return false;
    }

    // Stereo out, no sidechain/input needed: a synth. Try the layouts a synth is likely to accept.
    auto layout = s.instance->getBusesLayout();
    if (s.instance->getBusCount (false) > 0)
        layout.getChannelSet (false, 0) = juce::AudioChannelSet::stereo();
    if (s.instance->getBusCount (true) > 0)
        layout.getChannelSet (true, 0) = juce::AudioChannelSet::disabled();
    if (! s.instance->setBusesLayout (layout))
        s.instance->enableAllBuses();

    if (! s.instance->acceptsMidi())
    {
        error = "This plugin does not accept MIDI, so it is not an instrument.";
        return false;
    }

    s.instance->setRateAndBufferSizeDetails (sampleRate, blockSize);
    s.instance->prepareToPlay (sampleRate, blockSize);
    s.scratchAudio.setSize (juce::jmax (2, s.instance->getTotalNumOutputChannels()), blockSize, false, true, true);
    s.scratchMidi.ensureSize (8192);
    return true;
}

juce::String SynthHost::loadFromFile (const juce::File& vst3)
{
    if (! vst3.exists())
        return "File not found: " + vst3.getFullPathName();

    juce::OwnedArray<juce::PluginDescription> found;
    for (auto* format : formats.getFormats())
        format->findAllTypesForFile (found, vst3.getFullPathName());

    if (found.isEmpty())
        return "No VST3 plugin found in " + vst3.getFileName();

    // Prefer an instrument description when the file holds several (shell plugins).
    const juce::PluginDescription* chosen = found.getFirst();
    for (auto* d : found)
        if (d->isInstrument)
        {
            chosen = d;
            break;
        }

    juce::String error;
    auto instance = formats.createPluginInstance (*chosen, sampleRate, blockSize, error);
    if (instance == nullptr)
        return error.isEmpty() ? juce::String ("The plugin failed to load.") : error;

    auto fresh = std::make_unique<Slot>();
    fresh->instance = std::move (instance);
    if (! setUpSlot (*fresh, error))
        return error;

    unload();

    pluginFile = vst3;
    pluginName = chosen->name;
    slot = std::move (fresh);
    active.store (slot.get());
    return {};
}

void SynthHost::unload()
{
    // Stop the audio thread from picking the slot up, then wait for a block in flight to finish.
    active.store (nullptr);
    for (int i = 0; i < 2000 && inProcess.load(); ++i)
        juce::Thread::sleep (1);

    if (slot != nullptr)
    {
        slot->instance->releaseResources();
        slot.reset();
    }
    pluginName.clear();
    pluginFile = juce::File();
}

juce::String SynthHost::getName() const
{
    return pluginName;
}

bool SynthHost::process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    inProcess.store (true);
    auto* s = active.load();
    if (s == nullptr)
    {
        inProcess.store (false);
        return false;
    }

    const int n = buffer.getNumSamples();
    if (n > s->scratchAudio.getNumSamples())
    {
        inProcess.store (false);
        return false; // host gave a larger block than announced; skip rather than allocate on the audio thread
    }

    s->scratchAudio.clear();
    s->scratchMidi.clear();
    s->scratchMidi.addEvents (midi, 0, n, 0);

    juce::AudioBuffer<float> view (s->scratchAudio.getArrayOfWritePointers(), s->scratchAudio.getNumChannels(), n);
    s->instance->processBlock (view, s->scratchMidi);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.addFrom (ch, 0, s->scratchAudio, juce::jmin (ch, view.getNumChannels() - 1), 0, n);

    inProcess.store (false);
    return true;
}

std::unique_ptr<juce::AudioProcessorEditor> SynthHost::createEditor()
{
    if (auto* s = active.load())
        if (s->instance->hasEditor())
            return std::unique_ptr<juce::AudioProcessorEditor> (s->instance->createEditorIfNeeded());
    return nullptr;
}

void SynthHost::saveState (juce::XmlElement& parent) const
{
    auto* s = active.load();
    if (s == nullptr)
        return;

    auto* e = parent.createNewChildElement ("HostedSynth");
    e->setAttribute ("file", pluginFile.getFullPathName());
    e->setAttribute ("name", pluginName);

    juce::MemoryBlock state;
    s->instance->getStateInformation (state);
    e->setAttribute ("state", state.toBase64Encoding());
}

void SynthHost::restoreState (const juce::XmlElement& parent)
{
    const auto* e = parent.getChildByName ("HostedSynth");
    if (e == nullptr)
    {
        unload();
        return;
    }

    const juce::File file (e->getStringAttribute ("file"));
    if (loadFromFile (file).isNotEmpty())
        return; // synth missing on this machine: the pattern still loads, the user picks another synth

    juce::MemoryBlock state;
    if (state.fromBase64Encoding (e->getStringAttribute ("state")) && state.getSize() > 0)
        if (auto* s = active.load())
            s->instance->setStateInformation (state.getData(), (int) state.getSize());
}

juce::Array<juce::File> SynthHost::findInstalledVst3()
{
    juce::Array<juce::File> roots;
    roots.add (juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory).getChildFile ("Common Files/VST3"));
    roots.add (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getParentDirectory().getChildFile ("Local/Programs/Common/VST3"));
    roots.add (juce::File ("C:/Program Files/Common Files/VST3"));

    juce::Array<juce::File> result;
    for (const auto& root : roots)
    {
        if (! root.isDirectory())
            continue;
        for (const auto& f : root.findChildFiles (juce::File::findFilesAndDirectories, true, "*.vst3"))
        {
            if (f.getFileName().containsIgnoreCase ("PitchPaint"))
                continue; // never offer ourselves
            result.addIfNotAlreadyThere (f);
        }
    }
    return result;
}
