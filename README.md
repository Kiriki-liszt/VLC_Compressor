# VLC Compressor  

Port of VLC Compressor, a dynamic range compressor ported from plugins from LADSPA SWH  

[![GitHub Release](https://img.shields.io/github/v/release/kiriki-liszt/VLC_Compressor?style=flat-square&label=Get%20latest%20Release)](https://github.com/Kiriki-liszt/VLC_Compressor/releases/latest)
[![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/kiriki-liszt/VLC_Compressor/total?style=flat-square&label=total%20downloads&color=blue)](https://tooomm.github.io/github-release-stats/?username=Kiriki-liszt&repository=VLC_Compressor)  

[![Static Badge](https://img.shields.io/badge/coffee%20maybe%3F%20%3D%5D%20-gray?style=for-the-badge&logo=buy-me-a-coffee)](https://buymeacoffee.com/kirikiaris)

<img src="https://github.com/Kiriki-liszt/VLC_Compressor/raw/main/screenshot.png"  width="600"/>  


### Compatibility  

VST3, AUv2  

### System Requirements

Audio Units  
* Mac OS X 10.13 or later (Intel or Apple Silicon Native) 

VST3  
* Mac OS X 10.13 or later (Intel or Apple Silicon Native) 
* Windows 10 or later

## How to use  

### Windows  

Unzip Windows version from latest Release and copy to "C:\Program Files\Common Files\VST3".  

### MacOS  

Unzip macOS version from latest Release and copy vst3 to "/Library/Audio/Plug-Ins/VST3" and component to "/Library/Audio/Plug-Ins/Components".  

> If it doesn't go well, CodeSign plugins in console as  
>
> ``` console  
> sudo xattr -r -d com.apple.quarantine /Library/Audio/Plug-Ins/VST3/VLC_Compressor.vst3  
> sudo xattr -r -d com.apple.quarantine /Library/Audio/Plug-Ins/Components/VLC_Compressor.component
>
> sudo codesign --force --sign - /Library/Audio/Plug-Ins/VST3/VLC_Compressor.vst3  
> sudo codesign --force --sign - /Library/Audio/Plug-Ins/Components/VLC_Compressor.component  
> ```  

## Licensing  

> Q: I would like to share the source code of my VST 3 plug-in/host on GitHub or other such platform.  
>
> * You can choose the GPLv3 license and feel free to share your plug-ins/host's source code including or referencing the VST 3 SDK's sources on GitHub.  
> * **You are allowed to provide a binary form of your plug-ins/host too, provided that you provide its source code as GPLv3 too.**
> * Note that you have to follow the Steinberg VST usage guidelines.  
>
> <https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Licensing.html>  

<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/VST_Compatible_Logo_Steinberg_with_TM.png"  width="200"/>

VSTSDK 3.7.9 used  
VSTGUI 4.12 used  

## Project Build  

Use CMake to build itself or make IDE project file.  
Supports Windows, Mac, Linux(same as VSTSDK).  

## Version logs

v1.0.0: intial try.  

## references

<https://code.videolan.org/videolan/vlc/-/blob/master/modules/audio_filter/compressor.c>  
<https://github.com/videolan/vlc/blob/master/modules/audio_filter/compressor.c>  
