//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace yg331 {
//------------------------------------------------------------------------
static const Steinberg::FUID kVLC_CompProcessorUID (0x3101A44E, 0xE92E5D4E, 0x932A6FC6, 0xC08EE6F5);
static const Steinberg::FUID kVLC_CompControllerUID (0xB675CF13, 0x9A1E5AAC, 0x9989ADBB, 0x6B588482);

#define VLC_CompVST3Category "Fx"

//------------------------------------------------------------------------
} // namespace yg331
