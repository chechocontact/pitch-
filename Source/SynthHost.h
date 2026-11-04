#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <memory>

// Hosts one VST3 instrument inside PitchPaint, so the drawn notes / MPE bends reach the synth directly and never
// have to leave through the host's MIDI output (which FL Studio does not route for VST3 plugins).
//
// Threading: load / unload / getState / setState on the message thread; process() on the audio thread.
// A loaded plugin is published with an atomic pointer swap, and the old one is destroyed on the message thread
// after the audio thread is guaranteed to have moved on (see unload()).
class SynthHost
{
public:
    SynthHost();
    ~SynthHost();

    // Audio thread calls this from prepareToPlay on the message thread side of the host contract.
    void prepare (double sampleRate, int maxBlockSize);
    void release();

    // Loads a .vst3 file (or bundle folder). Returns an empty string on success, otherwise a readable error.
    juce::String loadFromFile (const juce::File& vst3);
    void unload();

    bool isLoaded() const { return active.load() != nullptr; }
    juce::String getName() const;
    juce::File getFile() const { return pluginFile; }

    // Audio thread: runs the synth for one block. `midi` is consumed. The synth's audio is added into `buffer`.
    // Returns false if nothing is loaded (the buffer is left untouched).
    bool process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    // Message thread: the synth's own editor, or nullptr.
    std::unique_ptr<juce::AudioProcessorEditor> createEditor();

    // Message thread: state for the project file (empty tree when nothing is loaded).
    void saveState (juce::XmlElement& parent) const;
    void restoreState (const juce::XmlElement& parent);

    // Message thread: scans the standard VST3 folders and returns the .vst3 files found.
    static juce::Array<juce::File> findInstalledVst3();

private:
    struct Slot
    {
        std::unique_ptr<juce::AudioPluginInstance> instance;
        juce::MidiBuffer scratchMidi;
        juce::AudioBuffer<float> scratchAudio;
    };

    bool setUpSlot (Slot&, juce::String& error);

    juce::AudioPluginFormatManager formats;
    std::unique_ptr<Slot> slot;
    std::atomic<Slot*> active { nullptr };
    std::atomic<bool> inProcess { false };
    juce::File pluginFile;
    juce::String pluginName;
    double sampleRate = 44100.0;
    int blockSize = 512;
};
