// Engine.h — Eliane-DSP synthesis engine
// M2: One oscillator pair + ring mod + filter — core sound validation.
// Full engine expanded in M3.

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

    // Pitch: base frequency for oscillator pair A (Hz)
    void SetPitchA(float freq_hz);

    // Spread: bipolar detuning of second oscillator in pair A (Hz)
    // Positive = osc2 higher, negative = osc2 lower, zero = unison
    void SetSpreadA(float delta_hz);

    // Filter resonance: SVF resonance for pair A's filter (0.0 - 1.0)
    // Channel parameter reserved for M3; in M2, channel 0 is used
    void SetResonance(int channel, float res);

    // Mix level: output level for pair A (0.0 - 1.0)
    // Channel parameter reserved for M3; in M2, channel 0 is used
    void SetMixLevel(int channel, float level);

private:
    float sample_rate_;

    // --- DSP modules for pair A ---
    daisysp::Oscillator osc_a1_;   // First oscillator in pair A
    daisysp::Oscillator osc_a2_;   // Second oscillator in pair A (for spread)
    daisysp::Svf lpf_a_;           // Tracking low-pass filter for pair A

    // --- Parameter targets (set by setters) ---
    float pitch_a_;
    float spread_a_;
    float mix_level_;
    float resonance_;

    // --- Smoothed values (converge toward targets via fonepole) ---
    float pitch_a_smooth_;
    float spread_a_smooth_;
    float mix_smooth_;
    float res_smooth_;

    // Non-copyable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
};

}  // namespace eliane
}  // namespace atelier
