//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#include "VLCComp_controller.h"
#include "VLCComp_cids.h"

#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstreamer.h"

#include "vstgui/vstgui_uidescription.h"
#include "vstgui/uidescription/detail/uiviewcreatorattributes.h"

using namespace Steinberg;

static const std::string kAttrVuOnColor = "vu-on-color";
static const std::string kAttrVuOffColor = "vu-off-color";
namespace VSTGUI {
class MyVUMeterFactory : public ViewCreatorAdapter
{
public:
    //register this class with the view factory
    MyVUMeterFactory() { UIViewFactory::registerViewCreator(*this); }

    //return an unique name here
    IdStringPtr getViewName() const override { return "My Vu Meter"; }

    //return the name here from where your custom view inherites.
    //    Your view automatically supports the attributes from it.
    IdStringPtr getBaseViewName() const override { return UIViewCreator::kCControl; }

    //create your view here.
    //    Note you don't need to apply attributes here as
    //    the apply method will be called with this new view
    CView* create(const UIAttributes & attributes, const IUIDescription * description) const override
    {
        CRect size(CPoint(45, 45), CPoint(400, 150));
        return new MyVuMeter(size, 2);
    }
    bool apply(
        CView* view,
        const UIAttributes& attributes,
        const IUIDescription* description) const
    {
        auto* vuMeter = dynamic_cast<MyVuMeter*> (view);

        if (!vuMeter)
            return false;

        const auto* attr = attributes.getAttributeValue(UIViewCreator::kAttrOrientation);
        if (attr)
            vuMeter->setStyle(*attr == UIViewCreator::strVertical ? MyVuMeter::kVertical : MyVuMeter::kHorizontal);

        CColor color;
        if (UIViewCreator::stringToColor(attributes.getAttributeValue(kAttrVuOnColor), color, description))
            vuMeter->setVuOnColor(color);
        if (UIViewCreator::stringToColor(attributes.getAttributeValue(kAttrVuOffColor), color, description))
            vuMeter->setVuOffColor(color);

        return true;
    }

    bool getAttributeNames(StringList& attributeNames) const
    {
        attributeNames.emplace_back(UIViewCreator::kAttrOrientation);
        attributeNames.emplace_back(kAttrVuOnColor);
        attributeNames.emplace_back(kAttrVuOffColor);
        return true;
    }

    AttrType getAttributeType(const std::string& attributeName) const
    {
        if (attributeName == UIViewCreator::kAttrOrientation)
            return kListType;
        if (attributeName == kAttrVuOnColor)
            return kColorType;
        if (attributeName == kAttrVuOffColor)
            return kColorType;
        return kUnknownType;
    }

    //------------------------------------------------------------------------
    bool getAttributeValue(
        CView* view,
        const string& attributeName,
        string& stringValue,
        const IUIDescription* desc) const
    {
        auto* vuMeter = dynamic_cast<MyVuMeter*> (view);

        if (!vuMeter)
            return false;

        if (attributeName == UIViewCreator::kAttrOrientation)
        {
            if (vuMeter->getStyle() & MyVuMeter::kVertical)
                stringValue = UIViewCreator::strVertical;
            else
                stringValue = UIViewCreator::strHorizontal;
            return true;
        }
        else if (attributeName == kAttrVuOnColor)
        {
            UIViewCreator::colorToString(vuMeter->getVuOnColor(), stringValue, desc);
            return true;
        }
        else if (attributeName == kAttrVuOffColor)
        {
            UIViewCreator::colorToString(vuMeter->getVuOffColor(), stringValue, desc);
            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------
    bool getPossibleListValues(
        const string& attributeName,
        ConstStringPtrList& values) const
    {
        if (attributeName == UIViewCreator::kAttrOrientation)
        {
            return UIViewCreator::getStandardAttributeListValues(UIViewCreator::kAttrOrientation, values);
        }
        return false;
    }

};

//create a static instance so that it registers itself with the view factory
MyVUMeterFactory __gMyVUMeterFactory;
} // namespace VSTGUI

namespace yg331 {
//------------------------------------------------------------------------
// VuMeterController
//------------------------------------------------------------------------
template<> void VLC_CompController::UIVuMeterController::updateVuMeterValue()
{
    if (mainController) {
        if (inMeter)          inMeter->      setValue(mainController->getVuMeterByTag(kIn));
        if (outMeter)         outMeter->     setValue(mainController->getVuMeterByTag(kOut));
        if (grMeter)          grMeter->      setValue(mainController->getVuMeterByTag(kGR));
        if (vuMeterInL)       vuMeterInL->   setValue(mainController->getVuMeterByTag(kInL));
        if (vuMeterInR)       vuMeterInR->   setValue(mainController->getVuMeterByTag(kInR));
        if (vuMeterOutL)      vuMeterOutL->  setValue(mainController->getVuMeterByTag(kOutL));
        if (vuMeterOutR)      vuMeterOutR->  setValue(mainController->getVuMeterByTag(kOutR));
        if (vuMeterGR)        vuMeterGR->    setValue(mainController->getVuMeterByTag(kGR));
        
        if (inMeter) inMeter->invalid();
        if (outMeter) outMeter->invalid();
        if (grMeter) grMeter->invalid();
    }
}

template<> VSTGUI::CView* VLC_CompController::UIVuMeterController::verifyView(
                                            VSTGUI::CView* view,
                                            const VSTGUI::UIAttributes&   /*attributes*/,
                                            const VSTGUI::IUIDescription* /*description*/
)
{
    if (CParamDisplay* control = dynamic_cast<CParamDisplay*>(view); control)
    {
        if (control->getTag() == kIn) {
            inMeter = control;
            inMeter->registerViewListener(this);
            inMeter->CParamDisplay::setValue(inMeter->getDefaultValue());
        }
        if (control->getTag() == kOut) {
            outMeter = control;
            outMeter->registerViewListener(this);
            outMeter->CParamDisplay::setValue(outMeter->getDefaultValue());
        }
        if (control->getTag() == kGR) {
            grMeter = control;
            grMeter->registerViewListener(this);
            grMeter->CParamDisplay::setValue(grMeter->getDefaultValue());
        }
    }

    if (MyVuMeter* control = dynamic_cast<MyVuMeter*>(view); control) {
        if (control->getTag() == kInL) {
            vuMeterInL = control;
            vuMeterInL->registerViewListener(this);
            ///vuMeterInL->setValue(0.0);
        }
        if (control->getTag() == kInR) {
            vuMeterInR = control;
            vuMeterInR->registerViewListener(this);
            //vuMeterInR->setValue(0.0);
        }
        if (control->getTag() == kOutL) {
            vuMeterOutL = control;
            vuMeterOutL->registerViewListener(this);
            //vuMeterOutL->setValue(0.0);
        }
        if (control->getTag() == kOutR) {
            vuMeterOutR = control;
            vuMeterOutR->registerViewListener(this);
            //vuMeterOutR->setValue(0.0);
        }
        if (control->getTag() == kGR) {
            vuMeterGR = control;
            vuMeterGR->registerViewListener(this);
            //vuMeterGR->setValue(0.0);
        }
    }

    return view;
}

template<> void VLC_CompController::UIVuMeterController::viewWillDelete(VSTGUI::CView* view)
{
    if (dynamic_cast<CParamDisplay*> (view) == inMeter && inMeter)
    {
        inMeter->unregisterViewListener(this);
        inMeter = nullptr;
    }
    if (dynamic_cast<CParamDisplay*> (view) == outMeter && outMeter)
    {
        outMeter->unregisterViewListener(this);
        outMeter = nullptr;
    }
    if (dynamic_cast<CParamDisplay*> (view) == grMeter && grMeter)
    {
        grMeter->unregisterViewListener(this);
        grMeter = nullptr;
    }
    
    if (dynamic_cast<MyVuMeter*>(view) == vuMeterInL && vuMeterInL)
    {
        vuMeterInL->unregisterViewListener(this);
        vuMeterInL = nullptr;
    }
    if (dynamic_cast<MyVuMeter*>(view) == vuMeterInR && vuMeterInR)
    {
        vuMeterInR->unregisterViewListener(this);
        vuMeterInR = nullptr;
    }
    if (dynamic_cast<MyVuMeter*>(view) == vuMeterOutL && vuMeterOutL)
    {
        vuMeterOutL->unregisterViewListener(this);
        vuMeterOutL = nullptr;
    }
    if (dynamic_cast<MyVuMeter*>(view) == vuMeterOutR && vuMeterOutR)
    {
        vuMeterOutR->unregisterViewListener(this);
        vuMeterOutR = nullptr;
    }
    if (dynamic_cast<MyVuMeter*>(view) == vuMeterGR && vuMeterGR)
    {
        vuMeterGR->unregisterViewListener(this);
        vuMeterGR = nullptr;
    }
}
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

VSTGUI::IController* VLC_CompController::createSubController(
    VSTGUI::UTF8StringPtr name,
    const VSTGUI::IUIDescription* description,
    VSTGUI::VST3Editor* editor)
{
    if (VSTGUI::UTF8StringView(name) == "VuMeterController")
    {
        auto* controller = new UIVuMeterController(this);
        addUIVuMeterController(controller);
        return controller;
    }
    return nullptr;
};


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
tresult PLUGIN_API VLC_CompController::notify(Vst::IMessage* message)
{
    if (!message)
        return kInvalidArgument;
    
    if (strcmp (message->getMessageID (), "VUmeter") == 0)
    {
        double data = 0.0;
        int64 update = 0.0;
        if (message->getAttributes ()->getFloat ("vuInL",    data) == kResultTrue) vuInL    = data;
        if (message->getAttributes ()->getFloat ("vuInR",    data) == kResultTrue) vuInR    = data;
        if (message->getAttributes ()->getFloat ("vuOutL",   data) == kResultTrue) vuOutL   = data;
        if (message->getAttributes ()->getFloat ("vuOutR",   data) == kResultTrue) vuOutR   = data;
        if (message->getAttributes ()->getFloat ("vuGR",     data) == kResultTrue) vuGR     = data;
        if (message->getAttributes ()->getInt   ("update", update) == kResultTrue) {
            if (!vuMeterControllers.empty()) {
                for (auto iter = vuMeterControllers.begin(); iter != vuMeterControllers.end(); iter++) {
                    (*iter)->updateVuMeterValue();
                }
            }
        }
        FDebugPrint("vuGR = %f\n",vuGR);
        return kResultOk;
    }

    return EditControllerEx1::notify(message);
}

//------------------------------------------------------------------------
} // namespace yg331
