---
project: Eliane-DSP
type: Architecture Decisions
---

# Architecture Decisions - Eliane-DSP

## Decisions Log

### ADR-001: Dual-Pair Ring Modulation Signal Path

**Date**: 2026-03-14  
**Status**: Accepted  
**Context**: Needed to define the core synthesis architecture inspired by Eliane Radigue's
closely-tuned oscillator techniques. Radigue used near-unison oscillator pairs to produce
slow beating patterns, combined with ring modulation and filtering for spectral complexity.  
**Decision**: Four oscillators in two pairs, each pair feeding a ring modulator, with a third
ring modulator cross-modulating the two pair outputs. Three tracking low-pass filters shape
the sidebands. Three-channel mixer produces the final output. See `signal-path-design.md`
for the full signal flow diagram.  
**Rationale**: 
- Paired oscillators with fine spread control directly implements Radigue's beating technique
- Ring modulation (multiplication) produces sum/difference frequencies — clean spectral results with sine waves
- The third cross-ring-mod adds harmonic complexity between the two pairs
- Tracking LPFs (cutoff = 2×pitch) naturally follow the upper sidebands
- Through-zero spread enables smooth transition through unison
**Alternatives Considered**:
- Single pair with more oscillators: less structured, harder to control
- AM instead of ring mod: DC offset, less interesting spectral content
- No cross-modulation (Ring Mod C): simpler but loses the inter-pair interaction
- Feedback paths: considered for later iteration, not in initial design
**Consequences**: 
- 7 ideal control parameters (4 osc + 3 mix) — exceeds Aurora's 6 knobs/CVs
- Hardware mapping requires creative solutions (see open questions)
- CPU budget: 4 oscillators + 3 multiplications + 3 filters + 3-ch mix — well within STM32H750 capacity
- LPF C cutoff tracking strategy still unresolved

---

### ADR-002: Cycled Parameter Knobs with Soft Takeover

**Date**: 2026-03-14  
**Status**: Accepted  
**Context**: The synthesis engine requires 7 primary parameters (4 osc controls + 3 mix levels)
plus 3 filter resonance values = 10 total controllable parameters. Aurora has only 6 knobs
and 6 CV inputs. Need a way to control more parameters than physical knobs allow.  
**Decision**: Use two "cycled" knobs — each paired with a momentary button that cycles through
3 targets. KNOB_BLUR cycles through Mix A/B/C levels (via SW_FREEZE). KNOB_WARP cycles through
LPF A/B/C resonance (via SW_REVERSE). Stored values persist when cycling away.
Soft takeover prevents audible parameter jumps: the knob has no effect until it physically
crosses the stored value. Directional LED feedback (inspired by Eventide Blackhole) indicates
which direction to turn and how far away the stored value is — blue = turn right, red = turn
left, pulse rate proportional to distance. See `signal-path-design.md` for full LED spec.  
**Rationale**:
- Fits 10 parameters into 6 knobs without sacrificing independent control
- Soft takeover prevents audible jumps — critical for Radigue-style music where any discontinuity is exposed
- LED feedback makes the system usable despite the indirection
- Mix levels and resonance are the least frequently adjusted parameters, making them good candidates for cycling
- Pitch and spread remain direct-mapped (always live) since they are the primary performance controls
**Alternatives Considered**:
- Shared spread knob (both pairs use same spread): loses independent control
- No mix CV control: chosen — mix levels are knob-only in this design
- Shift-mode (hold button to access alternate function): harder to set-and-forget
- Page-based UI (button toggles all knobs between two banks): too disorienting
**Consequences**:
- Mix levels are not CV-modulatable in this hardware mapping (acceptable for POC; DPT has enough CVs)
- Resonance is not CV-modulatable in this hardware mapping
- Requires LED feedback system for usability — adds UI complexity
- Soft takeover adds state machine complexity to the HAL layer
- 2 free CVs, 1 free switch, 2 free gates remain for future assignment

---

### ADR-003: DSP Module and Output Decisions

**Date**: 2026-03-14  
**Status**: Accepted  
**Context**: Multiple design decisions needed resolution before implementation can begin:
filter type, waveform, output mode, LPF C tracking, and pair pitch ratio.  
**Decision**:
1. **Filter type**: DaisySP `Svf` (state-variable filter) — clean, precise, ARP 2500-like character.
   Low CPU. Multi-mode (LP used, HP/BP/Notch/Peak available for future tapping).
2. **Waveform**: Pure sine (WAVE_SIN) — cleanest ring modulation sidebands, most faithful
   to Radigue's aesthetic. Produces only sum and difference frequencies with no additional harmonics.
3. **Output**: Mono — same signal to both stereo channels. Stereo panning deferred.
4. **LPF C cutoff**: Auto-tracked at 2×f₀ + 2×g₀ (sum of LPF A and LPF B cutoffs).
   Mathematically correct: Ring Mod C's inputs are band-limited by LPF A (passing up to 2×f₀)
   and LPF B (passing up to 2×g₀), so the max upper sideband is their sum.
5. **Pair ratio**: Default 1:2 octave (80 Hz / 160 Hz). Defined as an easily-tweakable constant.
   CV inputs set absolute pitch per pair, so the ratio is implicit when CV is connected.  
**Rationale**:
- Svf over MoogLadder: ARP 2500 used diode ladder (clean/precise), not Moog ladder (warm/saturated)
- Svf over Tone: need resonance control (Tone has no resonance parameter)
- Sine over triangle/saw: ring mod of two sines produces exactly two sidebands (sum and difference).
  Non-sinusoidal waves produce many more sidebands that can muddy the Radigue aesthetic.
- Mono: simplifies initial implementation. The sound is inherently mono (all sources sum to one path).
- LPF C tracking via sum: avoids needing another knob/CV. Mathematically grounded.
- 1:2 ratio as default: octave relationship is the most fundamental. Easy to hear the beating and
  cross-mod effects clearly. Other ratios (2:3, 3:4, 1:1) listed for experimentation.
**Alternatives Considered**:
- MoogLadder: warmer but less ARP-accurate, LGPL license
- LadderFilter: highest quality but 4x oversampled = CPU cost × 3 filters
- Switchable waveforms: more flexible but adds complexity, pure sine is the right starting point
- Stereo with panning: adds parameters without clear musical benefit at this stage
- Independent LPF C cutoff: would need a knob/CV, already constrained
**Consequences**:
- Pure sine means Ring Mod output is mathematically simple — easy to reason about
- Svf resonance at high values will add tonal color without the self-oscillation "scream" of Moog filters
- Mono output means we don't need to think about stereo field yet
- Default pitch ratio is a constant — changing it requires recompile (until we assign a control to it)

---

**Last Updated**: 2026-03-14
