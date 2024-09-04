//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#pragma once

#include "VLCComp_shared.h"
#include "public.sdk/source/vst/vstaudioeffect.h"

#include <cmath>
#define decibelsToGain(f_db)  ((f_db>-80)?(std::pow(10.0, f_db / 20.0)):(0.0))
#define gainToDecibels(f_lin) ((f_lin>0)?(20.0 * log10(f_lin)):(-80.0))

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

        double attackTimeInSeconds = 0.01 * DecayInSeconds;
        alphaAttack = exp(-1.0 / (sampleRate * attackTimeInSeconds));

        double releaseTimeInSeconds = DecayInSeconds;
        alphaRelease = exp(-1.0 / (sampleRate * releaseTimeInSeconds));

        for (auto& s : state)
            s = gainToDecibels(0.0);
    }

    template <typename SampleType>
    void update(SampleType** channelData, int numChannels, int numSamples)
    {
        if (numChannels <= 0) return;
        if (numSamples <= 0) return;
        if (numChannels > state.size()) return;

        for (int ch = 0; ch < numChannels; ch++) {
            for (int i = 0; i < numSamples; i++) {
                if (type == Peak) {
                    double in = gainToDecibels(std::abs(channelData[ch][i]));
                    if (in > state[ch])
                        state[ch] = alphaAttack * state[ch] + (1.0 - alphaAttack) * in;
                    else
                        state[ch] = alphaRelease * state[ch] + (1.0 - alphaRelease) * in;
                }
                else {
                    double pwr = gainToDecibels(std::abs(channelData[ch][i]) * std::abs(channelData[ch][i]));
                    state[ch] = alphaRelease * state[ch] + (1.0 - alphaRelease) * pwr;
                }
                
            }
        }
    }
    
    void processSample(double inputSample, int channel)
    {
        if (channel < 0) return;

        if (type == Peak) {
            double in = gainToDecibels(std::abs(inputSample));
            if (in > state[channel])
                state[channel] = alphaAttack * state[channel] + (1.0 - alphaAttack) * in;
            else
                state[channel] = alphaRelease * state[channel] + (1.0 - alphaRelease) * in;
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
        else return std::sqrt(decibelsToGain(state[channel]));
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
    
#define Plain2Norm(v, min, max) ((v - min) / (max - min))
#define Norm2Plain(v, min, max) (v * (max - min) + min)
    
#define LogPlain2Norm(v, min, max) (std::log(v / min) / std::log(max / min))
#define LogNorm2Plain(v, min, max) (min * std::exp(v * std::log(max / min)))
    
    // Parameters
    bool       pBypass    = false;
    ParamValue pInput     = Plain2Norm(dftInput,  minInput,  maxInput);
    ParamValue pOutput    = Plain2Norm(dftOutput, minOutput, maxOutput);
    
    ParamValue pRMS_PEAK  = Plain2Norm(dftRMS_PEAK,  minRMS_PEAK,  maxRMS_PEAK);
    ParamValue pAttack    = LogPlain2Norm(dftAttack,    minAttack,    maxAttack);
    ParamValue pRelease   = Plain2Norm(dftRelease,   minRelease,   maxRelease);
    ParamValue pThreshold = Plain2Norm(dftThreshold, minThreshold, maxThreshold);
    ParamValue pRatio     = Plain2Norm(dftRatio,     minRatio,     maxRatio);
    ParamValue pKnee      = Plain2Norm(dftKnee,      minKnee,      maxKnee);
    ParamValue pMakeup    = Plain2Norm(dftMakeup,    minMakeup,    maxMakeup);
    ParamValue pMix       = Plain2Norm(dftMix,       minMix,       maxMix);
    ParamValue pSoftBypass = 0.0;
    
    ParamValue pZoom  = 2.0 / 6.0;
    ParamValue pOS = 0.0;
    
    // VU metering ----------------------------------------------------------------
    LevelEnvelopeFollower VuInput, VuOutput;

    static SMTG_CONSTEXPR ParamValue init_meter = 0.0;
    ParamValue Meter = init_meter;
    std::vector<std::vector<ParamValue>> buff;
    std::vector<ParamValue*> buff_head;
    std::vector<ParamValue> fInputVu;
    std::vector<ParamValue> fOutputVu;
    ParamValue fMeterVu = init_meter;
    
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
