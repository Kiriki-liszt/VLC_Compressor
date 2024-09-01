//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#include "VLCComp_controller.h"
#include "VLCComp_cids.h"

#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstreamer.h"

using namespace Steinberg;

namespace yg331 {

//------------------------------------------------------------------------
// VLC_CompController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompController::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated

	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	// Here you could register some parameters
    int32 stepCount;
    int32 flags;
    int32 tag;
    Vst::ParamValue defaultVal;
    Vst::ParamValue minPlain;
    Vst::ParamValue maxPlain;
    Vst::ParamValue defaultPlain;
    
    tag          = kParamBypass;
    stepCount    = 1;
    defaultVal   = 0;
    flags        = Vst::ParameterInfo::kIsBypass | Vst::ParameterInfo::kCanAutomate;
    parameters.addParameter(STR16("Bypass"), nullptr, stepCount, defaultVal, flags, tag);
    
    Vst::StringListParameter* OS = new Vst::StringListParameter(STR("OS"), kParamOS);
    OS->appendString(STR("x1"));
    OS->appendString(STR("x2"));
    OS->appendString(STR("x4"));
    OS->appendString(STR("x8"));
    OS->setNormalized(OS->toNormalized(0));
    parameters.addParameter(OS);

    tag          = kParamInput;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minInput;
    maxPlain     = maxInput;
    defaultPlain = dftInput;
    stepCount    = 0;
    auto* ParamIn = new Vst::RangeParameter(STR16("Input"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamIn->setPrecision(2);
    parameters.addParameter(ParamIn);

    tag          = kParamOutput;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minOutput;
    maxPlain     = maxOutput;
    defaultPlain = dftOutput;
    stepCount    = 0;
    auto* ParamOut = new Vst::RangeParameter(STR16("Output"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamOut->setPrecision(2);
    parameters.addParameter(ParamOut);

    tag          = kParamRMS_PEAK;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minRMS_PEAK;
    maxPlain     = maxRMS_PEAK;
    defaultPlain = dftRMS_PEAK;
    stepCount    = 0;
    auto* ParamRMS_PEAK = new Vst::RangeParameter(STR16("RMS/PEAK"), tag, STR16("%"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamRMS_PEAK->setPrecision(2);
    parameters.addParameter(ParamRMS_PEAK);

    tag          = kParamAttack;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minAttack;
    maxPlain     = maxAttack;
    defaultPlain = dftAttack;
    stepCount    = 0;
    auto* ParamAttack = new Vst::RangeParameter(STR16("Attack"), tag, STR16("ms"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamAttack->setPrecision(2);
    parameters.addParameter(ParamAttack);

    tag          = kParamRelease;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minRelease;
    maxPlain     = maxRelease;
    defaultPlain = dftRelease;
    stepCount    = 0;
    auto* ParamRelease = new Vst::RangeParameter(STR16("Release"), tag, STR16("ms"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamRelease->setPrecision(2);
    parameters.addParameter(ParamRelease);

    tag          = kParamThreshold;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minThreshold;
    maxPlain     = maxThreshold;
    defaultPlain = dftThreshold;
    stepCount    = 0;
    auto* ParamThreshold = new Vst::RangeParameter(STR16("Threshold"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamThreshold->setPrecision(2);
    parameters.addParameter(ParamThreshold);

    tag          = kParamRatio;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minRatio;
    maxPlain     = maxRatio;
    defaultPlain = dftRatio;
    stepCount    = 0;
    auto* ParamRatio = new Vst::RangeParameter(STR16("Ratio"), tag, STR16(""), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamRatio->setPrecision(2);
    parameters.addParameter(ParamRatio);

    tag          = kParamKnee;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minKnee;
    maxPlain     = maxKnee;
    defaultPlain = dftKnee;
    stepCount    = 0;
    auto* ParamKnee = new Vst::RangeParameter(STR16("Knee"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamKnee->setPrecision(2);
    parameters.addParameter(ParamKnee);

    tag          = kParamMakeup;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minMakeup;
    maxPlain     = maxMakeup;
    defaultPlain = dftMakeup;
    stepCount    = 0;
    auto* ParamMakeup = new Vst::RangeParameter(STR16("Makeup"), tag, STR16("dB"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamKnee->setPrecision(2);
    parameters.addParameter(ParamMakeup);

    tag          = kParamMix;
    flags        = Vst::ParameterInfo::kCanAutomate;
    minPlain     = minMix;
    maxPlain     = maxMix;
    defaultPlain = dftMix;
    stepCount    = 0;
    auto* ParamMix = new Vst::RangeParameter(STR16("Mix"), tag, STR16("%"), minPlain, maxPlain, defaultPlain, stepCount, flags);
    ParamMix->setPrecision(2);
    parameters.addParameter(ParamMix);

    tag          = kParamSoftBypass;
    stepCount    = 1;
    defaultVal   = 0;
    flags        = Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList;
    parameters.addParameter(STR16("SoftBypass"), nullptr, stepCount, defaultVal, flags, tag);

    // GUI only parameter
    if (zoomFactors.empty())
    {
        zoomFactors.push_back(ZoomFactor(STR("50%"),  0.50)); // 0/6
        zoomFactors.push_back(ZoomFactor(STR("75%"),  0.75)); // 1/6
        zoomFactors.push_back(ZoomFactor(STR("100%"), 1.00)); // 2/6
        zoomFactors.push_back(ZoomFactor(STR("125%"), 1.25)); // 3/6
        zoomFactors.push_back(ZoomFactor(STR("150%"), 1.50)); // 4/6
        zoomFactors.push_back(ZoomFactor(STR("175%"), 1.75)); // 5/6
        zoomFactors.push_back(ZoomFactor(STR("200%"), 2.00)); // 6/6
    }

    Vst::StringListParameter* zoomParameter = new Vst::StringListParameter(STR("Zoom"), kParamZoom);
    for (ZoomFactorVector::const_iterator it = zoomFactors.begin(), end = zoomFactors.end(); it != end; ++it)
    {
        zoomParameter->appendString(it->title);
    }
    zoomParameter->setNormalized(zoomParameter->toNormalized(0));
    zoomParameter->addDependent(this);
    uiParameters.addParameter(zoomParameter);

	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompController::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
    getParameterObject(kParamZoom)->removeDependent(this);
	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompController::setComponentState (IBStream* state)
{
	// Here you get the state of the component (Processor part)
	if (!state)
		return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);

    int32           savedBypass     = 0;
    Vst::ParamValue savedZoom       = 0.0;
    Vst::ParamValue savedOS         = 0.0;
    Vst::ParamValue savedInput      = 0.0;
    Vst::ParamValue savedOutput     = 0.0;
    Vst::ParamValue savedRMS_PEAK   = 0.0;
    Vst::ParamValue savedAttack     = 0.0;
    Vst::ParamValue savedRelease    = 0.0;
    Vst::ParamValue savedThreshold  = 0.0;
    Vst::ParamValue savedRatio      = 0.0;
    Vst::ParamValue savedKnee       = 0.0;
    Vst::ParamValue savedMakeup     = 0.0;
    Vst::ParamValue savedMix        = 0.0;
    Vst::ParamValue savedSoftBypass = 0.0;
    
    if (streamer.readInt32 (savedBypass)     == false) return kResultFalse;
    if (streamer.readDouble(savedZoom)       == false) return kResultFalse;
    if (streamer.readDouble(savedOS)         == false) return kResultFalse;
    if (streamer.readDouble(savedInput)      == false) return kResultFalse;
    if (streamer.readDouble(savedOutput)     == false) return kResultFalse;
    if (streamer.readDouble(savedRMS_PEAK)   == false) return kResultFalse;
    if (streamer.readDouble(savedAttack)     == false) return kResultFalse;
    if (streamer.readDouble(savedRelease)    == false) return kResultFalse;
    if (streamer.readDouble(savedThreshold)  == false) return kResultFalse;
    if (streamer.readDouble(savedRatio)      == false) return kResultFalse;
    if (streamer.readDouble(savedKnee)       == false) return kResultFalse;
    if (streamer.readDouble(savedMakeup)     == false) return kResultFalse;
    if (streamer.readDouble(savedMix)        == false) return kResultFalse;
    if (streamer.readDouble(savedSoftBypass) == false) return kResultFalse;

    setParamNormalized(kParamBypass, savedBypass ? 1 : 0);
    setParamNormalized(kParamZoom,   savedZoom);
    setParamNormalized(kParamOS,     savedOS);
    setParamNormalized(kParamInput,  savedInput);
    setParamNormalized(kParamOutput,  savedOutput);
    setParamNormalized(kParamRMS_PEAK,  savedRMS_PEAK);
    setParamNormalized(kParamAttack,  savedAttack);
    setParamNormalized(kParamRelease,  savedRelease);
    setParamNormalized(kParamThreshold,  savedThreshold);
    setParamNormalized(kParamRatio,  savedRatio);
    setParamNormalized(kParamKnee,  savedKnee);
    setParamNormalized(kParamMakeup,  savedMakeup);
    setParamNormalized(kParamMix,  savedMix);
    setParamNormalized(kParamSoftBypass,  savedSoftBypass);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompController::setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompController::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API VLC_CompController::createView (FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
		// create your editor here and return a IPlugView ptr of it
		auto* view = new VSTGUI::VST3Editor (this, "view", "VLCComp_editor.uidesc");
        
        std::vector<double> _zoomFactors;
        _zoomFactors.push_back(0.50);
        _zoomFactors.push_back(0.75);
        _zoomFactors.push_back(1.00);
        _zoomFactors.push_back(1.25);
        _zoomFactors.push_back(1.50);
        _zoomFactors.push_back(1.75);
        _zoomFactors.push_back(2.00);
        view->setAllowedZoomFactors(_zoomFactors);
        view->setZoomFactor(0.5);
        view->setIdleRate(1.0/60.0);

        setKnobMode(Steinberg::Vst::KnobModes::kLinearMode);
        
		return view;
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompController::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompController::getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompController::getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

//------------------------------------------------------------------------
void VLC_CompController::editorAttached(Vst::EditorView* editor)
{
    editors.push_back(editor);
}

//------------------------------------------------------------------------
void VLC_CompController::editorRemoved(Vst::EditorView* editor)
{
    editors.erase(std::find(editors.begin(), editors.end(), editor));
}
//------------------------------------------------------------------------
void PLUGIN_API VLC_CompController::update(FUnknown* changedUnknown, int32 message)
{
    EditControllerEx1::update(changedUnknown, message);

    // GUI Resizing
    // check 'zoomtest' code at
    // https://github.com/steinbergmedia/vstgui/tree/vstgui4_10/vstgui/tests/uidescription%20vst3/source

    Vst::Parameter* param = FCast<Vst::Parameter>(changedUnknown);
    if (!param)
        return;

    if (param->getInfo().id == kParamZoom)
    {
        size_t index = static_cast<size_t> (param->toPlain(param->getNormalized()));

        if (index >= zoomFactors.size())
            return;

        for (EditorVector::const_iterator it = editors.begin(), end = editors.end(); it != end; ++it)
        {
            VSTGUI::VST3Editor* editor = dynamic_cast<VSTGUI::VST3Editor*>(*it);
            if (editor)
                editor->setZoomFactor(zoomFactors[index].factor);
        }
    }
}

//------------------------------------------------------------------------
} // namespace yg331
