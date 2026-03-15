// ElianeDSP_main.cpp — Milestone 3: Full signal path
//
// Complete synthesis engine: 4 oscillators, 3 ring mods, 3 filters, 3-channel mixer.
// Two independent oscillator pairs with cross-ring-modulation.
//
// Row-based knob layout:
// - Top row: Pitch A (WARP/A9), Pitch B (TIME/A0)
// - Mid row: Spread A (BLUR/A11), Spread B (REFLECT/A1)
// - Bottom row: Mix (MIX/A8), Resonance (ATMOSPHERE/A10)
//
// In M3, bottom row knobs affect all 3 channels uniformly.
// M4 will add button cycling for per-channel control.
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

    // Configure 6 ADC channels — row-based knob layout
    // Row 1 (top): Pitch A, Pitch B
    // Row 2 (mid): Spread A, Spread B
    // Row 3 (bottom): Mix, Resonance (affects all 3 channels in M3)
    AdcChannelConfig adc_config[6];
    adc_config[0].InitSingle(seed::A9);   // KNOB_WARP → Pitch A (top-left)
    adc_config[1].InitSingle(seed::A0);   // KNOB_TIME → Pitch B (top-right)
    adc_config[2].InitSingle(seed::A11);  // KNOB_BLUR → Spread A (mid-left)
    adc_config[3].InitSingle(seed::A1);   // KNOB_REFLECT → Spread B (mid-right)
    adc_config[4].InitSingle(seed::A8);   // KNOB_MIX → Mix Level (bot-left, all 3 ch)
    adc_config[5].InitSingle(seed::A10);  // KNOB_ATMOSPHERE → Resonance (bot-right, all 3 ch)
    hw.adc.Init(adc_config, 6);
    hw.adc.Start();

    // Initialize engine
    engine.Init(hw.AudioSampleRate());

    hw.StartAudio(AudioCallback);

    while (1)
    {
        // Read all 6 knobs
        float knob_warp = hw.adc.GetFloat(0);       // Pitch A (top-left)
        float knob_time = hw.adc.GetFloat(1);        // Pitch B (top-right)
        float knob_blur = hw.adc.GetFloat(2);       // Spread A (mid-left)
        float knob_reflect = hw.adc.GetFloat(3);    // Spread B (mid-right)
        float knob_mix = hw.adc.GetFloat(4);        // Mix (bot-left)
        float knob_atmos = hw.adc.GetFloat(5);       // Resonance (bot-right)

        // Row 1: Pitch controls (logarithmic 20-2000 Hz)
        float pitch_a = fmap(knob_warp, kMinPitchHz, kMaxPitchHz, Mapping::LOG);
        float pitch_b = fmap(knob_time, kMinPitchHz, kMaxPitchHz, Mapping::LOG);
        engine.SetPitchA(pitch_a);
        engine.SetPitchB(pitch_b);

        // Row 2: Spread controls (bipolar, through-zero)
        float spread_a = (knob_blur - 0.5f) * 2.0f * kMaxSpreadHz;
        float spread_b = (knob_reflect - 0.5f) * 2.0f * kMaxSpreadHz;
        engine.SetSpreadA(spread_a);
        engine.SetSpreadB(spread_b);

        // Row 3: Mix and Resonance (affect all 3 channels in M3)
        // M4 will add cycling for per-channel control
        for (int ch = 0; ch < 3; ch++) {
            engine.SetMixLevel(ch, knob_mix);
            engine.SetResonance(ch, knob_atmos);
        }
    }
}
