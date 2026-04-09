# GifDance

`GifDance` is a Windows VST3 effect plugin that loads a local GIF and snaps its playback to the host transport.

## Current behavior

- Audio is passthrough only.
- The editor uses the full plugin window as the GIF canvas.
- A settings cog appears on hover and opens an overlay for file loading and playback controls.
- GIF playback follows the host `PPQ` position while the transport is running.
- `Loop Beats` is automatable and supports fast sub-beat loops down to `1/8`.
- Playback can be limited to a selected `Start frame` and `End frame`.
- When the host stops, the preview returns to the selected start frame.
- GIFs are rendered with aspect ratio preserved while resizing the plugin window.
- The plugin window size is saved in plugin state and restored when the project is reopened.
- GIF decoding uses composed frames with transparency support.
- The plugin state stores the GIF absolute path, selected loop length, frame range, and editor size.

## Usage

1. Insert `GifDance` on a track in REAPER.
2. Hover the plugin window and click the cog icon.
3. Click `Open GIF` and choose a local `.gif` file.
4. Set `Loop Beats` to the musical length you want.
5. Adjust `Start frame` and `End frame` if you want to loop only part of the GIF.
6. Resize the plugin window as needed. The GIF will scale to fit without changing aspect ratio.

## State and sync details

- Playback is derived from host transport position, not from a free-running timer.
- Seeking, restarting playback, or changing tempo re-aligns the GIF immediately to the host beat position.
- If a saved GIF path is missing when the project reloads, the plugin keeps the stored path and shows an error state instead of crashing.

## Build

The project vendors JUCE and configures with CMake.

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release --target GifDance_VST3
```

The built plugin is written to:

`build-vs/GifDance_artefacts/Release/VST3/GifDance.vst3/Contents/x86_64-win/GifDance.vst3`

## Notes

- REAPER was detected on this machine, but the build does not auto-install into the global VST3 folder.
- The local `build/` directory may contain an abandoned Ninja configure attempt; `build-vs/` is the validated output.

## License

This repository is intended to be published under `AGPL-3.0-or-later`. See
`LICENSE`.

Third-party code is vendored under `vendor/`:

- `vendor/JUCE` is dual-licensed by its authors under `AGPLv3` or the commercial JUCE license.
- `vendor/gifdec` is documented by its upstream project as public domain.

See `THIRD_PARTY_NOTICES.md` for details.
