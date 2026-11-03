# Multiband Studio

A custom four-band dynamics processor for Logic Pro (Audio Unit v2).

## Current feature set
- Four-band serial Linkwitz-Riley-style crossover network
- Independent threshold, ratio, attack, release and makeup controls per band
- Live stereo spectrum analyzer
- Three adjustable crossover frequency controls
- Intel macOS Audio Unit build via GitHub Actions

## Build
The GitHub Actions workflow builds the AU on an Intel macOS runner. It fetches the free, open-source iPlug2 framework during CI and uploads a ZIP artifact containing the `.component` bundle.

The project is configured for macOS 13.0 and Intel x86_64 pending confirmation of the target Mac's macOS version.

## Install
After a successful build, download the `MultibandStudio-AU-Intel` artifact from the Actions run, unzip it, and copy `MultibandStudio.component` to:
`~/Library/Audio/Plug-Ins/Components/`

Then restart Logic Pro and rescan Audio Units if necessary.

## License
Plugin project code: MIT. iPlug2 remains under its own license in its upstream repository.
