#include "PluginEditor.h"

#include <array>
#include <cmath>

namespace
{
juce::Colour getEmptyBackground()
{
    return juce::Colour (0xff0a0d10);
}

juce::Colour getSettingsPanelBackground()
{
    return juce::Colour (0xee11161a);
}
}

void GifDanceAudioProcessorEditor::GifPreviewComponent::setPreviewState (GifDanceAudioProcessor::PreviewState newState)
{
    previewState = std::move (newState);
    repaint();
}

void GifDanceAudioProcessorEditor::GifPreviewComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll (juce::Colours::black);

    if (previewState.hasGif && previewState.frame.isValid())
    {
        const auto destination = juce::RectanglePlacement (juce::RectanglePlacement::centred)
                                     .appliedTo (previewState.frame.getBounds().toFloat(), bounds);

        g.drawImage (previewState.frame, destination);

        juce::ColourGradient overlay (juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getY(),
                                      juce::Colours::black.withAlpha (0.28f), bounds.getCentreX(), bounds.getBottom(),
                                      false);
        g.setGradientFill (overlay);
        g.fillRect (bounds);
    }
    else
    {
        juce::ColourGradient background (juce::Colour (0xff11191d), 0.0f, 0.0f,
                                         getEmptyBackground(), bounds.getRight(), bounds.getBottom(),
                                         false);
        g.setGradientFill (background);
        g.fillRect (bounds);

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::FontOptions (28.0f).withStyle ("Bold"));
        g.drawFittedText ("No GIF Loaded", getLocalBounds().reduced (40), juce::Justification::centred, 1);

        g.setColour (juce::Colours::white.withAlpha (0.65f));
        g.setFont (juce::FontOptions (16.0f));
        g.drawFittedText ("Hover the window and open settings to choose a GIF.", getLocalBounds().reduced (40, 90),
                          juce::Justification::centred, 2);
    }
}

void GifDanceAudioProcessorEditor::SettingsPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 6.0f), 24.0f);

    g.setColour (getSettingsPanelBackground());
    g.fillRoundedRectangle (bounds, 24.0f);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (bounds.reduced (1.0f), 24.0f, 1.0f);
}

GifDanceAudioProcessorEditor::SettingsButton::SettingsButton()
    : juce::Button ("settings")
{
}

void GifDanceAudioProcessorEditor::SettingsButton::paintButton (juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    const auto background = isButtonDown ? juce::Colour (0xdd11161a)
                                         : (isMouseOverButton ? juce::Colour (0xcc182127) : juce::Colour (0xbb11161a));

    g.setColour (background);
    g.fillEllipse (bounds);

    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (bounds, 1.0f);

    const auto centre = bounds.getCentre();
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.18f;

    g.setColour (juce::Colours::white.withAlpha (0.92f));

    for (int index = 0; index < 8; ++index)
    {
        const auto angle = juce::MathConstants<float>::twoPi * static_cast<float> (index) / 8.0f;
        const auto innerRadius = radius * 1.8f;
        const auto outerRadius = radius * 2.65f;
        const auto inner = centre + juce::Point<float> (std::cos (angle) * innerRadius, std::sin (angle) * innerRadius);
        const auto outer = centre + juce::Point<float> (std::cos (angle) * outerRadius, std::sin (angle) * outerRadius);
        g.drawLine ({ inner, outer }, 2.0f);
    }

    g.drawEllipse (juce::Rectangle<float> (centre.x - radius * 1.4f, centre.y - radius * 1.4f, radius * 2.8f, radius * 2.8f), 2.0f);
    g.fillEllipse (juce::Rectangle<float> (centre.x - radius * 0.55f, centre.y - radius * 0.55f, radius * 1.1f, radius * 1.1f));
}

GifDanceAudioProcessorEditor::GifDanceAudioProcessorEditor (GifDanceAudioProcessor& processor)
    : AudioProcessorEditor (processor),
      processorRef (processor),
      settingsButton()
{
    static constexpr std::array<const char*, 8> loopBeatLabels { "1/8", "1/4", "1/2", "1", "2", "4", "8", "16" };

    for (size_t index = 0; index < GifDanceAudioProcessor::loopBeatValues.size(); ++index)
        loopBeatsBox.addItem (loopBeatLabels[index], static_cast<int> (index) + 1);

    addAndMakeVisible (preview);
    addAndMakeVisible (settingsPanel);
    addAndMakeVisible (settingsButton);

    preview.setInterceptsMouseClicks (false, false);

    settingsPanel.addAndMakeVisible (openButton);
    settingsPanel.addAndMakeVisible (reloadButton);
    settingsPanel.addAndMakeVisible (pingPongToggle);
    settingsPanel.addAndMakeVisible (loopBeatsBox);
    settingsPanel.addAndMakeVisible (loopBeatsLabel);
    settingsPanel.addAndMakeVisible (startFrameSlider);
    settingsPanel.addAndMakeVisible (endFrameSlider);
    settingsPanel.addAndMakeVisible (offsetFrameSlider);
    settingsPanel.addAndMakeVisible (startFrameLabel);
    settingsPanel.addAndMakeVisible (endFrameLabel);
    settingsPanel.addAndMakeVisible (offsetFrameLabel);
    settingsPanel.addAndMakeVisible (fileLabel);
    settingsPanel.addAndMakeVisible (bpmLabel);
    settingsPanel.addAndMakeVisible (transportLabel);
    settingsPanel.addAndMakeVisible (statusLabel);

    loopBeatsLabel.attachToComponent (&loopBeatsBox, true);
    startFrameLabel.attachToComponent (&startFrameSlider, true);
    endFrameLabel.attachToComponent (&endFrameSlider, true);
    offsetFrameLabel.attachToComponent (&offsetFrameSlider, true);
    loopBeatsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.getValueTreeState(),
                                                                                                     GifDanceAudioProcessor::loopBeatsParameterId,
                                                                                                     loopBeatsBox);

    openButton.onClick = [this] { chooseGifFile(); };
    reloadButton.onClick = [this]
    {
        processorRef.reloadGif();
        refreshFromProcessor();
    };
    settingsButton.onClick = [this] { setSettingsVisible (! settingsVisible); };
    pingPongToggle.onClick = [this]
    {
        processorRef.setPingPongEnabled (pingPongToggle.getToggleState());
        refreshFromProcessor();
    };
    startFrameSlider.onValueChange = [this]
    {
        if (updatingFrameControls)
            return;

        const auto startFrame = juce::roundToInt (startFrameSlider.getValue()) - 1;
        const auto endFrame = juce::roundToInt (endFrameSlider.getValue()) - 1;
        processorRef.setFrameRange (startFrame, endFrame);
        refreshFromProcessor();
    };
    endFrameSlider.onValueChange = [this]
    {
        if (updatingFrameControls)
            return;

        const auto startFrame = juce::roundToInt (startFrameSlider.getValue()) - 1;
        const auto endFrame = juce::roundToInt (endFrameSlider.getValue()) - 1;
        processorRef.setFrameRange (startFrame, endFrame);
        refreshFromProcessor();
    };
    offsetFrameSlider.onValueChange = [this]
    {
        if (updatingFrameControls)
            return;

        processorRef.setOffsetFrame (juce::roundToInt (offsetFrameSlider.getValue()) - 1);
        refreshFromProcessor();
    };

    fileLabel.setJustificationType (juce::Justification::centredLeft);
    fileLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.82f));
    bpmLabel.setJustificationType (juce::Justification::centredLeft);
    bpmLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.82f));
    transportLabel.setJustificationType (juce::Justification::centredLeft);
    transportLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.82f));
    statusLabel.setJustificationType (juce::Justification::topLeft);
    statusLabel.setMinimumHorizontalScale (1.0f);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.68f));
    loopBeatsLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.82f));
    pingPongToggle.setColour (juce::ToggleButton::textColourId, juce::Colours::white.withAlpha (0.88f));
    pingPongToggle.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffffd247));
    pingPongToggle.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::white.withAlpha (0.12f));
    startFrameLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.82f));
    endFrameLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.82f));
    offsetFrameLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.82f));
    loopBeatsBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff182127));
    loopBeatsBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    loopBeatsBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.12f));
    startFrameSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    startFrameSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 24);
    startFrameSlider.setRange (1.0, 1.0, 1.0);
    startFrameSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xfff3bd18));
    startFrameSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffd247));
    startFrameSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    startFrameSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff182127));
    startFrameSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha (0.12f));
    endFrameSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    endFrameSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 24);
    endFrameSlider.setRange (1.0, 1.0, 1.0);
    endFrameSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xfff3bd18));
    endFrameSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffd247));
    endFrameSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    endFrameSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff182127));
    endFrameSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha (0.12f));
    offsetFrameSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    offsetFrameSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 24);
    offsetFrameSlider.setRange (1.0, 1.0, 1.0);
    offsetFrameSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xfff3bd18));
    offsetFrameSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffd247));
    offsetFrameSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    offsetFrameSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff182127));
    offsetFrameSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha (0.12f));

    settingsPanel.setVisible (false);
    settingsButton.setAlpha (0.0f);
    settingsButton.setEnabled (false);

    setResizable (true, true);
    setResizeLimits (320, 240, 2048, 2048);
    const auto savedSize = processorRef.getEditorSize();
    setSize (juce::jlimit (320, 2048, savedSize.x), juce::jlimit (240, 2048, savedSize.y));
    refreshFromProcessor();
    startTimerHz (60);
}

GifDanceAudioProcessorEditor::~GifDanceAudioProcessorEditor()
{
    stopTimer();
}

void GifDanceAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void GifDanceAudioProcessorEditor::resized()
{
    processorRef.setEditorSize (getWidth(), getHeight());
    preview.setBounds (getLocalBounds());

    const auto margin = 18;
    settingsButton.setBounds (getWidth() - margin - 42, margin, 42, 42);

    const auto panelWidth = juce::jmin (340, getWidth() - 36);
    const auto panelHeight = juce::jmin (392, getHeight() - 84);
    settingsPanel.setBounds (getWidth() - margin - panelWidth, margin + 54, panelWidth, panelHeight);

    auto panelBounds = settingsPanel.getLocalBounds().reduced (18);
    auto titleRow = panelBounds.removeFromTop (30);
    openButton.setBounds (titleRow.removeFromLeft (120));
    titleRow.removeFromLeft (10);
    reloadButton.setBounds (titleRow.removeFromLeft (92));

    panelBounds.removeFromTop (16);
    auto loopRow = panelBounds.removeFromTop (28);
    loopBeatsBox.setBounds (loopRow.removeFromLeft (120));

    panelBounds.removeFromTop (10);
    pingPongToggle.setBounds (panelBounds.removeFromTop (24));
    panelBounds.removeFromTop (12);
    startFrameSlider.setBounds (panelBounds.removeFromTop (28));
    panelBounds.removeFromTop (8);
    endFrameSlider.setBounds (panelBounds.removeFromTop (28));
    panelBounds.removeFromTop (8);
    offsetFrameSlider.setBounds (panelBounds.removeFromTop (28));

    panelBounds.removeFromTop (14);
    fileLabel.setBounds (panelBounds.removeFromTop (38));
    panelBounds.removeFromTop (8);

    auto infoRow = panelBounds.removeFromTop (20);
    bpmLabel.setBounds (infoRow.removeFromLeft (120));
    transportLabel.setBounds (infoRow);

    panelBounds.removeFromTop (10);
    statusLabel.setBounds (panelBounds);
}

void GifDanceAudioProcessorEditor::mouseUp (const juce::MouseEvent& event)
{
    if (settingsVisible
        && ! settingsPanel.getBounds().contains (event.getPosition())
        && ! settingsButton.getBounds().contains (event.getPosition()))
    {
        setSettingsVisible (false);
    }
}

void GifDanceAudioProcessorEditor::timerCallback()
{
    refreshFromProcessor();
    updateSettingsButtonState();
}

void GifDanceAudioProcessorEditor::refreshFromProcessor()
{
    const auto previewState = processorRef.getPreviewState();
    preview.setPreviewState (previewState);
    updateFrameControls (previewState);

    const auto resolvedPath = previewState.fullPath.isNotEmpty() ? previewState.fullPath : "No GIF selected";
    fileLabel.setText ("GIF: " + resolvedPath, juce::dontSendNotification);
    fileLabel.setTooltip (previewState.fullPath);
    pingPongToggle.setToggleState (previewState.pingPongEnabled, juce::dontSendNotification);

    bpmLabel.setText (previewState.transportAvailable ? "BPM: " + juce::String (previewState.bpm, 2) : "BPM: --",
                      juce::dontSendNotification);

    transportLabel.setText (previewState.transportAvailable
                                ? "Transport: " + juce::String (previewState.isPlaying ? "Playing" : "Stopped")
                                : "Transport: unavailable",
                            juce::dontSendNotification);

    statusLabel.setText ("Loop: " + previewState.loopBeatsLabel + " beats\n"
                         "Mode: " + juce::String (previewState.pingPongEnabled ? "Ping Pong" : "Loop") + "\n"
                         "Range: " + juce::String (previewState.startFrame + 1) + " - " + juce::String (previewState.endFrame + 1) + "\n"
                         "Offset: " + juce::String (previewState.offsetFrame + 1) + "\n"
                         "Frames: " + juce::String (previewState.frameCount) + "\n"
                         "Status: " + previewState.statusText,
                         juce::dontSendNotification);
    reloadButton.setEnabled (previewState.fullPath.isNotEmpty());
}

void GifDanceAudioProcessorEditor::chooseGifFile()
{
    const auto currentFile = processorRef.getCurrentGifFile();
    const auto initialLocation = currentFile.existsAsFile() ? currentFile : juce::File();

    fileChooser = std::make_unique<juce::FileChooser> ("Select a GIF", initialLocation, "*.gif");
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& chooser)
                              {
                                  const auto selectedFile = chooser.getResult();

                                  if (selectedFile.existsAsFile())
                                      processorRef.loadGifFromFile (selectedFile);

                                  refreshFromProcessor();
                                  fileChooser.reset();
                              });
}

void GifDanceAudioProcessorEditor::setSettingsVisible (bool shouldBeVisible)
{
    settingsVisible = shouldBeVisible;
    settingsPanel.setVisible (settingsVisible);
    updateSettingsButtonState();
}

void GifDanceAudioProcessorEditor::updateFrameControls (const GifDanceAudioProcessor::PreviewState& previewState)
{
    const auto maximumFrame = juce::jmax (1, previewState.frameCount);
    const auto startFrameValue = juce::jlimit (1, maximumFrame, previewState.startFrame + 1);
    const auto endFrameValue = juce::jlimit (startFrameValue, maximumFrame, previewState.endFrame + 1);
    const auto offsetFrameValue = juce::jlimit (startFrameValue, endFrameValue, previewState.offsetFrame + 1);

    updatingFrameControls = true;
    startFrameSlider.setRange (1.0, static_cast<double> (maximumFrame), 1.0);
    endFrameSlider.setRange (1.0, static_cast<double> (maximumFrame), 1.0);
    offsetFrameSlider.setRange (static_cast<double> (startFrameValue), static_cast<double> (endFrameValue), 1.0);
    startFrameSlider.setValue (startFrameValue, juce::dontSendNotification);
    endFrameSlider.setValue (endFrameValue, juce::dontSendNotification);
    offsetFrameSlider.setValue (offsetFrameValue, juce::dontSendNotification);
    updatingFrameControls = false;

    startFrameSlider.setEnabled (previewState.hasGif);
    endFrameSlider.setEnabled (previewState.hasGif);
    offsetFrameSlider.setEnabled (previewState.hasGif);
}

void GifDanceAudioProcessorEditor::updateSettingsButtonState()
{
    const auto shouldShow = settingsVisible || isMouseOverEditor();
    settingsButton.setEnabled (shouldShow);
    settingsButton.setAlpha (shouldShow ? 1.0f : 0.0f);
}

bool GifDanceAudioProcessorEditor::isMouseOverEditor() const
{
    const auto mousePosition = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition().roundToInt();
    return getScreenBounds().contains (mousePosition);
}
