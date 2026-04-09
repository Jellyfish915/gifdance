#include "GifDecoder.h"

#include <gifdec.h>

namespace
{
constexpr double fallbackFrameDurationSeconds = 0.1;

struct FrameCompositionState
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int disposal = 0;
};

juce::Image createImageFromRgbaFrame (const uint8_t* pixels, int width, int height)
{
    juce::Image image (juce::Image::ARGB, width, height, true);
    juce::Image::BitmapData bitmap (image, juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < height; ++y)
    {
        auto* destination = reinterpret_cast<juce::PixelARGB*> (bitmap.getLinePointer (y));
        auto* source = pixels + static_cast<size_t> (y) * static_cast<size_t> (width) * 4U;

        for (int x = 0; x < width; ++x)
        {
            destination[x].setARGB (source[3], source[0], source[1], source[2]);
            destination[x].premultiply();
            source += 4;
        }
    }

    return image;
}

void clearFrameRectToTransparent (std::vector<uint8_t>& canvas, int canvasWidth, int canvasHeight, const FrameCompositionState& frameState)
{
    const auto startX = juce::jlimit (0, canvasWidth, frameState.x);
    const auto startY = juce::jlimit (0, canvasHeight, frameState.y);
    const auto endX = juce::jlimit (0, canvasWidth, frameState.x + frameState.width);
    const auto endY = juce::jlimit (0, canvasHeight, frameState.y + frameState.height);

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            auto* pixel = canvas.data() + (static_cast<size_t> (y) * static_cast<size_t> (canvasWidth) + static_cast<size_t> (x)) * 4U;
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 0;
            pixel[3] = 0;
        }
    }
}

void applyDisposal (std::vector<uint8_t>& canvas,
                    const std::vector<uint8_t>& previousCanvasSnapshot,
                    int canvasWidth,
                    int canvasHeight,
                    const FrameCompositionState& previousFrame,
                    bool hasPreviousFrame)
{
    if (! hasPreviousFrame)
        return;

    switch (previousFrame.disposal)
    {
        case 2:
            clearFrameRectToTransparent (canvas, canvasWidth, canvasHeight, previousFrame);
            break;

        case 3:
            if (previousCanvasSnapshot.size() == canvas.size())
                canvas = previousCanvasSnapshot;
            break;

        default:
            break;
    }
}

void compositeCurrentFrame (std::vector<uint8_t>& canvas, const gd_GIF& gif)
{
    for (int localY = 0; localY < gif.fh; ++localY)
    {
        const auto canvasY = gif.fy + localY;

        if (canvasY < 0 || canvasY >= gif.height)
            continue;

        for (int localX = 0; localX < gif.fw; ++localX)
        {
            const auto canvasX = gif.fx + localX;

            if (canvasX < 0 || canvasX >= gif.width)
                continue;

            const auto paletteIndex = gif.frame[static_cast<size_t> (canvasY) * static_cast<size_t> (gif.width)
                                                + static_cast<size_t> (canvasX)];

            if (gif.gce.transparency && paletteIndex == gif.gce.tindex)
                continue;

            auto* destination = canvas.data() + (static_cast<size_t> (canvasY) * static_cast<size_t> (gif.width)
                                                 + static_cast<size_t> (canvasX)) * 4U;
            const auto* source = &gif.palette->colors[static_cast<size_t> (paletteIndex) * 3U];
            destination[0] = source[0];
            destination[1] = source[1];
            destination[2] = source[2];
            destination[3] = 0xff;
        }
    }
}
}

juce::Result GifDecoder::loadFromFile (const juce::File& file, GifAnimation& animation)
{
    animation.clear();

    if (! file.existsAsFile())
        return juce::Result::fail ("GIF file was not found.");

    const auto utf8Path = file.getFullPathName().toUTF8();
    std::unique_ptr<gd_GIF, decltype (&gd_close_gif)> gif (gd_open_gif (utf8Path.getAddress()), &gd_close_gif);

    if (gif == nullptr || gif->width <= 0 || gif->height <= 0)
        return juce::Result::fail ("Failed to decode GIF.");

    animation.width = gif->width;
    animation.height = gif->height;

    std::vector<uint8_t> composedCanvas (static_cast<size_t> (gif->width) * static_cast<size_t> (gif->height) * 4U, 0);
    std::vector<uint8_t> previousCanvasSnapshot;
    FrameCompositionState previousFrame;
    auto hasPreviousFrame = false;
    auto cumulativeTimeSeconds = 0.0;

    while (gd_get_frame (gif.get()) > 0)
    {
        applyDisposal (composedCanvas, previousCanvasSnapshot, gif->width, gif->height, previousFrame, hasPreviousFrame);

        if (gif->gce.disposal == 3)
            previousCanvasSnapshot = composedCanvas;
        else
            previousCanvasSnapshot.clear();

        compositeCurrentFrame (composedCanvas, *gif);

        auto frameDurationSeconds = fallbackFrameDurationSeconds;

        if (gif->gce.delay > 0)
            frameDurationSeconds = juce::jlimit (0.01, 10.0, static_cast<double> (gif->gce.delay) / 100.0);

        cumulativeTimeSeconds += frameDurationSeconds;

        animation.frames.push_back ({
            createImageFromRgbaFrame (composedCanvas.data(), gif->width, gif->height),
            frameDurationSeconds,
            cumulativeTimeSeconds
        });

        previousFrame = { gif->fx, gif->fy, gif->fw, gif->fh, gif->gce.disposal };
        hasPreviousFrame = true;
    }

    if (animation.frames.empty())
        return juce::Result::fail ("GIF did not contain any decodable frames.");

    animation.totalDurationSeconds = cumulativeTimeSeconds;
    return juce::Result::ok();
}
