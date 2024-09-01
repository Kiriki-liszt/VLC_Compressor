//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#pragma once

#include "VLCComp_shared.h"
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
    
    /** Gets the current Latency in samples. */
    Steinberg::uint32  PLUGIN_API getLatencySamples() SMTG_OVERRIDE;
	
	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
		
	/** For persistence */
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;

//------------------------------------------------------------------------
protected:
    using SampleRate = Steinberg::Vst::SampleRate;
    using ParamValue = Steinberg::Vst::ParamValue;
    using Sample64   = Steinberg::Vst::Sample64;
    using int32      = Steinberg::int32;
    using uint32     = Steinberg::uint32;
    
    template <typename SampleType>
    void processAudio(SampleType** inputs, SampleType** outputs, int32 numChannels, SampleRate getSampleRate, int32 sampleFrames);
    
#define Plain2Norm(v, min, max) ((v - min) / (max - min))
#define Norm2Plain(v, min, max) (v * (max - min) + min)
    // Parameters
    bool       pBypass    = false;
    ParamValue pInput     = Plain2Norm(dftInput,  minInput,  maxInput);
    ParamValue pOutput    = Plain2Norm(dftOutput, minOutput, maxOutput);
    
    ParamValue pRMS_PEAK  = Plain2Norm(dftRMS_PEAK,  minRMS_PEAK,  maxRMS_PEAK);
    ParamValue pAttack    = Plain2Norm(dftAttack,    minAttack,    maxAttack);
    ParamValue pRelease   = Plain2Norm(dftRelease,   minRelease,   maxRelease);
    ParamValue pThreshold = Plain2Norm(dftThreshold, minThreshold, maxThreshold);
    ParamValue pRatio     = Plain2Norm(dftRatio,     minRatio,     maxRatio);
    ParamValue pKnee      = Plain2Norm(dftKnee,      minKnee,      maxKnee);
    ParamValue pMakeup    = Plain2Norm(dftMakeup,    minMakeup,    maxMakeup);
    ParamValue pMix       = Plain2Norm(dftMix,       minMix,       maxMix);
    ParamValue pSoftBypass = 0.0;
    
    ParamValue pZoom  = 2.0 / 6.0;
    ParamValue pOS = 0.0;
    
    // Internal Variables
    SampleRate SR = 48000.0;
    Sample64 f_num = 0.0;
    
    Sample64 f_sum = 0.0;
    Sample64 f_amp = 0.0;
    Sample64 f_gain = 0.0;
    Sample64 f_gain_out = 0.0;
    Sample64 f_env = 0.0;
    Sample64 f_env_rms = 0.0;
    Sample64 f_env_peak = 0.0;
    uint32   i_count = 0;

    rms_env   p_rms;
    lookahead p_la;
    
    typedef union
    {
        float f;
        int32_t i;

    } ls_pcast32;
    
    static double Db2Lin(double f_db);
    static double Lin2Db(double f_lin);
    static void       RoundToZero     ( ParamValue * );
    static ParamValue Max             ( ParamValue, ParamValue );
    static ParamValue Clamp           ( ParamValue, ParamValue, ParamValue );
    static int        Round           ( float );
    static ParamValue RmsEnvProcess   ( rms_env *, const ParamValue );
    static void       BufferProcess   ( ParamValue *, ParamValue *, int, ParamValue, ParamValue, lookahead * );
};

//------------------------------------------------------------------------
} // namespace yg331
