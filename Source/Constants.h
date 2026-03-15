// Constants.h — Eliane-DSP tuning constants
// M2: Added spread, resonance, and smoothing constants.
// Full constants added incrementally in M2-M5.

#pragma once

namespace atelier {
namespace eliane {

// Default pitches (Hz) — 1:2 octave ratio (see ADR-003)
constexpr float kDefaultPitchA = 80.0f;
constexpr float kDefaultPitchB = 160.0f;

// Pitch range (Hz)
constexpr float kMinPitchHz = 20.0f;
constexpr float kMaxPitchHz = 2000.0f;

// Spread range — bipolar, through-zero (see signal-path-design.md)
// Exact max TBD by ear on hardware during M2. Starting at ±5 Hz.
constexpr float kMaxSpreadHz = 5.0f;

// Soft takeover (see ADR-002)
constexpr float kSoftTakeoverThreshold = 0.02f;  // knob catch threshold (0-1)
constexpr float kPulseHzMin = 0.5f;              // LED pulse rate at zero distance
constexpr float kPulseHzMax = 8.0f;              // LED pulse rate at max distance

// Parameter smoothing coefficient for fonepole()
// ~10ms convergence at sample rate (48 kHz). Called per-sample in Engine::Process().
constexpr float kParamSmoothCoeff = 0.002f;

// Channel count for cycled knobs (A, B, C)
constexpr int kNumChannels = 3;

// Maximum safe digital output amplitude to avoid analog clipping.
// Aurora REV4 positive rail clips at +4V. At 0.65, output is ~7.5Vpp clean.
// Empirically validated: 1.0 clips, 0.8 clips, 0.65 clean.
// See skills/aurora-hardware.skill.md for test data.
constexpr float kOutputGain = 0.65f;

}  // namespace eliane
}  // namespace atelier
