// Engine.cpp — Eliane-DSP synthesis engine
// M2: One oscillator pair + ring mod + filter — core sound validation.

#include "Engine.h"
#include <cmath>

namespace atelier {
namespace eliane {

void Engine::Init(float sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize oscillators
    osc_a1_.Init(sample_rate);
    osc_a2_.Init(sample_rate);
    osc_a1_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
    osc_a2_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
    osc_a1_.SetFreq(kDefaultPitchA);
    osc_a2_.SetFreq(kDefaultPitchA);
    osc_a1_.SetAmp(1.0f);
    osc_a2_.SetAmp(1.0f);

    // Initialize filter (defaults to low-pass in DaisySP)
    lpf_a_.Init(sample_rate);
    lpf_a_.SetFreq(kDefaultPitchA * 3.0f);
    lpf_a_.SetRes(0.0f);

    // Initialize parameters
    pitch_a_ = kDefaultPitchA;
    spread_a_ = 0.0f;
    mix_level_ = 1.0f;       // Full volume for ring mod testing
    resonance_ = 0.0f;

    // Initialize smoothed values
    pitch_a_smooth_ = kDefaultPitchA;
    spread_a_smooth_ = 0.0f;
    mix_smooth_ = 1.0f;       // Full volume
    res_smooth_ = 0.0f;
}

float Engine::Process() {
    // 1. Smooth all parameters
    daisysp::fonepole(pitch_a_smooth_, pitch_a_, kParamSmoothCoeff);
    daisysp::fonepole(spread_a_smooth_, spread_a_, kParamSmoothCoeff);
    daisysp::fonepole(mix_smooth_, mix_level_, kParamSmoothCoeff);
    daisysp::fonepole(res_smooth_, resonance_, kParamSmoothCoeff);

    // 2. Set oscillator frequencies
    float osc1_freq = pitch_a_smooth_;
    float osc2_freq = pitch_a_smooth_ + spread_a_smooth_;
    osc_a1_.SetFreq(osc1_freq);
    osc_a2_.SetFreq(osc2_freq);

    // 3. Set filter cutoff (tracking at 3× pitch)
    float cutoff = pitch_a_smooth_ * 3.0f;
    lpf_a_.SetFreq(cutoff);

    // 4. Set filter resonance (full range 0.0-1.0 for self-oscillation)
    lpf_a_.SetRes(res_smooth_);

    // 5. Generate oscillator samples
    float s1 = osc_a1_.Process();
    float s2 = osc_a2_.Process();

    // 6. Ring modulation (multiplication produces sum/difference frequencies)
    float rm_a = s1 * s2;

    // 7. Filter the ring mod output
    lpf_a_.Process(rm_a);
    float filtered = lpf_a_.Low();

    // 8. Output with kOutputGain to avoid analog clipping on Aurora REV4
    float out = filtered * mix_smooth_ * kOutputGain;

    return out;
}

void Engine::SetPitchA(float freq_hz) {
    pitch_a_ = daisysp::fclamp(freq_hz, kMinPitchHz, kMaxPitchHz);
}

void Engine::SetSpreadA(float delta_hz) {
    spread_a_ = daisysp::fclamp(delta_hz, -kMaxSpreadHz, kMaxSpreadHz);
}

void Engine::SetResonance(int channel, float res) {
    // In M2, only channel 0 is used. Full 0.0-1.0 range allows self-oscillation.
    // M3+ will use the channel parameter.
    (void)channel;
    resonance_ = daisysp::fclamp(res, 0.0f, 1.0f);
}

void Engine::SetMixLevel(int channel, float level) {
    // In M2, only channel 0 is used. M3+ will use the channel parameter.
    (void)channel;
    mix_level_ = daisysp::fclamp(level, 0.0f, 1.0f);
}

}  // namespace eliane
}  // namespace atelier
