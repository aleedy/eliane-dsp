// Constants.h — Eliane-DSP tuning constants
// M1: Minimal set for single-oscillator proof of build/deploy.
// Full constants added incrementally in M2-M5.

#pragma once

namespace atelier {
namespace eliane {

// Default pitches (Hz) — 1:2 octave ratio (see ADR-003)
constexpr float kDefaultPitchA = 80.0f;

// Pitch range (Hz)
constexpr float kMinPitchHz = 20.0f;
constexpr float kMaxPitchHz = 2000.0f;

}  // namespace eliane
}  // namespace atelier
