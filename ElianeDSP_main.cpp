// ElianeDSP_main.cpp — Milestone 1: Single oscillator proof of build/deploy
//
// Proves: toolchain, Makefile, USB deploy, and audio output all work.
// One sine oscillator at 80 Hz default, KNOB_TIME controls pitch.
//
// This is intentionally throwaway scaffolding. The Engine class replaces
// this in M2. See implementation-plan.md for details.

#include "aurora.h"
#include "daisysp.h"
#include "Constants.h"

using namespace daisy;
using namespace aurora;
using namespace daisysp;

Hardware hw;
Oscillator osc;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size)
{
    (void)in;  // Input buffer unused in M1
    hw.ProcessAllControls();

    // Map KNOB_TIME to pitch: 20-2000 Hz, logarithmic
    float knob = hw.GetKnobValue(KNOB_TIME);
    float freq = fmap(knob,
                      atelier::eliane::kMinPitchHz,
                      atelier::eliane::kMaxPitchHz,
                      Mapping::LOG);
    osc.SetFreq(freq);

    for (size_t i = 0; i < size; i++)
    {
        float sample = osc.Process();
        out[0][i] = sample;  // Left
        out[1][i] = sample;  // Right (mono — same signal both channels)
    }
}

int main(void)
{
    hw.Init();

    osc.Init(hw.AudioSampleRate());
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc.SetFreq(atelier::eliane::kDefaultPitchA);
    osc.SetAmp(0.7f);

    hw.StartAudio(AudioCallback);

    while (1)
    {
        // M1: no LED updates yet — that's M5
    }
}
