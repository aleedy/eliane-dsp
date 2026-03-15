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

### ADR-004: DaisySeed Direct — Aurora REV4 I2C Hardware Defect Workaround

**Date**: 2026-03-15  
**Status**: Accepted  
**Context**: Aurora REV4 unit has a defective I2C bus. Initializing the Aurora `Hardware`
class calls `ConfigureLeds()`, which initializes I2C_1 at 1 MHz and attempts to
communicate with two PCA9685 LED driver chips (addresses `0x40`, `0x41`). This locks up
the device during `Init()` — firmware never reaches the audio callback.

The I2C bus on Aurora carries only:
- 2x PCA9685 LED drivers (11 RGB LEDs) — **both revisions**
- PCM3060 audio codec — **REV3 only** (REV4/DFM has built-in codec, no I2C needed)

All other controls are on direct Daisy Seed pins:
- **6 knobs**: individual ADC pins (REV4: A0, A1, A8-A11)
- **6 CV inputs**: individual ADC pins (REV4: A2-A7), bipolar -5V to +5V
- **3 switches**: GPIO with internal pullup (D1, D9, D13)
- **2 gate inputs**: GPIO (D26, D14)

Since the REV4's audio codec is built-in (initialized by `seed.Init()`), I2C is only
needed for LEDs on this hardware revision.

**Decision**: Bypass `aurora::Hardware` entirely. Use `daisy::DaisySeed` directly for
all hardware initialization and audio. Configure ADC and GPIO pins manually using the
REV4 pin mappings extracted from `aurora.h`. LEDs are inaccessible until hardware is
repaired or replaced.

**Rationale**:
- `DaisySeed::Init()` brings up clocks, SDRAM, QSPI, and the built-in audio codec — no I2C
- Knobs, switches, and gates are all on direct ADC/GPIO pins — no I2C required to read them
- The Engine class is already platform-agnostic (depends only on DaisySP) — unaffected
- This approach was validated in M1: sine oscillator plays correctly through DaisySeed

**Deployment change**: `APP_TYPE` changed from `BOOT_SRAM` to `BOOT_NONE`. This means
firmware is flashed via DFU (`make program-dfu`) instead of the USB stick bootloader.
The USB stick process itself doesn't depend on I2C, but after flashing a `BOOT_NONE`
binary the bootloader is overwritten and USB stick deploy is no longer available.
DFU is the standard deploy method going forward.

**Consequences**:
- M5 (LED feedback) is **blocked** until I2C hardware is repaired or unit is replaced
  - Qu-Bit has been contacted about a replacement/repair
  - M5 remains fully designed and ready to implement when hardware is available
- M4 HAL class becomes `DaisySeedHal` instead of `AuroraHal` — configures ADC/GPIO
  directly using known REV4 pin mappings rather than wrapping `aurora::Hardware`
- Cycled knobs and soft takeover logic are unchanged — they operate on normalized
  0.0-1.0 knob values regardless of how those values are read
- No USB stick deploy — DFU only (acceptable for development; DPT and custom
  hardware will have their own deploy methods)
- The Daisy Seed's single onboard GPIO LED remains available for basic status indication

**REV4 Pin Reference** (extracted from `aurora.h`):

| Control | Type | Daisy Seed Pin | Notes |
|---------|------|---------------|-------|
| KNOB_TIME | ADC | `seed::A0` (D15) | Pitch A |
| KNOB_REFLECT | ADC | `seed::A1` (D16) | Pitch B |
| KNOB_MIX | ADC | `seed::A8` (D23) | Spread A |
| KNOB_WARP | ADC | `seed::A9` (D24) | Filter Res (cycled) |
| KNOB_ATMOSPHERE | ADC | `seed::A10` (D25) | Spread B |
| KNOB_BLUR | ADC | `seed::A11` (D28) | Mix Level (cycled) |
| CV_ATMOSPHERE | ADC | `seed::A2` (D17) | Bipolar |
| CV_TIME | ADC | `seed::A3` (D18) | Bipolar |
| CV_MIX | ADC | `seed::A4` (D19) | Bipolar |
| CV_REFLECT | ADC | `seed::A5` (D20) | Bipolar |
| CV_BLUR | ADC | `seed::A6` (D21) | Bipolar |
| CV_WARP | ADC | `seed::A7` (D22) | Bipolar |
| SW_FREEZE | GPIO | D1 (PORTC, 11) | Pullup, active-low |
| SW_REVERSE | GPIO | D9 (PORTB, 4) | Pullup, active-low |
| SW_SHIFT | GPIO | D13 (PORTB, 6) | Pullup, active-low |
| GATE_FREEZE | GPIO | D26 (PORTD, 11) | REV4 only |
| GATE_REVERSE | GPIO | D14 (PORTB, 7) | Both revisions |

**Alternatives Considered**:
- Selective Aurora init (skip `ConfigureLeds()` only): Aurora `Hardware::Init()` is
  monolithic — can't skip individual subsystems without modifying the SDK source
- Fork Aurora SDK to remove LED init: adds maintenance burden, coupling to forked SDK
- Use only the Daisy Seed onboard LED: insufficient for soft takeover feedback

---

**Last Updated**: 2026-03-15
