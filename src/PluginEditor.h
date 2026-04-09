#pragma once

#include "PluginProcessor.h"

class GifDanceAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit GifDanceAudioProcessorEditor (GifDanceAudioProcessor&);
    ~GifDanceAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    class GifPreviewComponent final : public juce::Component
    {
    public:
        void setPreviewState (GifDanceAudioProcessor::PreviewState newState);
        void paint (juce::Graphics& g) override;

    private:
        GifDanceAudioProcessor::PreviewState previewState;
    };

    class SettingsPanel final : public juce::Component
    {
    public:
        void paint (juce::Graphics& g) override;
    };

    class SettingsButton final : public juce::Button
    {
    public:
        SettingsButton();
        void paintButton (juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;
    };

    void timerCallback() override;
    void refreshFromProcessor();
    void chooseGifFile();
    void setSettingsVisible (bool shouldBeVisible);
    void updateSettingsButtonState();
    void updateFrameRangeControls (const GifDanceAudioProcessor::PreviewState& previewState);
    bool isMouseOverEditor() const;

    GifDanceAudioProcessor& processorRef;

    GifPreviewComponent preview;
    SettingsPanel settingsPanel;
    SettingsButton settingsButton;
    juce::TextButton openButton { "Open GIF" };
    juce::TextButton reloadButton { "Reload" };
    juce::ComboBox loopBeatsBox;
    juce::Label loopBeatsLabel { {}, "Loop beats" };
    juce::Slider startFrameSlider;
    juce::Slider endFrameSlider;
    juce::Label startFrameLabel { {}, "Start frame" };
    juce::Label endFrameLabel { {}, "End frame" };
    juce::Label fileLabel;
    juce::Label bpmLabel;
    juce::Label transportLabel;
    juce::Label statusLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> loopBeatsAttachment;
    bool settingsVisible = false;
    bool updatingFrameControls = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GifDanceAudioProcessorEditor)
};
