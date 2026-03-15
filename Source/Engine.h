// Engine.h — Eliane-DSP synthesis engine
// M3: Full signal path — 4 oscillators, 3 ring mods, 3 filters, 3-channel mixer.

#pragma once
#include <daisysp.h>
#include "Constants.h"

namespace atelier {
namespace eliane {

class Engine {
public:
    Engine() = default;
    ~Engine() = default;

    void Init(float sample_rate);

    // Per-sample processing — returns mono output
    float Process();

    // --- Parameter setters (called by HAL at block rate) ---

    // Pitch: base frequency for oscillator pair A/B (Hz)
    void SetPitchA(float freq_hz);
    void SetPitchB(float freq_hz);

    // Spread: bipolar detuning of second oscillator in each pair (Hz)
    // Positive = osc2 higher, negative = osc2 lower, zero = unison
    void SetSpreadA(float delta_hz);
    void SetSpreadB(float delta_hz);

    // Mix level: output level for channel (0=A, 1=B, 2=C)
    void SetMixLevel(int channel, float level);

    // Filter resonance: per-filter SVF resonance (0.0 - 1.0)
    // channel: 0=A, 1=B, 2=C
    void SetResonance(int channel, float res);

private:
    float sample_rate_;

    // --- DSP modules ---
    daisysp::Oscillator osc_[4];   // [0]=A1, [1]=A2, [2]=B1, [3]=B2
    daisysp::Svf lpf_[3];          // [0]=A, [1]=B, [2]=C (LP mode)

    // --- Parameter targets (set by setters) ---
    float pitch_a_, pitch_b_;
    float spread_a_, spread_b_;
    float mix_level_[3];
    float resonance_[3];

    // --- Smoothed values (converge toward targets via fonepole) ---
    float pitch_a_smooth_, pitch_b_smooth_;
    float spread_a_smooth_, spread_b_smooth_;
    float mix_smooth_[3];
    float res_smooth_[3];

    // Non-copyable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
};

}  // namespace eliane
}  // namespace atelier
