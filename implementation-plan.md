---
project: Eliane-DSP
type: Implementation Plan
status: Approved
target: Aurora POC
created: 2026-03-14
---

# Implementation Plan — Aurora POC

## Overview

Incremental implementation of the Eliane-DSP synthesis engine on Qu-Bit Aurora hardware.
Five milestones, each producing a deployable `.bin` testable on real hardware. Architecture
modeled after the Audrey II codebase layout (Synthux simple-designer-instruments).

**References**:
- `signal-path-design.md` — signal flow, control mapping, LED spec
- `architecture-decisions.md` — ADR-001 through ADR-003
- `skills/aurora-sdk.skill.md` — Aurora API and build system

---

## 1. Project Structure

Main entry point at project root, all DSP and HAL code in `Source/`. Follows the
Audrey II convention: main `.cpp` at root is glue/wiring, `Source/` contains all
reusable components.

```
eliane-dsp/
├── ElianeDSP_main.cpp              # Entry point: hardware init, audio callback, glue
├── Makefile                         # DFU-targeted build (BOOT_NONE, see ADR-004)
├── Source/
│   ├── Engine.h                     # DSP engine header (platform-agnostic)
│   ├── Engine.cpp                   # DSP engine implementation
│   ├── DaisySeedHal.h               # DaisySeed hardware abstraction header (replaces AuroraHal, see ADR-004)
│   ├── DaisySeedHal.cpp             # DaisySeed hardware abstraction implementation
│   ├── SoftTakeover.h               # Soft takeover state machine (header-only)
│   ├── CycledKnob.h                 # Cycled knob state machine (header-only)
│   └── Constants.h                  # Tuning constants, defaults, ranges
├── sdks/                            # Git submodules (Aurora-SDK, DaisySP, libDaisy, etc.)
├── skills/                          # Skill files (reference docs, not compiled)
├── signal-path-design.md
├── architecture-decisions.md
├── implementation-plan.md           # This file
└── project-context.md
```

---

## 2. Makefile

```makefile
TARGET = ElianeDSP
APP_TYPE = BOOT_NONE

CPP_SOURCES = \
    ElianeDSP_main.cpp \
    Source/Engine.cpp \
    Source/DaisySeedHal.cpp

AURORA_SDK_PATH = sdks/Aurora-SDK
C_INCLUDES += -I$(AURORA_SDK_PATH)/include/ -ISource/
LIBDAISY_DIR = $(AURORA_SDK_PATH)/libs/libDaisy/
DAISYSP_DIR = $(AURORA_SDK_PATH)/libs/DaisySP/

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
```

**Build**: `make` produces `build/ElianeDSP.bin`
**Deploy**: `make program-dfu` via Daisy Seed micro-USB (see ADR-004).

> **Note**: `APP_TYPE` is `BOOT_NONE` (not `BOOT_SRAM`) due to Aurora REV4 I2C hardware
> defect requiring DFU deploy. USB stick bootloader is not available after flashing
> `BOOT_NONE`. See ADR-004 for details.

---

## 3. Namespace Convention

All project code under `atelier::eliane`. Mirrors the Audrey II pattern of
`infrasonic::FeedbackSynth` — a parent namespace for the studio/author, a child
namespace for the instrument.

```cpp
namespace atelier {
namespace eliane {
    class Engine;      // DSP core
    class DaisySeedHal;// Hardware abstraction (DaisySeed direct, see ADR-004)
    class SoftTakeover;
    class CycledKnob;
}
}
```

---

## 4. Component Design

### 4.1 Constants (`Source/Constants.h` — header-only)

Centralized tuning constants. All values easily tweakable — change here, recompile,
redeploy.

```cpp
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

}  // namespace eliane
}  // namespace atelier
```

---

### 4.2 Engine (`Source/Engine.h` / `Source/Engine.cpp`)

**Platform-agnostic**. Only includes `<daisysp.h>` and `Constants.h`. Zero hardware
references. No Aurora SDK imports. Portable to DPT or custom hardware without changes.

#### Header

```cpp
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

    // Pitch: base frequency for each oscillator pair (Hz)
    void SetPitchA(float freq_hz);
    void SetPitchB(float freq_hz);

    // Spread: bipolar detuning of second oscillator in each pair (Hz)
    // Positive = osc2 higher, negative = osc2 lower, zero = unison
    void SetSpreadA(float delta_hz);
    void SetSpreadB(float delta_hz);

    // Mix levels: per-channel output level (0.0 - 1.0)
    // channel: 0=A (Ring Mod A), 1=B (Ring Mod B), 2=C (Cross Ring Mod)
    void SetMixLevel(int channel, float level);

    // Filter resonance: per-filter SVF resonance (0.0 - 1.0)
    // channel: 0=A, 1=B, 2=C
    void SetResonance(int channel, float res);

private:
    float sample_rate_;

    // --- DSP modules ---
    daisysp::Oscillator osc_[4];   // 0,1 = Pair A; 2,3 = Pair B
    daisysp::Svf lpf_[3];          // 0=A, 1=B, 2=C (LP mode)

    // --- Parameter targets (set by setters) ---
    float pitch_a_, pitch_b_;
    float spread_a_, spread_b_;
    float mix_level_[3];
    float resonance_[3];

    // --- Smoothed values (converge toward targets via fonepole) ---
    float pitch_a_smooth_, pitch_b_smooth_;
    float spread_a_smooth_, spread_b_smooth_;
    float mix_smooth_[3];
    float res_smooth_[3];

    // Non-copyable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
};

}  // namespace eliane
}  // namespace atelier
```

#### Process() Signal Flow (per sample)

```
 1. Smooth all parameters:  fonepole(pitch_a_smooth_, pitch_a_, coeff)  (×10 params)
 2. Set osc frequencies:    osc_[0] = pitch_a_smooth_
                            osc_[1] = pitch_a_smooth_ + spread_a_smooth_
                            osc_[2] = pitch_b_smooth_
                            osc_[3] = pitch_b_smooth_ + spread_b_smooth_
 3. Set filter cutoffs:     lpf_[0] = 2 × pitch_a_smooth_
                            lpf_[1] = 2 × pitch_b_smooth_
                            lpf_[2] = 2 × pitch_a_smooth_ + 2 × pitch_b_smooth_
 4. Set filter resonances:  lpf_[i].SetRes(res_smooth_[i])
 5. Generate osc samples:   s0 = osc_[0].Process()  ...  s3 = osc_[3].Process()
 6. Ring mod A:             rm_a = s0 × s1
 7. Ring mod B:             rm_b = s2 × s3
 8. Filter A:               lpf_[0].Process(rm_a)  →  lp_a = lpf_[0].Low()
 9. Filter B:               lpf_[1].Process(rm_b)  →  lp_b = lpf_[1].Low()
10. Ring mod C:             rm_c = lp_a × lp_b
11. Filter C:               lpf_[2].Process(rm_c)  →  lp_c = lpf_[2].Low()
12. Mix:                    out = lp_a × mix_smooth_[0]
                                + lp_b × mix_smooth_[1]
                                + lp_c × mix_smooth_[2]
13. Return out
```

**Note**: Setting osc frequencies and filter cutoffs every sample is intentional — the
parameter smoothing produces continuous changes, and both `Oscillator::SetFreq()` and
`Svf::SetFreq()` are cheap enough to call per-sample on Cortex-M7 @ 480 MHz.

---

### 4.3 SoftTakeover (`Source/SoftTakeover.h` — header-only)

State machine for preventing parameter jumps when cycling between channels.
When a knob switches to a new channel, it's "locked" — it has no effect until
the physical knob position crosses the stored parameter value.

```cpp
#pragma once
#include <cmath>
#include "Constants.h"

namespace atelier {
namespace eliane {

class SoftTakeover {
public:
    enum class State { Locked, Tracking };
    enum class Direction { Below, Above, Matched };

    void Init(float initial_value = 0.5f) {
        stored_value_ = initial_value;
        state_ = State::Locked;
    }

    // Call each audio block with current knob position (0.0 - 1.0).
    // Returns the effective parameter value (0.0 - 1.0).
    float Process(float knob_position) {
        if (state_ == State::Tracking) {
            stored_value_ = knob_position;
            return stored_value_;
        }

        // Locked: check if knob has crossed stored value
        if (fabsf(knob_position - stored_value_) < kSoftTakeoverThreshold) {
            state_ = State::Tracking;
            stored_value_ = knob_position;
        }

        // While locked, return the stored value (no change)
        return stored_value_;
    }

    // Freeze current value — called when cycling away from this channel
    void Lock() {
        state_ = State::Locked;
    }

    // Begin listening for knob crossing — called when cycling to this channel
    void Activate() {
        // Stays locked until knob physically crosses stored_value_
        state_ = State::Locked;
    }

    // --- Queries for LED feedback ---

    State GetState() const { return state_; }

    Direction GetDirection(float knob_position) const {
        float d = knob_position - stored_value_;
        if (fabsf(d) < kSoftTakeoverThreshold) return Direction::Matched;
        return (d < 0.0f) ? Direction::Below : Direction::Above;
    }

    float GetDistance(float knob_position) const {
        return fabsf(knob_position - stored_value_);
    }

    float GetStoredValue() const { return stored_value_; }

private:
    float stored_value_ = 0.5f;
    State state_ = State::Locked;
};

}  // namespace eliane
}  // namespace atelier
```

---

### 4.4 CycledKnob (`Source/CycledKnob.h` — header-only)

Manages cycling a single physical knob across 3 parameter channels, each with
independent soft takeover.

```cpp
#pragma once
#include "SoftTakeover.h"
#include "Constants.h"

namespace atelier {
namespace eliane {

class CycledKnob {
public:
    void Init(float default_value = 0.5f) {
        selected_ = 0;
        for (int i = 0; i < kNumChannels; i++) {
            takeover_[i].Init(default_value);
        }
        takeover_[0].Activate();
    }

    // Advance to next channel — call on button rising edge
    void Advance() {
        takeover_[selected_].Lock();
        selected_ = (selected_ + 1) % kNumChannels;
        takeover_[selected_].Activate();
    }

    // Process knob input — returns effective value for active channel (0.0 - 1.0)
    float Process(float knob_position) {
        return takeover_[selected_].Process(knob_position);
    }

    // Active channel index (0 = A, 1 = B, 2 = C)
    int GetSelected() const { return selected_; }

    // Access active channel's soft takeover state (for LED feedback)
    const SoftTakeover& GetCurrentTakeover() const {
        return takeover_[selected_];
    }

    // Access stored value for any channel
    float GetStoredValue(int channel) const {
        return takeover_[channel].GetStoredValue();
    }

private:
    int selected_ = 0;
    SoftTakeover takeover_[kNumChannels];
};

}  // namespace eliane
}  // namespace atelier
```

---

### 4.5 DaisySeedHal (`Source/DaisySeedHal.h` / `Source/DaisySeedHal.cpp`)

**Replaces `AuroraHal`** due to Aurora REV4 I2C hardware defect (see ADR-004).
Uses `daisy::DaisySeed` directly instead of `aurora::Hardware`. Configures ADC and
GPIO pins manually using the REV4 pin mappings. No I2C initialization — LEDs are
inaccessible.

**The only file that includes `daisy_seed.h`** (besides `main.cpp`). Reads hardware
controls, manages cycled knobs, maps raw values to engine parameters via direct
setter calls.

#### Header

```cpp
#pragma once
#include "daisy_seed.h"
#include "Engine.h"
#include "CycledKnob.h"

namespace atelier {
namespace eliane {

class DaisySeedHal {
public:
    DaisySeedHal() = default;

    // Configures ADC channels and GPIO pins for Aurora REV4 controls.
    // Must be called after seed.Init().
    void Init(daisy::DaisySeed& seed, Engine& engine);

    // Call once per audio block — reads ADC/GPIO, updates engine parameters
    void ProcessControls(daisy::DaisySeed& seed, Engine& engine);

    // Placeholder — LEDs blocked due to I2C defect (ADR-004).
    // Retained for API compatibility; will drive PCA9685 when hardware is repaired.
    void UpdateLeds(daisy::DaisySeed& seed);

private:
    CycledKnob mix_knob_;
    CycledKnob res_knob_;

    // Debounced switch state
    daisy::Switch sw_freeze_;
    daisy::Switch sw_reverse_;

    // Mapping helpers
    float MapPitch(float knob_val);       // fmap LOG: kMinPitchHz..kMaxPitchHz
    float MapSpread(float knob_val);      // bipolar center-zero: ±kMaxSpreadHz
    float MapResonance(float raw_val);    // 0.0..0.9 (avoid self-oscillation)
};

}  // namespace eliane
}  // namespace atelier
```

#### Control Mapping (Aurora REV4 via DaisySeed)

| Daisy Pin | Aurora Equivalent | Mapped To                           | Mapping Function              |
|-----------|-------------------|-------------------------------------|-------------------------------|
| `seed::A0`  | `KNOB_TIME`      | `engine.SetPitchA(freq)`            | `fmap(val, 20, 2000, LOG)`   |
| `seed::A1`  | `KNOB_REFLECT`   | `engine.SetPitchB(freq)`            | `fmap(val, 20, 2000, LOG)`   |
| `seed::A8`  | `KNOB_MIX`       | `engine.SetSpreadA(delta)`          | `(val - 0.5) * 2 * kMaxSpreadHz` |
| `seed::A10` | `KNOB_ATMOSPHERE` | `engine.SetSpreadB(delta)`         | `(val - 0.5) * 2 * kMaxSpreadHz` |
| `seed::A11` | `KNOB_BLUR`      | `engine.SetMixLevel(ch, level)`     | via `CycledKnob` + soft takeover |
| `seed::A9`  | `KNOB_WARP`      | `engine.SetResonance(ch, res)`      | via `CycledKnob` + soft takeover |
| D1          | `SW_FREEZE`      | `mix_knob_.Advance()`              | on `RisingEdge()`             |
| D9          | `SW_REVERSE`     | `res_knob_.Advance()`              | on `RisingEdge()`             |

See ADR-004 for full REV4 pin reference table.

---

### 4.6 Main Entry Point (`ElianeDSP_main.cpp`)

Minimal glue. Owns static instances, wires audio callback, starts the system.
Uses `DaisySeed` directly — no Aurora `Hardware` class (see ADR-004).

```cpp
#include "daisy_seed.h"
#include "daisysp.h"
#include "Engine.h"
#include "DaisySeedHal.h"

using namespace atelier::eliane;

static daisy::DaisySeed hw;
static Engine engine;
static DaisySeedHal hal;

void AudioCallback(daisy::AudioHandle::InputBuffer in,
                   daisy::AudioHandle::OutputBuffer out,
                   size_t size) {
    hal.ProcessControls(hw, engine);

    for (size_t i = 0; i < size; i++) {
        float sample = engine.Process();
        out[0][i] = sample;   // Left  (mono)
        out[1][i] = sample;   // Right (mono — same signal, see ADR-003)
    }
}

int main(void) {
    hw.Init();
    engine.Init(hw.AudioSampleRate());
    hal.Init(hw, engine);
    hw.StartAudio(AudioCallback);

    while (1) {
        hal.UpdateLeds(hw);  // No-op until I2C hardware repaired (ADR-004)
    }
}
```

---

## 4.7 Parameter Interface Contract

Defines the boundary between AuroraHal (or any future HAL) and Engine. Engine
accepts and clamps all values. HAL is responsible for mapping knob ranges to
musically useful subranges.

| Setter | Unit | Valid Range | Engine Clamping | HAL Mapping (Aurora) | Update Rate |
|--------|------|-------------|-----------------|----------------------|-------------|
| `SetPitchA(freq)` | Hz | 20.0 - 2000.0 | Clamp to range | `fmap(knob, 20, 2000, LOG)` | Block rate |
| `SetPitchB(freq)` | Hz | 20.0 - 2000.0 | Clamp to range | `fmap(knob, 20, 2000, LOG)` | Block rate |
| `SetSpreadA(delta)` | Hz | -5.0 to +5.0 | Clamp to ±kMaxSpreadHz | `(knob - 0.5) * 2 * kMaxSpreadHz` | Block rate |
| `SetSpreadB(delta)` | Hz | -5.0 to +5.0 | Clamp to ±kMaxSpreadHz | `(knob - 0.5) * 2 * kMaxSpreadHz` | Block rate |
| `SetMixLevel(ch, level)` | normalized | 0.0 - 1.0 | Clamp to 0-1 | Direct (via CycledKnob) | Block rate |
| `SetResonance(ch, res)` | normalized | 0.0 - 1.0 | Clamp to 0-1 | Maps knob to 0.0-0.9 (UX choice to avoid self-oscillation) | Block rate |

**Resonance note**: Engine accepts full 0.0-1.0 range. The HAL maps to 0.0-0.9 as a
UX decision. If self-oscillation is desired later, change the HAL mapping — the Engine
doesn't need to change.

**Smoothing**: All parameters are smoothed per-sample inside `Engine::Process()` via
`fonepole()`. HAL sends raw (unsmoothed) values at block rate (~500 Hz).

---

## 4.8 Real-Time Constraints

These rules apply to all milestones. Violation of any rule is a bug.

1. **No heap allocation** (`new`, `malloc`, `std::vector`, `std::string`) in the audio
   callback or any code called from it. All DSP objects are statically allocated.
2. **No blocking I/O** (I2C, SPI, UART, USB) in the audio callback. LED writes
   (when I2C is available) run in the `main()` loop, not the callback.
3. **Bounded per-sample operations**: No data-dependent loops, no recursion, no
   unbounded iteration inside `Engine::Process()`. Every code path has a fixed,
   deterministic cost.
4. **Block deadline**: `Engine::Process()` × block size (96 samples) plus control
   processing must complete within the block period (96 / 48000 = 2ms).
5. **Control-rate policy**: Control input reads (`ProcessControls`, soft takeover
   evaluation, button debounce) run at block rate (~500 Hz). Parameter smoothing
   (`fonepole`) runs at sample rate (48 kHz) inside `Engine::Process()`.

---

## 5. Milestones

### M1: Single Oscillator → Audio Out ✅ COMPLETE

**Goal**: Prove the toolchain, Makefile, DFU deploy, and audio output all work.
Get a sine tone playing through the Daisy Seed on the Aurora board.

**Files created**:
- `Makefile` (minimal — only `ElianeDSP_main.cpp` in `CPP_SOURCES`, `BOOT_NONE` for DFU)
- `ElianeDSP_main.cpp` (minimal: one `daisysp::Oscillator` → stereo out, using `DaisySeed` directly)
- `Source/Constants.h` (just `kDefaultPitchA`, `kMinPitchHz`, `kMaxPitchHz`)

**Engine class is NOT created yet** — oscillator lives directly in `main.cpp`.
This is intentionally throwaway scaffolding. The goal is proving hardware/build
before investing in architecture.

**Key discovery during M1**: Aurora REV4 I2C bus is defective — PCA9685 LED driver
initialization locks up the device. Switched from `aurora::Hardware` to `DaisySeed`
direct. See ADR-004 for details.

**Acceptance criteria**:
- [x] `make` produces `build/ElianeDSP.bin` without errors or warnings
- [x] Binary size < 256 KB
- [x] Flashing via `make program-dfu` and powering Aurora produces an audible 80 Hz sine tone
- [ ] ~~`KNOB_TIME` changes the pitch~~ — Deferred to M2 (M1 uses fixed frequency to isolate audio path from control path)

---

### M2: One Pair + Ring Mod + Filter (Core Sound)

**Goal**: Hear the fundamental Radigue sound — two near-unison sines ring-modulating,
producing slow beating, through a tracking LPF. This is the **artistic validation
milestone**: if this doesn't sound right, we adjust before building further.

**Files created**:
- `Source/Engine.h` / `Source/Engine.cpp` — 2 oscillators, 1 ring mod, 1 SVF, mono output
- Update `Source/Constants.h` — add spread constants

**Files modified**:
- `ElianeDSP_main.cpp` — refactored to use `Engine` class
- `Makefile` — add `Source/Engine.cpp` to `CPP_SOURCES`

**Temporary control mapping (M2 only — via DaisySeed ADC, see ADR-004)**:
| Daisy Pin | Aurora Equiv. | Function |
|-----------|--------------|----------|
| `seed::A0` | `KNOB_TIME` | Pitch A (base frequency) |
| `seed::A8` | `KNOB_MIX` | Spread A (through-zero detuning) |
| `seed::A9` | `KNOB_WARP` | LPF A resonance |

**Acceptance criteria**:
- [ ] `seed::A0` (KNOB_TIME) controls base frequency of both oscillators
- [ ] `seed::A8` (KNOB_MIX) controls spread — audible slow beating at small values (~1-2 Hz)
- [ ] Through-zero behavior: center position = perfect unison (no beating)
- [ ] LPF tracks at 2 × pitch, cleaning up the output
- [ ] `seed::A9` (KNOB_WARP) sweeps filter resonance (audible peak)
- [ ] Sound is recognizably "Radigue" — slow pulsing drones
- [ ] No audible zipper noise when sweeping spread continuously
- [ ] CPU load < 20% (measured via `CpuLoadMeter` or block timing)

**Open question to resolve during M2**: Is ±5 Hz the right max spread? Adjust
`kMaxSpreadHz` by ear.

---

### M3: Full Signal Path (Both Pairs + Cross-Mod + Mixer)

**Goal**: Complete synthesis engine — 4 oscillators, 3 ring mods, 3 filters,
3-channel mixer. All parameter setters exposed and functional.

**Files modified**:
- `Source/Engine.h` / `Source/Engine.cpp` — expand to full architecture:
  - 4 × `Oscillator` (WAVE_SIN)
  - 3 × ring mod (multiplication)
  - 3 × `Svf` (LP mode, tracking cutoffs, per-filter resonance)
  - 3-channel mono mixer with per-channel gain
- `Source/Constants.h` — add Pitch B default
- `ElianeDSP_main.cpp` — map all 4 direct knobs

**Temporary control mapping (M3 — before cycled knobs, via DaisySeed ADC)**:
| Daisy Pin | Aurora Equiv. | Function |
|-----------|--------------|----------|
| `seed::A0` | `KNOB_TIME` | Pitch A |
| `seed::A1` | `KNOB_REFLECT` | Pitch B |
| `seed::A8` | `KNOB_MIX` | Spread A |
| `seed::A10` | `KNOB_ATMOSPHERE` | Spread B |
| `seed::A11` | `KNOB_BLUR` | Mix (all 3 channels equally — temporary) |
| `seed::A9` | `KNOB_WARP` | Resonance (all 3 filters equally — temporary) |

**Acceptance criteria**:
- [ ] Two independent oscillator pairs audible with separate pitch control
- [ ] Cross-ring-mod (Ring Mod C) producing additional tonal complexity
- [ ] LPF C tracking at `2×pitchA + 2×pitchB`
- [ ] All 4 direct knobs affecting their respective parameters
- [ ] `seed::A11` (KNOB_BLUR) fading all 3 channels uniformly
- [ ] `seed::A9` (KNOB_WARP) sweeping all 3 filters uniformly
- [ ] Different Pair A / Pair B pitches produce clearly different beating patterns
- [ ] CPU load < 30% (full signal path)
- [ ] No audible zipper noise on any parameter sweep

**Portability check** (non-blocking): Verify `Engine.cpp` compiles without Aurora SDK
headers on the include path to catch accidental platform coupling.

---

### M4: DaisySeed HAL + Cycled Knobs + Soft Takeover

**Goal**: Full control surface. Extract hardware-specific code into `DaisySeedHal`.
Implement cycled knobs for independent mix/resonance control per channel.

> **Note**: Originally planned as `AuroraHal` wrapping `aurora::Hardware`. Changed to
> `DaisySeedHal` using `daisy::DaisySeed` directly due to Aurora REV4 I2C defect
> (ADR-004). The HAL configures ADC channels and GPIO pins manually using known REV4
> pin mappings. LED feedback is deferred to M5 (blocked on I2C repair).

**Files created**:
- `Source/DaisySeedHal.h` / `Source/DaisySeedHal.cpp`
- `Source/SoftTakeover.h`
- `Source/CycledKnob.h`

**Files modified**:
- `ElianeDSP_main.cpp` — simplified to thin glue (as shown in section 4.6)
- `Makefile` — add `Source/DaisySeedHal.cpp` to `CPP_SOURCES`

**Acceptance criteria**:
- [ ] `SW_FREEZE` (D1) cycles mix channel: A → B → C → A
- [ ] `SW_REVERSE` (D9) cycles resonance filter: A → B → C → A
- [ ] Soft takeover prevents audible parameter jumps when cycling
- [ ] Stored values persist when cycling away from and back to a channel
- [ ] Each mix channel independently controllable via `seed::A11` (KNOB_BLUR)
- [ ] Each filter resonance independently controllable via `seed::A9` (KNOB_WARP)
- [ ] All 10 parameters (4 direct + 3 mix + 3 resonance) independently controllable
- [ ] Engine has zero Aurora SDK imports (verified: `grep -r "aurora" Source/Engine.*` returns nothing)
- [ ] DaisySeedHal has zero `aurora.h` imports (verified: `grep -r "aurora" Source/DaisySeedHal.*` returns nothing)
- [ ] No audible parameter jumps when cycling channels (soft takeover working)

---

### M5: LED Feedback System — ⛔ BLOCKED (I2C Hardware Defect)

**Status**: Blocked — requires functional I2C bus for PCA9685 LED drivers.
Qu-Bit has been contacted about repair/replacement. This milestone is fully designed
and ready to implement when hardware is available. See ADR-004.

**Goal**: Visual feedback for channel selection and soft takeover direction.
Makes the cycled knob system actually usable.

**Files modified**:
- `Source/AuroraHal.cpp` — implement `UpdateLeds()` method
- `Source/Constants.h` — LED pulse rate constants (already defined)

**LED allocation** (LEDs 1-4 of Aurora's 11):

| LED | Purpose | Colors |
|-----|---------|--------|
| LED 1 (RGB) | Mix: active channel indicator | Red=A, Green=B, Blue=C |
| LED 2 (RGB) | Mix: soft takeover direction | Blue=turn right, Red=turn left |
| LED 3 (RGB) | Resonance: active channel indicator | Red=A, Green=B, Blue=C |
| LED 4 (RGB) | Resonance: soft takeover direction | Blue=turn right, Red=turn left |
| LEDs 5-11 | Unassigned | Reserved for future use |

**Soft takeover LED behavior**:

| Knob State | LED Color | Pulse Rate |
|------------|-----------|------------|
| Knob far below stored value | Blue (turn right) | Fast (~8 Hz) |
| Knob approaching from below | Blue | Slowing (rate ∝ distance) |
| Knob catches stored value | Solid green flash → channel color | — |
| Knob is live-tracking | Solid channel color | Steady |
| Knob far above stored value | Red (turn left) | Fast (~8 Hz) |
| Knob approaching from above | Red | Slowing (rate ∝ distance) |

**Pulse rate formula**:
```
distance = abs(knob_position - stored_value)     // 0.0 to 1.0
pulse_hz = fmap(distance, 0.0, 1.0, 0.5, 8.0)   // slow when near, fast when far
color    = (knob_position < stored_value) ? BLUE : RED
```

**Acceptance criteria**:
- [ ] Channel indicator LEDs show correct color immediately on button press
- [ ] Soft takeover LEDs pulse blue (knob below stored) or red (knob above stored)
- [ ] Pulse rate increases visibly with distance from stored value
- [ ] LEDs go solid green/white briefly when knob catches, then steady channel color
- [ ] LEDs show solid channel color while knob is live-tracking
- [ ] LED updates run in main loop (not audio callback) to avoid timing jitter

---

## 6. Milestone Dependency Graph

```
M1 ──► M2 ──► M3 ──► M4 ──► M5
 ✅                           ⛔ BLOCKED (I2C)

M1: Prove build/deploy             ✅ COMPLETE
M2: Core sound validation          (hours)
M3: Complete synthesis              (hours)
M4: Full control surface           (day)
M5: LED polish                     ⛔ BLOCKED — awaiting I2C hardware repair
```

Each milestone is independently deployable and testable on hardware.
All milestones use DaisySeed directly (no `aurora::Hardware`) — see ADR-004.

---

## 7. Architecture Decisions Specific to This Plan

| Decision | Rationale |
|----------|-----------|
| Audrey II layout (main at root, `Source/`) | Proven pattern in the Daisy/Synthux ecosystem |
| Engine uses setter methods | Simple, no heap allocation, each setter can smooth internally |
| Direct setter calls (no ParameterRegistry) | Avoids `std::bind`, `std::unordered_map` on embedded — zero heap use |
| Parameter smoothing via `fonepole()` in Engine | Cheapest smoothing — single multiply+add per param per sample. DaisySP-native. |
| SoftTakeover + CycledKnob as header-only | Small classes (~40 lines each), no `.cpp` needed, easy to test/reuse |
| M1 skips Engine class | Fastest path to proving build/deploy works — refactored in M2 |
| AuroraHal → DaisySeedHal | Aurora REV4 I2C defect (ADR-004). DaisySeed direct access to ADC/GPIO. No LED support until hardware repaired. |
| `atelier::eliane` namespace | Room for future instruments under `atelier::` |
| DaisySeedHal is the only file importing `daisy_seed.h` (besides main) | Engine has zero hardware coupling — portable to DPT or custom HAL |
| LEDs blocked; `UpdateLeds()` is a no-op | M5 design preserved for when I2C is repaired. HAL API stays stable. |

---

## 8. What This Plan Does NOT Cover (Deferred)

These are explicitly deferred — not forgotten, just not part of the first 5 milestones.

| Item | Why Deferred |
|------|-------------|
| CV input handling (attenuverter mode) | Knobs work standalone first. CV adds complexity. |
| V/Oct calibration | Requires Aurora's `GetWarpVoct()` + `PersistentStorage`. Later. |
| Spread range calibration | Determined by ear during M2. `kMaxSpreadHz` is a constant. |
| Reserved controls (CV5, CV6, SW_SHIFT, gates) | No assigned function yet. |
| LEDs 5-11 | Available for visualization. No spec yet. |
| DPT port | Validates architecture, but only after Aurora POC is solid. |
| Stereo panning | Mono for now (ADR-003). |
| Waveform switching | Pure sine only (ADR-003). May revisit. |
| Pair ratio presets | Default 1:2 is a constant. Could be a control later. |
| Feedback paths | Considered in ADR-001. Deferred to post-POC iteration. |

---

## 9. Continuation Prompt

For use when resuming implementation in a new session:

```
Load the eliane-dsp project:
- /Users/adamleedy/claude/projects/eliane-dsp/project-context.md
- /Users/adamleedy/claude/projects/eliane-dsp/signal-path-design.md
- /Users/adamleedy/claude/projects/eliane-dsp/architecture-decisions.md
- /Users/adamleedy/claude/projects/eliane-dsp/implementation-plan.md

Load skill files:
- /Users/adamleedy/claude/projects/eliane-dsp/skills/aurora-sdk.skill.md
- /Users/adamleedy/claude/projects/eliane-dsp/skills/daisysp.skill.md
- /Users/adamleedy/claude/projects/eliane-dsp/skills/libdaisy.skill.md

SDKs are cloned at: /Users/adamleedy/claude/projects/eliane-dsp/sdks/
  Aurora-SDK/, DaisySP/, libDaisy/, dpt/, simple-designer-instruments/

CURRENT STATUS: M1 complete. Using DaisySeed directly (not aurora::Hardware) due to
I2C hardware defect — see ADR-004. DFU deploy via `make program-dfu`. M5 blocked
pending I2C repair. Qu-Bit contacted.

TASK: [describe what to work on next]
```

---

**Last Updated**: 2026-03-15 (rev 3 — DaisySeed direct approach, ADR-004, M1 complete, M5 blocked)
