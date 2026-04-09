#pragma once

#include <JuceHeader.h>

#include <vector>

struct GifFrame
{
    juce::Image image;
    double durationSeconds = 0.1;
    double endTimeSeconds = 0.1;
};

struct GifAnimation
{
    std::vector<GifFrame> frames;
    int width = 0;
    int height = 0;
    double totalDurationSeconds = 0.0;

    void clear()
    {
        frames.clear();
        width = 0;
        height = 0;
        totalDurationSeconds = 0.0;
    }

    bool isValid() const noexcept
    {
        return ! frames.empty() && totalDurationSeconds > 0.0;
    }
};

class GifDecoder
{
public:
    static juce::Result loadFromFile (const juce::File& file, GifAnimation& animation);
};
