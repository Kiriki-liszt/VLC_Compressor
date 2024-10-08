//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#pragma once

#include "VLCComp_shared.h"
#include "public.sdk/source/vst/vstaudioeffect.h"

#include <cmath>
#define decibelsToGain(f_db)  (std::pow(10.0, f_db / 20.0))
#define gainToDecibels(f_lin) ((f_lin>0)?(20.0 * log10(f_lin)):(-100.0))

namespace yg331 {
class LevelEnvelopeFollower
{
public:
    LevelEnvelopeFollower() = default;

    ~LevelEnvelopeFollower() {
        state.clear();
        state.shrink_to_fit();
    }

    void setChannel(const int channels) {
        state.resize(channels, 0.0);
    }

    enum detectionType {Peak, RMS};
    void setType(detectionType _type)
    {
        type = _type;
    }

    void setDecay(double val)
    {
        DecayInSeconds = val;
    }

    void prepare(const double& fs)
    {
        sampleRate = fs;

        double attackTimeInSeconds = 0.005 * DecayInSeconds;
        alphaAttack = exp(-1.0 / (sampleRate * attackTimeInSeconds));

        double releaseTimeInSeconds = DecayInSeconds;
        alphaRelease = exp(-1.0 / (sampleRate * releaseTimeInSeconds));

        for (auto& s : state)
            s = gainToDecibels(0.0);
    }
    
    void processSample(double inputSample, int channel)
    {
        if (channel < 0) return;

        if (type == Peak) {
            double _in = gainToDecibels(std::abs(inputSample));
            if (_in > state[channel])
                state[channel] = _in; // alphaAttack * state[channel] + (1.0 - alphaAttack) * _in;
            else
                state[channel] = alphaRelease * state[channel] + (1.0 - alphaRelease) * _in;
        }
        else {
            double pwr = gainToDecibels(std::abs(inputSample) * std::abs(inputSample));
            state[channel] = alphaRelease * state[channel] + (1.0 - alphaRelease) * pwr;
        }
    }
    
    double getEnv(int channel) {
        if (channel < 0) return 0.0;
        if (channel >= state.size()) return 0.0;

        if (type == Peak) return decibelsToGain(state[channel]);
        else return std::sqrt( (decibelsToGain(state[channel]) > 0.0) ? decibelsToGain(state[channel]) : 0.0 );
    }

private:
    double sampleRate = 0.0;

    double DecayInSeconds = 0.5;
    double DecayCoef = 0.99992;

    detectionType type = Peak;

    std::vector<double> state;
    double alphaAttack = 0.0;
    double alphaRelease = 0.0;
};
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
    
    // Parameters
    bool       pBypass     = false;
    ParamValue pInput      = nrmInput;
    ParamValue pOutput     = nrmOutput;
    
    ParamValue pRMS_PEAK   = nrmRMS_PEAK;
    ParamValue pAttack     = nrmAttack;
    ParamValue pRelease    = nrmRelease;
    ParamValue pThreshold  = nrmThreshold;
    ParamValue pRatio      = nrmRatio;
    ParamValue pKnee       = nrmKnee;
    ParamValue pMakeup     = nrmMakeup;
    ParamValue pMix        = nrmMix;
    bool       pSoftBypass = false;
    
    ParamValue pZoom       = 2.0 / 6.0;
    ParamValue pOS         = 0.0;
    
    // VU metering ----------------------------------------------------------------
    LevelEnvelopeFollower VuInputRMS, VuOutputRMS;
    LevelEnvelopeFollower VuInputPeak, VuOutputPeak;
    std::vector<ParamValue> fInputVuRMS, fOutputVuRMS;  // for each channel
    std::vector<ParamValue> fInputVuPeak, fOutputVuPeak;
    Sample64 truePeakIn = 0.0, truePeakOut = 0.0;
    Sample64 gainReduction = 0.0;
    
    // Internal Variables
    SampleRate SR = 48000.0;
    Sample64 f_num = 0.0;
    
    Sample64 f_sum = 0.0;
    Sample64 f_amp = 0.0;
    Sample64 f_gain = 1.0;
    Sample64 f_gain_out = 1.0;
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
