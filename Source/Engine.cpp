// Engine.cpp — Eliane-DSP synthesis engine
// M3: Full signal path — 4 oscillators, 3 ring mods, 3 filters, 3-channel mixer.

#include "Engine.h"
#include <cmath>

namespace atelier {
namespace eliane {

void Engine::Init(float sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize oscillators
    for (int i = 0; i < 4; i++) {
        osc_[i].Init(sample_rate);
        osc_[i].SetWaveform(daisysp::Oscillator::WAVE_SIN);
        osc_[i].SetAmp(1.0f);
    }
    osc_[0].SetFreq(kDefaultPitchA);
    osc_[1].SetFreq(kDefaultPitchA);
    osc_[2].SetFreq(kDefaultPitchB);
    osc_[3].SetFreq(kDefaultPitchB);

    // Initialize filters (defaults to low-pass in DaisySP)
    for (int i = 0; i < 3; i++) {
        lpf_[i].Init(sample_rate);
        lpf_[i].SetFreq(1000.0f);  // Will be overwritten by Process()
        lpf_[i].SetRes(0.0f);
    }

    // Initialize parameters
    pitch_a_ = kDefaultPitchA;
    pitch_b_ = kDefaultPitchB;
    spread_a_ = 0.0f;
    spread_b_ = 0.0f;
    for (int i = 0; i < 3; i++) {
        mix_level_[i] = 1.0f;
        resonance_[i] = 0.0f;
    }

    // Initialize smoothed values
    pitch_a_smooth_ = kDefaultPitchA;
    pitch_b_smooth_ = kDefaultPitchB;
    spread_a_smooth_ = 0.0f;
    spread_b_smooth_ = 0.0f;
    for (int i = 0; i < 3; i++) {
        mix_smooth_[i] = 1.0f;
        res_smooth_[i] = 0.0f;
    }
}

float Engine::Process() {
    // 1. Smooth all parameters (12 fonepole calls)
    daisysp::fonepole(pitch_a_smooth_, pitch_a_, kParamSmoothCoeff);
    daisysp::fonepole(pitch_b_smooth_, pitch_b_, kParamSmoothCoeff);
    daisysp::fonepole(spread_a_smooth_, spread_a_, kParamSmoothCoeff);
    daisysp::fonepole(spread_b_smooth_, spread_b_, kParamSmoothCoeff);
    for (int i = 0; i < 3; i++) {
        daisysp::fonepole(mix_smooth_[i], mix_level_[i], kParamSmoothCoeff);
        daisysp::fonepole(res_smooth_[i], resonance_[i], kParamSmoothCoeff);
    }

    // 2. Set oscillator frequencies
    // Pair A: osc[0] = pitch_a, osc[1] = pitch_a + spread_a
    // Pair B: osc[2] = pitch_b, osc[3] = pitch_b + spread_b
    osc_[0].SetFreq(pitch_a_smooth_);
    osc_[1].SetFreq(pitch_a_smooth_ + spread_a_smooth_);
    osc_[2].SetFreq(pitch_b_smooth_);
    osc_[3].SetFreq(pitch_b_smooth_ + spread_b_smooth_);

    // 3. Set filter cutoffs (tracking at 3× pitch)
    float cutoff_a = pitch_a_smooth_ * kFilterTrackingMultiplier;
    float cutoff_b = pitch_b_smooth_ * kFilterTrackingMultiplier;
    float cutoff_c = cutoff_a + cutoff_b;  // 3×pitchA + 3×pitchB

    lpf_[0].SetFreq(cutoff_a);  // LPF A
    lpf_[1].SetFreq(cutoff_b);  // LPF B
    lpf_[2].SetFreq(cutoff_c);  // LPF C

    // 4. Set filter resonances (full range 0.0-1.0 allows self-oscillation)
    for (int i = 0; i < 3; i++) {
        lpf_[i].SetRes(res_smooth_[i]);
    }

    // 5. Generate oscillator samples
    float s0 = osc_[0].Process();
    float s1 = osc_[1].Process();
    float s2 = osc_[2].Process();
    float s3 = osc_[3].Process();

    // 6. Ring mod A: osc[0] × osc[1]
    float rm_a = s0 * s1;

    // 7. Ring mod B: osc[2] × osc[3]
    float rm_b = s2 * s3;

    // 8. Filter A
    lpf_[0].Process(rm_a);
    float lp_a = lpf_[0].Low();

    // 9. Filter B
    lpf_[1].Process(rm_b);
    float lp_b = lpf_[1].Low();

    // 10. Ring mod C: LPF A output × LPF B output (cross-modulation)
    float rm_c = lp_a * lp_b;

    // 11. Filter C
    lpf_[2].Process(rm_c);
    float lp_c = lpf_[2].Low();

    // 12. Mix: weighted sum of all three channels
    float out = lp_a * mix_smooth_[0]
              + lp_b * mix_smooth_[1]
              + lp_c * mix_smooth_[2];

    // 13. Apply output gain to avoid analog clipping on Aurora REV4
    out *= kOutputGain;

    return out;
}

void Engine::SetPitchA(float freq_hz) {
    pitch_a_ = daisysp::fclamp(freq_hz, kMinPitchHz, kMaxPitchHz);
}

void Engine::SetPitchB(float freq_hz) {
    pitch_b_ = daisysp::fclamp(freq_hz, kMinPitchHz, kMaxPitchHz);
}

void Engine::SetSpreadA(float delta_hz) {
    spread_a_ = daisysp::fclamp(delta_hz, -kMaxSpreadHz, kMaxSpreadHz);
}

void Engine::SetSpreadB(float delta_hz) {
    spread_b_ = daisysp::fclamp(delta_hz, -kMaxSpreadHz, kMaxSpreadHz);
}

void Engine::SetMixLevel(int channel, float level) {
    if (channel >= 0 && channel < 3) {
        mix_level_[channel] = daisysp::fclamp(level, 0.0f, 1.0f);
    }
}

void Engine::SetResonance(int channel, float res) {
    if (channel >= 0 && channel < 3) {
        resonance_[channel] = daisysp::fclamp(res, 0.0f, 1.0f);
    }
}

}  // namespace eliane
}  // namespace atelier
