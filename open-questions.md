---
project: Eliane-DSP
type: Open Questions
---

# Open Questions - Eliane-DSP

## Resolved Questions

### Platform Abstraction Layer Design
**Status**: Resolved (see `implementation-plan.md` section 4.5, 4.7)  
**Resolution**: Engine class with setter methods, DaisySeedHal as sole hardware consumer
(originally AuroraHal — renamed due to ADR-004). Composition over inheritance. No abstract
interface class — Engine takes float params, HAL reads hardware and calls setters directly.
Parameter interface contract defined in section 4.7 of implementation plan.

---

### Synthesis Engine Architecture
**Status**: Resolved (see `signal-path-design.md`, `architecture-decisions.md`)  
**Resolution**: 4 oscillators (DaisySP `Oscillator`, WAVE_SIN) in 2 pairs, 3 ring mods
(multiplication), 3 DaisySP `Svf` filters (LP mode, tracking cutoffs), 3-channel mono
mixer. Smoothing via `fonepole()`. Full signal flow documented in `signal-path-design.md`.
ADR-001 through ADR-003 capture waveform, filter, and output decisions.

---

### Control Mapping Strategy
**Status**: Resolved (see `implementation-plan.md` section 4.5)  
**Resolution**: 4 direct knobs (Pitch A/B, Spread A/B), 2 cycled knobs (Mix levels,
Filter resonance) with button cycling and soft takeover. 10 parameters mapped to 6
knobs + 2 buttons. No preset system for POC. No MIDI for POC.

---

### Build & Deployment
**Status**: Resolved (see `implementation-plan.md` sections 1, 2)  
**Resolution**: Audrey II layout pattern. Makefile targeting Aurora SDK with
`CPP_SOURCES` listing all source files. `BOOT_SRAM` app type. Deploy via USB drive.
Source tree organized by layer (`Source/Engine.cpp` = DSP, `Source/AuroraHal.cpp` = HAL).
SDKs as git submodules in `sdks/`.

---

### Eliane Radigue Synthesis Specifics
**Status**: Resolved (see `signal-path-design.md`)  
**Resolution**: Near-unison oscillator pairs with through-zero spread for slow beating,
ring modulation for sum/difference sidebands, tracking LPFs (2×f₀) to shape spectral
content. Cross-ring-modulation between pairs for harmonic complexity. Pure sine
waveforms for cleanest sidebands. Svf filter for ARP 2500-like character.

---

## Remaining Open Questions

### Aurora REV4 I2C Hardware Repair
**Status**: Open — awaiting Qu-Bit response  
**Priority**: Medium (blocks M5 only)  
**Blocker**: Yes — blocks M5 (LED feedback)  
**When**: When Qu-Bit responds with repair/replacement path  
**Context**: Aurora REV4 unit has defective I2C bus. PCA9685 LED driver initialization
locks up the device during `Init()`. All other controls (knobs, switches, gates) work
fine via direct ADC/GPIO. Qu-Bit has been contacted. See ADR-004.  
**Options**: Repair current unit, receive replacement, or defer LEDs to DPT/custom hardware.

---

### Spread Range Calibration
**Status**: Open  
**Priority**: Medium  
**Blocker**: No  
**When**: During M2 (artistic validation milestone)  
**Context**: `kMaxSpreadHz` is set to ±5 Hz. Actual sweet spot determined by ear on
hardware. At 80 Hz, 2 Hz spread = ~2 Hz beating. May need ±3 Hz or ±10 Hz.

---

### Pitch Input Mode
**Status**: Open  
**Priority**: Low  
**Blocker**: No  
**When**: Post-POC (when CV input handling is added)  
**Context**: V/Oct (exponential, standard eurorack) vs. linear frequency for CV input.
V/Oct is conventional but linear may be more intuitive for direct knob control.
Currently knobs use logarithmic mapping via `fmap(val, 20, 2000, LOG)`.

---

### Reserved Control Assignments
**Status**: Open  
**Priority**: Low  
**Blocker**: No  
**When**: Post-POC  
**Context**: CV_BLUR, CV_WARP, SW_SHIFT, GATE_FREEZE, GATE_REVERSE, LEDs 5-11 are
all unassigned. Candidates: waveform switching, pair ratio presets, freeze, external
clock sync, visual feedback.

---

**Last Updated**: 2026-03-15
