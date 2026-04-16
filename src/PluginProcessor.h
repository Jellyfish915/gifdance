#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>

#include "GifDecoder.h"

class GifDanceAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr const char* loopBeatsParameterId = "loopBeats";
    static constexpr const char* gifPathPropertyId = "gifPath";
    static constexpr const char* startFramePropertyId = "startFrame";
    static constexpr const char* endFramePropertyId = "endFrame";
    static constexpr const char* offsetFramePropertyId = "offsetFrame";
    static constexpr const char* pingPongPropertyId = "pingPong";
    static constexpr const char* editorWidthPropertyId = "editorWidth";
    static constexpr const char* editorHeightPropertyId = "editorHeight";
    static constexpr std::array<double, 8> loopBeatValues { 0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0 };
    static constexpr int defaultEditorWidth = 760;
    static constexpr int defaultEditorHeight = 520;

    struct PreviewState
    {
        juce::Image frame;
        juce::String fileName;
        juce::String fullPath;
        juce::String statusText;
        juce::String loopBeatsLabel;
        double bpm = 120.0;
        double ppqPosition = 0.0;
        double loopBeats = 4.0;
        int frameCount = 0;
        int startFrame = 0;
        int endFrame = 0;
        int offsetFrame = 0;
        bool pingPongEnabled = false;
        bool isPlaying = false;
        bool transportAvailable = false;
        bool hasGif = false;
    };

    GifDanceAudioProcessor();
    ~GifDanceAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    PreviewState getPreviewState() const;
    juce::Result loadGifFromFile (const juce::File& file);
    juce::Result reloadGif();
    juce::File getCurrentGifFile() const;
    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept;
    void setFrameRange (int startFrame, int endFrame);
    std::pair<int, int> getFrameRange() const;
    void setOffsetFrame (int offsetFrame);
    int getOffsetFrame() const;
    void setPingPongEnabled (bool shouldBeEnabled);
    bool isPingPongEnabled() const;
    void setEditorSize (int width, int height);
    juce::Point<int> getEditorSize() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static BusesProperties createBusLayout();

    void updateTransportFromHost();
    juce::Result loadGifFromFileInternal (const juce::File& file, bool clearOnFailure);
    void clearGif (const juce::String& newStatusText, const juce::File& replacementPath);
    void restoreGifFromState();
    static std::pair<int, int> clampFrameRange (int startFrame, int endFrame, int frameCount);
    static int clampOffsetFrame (int offsetFrame, int startFrame, int endFrame);
    std::pair<int, int> getStoredFrameRangeLocked() const;
    std::pair<int, int> getClampedFrameRangeLocked() const;
    void storeFrameRangeLocked (int startFrame, int endFrame);
    int getStoredOffsetFrameLocked() const;
    int getClampedOffsetFrameLocked() const;
    void storeOffsetFrameLocked (int offsetFrame);
    bool getPingPongEnabledLocked() const;
    void storePingPongEnabledLocked (bool shouldBeEnabled);
    int getLoopBeatIndex() const;
    double getLoopBeats() const;
    juce::String getLoopBeatsLabel() const;

    mutable juce::CriticalSection stateLock;

    juce::AudioProcessorValueTreeState valueTreeState;
    GifAnimation animation;
    juce::File currentGifFile;
    juce::String statusText { "No GIF loaded." };

    std::atomic<double> bpmAtomic { 120.0 };
    std::atomic<double> ppqAtomic { 0.0 };
    std::atomic<bool> playingAtomic { false };
    std::atomic<bool> transportAvailableAtomic { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GifDanceAudioProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
