//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#pragma once

#include "VLCComp_shared.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"

namespace VSTGUI {
class PDisplay : public CParamDisplay {
public:
    enum updateStyle
    {
        kUpdateMin = 1 << 0,
        kUpdateMax = 1 << 1,
    };
    PDisplay (const CRect& size, CBitmap* background = nullptr, int32_t style = 0)
        : CParamDisplay(size, background, style)
    {setWantsIdle(true); originalBack = getBackColor();};
    PDisplay (const CParamDisplay& paramDisplay)
        : CParamDisplay(paramDisplay)
    {setWantsIdle(true); originalBack = getBackColor();};
    void onIdle() override {
        invalid();
    };
    void setValue(float val) override {
        directValue = val;
        if (_style == kUpdateMax)
            CParamDisplay::setValue(std::max(getValue(), val));
        else
            CParamDisplay::setValue(std::min(getValue(), val));
        
        if (_style == kUpdateMax)
            if (getValue()>0.0)
                if (!over) {
                    originalBack = getBackColor();
                    setBackColor(VSTGUI::kRedCColor);
                    over = true;
                }
    };
    void onMouseDownEvent(MouseDownEvent& event) override {
        if (over) {
            setBackColor(originalBack);
            over = false;
        }
        CParamDisplay::setValue(directValue);
        CParamDisplay::onMouseDownEvent(event);
    };
    void setStyle_(int32_t newStyle) { _style = newStyle; }
    int32_t getStyle_() const { return _style; }
    int32_t     _style;
    float directValue = 0.0;
    bool over = false;
    CColor originalBack;
};
//------------------------------------------------------------------------
//  Metering reset container
//------------------------------------------------------------------------
class MeterViewContainer : public CViewContainer {
public:
    MeterViewContainer(const CRect& size) : CViewContainer(size) {};
    void onMouseDownEvent(MouseDownEvent& event) override {
        for (auto& child : getChildren())
        {
            child->onMouseDownEvent(event);
        }
        CViewContainer::onMouseDownEvent(event);
    };
};
//------------------------------------------------------------------------
//  VU meter view
//------------------------------------------------------------------------
class MyVuMeter : public CControl {
private:
    enum StyleEnum
    {
        StyleHorizontal = 0,
        StyleVertical,
    };
public:
    enum Style
    {
        kHorizontal = 1 << StyleHorizontal,
        kVertical = 1 << StyleVertical,
    };

    MyVuMeter(const CRect& size, int32_t style = kVertical)
        : CControl(size, nullptr, 0)
        , style(style)
    {
        vuOnColor = kWhiteCColor;
        vuOffColor = kBlackCColor;

        rectOn(size.left, size.top, size.right, size.bottom);
        rectOff(size.left, size.top, size.right, size.bottom);

        setWantsIdle(true);
    }
    MyVuMeter(const MyVuMeter& vuMeter)
        : CControl(vuMeter)
        , style(vuMeter.style)
        , vuOnColor(vuMeter.vuOnColor)
        , vuOffColor(vuMeter.vuOffColor)
        , rectOn(vuMeter.rectOn)
        , rectOff(vuMeter.rectOff)
    {
        setWantsIdle(true);
    }

    void setStyle(int32_t newStyle) { style = newStyle; invalid(); }
    int32_t getStyle() const { return style; }

    virtual void setVuOnColor(CColor color) {
        if (vuOnColor != color) { vuOnColor = color; setDirty(true); }
    }
    CColor getVuOnColor() const { return vuOnColor; }

    virtual void setVuOffColor(CColor color) {
        if (vuOffColor != color) { vuOffColor = color; setDirty(true); }
    }
    CColor getVuOffColor() const { return vuOffColor; }

    // overrides
    void setDirty(bool state) override
    {
        CView::setDirty(state);
    };
    void draw(CDrawContext* _pContext) override {

        CRect _rectOn(rectOn);
        CRect _rectOff(rectOff);
        CPoint pointOn;
        CPoint pointOff;
        CDrawContext* pContext = _pContext;

        bounceValue();

        float newValue = getValueNormalized(); // normalize

        if (style & kHorizontal)
        {
            auto tmp = (CCoord)((int32_t)(newValue) * getViewSize().getWidth());
            pointOff(tmp, 0);

            _rectOff.left += tmp;
            _rectOn.right = tmp + rectOn.left;
        }
        else
        {
            auto tmp = (CCoord)((int32_t)(newValue * getViewSize().getHeight()));
            pointOn(0, tmp);

            //_rectOff.bottom = tmp + rectOff.top;
            //_rectOn.top += tmp;
            _rectOn.top = _rectOff.bottom - tmp;
        }

        pContext->setFillColor(vuOffColor);
        pContext->drawRect(rectOff, kDrawFilled);

        pContext->setFillColor(vuOnColor);
        pContext->drawRect(_rectOn, kDrawFilled);

        setDirty(false);
    };
    void setViewSize(const CRect& newSize, bool invalid = true) override
    {
        CControl::setViewSize(newSize, invalid);
        rectOn = getViewSize();
        rectOff = getViewSize();
    };
    bool sizeToFit() override {
        if (getDrawBackground())
        {
            CRect vs(getViewSize());
            vs.setWidth(getDrawBackground()->getWidth());
            vs.setHeight(getDrawBackground()->getHeight());
            setViewSize(vs);
            setMouseableArea(vs);
            return true;
        }
        return false;
    };
    
    void onIdle() override {
        invalid();
    };
    

    CLASS_METHODS(MyVuMeter, CControl)

protected:
    ~MyVuMeter() noexcept override
    {
        //setOnBitmap(nullptr);
        //setOffBitmap(nullptr);
    };

    int32_t     style;

    CColor        vuOnColor;
    CColor        vuOffColor;

    CRect    rectOn;
    CRect    rectOff;
};
}

namespace yg331 {
//------------------------------------------------------------------------
// VuMeterController
//------------------------------------------------------------------------
template <typename ControllerType>
class VuMeterController : public VSTGUI::IController, public VSTGUI::ViewListenerAdapter
{
public:
    VuMeterController(ControllerType* _mainController) :
        mainController(_mainController),
        inMeter(nullptr),
        outMeter(nullptr),
        grMeter(nullptr),
        vuMeterInL(nullptr),
        vuMeterInR(nullptr),
        vuMeterOutL(nullptr),
        vuMeterOutR(nullptr),
        vuMeterGR(nullptr)
    {
    }
    ~VuMeterController() override
    {
        if (inMeter)     viewWillDelete(inMeter);
        if (outMeter)    viewWillDelete(outMeter);
        if (grMeter)     viewWillDelete(grMeter);
        if (vuMeterInL)  viewWillDelete(vuMeterInL);
        if (vuMeterInR)  viewWillDelete(vuMeterInR);
        if (vuMeterOutL) viewWillDelete(vuMeterOutL);
        if (vuMeterOutR) viewWillDelete(vuMeterOutR);
        if (vuMeterGR)   viewWillDelete(vuMeterGR);

        mainController->removeUIVuMeterController(this);
    }

    void updateVuMeterValue();

private:
    using CControl       = VSTGUI::CControl;
    using CView          = VSTGUI::CView;
    using PDisplay       = VSTGUI::PDisplay;
    using MyVuMeter      = VSTGUI::MyVuMeter;
    using UTF8String     = VSTGUI::UTF8String;
    using UIAttributes   = VSTGUI::UIAttributes;
    using IUIDescription = VSTGUI::IUIDescription;

    //--- from IControlListener ----------------------
    void valueChanged(CControl* /*pControl*/) SMTG_OVERRIDE {}
    void controlBeginEdit(CControl* /*pControl*/) SMTG_OVERRIDE {}
    void controlEndEdit(CControl* pControl) SMTG_OVERRIDE {}
    //--- is called when a view is created -----
    CView* verifyView(CView* view,
                      const UIAttributes&   /*attributes*/,
                      const IUIDescription* /*description*/) SMTG_OVERRIDE;
    //--- from IViewListenerAdapter ----------------------
    //--- is called when a view will be deleted: the editor is closed -----
    void viewWillDelete(CView* view) SMTG_OVERRIDE;

    ControllerType* mainController;
    PDisplay* inMeter;
    PDisplay* outMeter;
    PDisplay* grMeter;
    MyVuMeter* vuMeterInL;
    MyVuMeter* vuMeterInR;
    MyVuMeter* vuMeterOutL;
    MyVuMeter* vuMeterOutR;
    MyVuMeter* vuMeterGR;
};

//------------------------------------------------------------------------
//  VLC_CompController
//------------------------------------------------------------------------
class VLC_CompController
    : public Steinberg::Vst::EditControllerEx1
    , public VSTGUI::VST3EditorDelegate
{
public:
//------------------------------------------------------------------------
	VLC_CompController () = default;
	~VLC_CompController () SMTG_OVERRIDE = default;
    
    using UIVuMeterController = VuMeterController<VLC_CompController>;

    // Create function
	static Steinberg::FUnknown* createInstance (void* /*context*/)
	{
		return (Steinberg::Vst::IEditController*)new VLC_CompController;
	}

	// IPluginBase
	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	// EditController
	Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setParamNormalized (Steinberg::Vst::ParamID tag,
                                                      Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getParamStringByValue (Steinberg::Vst::ParamID tag,
                                                         Steinberg::Vst::ParamValue valueNormalized,
                                                         Steinberg::Vst::String128 string) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getParamValueByString (Steinberg::Vst::ParamID tag,
                                                         Steinberg::Vst::TChar* string,
                                                         Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

    //---from VST3EditorDelegate-----------
    VSTGUI::IController* createSubController (VSTGUI::UTF8StringPtr name,
                                              const VSTGUI::IUIDescription* description,
                                              VSTGUI::VST3Editor* editor) SMTG_OVERRIDE;
    
    //---from ComponentBase-----
    // EditController
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;
    //Steinberg::tresult PLUGIN_API receiveText(const char* text) SMTG_OVERRIDE;
    void PLUGIN_API update(Steinberg::FUnknown* changedUnknown, Steinberg::int32 message) SMTG_OVERRIDE;
    void editorAttached(Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE; ///< called from EditorView if it was attached to a parent
    void editorRemoved (Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE; ///< called from EditorView if it was removed from a parent

    //------------------------------------------------------------------------
    Steinberg::Vst::Parameter* getParameterObject(Steinberg::Vst::ParamID tag) SMTG_OVERRIDE
    {
        Steinberg::Vst::Parameter* param = EditController::getParameterObject(tag);
        if (param == 0)
        {
            param = uiParameters.getParameter(tag);
        }
        return param;
    }
    bool isPrivateParameter(const Steinberg::Vst::ParamID paramID) SMTG_OVERRIDE
    {
        return uiParameters.getParameter(paramID) != 0 ? true : false;
    }

    // make sure that our UI only parameters doesn't call the following three EditController methods: beginEdit, endEdit, performEdit
    //------------------------------------------------------------------------
    Steinberg::tresult beginEdit(Steinberg::Vst::ParamID tag) SMTG_OVERRIDE
    {
        if (EditController::getParameterObject(tag))
            return EditController::beginEdit(tag);
        return Steinberg::kResultFalse;
    }

    //------------------------------------------------------------------------
    Steinberg::tresult performEdit(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized) SMTG_OVERRIDE
    {
        if (EditController::getParameterObject(tag))
            return EditController::performEdit(tag, valueNormalized);
        return Steinberg::kResultFalse;
    }

    //------------------------------------------------------------------------
    Steinberg::tresult endEdit(Steinberg::Vst::ParamID tag) SMTG_OVERRIDE
    {
        if (EditController::getParameterObject(tag))
            return EditController::endEdit(tag);
        return Steinberg::kResultFalse;
    }
    
    //---Internal functions-------
    void addUIVuMeterController(UIVuMeterController* controller)
    {
        vuMeterControllers.push_back(controller);
    };
    void removeUIVuMeterController(UIVuMeterController* controller)
    {
        auto it = std::find(vuMeterControllers.begin(), vuMeterControllers.end(), controller);
        if (it != vuMeterControllers.end())
            vuMeterControllers.erase(it);
    };
    Steinberg::Vst::ParamValue getVuMeterByTag(Steinberg::Vst::ParamID tag)
   {
       switch (tag) {
           case kIn:     return tpIn;    break;
           case kInLRMS:    return vuInLRMS;    break;
           case kInRRMS:    return vuInRRMS;    break;
           case kInLPeak:    return vuInLPeak;    break;
           case kInRPeak:    return vuInRPeak;    break;
           case kOut:    return tpOut;   break;
           case kOutLRMS:   return vuOutLRMS;   break;
           case kOutRRMS:   return vuOutRRMS;   break;
           case kOutLPeak:   return vuOutLPeak;   break;
           case kOutRPeak:   return vuOutRPeak;   break;
           case kGR:     return vuGR;     break;
           default: break;
       }
       return 0;
   }

 	//---Interface---------
	DEFINE_INTERFACES
		// Here you can add more supported VST3 interfaces
		// DEF_INTERFACE (Vst::IXXX)
	END_DEFINE_INTERFACES (EditController)
    DELEGATE_REFCOUNT (EditController)

//------------------------------------------------------------------------
protected:
    
    // UI only parameter list
    Steinberg::Vst::ParameterContainer uiParameters;

    // editor list
    typedef std::vector<Steinberg::Vst::EditorView*> EditorVector;
    EditorVector editors;
    
    // zoom title-value struct
    struct ZoomFactor
    {
        const Steinberg::tchar* title;
        double factor;

        ZoomFactor(const Steinberg::tchar* title, double factor) : title(title), factor(factor) {}
    };
    typedef std::vector<ZoomFactor> ZoomFactorVector;
    ZoomFactorVector zoomFactors;
    
    // sub-controller list
    using UIVuMeterControllerList = std::vector<UIVuMeterController*>;
    UIVuMeterControllerList vuMeterControllers;
    
    Steinberg::Vst::ParamValue vuInLRMS = 0.0, vuInRRMS = 0.0;
    Steinberg::Vst::ParamValue vuInLPeak = 0.0, vuInRPeak = 0.0;
    Steinberg::Vst::ParamValue vuOutLRMS = 0.0, vuOutRRMS = 0.0;
    Steinberg::Vst::ParamValue vuOutLPeak = 0.0, vuOutRPeak = 0.0;
    Steinberg::Vst::ParamValue vuGR = 0.0;
    Steinberg::Vst::ParamValue tpIn = 0.0, tpOut = 0.0;
};

//------------------------------------------------------------------------
} // namespace yg331
