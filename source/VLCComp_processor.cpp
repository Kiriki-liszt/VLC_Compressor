//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#include "VLCComp_processor.h"
#include "VLCComp_cids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/vsthelpers.h"

#include <cstdio>
#include <cmath>

using namespace Steinberg;

namespace yg331 {
//------------------------------------------------------------------------
// VLC_CompProcessor
//------------------------------------------------------------------------
VLC_CompProcessor::VLC_CompProcessor ()
{
	//--- set the wanted controller for our processor
	setControllerClass (kVLC_CompControllerUID);
}

//------------------------------------------------------------------------
VLC_CompProcessor::~VLC_CompProcessor ()
{}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated
	
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//--- create Audio IO ------
	addAudioInput  (STR16 ("Stereo In"),  Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	/* If you don't need an event bus, you can remove the next line */
	// addEventInput (STR16 ("Event In"), 1);
    
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
#define clear_delete(vec) {vec.clear(); vec.shrink_to_fit();}

    clear_delete(fInputVuRMS);
    clear_delete(fOutputVuRMS);
    clear_delete(fInputVuPeak);
    clear_delete(fOutputVuPeak);
    
	//---do not forget to call parent ------
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::setActive (TBool state)
{
	//--- called when the Plug-in is enable/disable (On/Off) -----
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::process (Vst::ProcessData& data)
{
    Vst::IParameterChanges* paramChanges = data.inputParameterChanges;

    if (paramChanges)
    {
        int32 numParamsChanged = paramChanges->getParameterCount();

        for (int32 index = 0; index < numParamsChanged; index++)
        {
            Vst::IParamValueQueue* paramQueue = paramChanges->getParameterData(index);

            if (paramQueue)
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();

                if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                    switch (paramQueue->getParameterId()) {
                        case kParamBypass:     pBypass     = (value > 0.5); break;
                        case kParamZoom:       pZoom       = value; break;
                        case kParamOS:         pOS         = value; break;
                        case kParamInput:      pInput      = value; break;
                        case kParamOutput:     pOutput     = value; break;
                        case kParamRMS_PEAK:   pRMS_PEAK   = value; break;
                        case kParamAttack:     pAttack     = value; break;
                        case kParamRelease:    pRelease    = value; break;
                        case kParamThreshold:  pThreshold  = value; break;
                        case kParamRatio:      pRatio      = value; break;
                        case kParamKnee:       pKnee       = value; break;
                        case kParamMakeup:     pMakeup     = value; break;
                        case kParamMix:        pMix        = value; break;
                        case kParamSoftBypass: pSoftBypass = (value > 0.5); break;
                        default: break;
                    }
                }
            }
        }
    }

    if (data.numInputs == 0 || data.numOutputs == 0)
    {
        // nothing to do
        return kResultOk;
    }

    // (simplification) we suppose in this example that we have the same input channel count than the output
    int32 numChannels = data.inputs[0].numChannels;

    //---get audio buffers----------------
    uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);
    void** in  = getChannelBuffersPointer(processSetup, data.inputs[0]);
    void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
    Vst::SampleRate SampleRate = processSetup.sampleRate;
    
    // Reset values, linear
    gainReduction = 1.0;
    truePeakIn  = 0.0;
    truePeakOut = 0.0;
    for (auto& loop : fInputVuRMS) loop = 0.0;
    for (auto& loop : fOutputVuRMS) loop = 0.0;
    for (auto& loop : fInputVuPeak) loop = 0.0;
    for (auto& loop : fOutputVuPeak) loop = 0.0;

    //---check if silence---------------
    // check if all channel are silent then process silent
    if (data.inputs[0].silenceFlags == Vst::getChannelMask(data.inputs[0].numChannels))
    {
        // mark output silence too (it will help the host to propagate the silence)
        data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;

        // the plug-in has to be sure that if it sets the flags silence that the output buffer are
        // clear
        for (int32 i = 0; i < numChannels; i++)
        {
            // do not need to be cleared if the buffers are the same (in this case input buffer are
            // already cleared by the host)
            if (in[i] != out[i])
            {
                memset(out[i], 0, sampleFramesSize);
            }
        }
    }
    else {

        data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
        //---in bypass mode outputs should be like inputs-----
        if (pBypass)
        {
            for (int32 channel = 0; channel < numChannels; channel++)
            {
                memcpy (out[channel], in[channel], sampleFramesSize);
            }
        }
        else
        {
            if (data.symbolicSampleSize == Vst::kSample32) {
                processAudio<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, numChannels, SampleRate, data.numSamples);
            }
            else {
                processAudio<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, numChannels, SampleRate, data.numSamples);
            }
        }
        
        // Linear to dB
        for (auto& loop : fInputVuRMS) loop = Lin2Db(loop);
        for (auto& loop : fOutputVuRMS) loop = Lin2Db(loop);
        for (auto& loop : fInputVuPeak) loop = Lin2Db(loop);
        for (auto& loop : fOutputVuPeak) loop = Lin2Db(loop);
        truePeakIn = Lin2Db(truePeakIn);
        truePeakOut = Lin2Db(truePeakOut);
        gainReduction = Lin2Db(gainReduction);
    }
    
    //---send a message
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = (numChannels > 0) ? fInputVuRMS[0] : 0.0;
        message->getAttributes()->setFloat ("vuInLRMS", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = (numChannels > 1) ? fInputVuRMS[1] : ((numChannels > 0) ? fInputVuRMS[0] : 0.0);
        message->getAttributes ()->setFloat ("vuInRRMS", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = (numChannels > 0) ? fInputVuPeak[0] : 0.0;
        message->getAttributes()->setFloat ("vuInLPeak", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = (numChannels > 1) ? fInputVuPeak[1] : ((numChannels > 0) ? fInputVuPeak[0] : 0.0);
        message->getAttributes ()->setFloat ("vuInRPeak", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = truePeakIn;
        message->getAttributes ()->setFloat ("tpIn", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = (numChannels > 0) ? fOutputVuRMS[0] : 0.0;
        message->getAttributes ()->setFloat ("vuOutLRMS", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = (numChannels > 1) ? fOutputVuRMS[1] : ((numChannels > 0) ? fOutputVuRMS[0] : 0.0);
        message->getAttributes ()->setFloat ("vuOutRRMS", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = (numChannels > 0) ? fOutputVuPeak[0] : 0.0;
        message->getAttributes ()->setFloat ("vuOutLPeak", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = (numChannels > 1) ? fOutputVuPeak[1] : ((numChannels > 0) ? fOutputVuPeak[0] : 0.0);
        message->getAttributes ()->setFloat ("vuOutRPeak", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = truePeakOut;
        message->getAttributes ()->setFloat ("tpOut", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        double data = gainReduction;
        message->getAttributes ()->setFloat ("vuGR", data);
        sendMessage (message);
    }
    if (IPtr<Vst::IMessage> message = owned (allocateMessage ()))
    {
        message->setMessageID ("VUmeter");
        message->getAttributes ()->setInt ("update", true);
        sendMessage (message);
    }
    return kResultOk;
}

uint32 PLUGIN_API VLC_CompProcessor::getLatencySamples()
{
    return Round( Clamp( SR * 0.01, 1.0, LOOKAHEAD_SIZE ) ); //10.0ms
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::setupProcessing (Vst::ProcessSetup& newSetup)
{
    /* Calculate the RMS and lookahead sizes from the sample rate */
    SR = newSetup.sampleRate;
    f_num = 0.01 * newSetup.sampleRate;
    p_rms.i_count = Round( Clamp( 0.5 * f_num, 1.0, RMS_BUF_SIZE ) );
    p_la.i_count  = Round( Clamp( f_num, 1.0, LOOKAHEAD_SIZE ) );
    
    Vst::SpeakerArrangement arr;
    getBusArrangement(Vst::BusDirections::kInput, 0, arr);
    uint16_t numChannels = static_cast<uint16_t> (Vst::SpeakerArr::getChannelCount(arr));

    VuInputRMS.setChannel(numChannels);
    VuInputRMS.setType(LevelEnvelopeFollower::RMS);
    VuInputRMS.setDecay(0.3);
    VuInputRMS.prepare(newSetup.sampleRate);

    VuOutputRMS.setChannel(numChannels);
    VuOutputRMS.setType(LevelEnvelopeFollower::RMS);
    VuOutputRMS.setDecay(0.3);
    VuOutputRMS.prepare(newSetup.sampleRate);
    
    VuInputPeak.setChannel(numChannels);
    VuInputPeak.setType(LevelEnvelopeFollower::Peak);
    VuInputPeak.setDecay(1.0);
    VuInputPeak.prepare(newSetup.sampleRate);

    VuOutputPeak.setChannel(numChannels);
    VuOutputPeak.setType(LevelEnvelopeFollower::Peak);
    VuOutputPeak.setDecay(1.0);
    VuOutputPeak.prepare(newSetup.sampleRate);

    fInputVuRMS.resize(numChannels, 0.0);
    fOutputVuRMS.resize(numChannels, 0.0);
    fInputVuPeak.resize(numChannels, 0.0);
    fOutputVuPeak.resize(numChannels, 0.0);

	//--- called before any processing ----
	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	// disable the following comment if your processing support kSample64
	if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue;

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::setState (IBStream* state)
{
	// called when we load a preset, the model has to be reloaded
	IBStreamer streamer (state, kLittleEndian);

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
    int32           savedSoftBypass = 0.0;
    
    if (streamer.readInt32 (savedBypass)     == false) savedBypass     = 0;
    if (streamer.readDouble(savedZoom)       == false) savedZoom       = 2.0 / 6.0;
    if (streamer.readDouble(savedOS)         == false) savedOS         = 0.0;
    if (streamer.readDouble(savedInput)      == false) savedInput      = nrmInput;
    if (streamer.readDouble(savedOutput)     == false) savedOutput     = nrmOutput;
    if (streamer.readDouble(savedRMS_PEAK)   == false) savedRMS_PEAK   = nrmRMS_PEAK;
    if (streamer.readDouble(savedAttack)     == false) savedAttack     = nrmAttack;
    if (streamer.readDouble(savedRelease)    == false) savedRelease    = nrmRelease;
    if (streamer.readDouble(savedThreshold)  == false) savedThreshold  = nrmThreshold;
    if (streamer.readDouble(savedRatio)      == false) savedRatio      = nrmRatio;
    if (streamer.readDouble(savedKnee)       == false) savedKnee       = nrmKnee;
    if (streamer.readDouble(savedMakeup)     == false) savedMakeup     = nrmMakeup;
    if (streamer.readDouble(savedMix)        == false) savedMix        = nrmMix;
    if (streamer.readInt32 (savedSoftBypass) == false) savedSoftBypass = 0;
    
    pBypass     = savedBypass > 0;
    pZoom       = savedZoom;
    pOS         = static_cast<overSample>(Steinberg::FromNormalized<ParamValue> (savedOS, overSample_num));
    pInput      = savedInput;
    pOutput     = savedOutput;
    pRMS_PEAK   = savedRMS_PEAK;
    pAttack     = savedAttack;
    pRelease    = savedRelease;
    pThreshold  = savedThreshold;
    pRatio      = savedRatio;
    pKnee       = savedKnee;
    pMakeup     = savedMakeup;
    pMix        = savedMix;
    pSoftBypass = savedSoftBypass > 0;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::getState (IBStream* state)
{
	// here we need to save the model
	IBStreamer streamer (state, kLittleEndian);
    
    streamer.writeInt32(pBypass ? 1 : 0);
    streamer.writeDouble(pZoom);
    streamer.writeDouble(Steinberg::ToNormalized<ParamValue> (static_cast<ParamValue>(pOS), overSample_num));
    streamer.writeDouble(pInput);
    streamer.writeDouble(pOutput);
    streamer.writeDouble(pRMS_PEAK);
    streamer.writeDouble(pAttack);
    streamer.writeDouble(pRelease);
    streamer.writeDouble(pThreshold);
    streamer.writeDouble(pRatio);
    streamer.writeDouble(pKnee);
    streamer.writeDouble(pMakeup);
    streamer.writeDouble(pMix);
    streamer.writeInt32(pSoftBypass ? 1 : 0);
    
	return kResultOk;
}


template <typename SampleType>
void VLC_CompProcessor::processAudio(
    SampleType** inputs,
    SampleType** outputs,
    int32 numChannels,
    Vst::SampleRate SampleRate,
    int32 sampleFrames
)
{
    // Make variable from Parameter
    Vst::Sample64 inputGain  = Db2Lin(Norm2Plain(pInput,  minInput,  maxInput));
    Vst::Sample64 outputGain = Db2Lin(Norm2Plain(pOutput, minOutput, maxOutput));
    
    int i_samples = sampleFrames;
    int i_channels = numChannels;

    Vst::Sample64 f_rms_peak    = pRMS_PEAK;     /* RMS/peak */
    Vst::Sample64 f_attack      = LogNorm2Plain(pAttack,    minAttack,    maxAttack);    /* Attack time (ms)     */
    Vst::Sample64 f_release     = Norm2Plain(pRelease,   minRelease,   maxRelease);   /* Release time (ms)    */
    Vst::Sample64 f_threshold   = Norm2Plain(pThreshold, minThreshold, maxThreshold); /* Threshold level (dB) */
    Vst::Sample64 f_ratio       = Norm2Plain(pRatio,     minRatio,     maxRatio);     /* Ratio (n:1)          */
    Vst::Sample64 f_knee        = Norm2Plain(pKnee,      minKnee,      maxKnee);      /* Knee radius (dB)     */
    Vst::Sample64 f_makeup_gain = Norm2Plain(pMakeup,    minMakeup,    maxMakeup);    /* Makeup gain (dB)     */

    /* Prepare other compressor parameters */
    Vst::Sample64 f_ga       = f_attack < 2.0 ? 0.0 : exp(-1.0 / (SampleRate * f_attack * 0.001));
    Vst::Sample64 f_gr       = exp(-1.0 / (SampleRate * f_release * 0.001));
    Vst::Sample64 f_rs       = ( f_ratio - 1.0 ) / f_ratio;
    Vst::Sample64 f_mug      = Db2Lin( f_makeup_gain );
    Vst::Sample64 f_knee_min = Db2Lin( f_threshold - f_knee );
    Vst::Sample64 f_knee_max = Db2Lin( f_threshold + f_knee );
    Vst::Sample64 f_ef_a     = f_ga * 0.25;
    //Vst::Sample64 f_ef_ai    = 1.0 - f_ef_a;
    
    /* Process the current buffer */
    for( int i = 0; i < i_samples; i++ )
    {
        Vst::Sample64 f_lev_in_old;
        Vst::Sample64 f_lev_in_new;

        /* Now, compress the pre-equalized audio (ported from sc4_1882 plugin with a few modifications) */

        /* Fetch the old delayed buffer value */
        f_lev_in_old = p_la.p_buf[p_la.i_pos].f_lev_in;

        /* Find the peak value of current sample.  
         * This becomes the new delayed buffer value that replaces the old one in the lookahead array */
        f_lev_in_new = std::abs( (Vst::Sample64) inputs[0][i] * inputGain);
        for( int i_chan = 0; i_chan < i_channels; i_chan++ )
        {
            f_lev_in_new = Max( f_lev_in_new, std::abs( (Vst::Sample64) inputs[i_chan][i] * inputGain) );
        }
        p_la.p_buf[p_la.i_pos].f_lev_in = f_lev_in_new;

        /* Add the square of the peak value to a running sum */
        f_sum += f_lev_in_new * f_lev_in_new;

        /* Update the RMS envelope */
        if( f_amp > f_env_rms )
        {
            f_env_rms = f_env_rms * f_ga + f_amp * ( 1.0 - f_ga );
        }
        else
        {
            f_env_rms = f_env_rms * f_gr + f_amp * ( 1.0 - f_gr );
        }
        RoundToZero( &f_env_rms );

        /* Update the peak envelope */
        if( f_lev_in_old > f_env_peak )
        {
            f_env_peak = f_env_peak * f_ga + f_lev_in_old * ( 1.0 - f_ga );
        }
        else
        {
            f_env_peak = f_env_peak * f_gr + f_lev_in_old * ( 1.0 - f_gr );
        }
        RoundToZero( &f_env_peak );

        /* Process the RMS value and update the output gain every 4 samples */
        if( ( i_count++ & 3 ) == 3 )
        {
            /* Process the RMS value by placing in the mean square value, and reset the running sum */
            f_amp = RmsEnvProcess( &p_rms, f_sum * 0.25 );
            f_sum = 0.0;
            if( isnan( f_env_rms ) )
            {
                /* This can happen sometimes, but I don't know why. */
                f_env_rms = 0.0;
            }

            /* Find the superposition of the RMS and peak envelopes */
            f_env = LIN_INTERP( pRMS_PEAK, f_env_rms, f_env_peak );

            /* Update the output gain */
            if( f_env <= f_knee_min )
            {
                /* Gain below the knee (and below the threshold) */
                f_gain_out = 1.0;
            }
            else if( f_env < f_knee_max )
            {
                /* Gain within the knee */
                const Vst::Sample64 f_x = -( f_threshold - f_knee - Lin2Db( f_env ) ) / f_knee;
                f_gain_out = Db2Lin( -f_knee * f_rs * f_x * f_x * 0.25 );
            }
            else
            {
                /* Gain above the knee (and above the threshold) */
                f_gain_out = Db2Lin( ( f_threshold - Lin2Db( f_env ) ) * f_rs );
            }
        }

        /* Find the total gain */
        f_gain = f_gain * f_ef_a + f_gain_out * (1.0 - f_ef_a); //inertia to the gain change, with quater of attack

        /* Write the resulting buffer to the output */
        //BufferProcess( inputs, outputs, i_channels, f_gain, f_mug, p_la );
        for( int i_chan = 0; i_chan < i_channels; i_chan++ )
        {
            /* Current buffer value */
            Vst::ParamValue f_x = inputs[i_chan][i];

            /* Output the compressed delayed buffer value */
            outputs[i_chan][i] = p_la.p_buf[p_la.i_pos].pf_vals[i_chan] * f_gain * f_mug * inputGain;
            outputs[i_chan][i] = outputs[i_chan][i] * pMix + p_la.p_buf[p_la.i_pos].pf_vals[i_chan] * (1.0 - pMix);
            outputs[i_chan][i] *= outputGain;
            
            // Update VU meter variables
            if(truePeakIn < f_x) truePeakIn = f_x;
            if(truePeakOut < outputs[i_chan][i]) truePeakOut = outputs[i_chan][i];
            if(gainReduction > f_gain) gainReduction = f_gain;
            VuInputRMS.processSample(f_x, i_chan);
            VuOutputRMS.processSample(outputs[i_chan][i], i_chan);
            VuInputPeak.processSample(f_x, i_chan);
            VuOutputPeak.processSample(outputs[i_chan][i], i_chan);
            
            // BYPASS
            if(pSoftBypass) outputs[i_chan][i] = p_la.p_buf[p_la.i_pos].pf_vals[i_chan];

            /* Update the delayed buffer value */
            p_la.p_buf[p_la.i_pos].pf_vals[i_chan] = f_x;
        }

        /* Go to the next delayed buffer value for the next run */
        p_la.i_pos = ( p_la.i_pos + 1 ) % ( p_la.i_count );
    }

    // evaluate max values from this sample block
    for (int ch = 0; ch < numChannels; ch++)
    {
        fInputVuRMS[ch]  = VuInputRMS.getEnv(ch);
        fOutputVuRMS[ch] = VuOutputRMS.getEnv(ch);
        fInputVuPeak[ch]  = VuInputPeak.getEnv(ch);
        fOutputVuPeak[ch] = VuOutputPeak.getEnv(ch);
    }

    return;
}

/*****************************************************************************
 * Helper functions for compressor
 *****************************************************************************/
double VLC_CompProcessor::Db2Lin(double f_db)
{
    return std::pow(10.0, f_db / 20.0);
}

double VLC_CompProcessor::Lin2Db(double f_lin)
{
    return (f_lin>0.0)?(20.0 * std::log10(f_lin)):(-100.0);
}
/* Zero out denormals by adding and subtracting a small number, from Laurent de Soras */
void VLC_CompProcessor::RoundToZero( Vst::ParamValue *pf_x )
{
    static const Vst::ParamValue f_anti_denormal = 1e-18;

    *pf_x += f_anti_denormal;
    *pf_x -= f_anti_denormal;
}

/* A set of branchless clipping operations from Laurent de Soras */

Vst::ParamValue VLC_CompProcessor::Max( Vst::ParamValue f_x, Vst::ParamValue f_a )
{
    f_x -= f_a;
    f_x += std::abs( f_x );
    f_x *= 0.5;
    f_x += f_a;

    return f_x;
}

Vst::ParamValue VLC_CompProcessor::Clamp( Vst::ParamValue f_x, Vst::ParamValue f_a, Vst::ParamValue f_b )
{
    const Vst::ParamValue f_x1 = std::abs( f_x - f_a );
    const Vst::ParamValue f_x2 = std::abs( f_x - f_b );

    f_x = f_x1 + f_a + f_b;
    f_x -= f_x2;
    f_x *= 0.5;

    return f_x;
}

/* Round float to int using IEEE int* hack */
int VLC_CompProcessor::Round( float f_x )
{
    ls_pcast32 p;

    p.f = f_x;
    p.f += ( 3 << 22 );

    return p.i - 0x4b400000;
}

/* Calculate current level from root-mean-squared of circular buffer ("RMS") */
Vst::ParamValue VLC_CompProcessor::RmsEnvProcess( rms_env * p_r, const Vst::ParamValue f_x )
{
    /* Remove the old term from the sum */
    p_r->f_sum -= p_r->pf_buf[p_r->i_pos];

    /* Add the new term to the sum */
    p_r->f_sum += f_x;

    /* If the sum is small enough, make it zero */
    if( p_r->f_sum < 1.0e-6f )
    {
        p_r->f_sum = 0.0f;
    }

    /* Replace the old term in the array with the new one */
    p_r->pf_buf[p_r->i_pos] = f_x;

    /* Go to the next position for the next RMS calculation */
    p_r->i_pos = ( p_r->i_pos + 1 ) % ( p_r->i_count );

    /* Return the RMS value */
    return sqrt( p_r->f_sum / p_r->i_count );
}

/* Output the compressed delayed buffer and store the current buffer.  
 * Uses a circular array, just like the one used in calculating the RMS of the buffer
 */
void VLC_CompProcessor::BufferProcess(Vst::ParamValue * pf_buf_in,
                                             Vst::ParamValue * pf_buf_out,
                                             int i_channels,
                                             Vst::ParamValue f_gain,
                                             Vst::ParamValue f_mug,
                                             lookahead * p_la )
{
    /* Loop through every channel */
    for( int i_chan = 0; i_chan < i_channels; i_chan++ )
    {
        /* Current buffer value */
        Vst::ParamValue f_x = pf_buf_in[i_chan];

        /* Output the compressed delayed buffer value */
        pf_buf_out[i_chan] = p_la->p_buf[p_la->i_pos].pf_vals[i_chan] * f_gain * f_mug;

        /* Update the delayed buffer value */
        p_la->p_buf[p_la->i_pos].pf_vals[i_chan] = f_x;
    }

    /* Go to the next delayed buffer value for the next run */
    p_la->i_pos = ( p_la->i_pos + 1 ) % ( p_la->i_count );
}

//------------------------------------------------------------------------
} // namespace yg331
