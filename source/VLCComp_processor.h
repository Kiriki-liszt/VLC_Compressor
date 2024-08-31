//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"

namespace yg331 {
//------------------------------------------------------------------------
//  VLC_CompProcessor
//------------------------------------------------------------------------
class VLC_CompProcessor : public Steinberg::Vst::AudioEffect
{
public:
	VLC_CompProcessor ();
	~VLC_CompProcessor () SMTG_OVERRIDE;

    // Create function
	static Steinberg::FUnknown* createInstance (void* /*context*/) 
	{ 
		return (Steinberg::Vst::IAudioProcessor*)new VLC_CompProcessor; 
	}

	//--- ---------------------------------------------------------------------
	// AudioEffect overrides:
	//--- ---------------------------------------------------------------------
	/** Called at first after constructor */
	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	
	/** Called at the end before destructor */
	Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	
	/** Switch the Plug-in on/off */
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE;

	/** Will be called before any process call */
	Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
	
	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
		
	/** For persistence */
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
    #define RMS_BUF_SIZE    (960)
    #define LOOKAHEAD_SIZE  ((RMS_BUF_SIZE)<<1)

    #define LIN_INTERP(f,a,b) ((a) + (f) * ( (b) - (a) ))
    #define LIMIT(v,l,u)      (v < l ? l : ( v > u ? u : v ))

    typedef struct
    {
        ParamValue        pf_buf[RMS_BUF_SIZE];
        unsigned int i_pos;
        unsigned int i_count;
        ParamValue        f_sum;

    } rms_env;

    typedef struct
    {
        struct
        {
            ParamValue pf_vals[AOUT_CHAN_MAX];
            ParamValue f_lev_in;

        } p_buf[LOOKAHEAD_SIZE];
        unsigned int i_pos;
        unsigned int i_count;

    } lookahead;
    
    using SampleRate = Steinberg::Vst::SampleRate;
    using ParamValue = Steinberg::Vst::ParamValue;
    using Sample64   = Steinberg::Vst::Sample64;
    using int32      = Steinberg::int32;
    
    template <typename SampleType>
    void processAudio(SampleType** inputs, SampleType** outputs, int32 numChannels, SampleRate getSampleRate, int32 sampleFrames);
    
    // Parameters
    bool       pBypass  = false;
    ParamValue pInput = 0.0;
    ParamValue pOutput = 0.0;
    
    ParamValue pRMS_PEAK = 0.0;
    ParamValue pAttack = 0.0;
    ParamValue pRelease = 1.0;
    ParamValue pThreshold = 1.0;
    ParamValue pRatio = 0.0;
    ParamValue pKnee = 0.0;
    ParamValue pMakeup = 0.0;
    ParamValue pMix = 1.0;
    
    ParamValue pZoom    = 2.0 / 6.0;
    overSample pParamOS = overSample_2x;
    
    // Internal Variables
    ParamValue f_sample_rate = 48000.0;
    ParamValue f_num = 0.0;
    
    ParamValue f_amp;
    unsigned int i_count;
    ParamValue f_env;
    ParamValue f_env_peak;
    ParamValue f_env_rms;
    ParamValue f_gain;
    ParamValue f_gain_out;
    rms_env rms;
    ParamValue f_sum;
    lookahead la;

    
    typedef union
    {
        float f;
        int32_t i;

    } ls_pcast32;

    static void     RoundToZero     ( ParamValue * );
    static ParamValue    Max             ( ParamValue, ParamValue );
    static ParamValue    Clamp           ( ParamValue, ParamValue, ParamValue );
    static int      Round           ( float );
    static ParamValue    RmsEnvProcess   ( rms_env *, const ParamValue );
    static void     BufferProcess   ( ParamValue *, int, ParamValue, ParamValue, lookahead * );

    static int RMSPeakCallback      ( vlc_object_t *, char const *, vlc_value_t, vlc_value_t, void * );
    static int AttackCallback       ( vlc_object_t *, char const *, vlc_value_t, vlc_value_t, void * );
    static int ReleaseCallback      ( vlc_object_t *, char const *, vlc_value_t, vlc_value_t, void * );
    static int ThresholdCallback    ( vlc_object_t *, char const *, vlc_value_t, vlc_value_t, void * );
    static int RatioCallback        ( vlc_object_t *, char const *, vlc_value_t, vlc_value_t, void * );
    static int KneeCallback         ( vlc_object_t *, char const *, vlc_value_t, vlc_value_t, void * );
    static int MakeupGainCallback   ( vlc_object_t *, char const *, vlc_value_t, vlc_value_t, void * );
};

//------------------------------------------------------------------------
} // namespace yg331
