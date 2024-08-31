//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#include "VLCComp_processor.h"
#include "VLCComp_cids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

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
	addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	/* If you don't need an event bus, you can remove the next line */
	// addEventInput (STR16 ("Event In"), 1);
    
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
	
    vlc_object_t *p_aout = vlc_object_parent(p_filter);
    filter_sys_t *p_sys = p_filter->p_sys;

    /* Remove our callbacks */
    var_DelCallback( p_aout, "compressor-rms-peak", RMSPeakCallback, p_sys );
    var_DelCallback( p_aout, "compressor-attack", AttackCallback, p_sys );
    var_DelCallback( p_aout, "compressor-release", ReleaseCallback, p_sys );
    var_DelCallback( p_aout, "compressor-threshold", ThresholdCallback, p_sys );
    var_DelCallback( p_aout, "compressor-ratio", RatioCallback, p_sys );
    var_DelCallback( p_aout, "compressor-knee", KneeCallback, p_sys );
    var_DelCallback( p_aout, "compressor-makeup-gain", MakeupGainCallback, p_sys );

    /* Destroy the filter parameter structure */
    free( p_sys );

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

                /*/*/
                if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue) {
                    switch (paramQueue->getParameterId()) {
                    case kParamBypass: bBypass = (value > 0.5f); break;
                    case kParamZoom:   fZoom = value; break;
                    case kParamLevel:  fLevel = value; break;
                    case kParamOutput: fOutput = value; break;
                        
                    case kParamBand1_In: fParamBand1_Array[ParamArray_In] = value; break;
                    case kParamBand2_In: fParamBand2_Array[ParamArray_In] = value; break;
                    case kParamBand3_In: fParamBand3_Array[ParamArray_In] = value; break;
                    case kParamBand4_In: fParamBand4_Array[ParamArray_In] = value; break;
                    case kParamBand5_In: fParamBand5_Array[ParamArray_In] = value; break;

                    case kParamBand1_Hz: fParamBand1_Array[ParamArray_Hz] = value; break;
                    case kParamBand2_Hz: fParamBand2_Array[ParamArray_Hz] = value; break;
                    case kParamBand3_Hz: fParamBand3_Array[ParamArray_Hz] = value; break;
                    case kParamBand4_Hz: fParamBand4_Array[ParamArray_Hz] = value; break;
                    case kParamBand5_Hz: fParamBand5_Array[ParamArray_Hz] = value; break;

                    case kParamBand1_Q: fParamBand1_Array[ParamArray_Q] = value; break;
                    case kParamBand2_Q: fParamBand2_Array[ParamArray_Q] = value; break;
                    case kParamBand3_Q: fParamBand3_Array[ParamArray_Q] = value; break;
                    case kParamBand4_Q: fParamBand4_Array[ParamArray_Q] = value; break;
                    case kParamBand5_Q: fParamBand5_Array[ParamArray_Q] = value; break;

                    case kParamBand1_dB: fParamBand1_Array[ParamArray_dB] = value; break;
                    case kParamBand2_dB: fParamBand2_Array[ParamArray_dB] = value; break;
                    case kParamBand3_dB: fParamBand3_Array[ParamArray_dB] = value; break;
                    case kParamBand4_dB: fParamBand4_Array[ParamArray_dB] = value; break;
                    case kParamBand5_dB: fParamBand5_Array[ParamArray_dB] = value; break;

                    case kParamBand1_Type: fParamBand1_Array[ParamArray_Type] = value; break;
                    case kParamBand2_Type: fParamBand2_Array[ParamArray_Type] = value; break;
                    case kParamBand3_Type: fParamBand3_Array[ParamArray_Type] = value; break;
                    case kParamBand4_Type: fParamBand4_Array[ParamArray_Type] = value; break;
                    case kParamBand5_Type: fParamBand5_Array[ParamArray_Type] = value; break;

                    case kParamBand1_Order: fParamBand1_Array[ParamArray_Order] = value; break;
                    case kParamBand2_Order: fParamBand2_Array[ParamArray_Order] = value; break;
                    case kParamBand3_Order: fParamBand3_Array[ParamArray_Order] = value; break;
                    case kParamBand4_Order: fParamBand4_Array[ParamArray_Order] = value; break;
                    case kParamBand5_Order: fParamBand5_Array[ParamArray_Order] = value; break;
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

    // (simplification) we suppose in this example that we have the same input channel count than
    // the output
    // int32 numChannels = data.inputs[0].numChannels;
    numChannels = data.inputs[0].numChannels;

    //---get audio buffers----------------
    uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);
    void** in  = getChannelBuffersPointer(processSetup, data.inputs[0]);
    void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
    Vst::SampleRate getSampleRate = processSetup.sampleRate;

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

        if (data.symbolicSampleSize == Vst::kSample32) {
            processSVF<Vst::Sample32>((Vst::Sample32**)in, (Vst::Sample32**)out, numChannels, getSampleRate, data.numSamples);
        }
        else {
            processSVF<Vst::Sample64>((Vst::Sample64**)in, (Vst::Sample64**)out, numChannels, getSampleRate, data.numSamples);
        }
    }

    int data_avail = FFT.getData(fft_out.data());

    //--- send data ----------------
    if (currentExchangeBlock.blockID == Vst::InvalidDataExchangeBlockID)
        acquireNewExchangeBlock();
    if (auto block = toDataBlock(currentExchangeBlock))
    {
        memcpy(block->Band1, fParamBand1_Array, ParamArray::ParamArray_size * sizeof(double));
        memcpy(block->Band2, fParamBand2_Array, ParamArray::ParamArray_size * sizeof(double));
        memcpy(block->Band3, fParamBand3_Array, ParamArray::ParamArray_size * sizeof(double));
        memcpy(block->Band4, fParamBand4_Array, ParamArray::ParamArray_size * sizeof(double));
        memcpy(block->Band5, fParamBand5_Array, ParamArray::ParamArray_size * sizeof(double));
        memcpy(&block->samples[0], fft_out.data(), _numBins * sizeof(float));
        block->FFTSampleRate = getSampleRate;
        block->FFTDataAvail = data_avail;
        block->numSamples = data.numSamples;
        block->filterSampleRate = OS_target;
        block->filterBypass = bBypass;
        block->filterLevel = fLevel;
        dataExchange->sendCurrentBlock();
        acquireNewExchangeBlock();
    }

    return kResultOk;
    
	//--- First : Read inputs parameter changes-----------

    /*if (data.inputParameterChanges)
    {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
        for (int32 index = 0; index < numParamsChanged; index++)
        {
            if (auto* paramQueue = data.inputParameterChanges->getParameterData (index))
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount ();
                switch (paramQueue->getParameterId ())
                {
				}
			}
		}
	}*/
	
	//--- Here you have to implement your processing

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::setupProcessing (Vst::ProcessSetup& newSetup)
{
    /// VLC
    f_sample_rate = newSetup.sampleRate;

    /* Calculate the RMS and lookahead sizes from the sample rate */
    f_num = 0.01 * f_sample_rate;
    rms.i_count = Round( Clamp( 0.5 * f_num, 1.0, RMS_BUF_SIZE ) );
    la.i_count = Round( Clamp( f_num, 1.0, LOOKAHEAD_SIZE ) );
    
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
    
    /* Restore the last saved settings */
    p_sys->f_rms_peak    = var_CreateGetFloat( p_aout, "compressor-rms-peak" );
    p_sys->f_attack      = var_CreateGetFloat( p_aout, "compressor-attack" );
    p_sys->f_release     = var_CreateGetFloat( p_aout, "compressor-release" );
    p_sys->f_threshold   = var_CreateGetFloat( p_aout, "compressor-threshold" );
    p_sys->f_ratio       = var_CreateGetFloat( p_aout, "compressor-ratio" );
    p_sys->f_knee        = var_CreateGetFloat( p_aout, "compressor-knee" );
    p_sys->f_makeup_gain = var_CreateGetFloat( p_aout, "compressor-makeup-gain" );
	
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VLC_CompProcessor::getState (IBStream* state)
{
	// here we need to save the model
	IBStreamer streamer (state, kLittleEndian);

	return kResultOk;
}

/*****************************************************************************
 * Helper functions for compressor
 *****************************************************************************/

/* Zero out denormals by adding and subtracting a small number, from Laurent de Soras */
static void VLC_CompProcessor::RoundToZero( Vst::ParamValue *pf_x )
{
    static const Vst::ParamValue f_anti_denormal = 1e-18;

    *pf_x += f_anti_denormal;
    *pf_x -= f_anti_denormal;
}

/* A set of branchless clipping operations from Laurent de Soras */

static Vst::ParamValue VLC_CompProcessor::Max( Vst::ParamValue f_x, Vst::ParamValue f_a )
{
    f_x -= f_a;
    f_x += std::abs( f_x );
    f_x *= 0.5;
    f_x += f_a;

    return f_x;
}

static Vst::ParamValue VLC_CompProcessor::Clamp( Vst::ParamValue f_x, Vst::ParamValue f_a, Vst::ParamValue f_b )
{
    const Vst::ParamValue f_x1 = std::abs( f_x - f_a );
    const Vst::ParamValue f_x2 = std::abs( f_x - f_b );

    f_x = f_x1 + f_a + f_b;
    f_x -= f_x2;
    f_x *= 0.5;

    return f_x;
}

/* Round float to int using IEEE int* hack */
static int VLC_CompProcessor::Round( float f_x )
{
    ls_pcast32 p;

    p.f = f_x;
    p.f += ( 3 << 22 );

    return p.i - 0x4b400000;
}

/* Calculate current level from root-mean-squared of circular buffer ("RMS") */
static Vst::ParamValue VLC_CompProcessor::RmsEnvProcess( rms_env * p_r, const Vst::ParamValue f_x )
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

/* Output the compressed delayed buffer and store the current buffer.  Uses a
 * circular array, just like the one used in calculating the RMS of the buffer
 */
static void VLC_CompProcessor::BufferProcess( Vst::ParamValue * pf_buf, int i_channels, Vst::ParamValue f_gain,
                                             Vst::ParamValue f_mug, lookahead * p_la )
{
    /* Loop through every channel */
    for( int i_chan = 0; i_chan < i_channels; i_chan++ )
    {
        Vst::ParamValue f_x = pf_buf[i_chan]; /* Current buffer value */

        /* Output the compressed delayed buffer value */
        pf_buf[i_chan] = p_la->p_buf[p_la->i_pos].pf_vals[i_chan]
                       * f_gain * f_mug;

        /* Update the delayed buffer value */
        p_la->p_buf[p_la->i_pos].pf_vals[i_chan] = f_x;
    }

    /* Go to the next delayed buffer value for the next run */
    p_la->i_pos = ( p_la->i_pos + 1 ) % ( p_la->i_count );
}

/*****************************************************************************
 * Callback functions
 *****************************************************************************/
static int VLC_CompProcessor::RMSPeakCallback( vlc_object_t *p_this, char const *psz_cmd,
                            vlc_value_t oldval, vlc_value_t newval,
                            void * p_data )
{
    VLC_UNUSED(p_this); VLC_UNUSED(psz_cmd); VLC_UNUSED(oldval);
    filter_sys_t *p_sys = p_data;

    vlc_mutex_lock( &p_sys->lock );
    p_sys->f_rms_peak = Clamp( newval.f_float, 0.0f, 1.0f );
    vlc_mutex_unlock( &p_sys->lock );

    return VLC_SUCCESS;
}

static int VLC_CompProcessor::AttackCallback( vlc_object_t *p_this, char const *psz_cmd,
                           vlc_value_t oldval, vlc_value_t newval,
                           void * p_data )
{
    VLC_UNUSED(p_this); VLC_UNUSED(psz_cmd); VLC_UNUSED(oldval);
    filter_sys_t *p_sys = p_data;

    vlc_mutex_lock( &p_sys->lock );
    p_sys->f_attack = Clamp( newval.f_float, 1.5f, 400.0f );
    vlc_mutex_unlock( &p_sys->lock );

    return VLC_SUCCESS;
}

static int VLC_CompProcessor::ReleaseCallback( vlc_object_t *p_this, char const *psz_cmd,
                            vlc_value_t oldval, vlc_value_t newval,
                            void * p_data )
{
    VLC_UNUSED(p_this); VLC_UNUSED(psz_cmd); VLC_UNUSED(oldval);
    filter_sys_t *p_sys = p_data;

    vlc_mutex_lock( &p_sys->lock );
    p_sys->f_release = Clamp( newval.f_float, 2.0f, 800.0f );
    vlc_mutex_unlock( &p_sys->lock );

    return VLC_SUCCESS;
}

static int VLC_CompProcessor::ThresholdCallback( vlc_object_t *p_this, char const *psz_cmd,
                              vlc_value_t oldval, vlc_value_t newval,
                              void * p_data )
{
    VLC_UNUSED(p_this); VLC_UNUSED(psz_cmd); VLC_UNUSED(oldval);
    filter_sys_t *p_sys = p_data;

    vlc_mutex_lock( &p_sys->lock );
    p_sys->f_threshold = Clamp( newval.f_float, -30.0f, 0.0f );
    vlc_mutex_unlock( &p_sys->lock );

    return VLC_SUCCESS;
}

static int VLC_CompProcessor::RatioCallback( vlc_object_t *p_this, char const *psz_cmd,
                          vlc_value_t oldval, vlc_value_t newval,
                          void * p_data )
{
    VLC_UNUSED(p_this); VLC_UNUSED(psz_cmd); VLC_UNUSED(oldval);
    filter_sys_t *p_sys = p_data;

    vlc_mutex_lock( &p_sys->lock );
    p_sys->f_ratio = Clamp( newval.f_float, 1.0f, 20.0f );
    vlc_mutex_unlock( &p_sys->lock );

    return VLC_SUCCESS;
}

static int VLC_CompProcessor::KneeCallback( vlc_object_t *p_this, char const *psz_cmd,
                         vlc_value_t oldval, vlc_value_t newval,
                         void * p_data )
{
    VLC_UNUSED(p_this); VLC_UNUSED(psz_cmd); VLC_UNUSED(oldval);
    filter_sys_t *p_sys = p_data;

    vlc_mutex_lock( &p_sys->lock );
    p_sys->f_knee = Clamp( newval.f_float, 1.0f, 10.0f );
    vlc_mutex_unlock( &p_sys->lock );

    return VLC_SUCCESS;
}

static int VLC_CompProcessor::MakeupGainCallback( vlc_object_t *p_this, char const *psz_cmd,
                               vlc_value_t oldval, vlc_value_t newval,
                               void * p_data )
{
    VLC_UNUSED(p_this); VLC_UNUSED(psz_cmd); VLC_UNUSED(oldval);
    filter_sys_t *p_sys = p_data;

    vlc_mutex_lock( &p_sys->lock );
    p_sys->f_makeup_gain = Clamp( newval.f_float, 0.0f, 24.0f );
    vlc_mutex_unlock( &p_sys->lock );

    return VLC_SUCCESS;
}



//------------------------------------------------------------------------
} // namespace yg331
