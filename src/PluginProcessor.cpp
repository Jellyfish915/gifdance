#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
juce::StringArray createLoopBeatLabels()
{
    return { "1/8", "1/4", "1/2", "1", "2", "4", "8", "16" };
}
}

GifDanceAudioProcessor::GifDanceAudioProcessor()
    : AudioProcessor (createBusLayout()),
      valueTreeState (*this, nullptr, "state", createParameterLayout())
{
    valueTreeState.state.setProperty (gifPathPropertyId, {}, nullptr);
    valueTreeState.state.setProperty (startFramePropertyId, 0, nullptr);
    valueTreeState.state.setProperty (endFramePropertyId, -1, nullptr);
    valueTreeState.state.setProperty (editorWidthPropertyId, defaultEditorWidth, nullptr);
    valueTreeState.state.setProperty (editorHeightPropertyId, defaultEditorHeight, nullptr);
}

void GifDanceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void GifDanceAudioProcessor::releaseResources()
{
}

bool GifDanceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainInput = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput != mainOutput)
        return false;

    return mainInput == juce::AudioChannelSet::mono() || mainInput == juce::AudioChannelSet::stereo();
}

void GifDanceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    updateTransportFromHost();
}

juce::AudioProcessorEditor* GifDanceAudioProcessor::createEditor()
{
    return new GifDanceAudioProcessorEditor (*this);
}

bool GifDanceAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String GifDanceAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GifDanceAudioProcessor::acceptsMidi() const
{
    return false;
}

bool GifDanceAudioProcessor::producesMidi() const
{
    return false;
}

bool GifDanceAudioProcessor::isMidiEffect() const
{
    return false;
}

double GifDanceAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GifDanceAudioProcessor::getNumPrograms()
{
    return 1;
}

int GifDanceAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GifDanceAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String GifDanceAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void GifDanceAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void GifDanceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    {
        const juce::ScopedLock lock (stateLock);
        valueTreeState.state.setProperty (gifPathPropertyId, currentGifFile.getFullPathName(), nullptr);
    }

    if (auto xml = valueTreeState.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void GifDanceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        const auto newState = juce::ValueTree::fromXml (*xml);

        if (newState.isValid())
            valueTreeState.replaceState (newState);
    }

    restoreGifFromState();
}

GifDanceAudioProcessor::PreviewState GifDanceAudioProcessor::getPreviewState() const
{
    PreviewState preview;
    preview.bpm = bpmAtomic.load();
    preview.ppqPosition = ppqAtomic.load();
    preview.isPlaying = playingAtomic.load();
    preview.transportAvailable = transportAvailableAtomic.load();
    preview.loopBeats = getLoopBeats();
    preview.loopBeatsLabel = getLoopBeatsLabel();

    const juce::ScopedLock lock (stateLock);
    preview.fileName = currentGifFile.getFileName();
    preview.fullPath = currentGifFile.getFullPathName();
    preview.statusText = statusText;
    preview.frameCount = static_cast<int> (animation.frames.size());
    preview.hasGif = animation.isValid();
    const auto frameRange = getClampedFrameRangeLocked();
    preview.startFrame = frameRange.first;
    preview.endFrame = frameRange.second;

    if (! preview.hasGif)
        return preview;

    if (! preview.isPlaying || ! preview.transportAvailable || preview.loopBeats <= 0.0 || animation.totalDurationSeconds <= 0.0)
    {
        preview.frame = animation.frames[preview.startFrame].image;
        return preview;
    }

    auto wrappedPpq = std::fmod (preview.ppqPosition, preview.loopBeats);

    if (wrappedPpq < 0.0)
        wrappedPpq += preview.loopBeats;

    const auto startTimeSeconds = preview.startFrame > 0 ? animation.frames[preview.startFrame - 1].endTimeSeconds : 0.0;
    const auto endTimeSeconds = animation.frames[preview.endFrame].endTimeSeconds;
    const auto activeDurationSeconds = endTimeSeconds - startTimeSeconds;

    if (activeDurationSeconds <= 0.0)
    {
        preview.frame = animation.frames[preview.startFrame].image;
        return preview;
    }

    const auto normalizedPhase = juce::jlimit (0.0,
                                               std::nextafter (1.0, 0.0),
                                               wrappedPpq / preview.loopBeats);
    const auto targetTimeSeconds = startTimeSeconds + normalizedPhase * activeDurationSeconds;

    const auto rangeStart = animation.frames.begin() + preview.startFrame;
    const auto rangeEnd = animation.frames.begin() + preview.endFrame + 1;
    const auto frameIt = std::lower_bound (rangeStart,
                                           rangeEnd,
                                           targetTimeSeconds,
                                           [] (const GifFrame& frame, double targetTime)
                                           {
                                               return frame.endTimeSeconds <= targetTime;
                                           });

    preview.frame = (frameIt != rangeEnd ? frameIt->image : animation.frames[preview.endFrame].image);
    return preview;
}

juce::Result GifDanceAudioProcessor::loadGifFromFile (const juce::File& file)
{
    return loadGifFromFileInternal (file, false);
}

juce::Result GifDanceAudioProcessor::reloadGif()
{
    const auto file = getCurrentGifFile();

    if (file == juce::File())
        return juce::Result::fail ("Choose a GIF before reloading.");

    return loadGifFromFileInternal (file, true);
}

juce::File GifDanceAudioProcessor::getCurrentGifFile() const
{
    const juce::ScopedLock lock (stateLock);
    return currentGifFile;
}

juce::AudioProcessorValueTreeState& GifDanceAudioProcessor::getValueTreeState() noexcept
{
    return valueTreeState;
}

void GifDanceAudioProcessor::setFrameRange (int startFrame, int endFrame)
{
    const juce::ScopedLock lock (stateLock);
    const auto clampedRange = clampFrameRange (startFrame, endFrame, static_cast<int> (animation.frames.size()));
    storeFrameRangeLocked (clampedRange.first, clampedRange.second);
}

std::pair<int, int> GifDanceAudioProcessor::getFrameRange() const
{
    const juce::ScopedLock lock (stateLock);
    return getClampedFrameRangeLocked();
}

void GifDanceAudioProcessor::setEditorSize (int width, int height)
{
    const juce::ScopedLock lock (stateLock);
    valueTreeState.state.setProperty (editorWidthPropertyId, width, nullptr);
    valueTreeState.state.setProperty (editorHeightPropertyId, height, nullptr);
}

juce::Point<int> GifDanceAudioProcessor::getEditorSize() const
{
    const juce::ScopedLock lock (stateLock);
    const auto width = static_cast<int> (valueTreeState.state.getProperty (editorWidthPropertyId, defaultEditorWidth));
    const auto height = static_cast<int> (valueTreeState.state.getProperty (editorHeightPropertyId, defaultEditorHeight));
    return { width, height };
}

juce::AudioProcessorValueTreeState::ParameterLayout GifDanceAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { loopBeatsParameterId, 1 },
                                                                        "Loop Beats",
                                                                        createLoopBeatLabels(),
                                                                        5));

    return { parameters.begin(), parameters.end() };
}

GifDanceAudioProcessor::BusesProperties GifDanceAudioProcessor::createBusLayout()
{
    return BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true);
}

void GifDanceAudioProcessor::updateTransportFromHost()
{
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
        {
            if (const auto bpm = position->getBpm())
                bpmAtomic.store (*bpm);

            if (const auto ppq = position->getPpqPosition())
                ppqAtomic.store (*ppq);
            else
                ppqAtomic.store (0.0);

            playingAtomic.store (position->getIsPlaying());
            transportAvailableAtomic.store (position->getBpm().hasValue() && position->getPpqPosition().hasValue());
            return;
        }
    }

    transportAvailableAtomic.store (false);
    playingAtomic.store (false);
    ppqAtomic.store (0.0);
}

juce::Result GifDanceAudioProcessor::loadGifFromFileInternal (const juce::File& file, bool clearOnFailure)
{
    GifAnimation decodedAnimation;
    const auto result = GifDecoder::loadFromFile (file, decodedAnimation);

    const juce::ScopedLock lock (stateLock);

    if (result.wasOk())
    {
        animation = std::move (decodedAnimation);
        currentGifFile = file;
        const auto clampedRange = getClampedFrameRangeLocked();
        storeFrameRangeLocked (clampedRange.first, clampedRange.second);
        statusText = "Loaded " + file.getFileName() + " (" + juce::String (static_cast<int> (animation.frames.size()))
                     + " frames, " + juce::String (animation.width) + "x" + juce::String (animation.height) + ").";
        valueTreeState.state.setProperty (gifPathPropertyId, currentGifFile.getFullPathName(), nullptr);
        return result;
    }

    statusText = result.getErrorMessage();

    if (clearOnFailure)
    {
        animation.clear();
        currentGifFile = file;
        valueTreeState.state.setProperty (gifPathPropertyId, currentGifFile.getFullPathName(), nullptr);
    }

    return result;
}

void GifDanceAudioProcessor::clearGif (const juce::String& newStatusText, const juce::File& replacementPath)
{
    const juce::ScopedLock lock (stateLock);
    animation.clear();
    currentGifFile = replacementPath;
    statusText = newStatusText;
    valueTreeState.state.setProperty (gifPathPropertyId, currentGifFile.getFullPathName(), nullptr);
}

void GifDanceAudioProcessor::restoreGifFromState()
{
    const auto storedPath = valueTreeState.state.getProperty (gifPathPropertyId).toString();

    if (storedPath.isEmpty())
    {
        clearGif ("No GIF loaded.", {});
        return;
    }

    const auto file = juce::File (storedPath);
    const auto result = loadGifFromFileInternal (file, true);

    if (result.failed())
    {
        const juce::ScopedLock lock (stateLock);
        statusText = "Saved GIF could not be restored: " + storedPath;
    }
}

std::pair<int, int> GifDanceAudioProcessor::clampFrameRange (int startFrame, int endFrame, int frameCount)
{
    if (frameCount <= 0)
        return { 0, 0 };

    startFrame = juce::jlimit (0, frameCount - 1, startFrame);

    if (endFrame < 0)
        endFrame = frameCount - 1;
    else
        endFrame = juce::jlimit (0, frameCount - 1, endFrame);

    if (endFrame < startFrame)
        endFrame = startFrame;

    return { startFrame, endFrame };
}

std::pair<int, int> GifDanceAudioProcessor::getStoredFrameRangeLocked() const
{
    const auto startFrame = static_cast<int> (valueTreeState.state.getProperty (startFramePropertyId, 0));
    const auto endFrame = static_cast<int> (valueTreeState.state.getProperty (endFramePropertyId, -1));
    return { startFrame, endFrame };
}

std::pair<int, int> GifDanceAudioProcessor::getClampedFrameRangeLocked() const
{
    const auto storedRange = getStoredFrameRangeLocked();
    return clampFrameRange (storedRange.first, storedRange.second, static_cast<int> (animation.frames.size()));
}

void GifDanceAudioProcessor::storeFrameRangeLocked (int startFrame, int endFrame)
{
    valueTreeState.state.setProperty (startFramePropertyId, startFrame, nullptr);
    valueTreeState.state.setProperty (endFramePropertyId, endFrame, nullptr);
}

int GifDanceAudioProcessor::getLoopBeatIndex() const
{
    const auto* rawValue = valueTreeState.getRawParameterValue (loopBeatsParameterId);

    if (rawValue == nullptr)
        return 5;

    return juce::jlimit (0,
                         static_cast<int> (loopBeatValues.size()) - 1,
                         juce::roundToInt (rawValue->load()));
}

double GifDanceAudioProcessor::getLoopBeats() const
{
    return loopBeatValues[static_cast<size_t> (getLoopBeatIndex())];
}

juce::String GifDanceAudioProcessor::getLoopBeatsLabel() const
{
    return createLoopBeatLabels()[getLoopBeatIndex()];
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GifDanceAudioProcessor();
}
