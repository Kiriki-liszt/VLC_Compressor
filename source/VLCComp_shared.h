//------------------------------------------------------------------------
// Copyright(c) 2024 yg331.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/vsttypes.h"

#include <cmath>

namespace yg331 {
//------------------------------------------------------------------------
using ParamValue = Steinberg::Vst::ParamValue;
using int32      = Steinberg::int32;
using uint32     = Steinberg::uint32;

#define RMS_BUF_SIZE    (1920)
#define LOOKAHEAD_SIZE  ((RMS_BUF_SIZE)<<1)
#define AOUT_CHAN_MAX   9

#define LIN_INTERP(f,a,b) ((a) + (f) * ( (b) - (a) ))
#define LIMIT(v,l,u)      (v < l ? l : ( v > u ? u : v ))

typedef struct rms_env
{
    ParamValue pf_buf[RMS_BUF_SIZE] = {0.0, };
    uint32     i_pos = 0;
    uint32     i_count = 0;
    ParamValue f_sum = 0.0;

} rms_env;

typedef struct lookahead
{
    struct
    {
        ParamValue pf_vals[AOUT_CHAN_MAX] = {0.0, };
        ParamValue f_lev_in = 0.0;

    } p_buf[LOOKAHEAD_SIZE];
    uint32 i_pos = 0;
    uint32 i_count = 0;

} lookahead;

typedef enum {
    overSample_1x,
    overSample_2x,
    overSample_4x,
    overSample_8x,
    overSample_num = 3
} overSample;

#define Plain2Norm(v, min, max) ((v - min) / (max - min))
#define Norm2Plain(v, min, max) (v * (max - min) + min)
    
#define LogPlain2Norm(v, min, max) (std::log(v / min) / std::log(max / min))
#define LogNorm2Plain(v, min, max) (min * std::exp(v * std::log(max / min)))

static constexpr ParamValue minInput     = -12.0;
static constexpr ParamValue maxInput     = 12.0;
static constexpr ParamValue dftInput     = 0.0;
static constexpr ParamValue nrmInput     = Plain2Norm(dftInput,  minInput,  maxInput);

static constexpr ParamValue minOutput    = -12.0;
static constexpr ParamValue maxOutput    = 12.0;
static constexpr ParamValue dftOutput    = 0.0;
static constexpr ParamValue nrmOutput    = Plain2Norm(dftOutput, minOutput, maxOutput);

static constexpr ParamValue minRMS_PEAK  = 0.0;
static constexpr ParamValue maxRMS_PEAK  = 100.0;
static constexpr ParamValue dftRMS_PEAK  = 20.0;
static constexpr ParamValue nrmRMS_PEAK  = Plain2Norm(dftRMS_PEAK,  minRMS_PEAK,  maxRMS_PEAK);

static constexpr ParamValue minAttack    = 1.5;
static constexpr ParamValue maxAttack    = 400.0;
static constexpr ParamValue dftAttack    = 25.0;
// static constexpr ParamValue nrmAttack    = LogPlain2Norm(dftAttack,    minAttack,    maxAttack);
#define nrmAttack LogPlain2Norm(dftAttack,    minAttack,    maxAttack)

static constexpr ParamValue minRelease   = 2.0;
static constexpr ParamValue maxRelease   = 800.0;
static constexpr ParamValue dftRelease   = 200.0;
static constexpr ParamValue nrmRelease   = Plain2Norm(dftRelease,   minRelease,   maxRelease);

static constexpr ParamValue minThreshold = -30.0;
static constexpr ParamValue maxThreshold = 0.0;
static constexpr ParamValue dftThreshold = -11.0;
static constexpr ParamValue nrmThreshold = Plain2Norm(dftThreshold, minThreshold, maxThreshold);

static constexpr ParamValue minRatio     = 1.0;
static constexpr ParamValue maxRatio     = 20.0;
static constexpr ParamValue dftRatio     = 4.0;
static constexpr ParamValue nrmRatio     = Plain2Norm(dftRatio,     minRatio,     maxRatio);

static constexpr ParamValue minKnee      = 1.0;
static constexpr ParamValue maxKnee      = 10.0;
static constexpr ParamValue dftKnee      = 5.0;
static constexpr ParamValue nrmKnee      = Plain2Norm(dftKnee,      minKnee,      maxKnee);

static constexpr ParamValue minMakeup    = 0.0;
static constexpr ParamValue maxMakeup    = 24.0;
static constexpr ParamValue dftMakeup    = 0.0;
static constexpr ParamValue nrmMakeup    = Plain2Norm(dftMakeup,    minMakeup,    maxMakeup);

static constexpr ParamValue minMix       = 0.0;
static constexpr ParamValue maxMix       = 100.0;
static constexpr ParamValue dftMix       = 100.0;
static constexpr ParamValue nrmMix       = Plain2Norm(dftMix,       minMix,       maxMix);

enum
{
    kIn = 100,
    kInLRMS,
    kInRRMS,
    kInLPeak,
    kInRPeak,
    kOut,
    kOutLRMS,
    kOutRRMS,
    kOutLPeak,
    kOutRPeak,
    kGR
};
//------------------------------------------------------------------------
} // namespace yg331
