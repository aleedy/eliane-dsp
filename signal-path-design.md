---
project: Eliane-DSP
type: Signal Path Design
status: Approved
created: 2026-03-14
---

# Signal Path Design — Eliane-DSP

## Design Philosophy

Inspired by Eliane Radigue's synthesis techniques: closely-tuned oscillator pairs
producing slow beating patterns, ring modulation for spectral complexity, and low-pass
filtering to shape the resulting sidebands. The architecture prioritizes subtle,
glacially-evolving timbral shifts over aggressive modulation.

## Signal Flow Overview

Four oscillators arranged in two pairs. Each pair feeds a ring modulator. The two
ring mod outputs are then cross-ring-modulated through a third ring mod. All three
ring mod outputs pass through tracking low-pass filters before reaching a 3-channel
mixer that produces the final stereo output.

```
 ┌─────────────────────────────────────────────────────────────────┐
 │                        PAIR A                                   │
 │                                                                 │
 │   Pitch A ──┬──► Osc 1 (f₀) ──┐                                │
 │             │                  ├──► Ring Mod A ──► LPF A ──┬──► │ ──► Mix Ch.1
 │             └──► Osc 2 (f₀±Δ)─┘        │                  │    │
 │                       ▲                 │                  │    │
 │                       │                 │                  │    │
 │               Spread A (±Δ Hz)    cutoff = 2 × f₀         │    │
 │               (through-zero,      (tracks Pitch A,        │    │
 │                fine, few Hz max)    doubled for             │    │
 │                                    upper sidebands)        │    │
 └────────────────────────────────────────────────────────────┼────┘
                                                              │
                                                              ▼
                                                     Ring Mod C (input 1)
                                                              │
                                                              │
                                                     Ring Mod C (input 2)
                                                              ▲
                                                              │
 ┌────────────────────────────────────────────────────────────┼────┐
 │                        PAIR B                              │    │
 │                                                            │    │
 │   Pitch B ──┬──► Osc 3 (g₀) ──┐                           │    │
 │             │                  ├──► Ring Mod B ──► LPF B ──┴──► │ ──► Mix Ch.2
 │             └──► Osc 4 (g₀±Δ)─┘        │                       │
 │                       ▲                 │                       │
 │                       │                 │                       │
 │               Spread B (±Δ Hz)    cutoff = 2 × g₀              │
 │               (through-zero,      (tracks Pitch B,             │
 │                fine, few Hz max)    doubled for                  │
 │                                    upper sidebands)             │
 └─────────────────────────────────────────────────────────────────┘


 ┌─────────────────────────────────────────────────────────────────┐
 │                     CROSS-MODULATION                            │
 │                                                                 │
 │   LPF A out ──► Ring Mod C ──► LPF C ──► Mix Ch.3              │
 │   LPF B out ──┘                                                 │
 │                                                                 │
 │   LPF C cutoff: TBD (see open questions)                        │
 └─────────────────────────────────────────────────────────────────┘


 ┌─────────────────────────────────────────────────────────────────┐
 │                        MIXER                                    │
 │                                                                 │
 │   Ch.1 (LPF A out) ──┐                                         │
 │   Ch.2 (LPF B out) ──┼──► Mix ──► Stereo Audio Out             │
 │   Ch.3 (LPF C out) ──┘                                         │
 │                                                                 │
 │   Each channel level independently adjustable                   │
 └─────────────────────────────────────────────────────────────────┘
```

## DSP Modules Required

### Oscillators (×4)
- **Count**: 4 (Osc 1-4)
- **Waveform**: Pure sine (WAVE_SIN) — cleanest ring mod output, most Radigue-like
- **Frequency range**: Sub-audio through audio range (tentatively ~20 Hz - 2 kHz)
- **DaisySP candidate**: `Oscillator` (WAVE_SIN mode)
- **Custom needs**: None anticipated — DaisySP Oscillator has sufficient resolution
- **Default tuning**: Pair A = 80 Hz, Pair B = 160 Hz (1:2 octave ratio, easily tweakable)

### Ring Modulators (×3)
- **Ring Mod A**: Osc 1 × Osc 2
- **Ring Mod B**: Osc 3 × Osc 4
- **Ring Mod C**: LPF A output × LPF B output
- **Implementation**: Simple multiplication (sample-by-sample: `out = in1 * in2`)
- **DaisySP candidate**: None needed — ring modulation is just multiplication
- **Note**: True ring mod (4-quadrant multiplication), NOT amplitude modulation

### Low-Pass Filters (×3)
- **LPF A**: cutoff = 3 × f₀ (tracks Pitch A, tripled to pass upper sidebands — validated empirically in M2)
- **LPF B**: cutoff = 3 × g₀ (tracks Pitch B, tripled to pass upper sidebands)
- **LPF C**: cutoff = 3×f₀ + 3×g₀ (sum of LPF A and LPF B cutoffs — theoretically
  correct upper sideband limit for Ring Mod C, since its inputs are already band-limited
  by LPF A and LPF B)
- **Filter type**: `Svf` (DaisySP state-variable filter)
  - Clean, precise character — closest to ARP 2500 diode ladder aesthetic
  - Multi-mode: simultaneous LP/HP/BP/Notch/Peak outputs (LP used, others available for future)
  - Controllable resonance via `SetRes()` — driven by cycled knob on Aurora
  - Low CPU cost compared to LadderFilter (no oversampling)
- **Resonance**: Per-filter, controlled via KNOB_WARP (cycled with SW_REVERSE)

### Mixer (1 × 3-channel, mono)
- **Channels**: 3 (one per ring mod output, post-filter)
- **Level control**: Each channel independently adjustable via cycled knob (KNOB_BLUR)
- **Implementation**: Weighted sum with gain per channel
- **Output**: Mono — same signal to both stereo channels. Stereo panning may be
  explored in future iterations but is not part of the initial design.

## Control Parameters (Ideal, Hardware-Agnostic)

These are the parameters the synthesis engine needs, independent of any hardware mapping:

| # | Parameter | Range | Character | Notes |
|---|-----------|-------|-----------|-------|
| 1 | Pitch A | frequency or V/Oct | Continuous | Sets base frequency of Osc 1 and Osc 2 |
| 2 | Pitch B | frequency or V/Oct | Continuous | Sets base frequency of Osc 3 and Osc 4 |
| 3 | Spread A | ±Δ Hz (few Hz max) | Bipolar, fine | Through-zero: center=unison, +=Osc2 higher, -=Osc2 lower |
| 4 | Spread B | ±Δ Hz (few Hz max) | Bipolar, fine | Through-zero: center=unison, +=Osc4 higher, -=Osc4 lower |
| 5 | Mix Level A | 0.0 - 1.0 | Unipolar | Level of Ring Mod A (post-LPF A) in final mix |
| 6 | Mix Level B | 0.0 - 1.0 | Unipolar | Level of Ring Mod B (post-LPF B) in final mix |
| 7 | Mix Level C | 0.0 - 1.0 | Unipolar | Level of Ring Mod C (post-LPF C) in final mix |

### Derived Parameters (Auto-Tracked, Not User-Controlled)
| Parameter | Derived From | Formula |
|-----------|-------------|---------|
| LPF A cutoff | Pitch A | 3 × f₀ (tripled to pass upper sidebands) |
| LPF B cutoff | Pitch B | 3 × g₀ (tripled to pass upper sidebands) |
| LPF C cutoff | Pitch A + Pitch B | 3×f₀ + 3×g₀ (sum — max upper sideband of cross-mod) |
| Osc 1 frequency | Pitch A | f₀ |
| Osc 2 frequency | Pitch A + Spread A | f₀ + Δ |
| Osc 3 frequency | Pitch B | g₀ |
| Osc 4 frequency | Pitch B + Spread B | g₀ + Δ |

### Additional Parameters (Resolved or Deferred)
| Parameter | Decision | Notes |
|-----------|----------|-------|
| LPF C cutoff | Auto-tracked: 3×f₀ + 3×g₀ | Sum of LPF A and LPF B cutoffs. 3× multiplier empirically validated in M2. |
| LPF resonance (×3) | Controllable per filter | Via KNOB_WARP, cycled with SW_REVERSE |
| Pair ratio | Default 1:2 (80/160 Hz), easily tweakable | Defined as a constant for experimentation |
| Waveform select | Pure sine (WAVE_SIN) | Cleanest ring mod, most Radigue-like |
| Stereo spread/pan | Mono output for now | Same signal to both channels |

### Default Pitch Configuration
Pair A and Pair B are tuned at a 1:2 octave ratio by default. This is defined as an
easily-tweakable constant so different ratios can be explored (e.g., 2:3 fifth, 3:4 fourth,
1:1 unison). The CV inputs set absolute pitch per pair, so the ratio is implicit from
the two pitch settings. The default serves as a starting point when no CV is connected.

```
Default:  Pair A = 80 Hz,  Pair B = 160 Hz  (1:2 octave)
```

Other ratios to experiment with:
- 1:1 (unison) — both pairs at same base frequency, cross-mod produces very low difference tones
- 2:3 (fifth) — consonant, bell-like ring mod sidebands
- 3:4 (fourth) — similar to fifth, slightly different character
- 1:3 (octave + fifth) — wider interval, more complex sidebands

## Spread Parameter Design

The spread control is central to the Radigue aesthetic. Key design choices:

- **Through-zero**: At center position, both oscillators are in perfect unison (no beating).
  Turning CW detunes the second oscillator sharp; CCW detunes it flat.
- **Fine resolution**: Maximum spread should be calibrated to a few Hz (e.g., ±3 Hz or ±5 Hz).
  At 80 Hz base frequency, a 2 Hz spread produces a ~2 Hz beating pattern — this is the
  sweet spot for Radigue-style slow pulsation.
- **CV behavior**: When a cable is plugged into the spread CV jack, the knob acts as an
  attenuator on the incoming CV signal. With nothing plugged in, the knob sets a static
  spread offset. The CV input is bipolar, so external modulation can sweep through zero.

## Ring Modulation Theory (for reference)

Ring modulation produces sum and difference frequencies:

```
Osc1 = sin(2π × f₁ × t)
Osc2 = sin(2π × f₂ × t)

Ring Mod = Osc1 × Osc2 = 0.5 × [cos(2π(f₁-f₂)t) - cos(2π(f₁+f₂)t)]
```

When f₁ ≈ f₂ (near-unison, small spread):
- **Difference frequency** (f₁-f₂) is very low → produces the slow beating/amplitude pulsation
- **Sum frequency** (f₁+f₂) ≈ 2×f₀ → produces the audio-rate upper sideband

This is why LPF cutoff is set to 2×f₀: it passes the upper sideband while the difference
frequency is already well below it. The filter mainly serves to clean up harmonics from
non-sinusoidal waveforms and prevent aliasing in the cross-modulation stage.

## Aurora Hardware Mapping (POC)

Aurora has 6 knobs (normalled to CV), 6 CV inputs, 3 momentary switches, 2 gate inputs,
and 11 RGB LEDs. The synthesis engine needs 7 primary parameters (4 osc + 3 mix) plus
3 resonance controls. This is solved by using two button-cycled knobs.

### Control Assignment

**Row-based layout** (see aurora-hardware.skill.md for pin details):

| Row | Left Knob | Right Knob | Function |
|-----|-----------|------------|----------|
| Top | KNOB_WARP (A9) | KNOB_TIME (A0) | Pitch A / Pitch B |
| Mid | KNOB_BLUR (A11) | KNOB_REFLECT (A1) | Spread A / Spread B |
| Bottom | KNOB_MIX (A8) | KNOB_ATMOSPHERE (A10) | Mix Level (cycled) / Resonance (cycled) |

| Aurora Control | Function | Range / Notes |
|----------------|----------|---------------|
| KNOB_WARP | Pitch A | Sets base frequency of Pair A |
| KNOB_TIME | Pitch B | Sets base frequency of Pair B |
| KNOB_BLUR | Spread A | Bipolar, fine (±few Hz), through-zero |
| KNOB_REFLECT | Spread B | Bipolar, fine (±few Hz), through-zero |
| KNOB_MIX | Mix level (cycled) | Adjusts level for selected channel (A/B/C) |
| KNOB_ATMOSPHERE | LPF resonance (cycled) | Adjusts resonance for selected filter (A/B/C) |
| CV_TIME | Pitch A CV | Knob becomes attenuator when cable inserted |
| CV_REFLECT | Pitch B CV | Knob becomes attenuator when cable inserted |
| CV_MIX | Spread A CV | Bipolar CV, knob attenuates |
| CV_ATMOSPHERE | Spread B CV | Bipolar CV, knob attenuates |
| CV_BLUR | *Reserved* | Unassigned — future use |
| CV_WARP | *Reserved* | Unassigned — future use |
| SW_SHIFT | Cycle Mix + Resonance together | Both knobs target the same channel: A → B → C → A |
| SW_SHIFT | *Reserved* | Unassigned — future use |
| GATE_FREEZE | *Reserved* | Unassigned — future use |
| GATE_REVERSE | *Reserved* | Unassigned — future use |

### Cycled Knob Pattern (Shared Cycle)

One button (SW_SHIFT) cycles **both** Mix and Resonance knobs together to the same
channel. This keeps the two knobs synchronized — when you adjust Mix for stage A,
Resonance is also targeting stage A.

```
  SW_SHIFT press:
  Mix A + Res A → Mix B + Res B → Mix C + Res C → Mix A + Res A
    │
    └──► KNOB_MIX adjusts selected mix level
    └──► KNOB_ATMOSPHERE adjusts selected filter resonance
```

Stored values persist when cycling away — each knob maintains its own stored value
for each channel.

### Soft Takeover

When cycling to a new target, the physical knob position likely differs from the stored
parameter value. To prevent audible jumps, **soft takeover** is used: the knob has no
effect until it physically crosses the stored value, at which point it "catches" and
begins tracking normally.

**Directional LED indication** (inspired by Eventide Blackhole):

One LED serves as a shared soft takeover indicator for both cycled knobs (since they
cycle together). It communicates direction and distance to the stored value:

| Knob vs. Stored Value | LED Color | Pulse Rate |
|-----------------------|-----------|------------|
| Knob far below stored value | Blue (turn right) | Fast (~8 Hz) |
| Knob approaching from below | Blue | Slowing as distance decreases |
| Knob catches stored value | Solid green/white flash, then steady | — |
| Knob is live-tracking | Solid (channel color) | — |
| Knob far above stored value | Red (turn left) | Fast (~8 Hz) |
| Knob approaching from above | Red | Slowing as distance decreases |

Pulse rate formula:
```
distance = abs(knob_position - stored_value)    // 0.0 to 1.0
pulse_hz = fmap(distance, 0.0, 1.0, 0.5, 8.0)  // slow near, fast far
color    = (knob_position < stored_value) ? BLUE : RED
```

When the knob catches (distance < threshold, e.g., 0.02), the LED briefly flashes
solid green/white to confirm the catch, then displays the active channel color at
steady brightness.

### LED Allocation

Aurora has 11 RGB LEDs: 8 full RGB (LEDs 1-8) + 3 green/blue only (LEDs 9-11).

| LED | Purpose |
|-----|---------|
| LED 1 (RGB) | Channel indicator: active channel (Red=A, Green=B, Blue=C) — shared for Mix and Res |
| LED 2 (RGB) | Soft takeover direction/distance — shared for Mix and Res (both knobs use same channel) |
| LEDs 3-4 (RGB) | *Unassigned* — available for future use |
| LEDs 5-8 (RGB) | *Unassigned* — available for visual feedback (levels, activity, etc.) |
| LEDs 9-11 (G/B) | *Unassigned* — limited to green/blue only |

### Reserved Controls

The following controls are explicitly unassigned for future use:

- **CV_BLUR** — could modulate mix levels, LPF C cutoff, or other parameters
- **CV_WARP** — could modulate resonance, waveform, or pair ratio
- **SW_SHIFT** — could toggle waveforms, select pair ratio presets, or freeze state
- **GATE_FREEZE** — could trigger events, sync to external clock
- **GATE_REVERSE** — could trigger events, reset oscillator phase
- **LEDs 5-11** — could visualize oscillator beating, filter response, output levels

## Resolved Design Decisions

| # | Question | Decision | Rationale |
|---|----------|----------|-----------|
| 1 | LPF C cutoff tracking | 3×f₀ + 3×g₀ (sum of LPF A+B cutoffs) | Theoretically correct max upper sideband for Ring Mod C, whose inputs are band-limited by LPF A and LPF B. Empirically validated: 3× multiplier (vs 2×) preserves sum frequency in M2 testing. |
| 2 | Stereo strategy | Mono output | Same signal to both channels. Stereo panning deferred to future iteration. |
| 3 | Waveform | Pure sine | Cleanest ring mod sidebands, most faithful to Radigue aesthetic |
| 4 | Pair A:B ratio | Default 1:2 (80/160 Hz), easily tweakable constant | Starting point for experimentation. CVs set absolute pitch so ratio is implicit. |
| 5 | Filter type | DaisySP `Svf` (state-variable) | Clean/precise like ARP 2500 diode ladder. Multi-mode. Low CPU. Controllable resonance. |

## Remaining Open Questions

1. **Spread range calibration**: Maximum spread is "a few Hz" — exact range (±3 Hz? ±5 Hz?
   ±10 Hz?) to be determined through listening tests on hardware.

2. **Pitch input mode**: V/Oct (exponential, standard eurorack) vs. linear frequency?
   V/Oct is conventional for CV, but linear might be more intuitive for static knob control.

3. **Reserved control assignment**: CV_BLUR, CV_WARP, SW_SHIFT, GATE_FREEZE, GATE_REVERSE,
   LEDs 5-11 are all unassigned. Candidates: waveform switching, pair ratio presets,
   freeze, external clock sync, visual feedback.

---

**Last Updated**: 2026-03-14
