// ElianeDSP_main.cpp — Milestone 2: One oscillator pair + ring mod + filter
//
// Core sound validation: two near-unison sines ring-modulating, producing slow
// beating, through a tracking LPF. This is the artistic validation milestone.
//
// Controls (via DaisySeed ADC on Aurora REV4):
// - seed::A0 (KNOB_TIME): Pitch A (20-2000 Hz, logarithmic)
// - seed::A8 (KNOB_MIX): Spread A (±5 Hz through-zero)
// - seed::A9 (KNOB_WARP): LPF A resonance (0.0-0.9)
//
// NOTE: Using DaisySeed directly instead of Aurora Hardware due to Aurora REV4
// I2C hardware defect. See ADR-004.

#include "daisy_seed.h"
#include "daisysp.h"
#include "Source/Engine.h"
#include "Source/Constants.h"

using namespace daisy;
using namespace daisysp;
using namespace atelier::eliane;

DaisySeed hw;
Engine engine;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size)
{
    (void)in;
    
    for (size_t i = 0; i < size; i++)
    {
        float sample = engine.Process();
        out[0][i] = sample;  // Left
        out[1][i] = sample;  // Right (mono)
    }
}

int main(void)
{
    hw.Init();

    // Configure ADC for control knobs (REV4 pin mappings)
    // A0 = Pitch, A8 = Spread, A9 = Resonance
    AdcChannelConfig adc_config[3];
    adc_config[0].InitSingle(seed::A0);  // KNOB_TIME -> Pitch
    adc_config[1].InitSingle(seed::A8);  // KNOB_MIX -> Spread
    adc_config[2].InitSingle(seed::A9);  // KNOB_WARP -> Resonance
    hw.adc.Init(adc_config, 3);
    hw.adc.Start();

    // Initialize engine
    engine.Init(hw.AudioSampleRate());

    hw.StartAudio(AudioCallback);

    while (1)
    {
        // Read knobs and update engine parameters
        float knob_time = hw.adc.GetFloat(0);        // 0.0 - 1.0
        float knob_mix = hw.adc.GetFloat(1);         // 0.0 - 1.0  
        float knob_warp = hw.adc.GetFloat(2);         // 0.0 - 1.0

        // Map KNOB_TIME (A0) to pitch: 20-2000 Hz, logarithmic
        float pitch = fmap(knob_time, kMinPitchHz, kMaxPitchHz, Mapping::LOG);
        engine.SetPitchA(pitch);

        // Map KNOB_MIX (A8) to spread: bipolar, through-zero (±kMaxSpreadHz)
        float spread = (knob_mix - 0.5f) * 2.0f * kMaxSpreadHz;
        engine.SetSpreadA(spread);

        // Map KNOB_WARP (A9) to resonance: 0.0-1.0 (full range, allows self-oscillation)
        float resonance = knob_warp;
        engine.SetResonance(0, resonance);
    }
}
