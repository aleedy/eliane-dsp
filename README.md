# Eliane-DSP

Audio DSP synthesis firmware for Daisy-based eurorack hardware, inspired by the work of Eliane Radigue.

Closely-tuned oscillator pairs produce slow beating patterns through ring modulation and filtering — glacially evolving drones built from pure sine waves.

## The Idea

Eliane Radigue's synthesis technique centers on near-unison oscillator pairs. When two sine waves are tuned almost identically, their interaction produces slow amplitude pulsations (beating) at a rate equal to their frequency difference. Ring modulation extracts these beating patterns as clean sum and difference tones, and low-pass filtering shapes the resulting sidebands.

Eliane-DSP implements this technique as a portable firmware:

- **4 sine oscillators** arranged in 2 pairs, each with independent pitch and fine spread control
- **3 ring modulators** — one per pair, plus a cross-modulation stage between the two pairs
- **3 tracking low-pass filters** (state-variable) with per-filter resonance control
- **3-channel mixer** for blending the three ring mod outputs

The spread parameter is the core musical control: at center position both oscillators in a pair are in perfect unison. Small detuning (1-5 Hz) produces the characteristic Radigue slow pulsation.

## Signal Path

```
Pair A:  Osc1(f) + Osc2(f+spread) --> Ring Mod A --> LPF A --> Mix Ch.1
Pair B:  Osc3(g) + Osc4(g+spread) --> Ring Mod B --> LPF B --> Mix Ch.2
Cross:   LPF A out * LPF B out    --> Ring Mod C --> LPF C --> Mix Ch.3

Mix Ch.1 + Ch.2 + Ch.3 --> Mono Output
```

Filter cutoffs auto-track the oscillator pitches: LPF A = 2x pitch A, LPF B = 2x pitch B, LPF C = 2x pitch A + 2x pitch B.

## Architecture

The DSP engine is **platform-agnostic** — it depends only on [DaisySP](https://github.com/electro-smith/DaisySP) and [libDaisy](https://github.com/electro-smith/libDaisy), with no direct hardware coupling. A separate hardware abstraction layer (HAL) handles platform-specific control mapping.

> **Note**: Due to an I2C hardware defect on the Aurora REV4 unit, we bypass `aurora::Hardware`
> entirely and use `daisy::DaisySeed` directly. All controls (knobs, switches, gates) are
> accessible via direct ADC/GPIO pins. Only the LEDs (PCA9685 I2C drivers) are inaccessible.
> See ADR-004 in [Architecture Decisions](architecture-decisions.md).

```
eliane-dsp/
├── ElianeDSP_main.cpp          # Entry point: hardware init, audio callback
├── Makefile                     # DFU-targeted build (BOOT_NONE)
├── Source/
│   ├── Constants.h              # Tuning constants, defaults, ranges
│   ├── Engine.h / Engine.cpp    # Platform-agnostic DSP engine (M2+)
│   ├── DaisySeedHal.h / .cpp    # DaisySeed hardware abstraction (M4+)
│   ├── SoftTakeover.h           # Knob catch state machine (M4+)
│   └── CycledKnob.h            # Multi-parameter knob cycling (M4+)
└── sdks/                        # Git submodules
    ├── Aurora-SDK/              # Qu-Bit Aurora platform SDK
    ├── DaisySP/                 # Electrosmith DSP library
    ├── libDaisy/                # Electrosmith hardware abstraction
    ├── dpt/                     # dsp.coffee DPT platform SDK
    └── simple-designer-instruments/  # Synthux reference projects
```

Key design decisions:

- **Pure sine oscillators** — cleanest ring mod sidebands, faithful to the source aesthetic
- **Engine with setter methods** — HAL calls `SetPitchA()`, `SetSpreadA()`, etc. at block rate; Engine smooths parameters per-sample via `fonepole()`
- **No heap allocation** in the audio path — all DSP objects statically allocated
- **`atelier::eliane` namespace** — room for future instruments under `atelier::`
- **DaisySeed direct** — bypasses Aurora HAL due to I2C defect (ADR-004); all controls read via direct ADC/GPIO

## Hardware Target: Qu-Bit Aurora

The initial proof-of-concept targets the [Qu-Bit Aurora](https://www.qubitelectronix.com/shop/aurora), a Daisy Seed-based eurorack module. The Aurora provides 6 knobs (normalled to CV), 3 momentary switches, 2 gate inputs, and 11 RGB LEDs.

> **Hardware limitation**: The current Aurora REV4 unit has a defective I2C bus. The PCA9685
> LED drivers lock up during initialization. We bypass `aurora::Hardware` and use `DaisySeed`
> directly — knobs, switches, and gates all work via direct ADC/GPIO. LEDs are unavailable
> until the hardware is repaired. See ADR-004.

### Control Mapping

| Control | Function | Notes |
|---------|----------|-------|
| KNOB_TIME | Pitch A | 20-2000 Hz, logarithmic |
| KNOB_REFLECT | Pitch B | 20-2000 Hz, logarithmic |
| KNOB_MIX | Spread A | Bipolar, through-zero, +/- few Hz |
| KNOB_ATMOSPHERE | Spread B | Bipolar, through-zero, +/- few Hz |
| KNOB_BLUR | Mix level (cycled) | SW_FREEZE cycles through channels A/B/C |
| KNOB_WARP | Filter resonance (cycled) | SW_REVERSE cycles through filters A/B/C |

Cycled knobs use **soft takeover** to prevent parameter jumps when switching channels, with LED feedback indicating direction and distance to the stored value.

### Future Targets

The platform-agnostic Engine design supports porting to:

- **dsp.coffee DPT** — 8 pots + 8 CV inputs, enough for all parameters without cycling
- **Custom Daisy-based hardware** — optimized panel layout for this instrument

## Building

### Prerequisites

- [GNU ARM Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)

### Clone and Build

```bash
git clone --recurse-submodules https://github.com/aleedy/eliane-dsp.git
cd eliane-dsp

# Build SDK libraries (first time only)
make -C sdks/Aurora-SDK/libs/libDaisy/
make -C sdks/Aurora-SDK/libs/DaisySP/

# Build firmware
make
```

This produces `build/ElianeDSP.bin`.

### Deploy to Aurora

Firmware is deployed via DFU over the Daisy Seed's micro-USB port:

```bash
# Put Daisy into DFU mode (hold BOOT, press RESET, release BOOT)
make program-dfu
```

> **Note**: USB stick deployment is not available. The firmware uses `APP_TYPE = BOOT_NONE`
> (required for DFU), which overwrites the Daisy bootloader. See ADR-004 for context.

## Development Roadmap

| Milestone | Description | Status |
|-----------|-------------|--------|
| **M1** | Single oscillator — prove build/deploy pipeline | **Complete** |
| **M2** | One pair + ring mod + filter — core sound validation | Pending |
| **M3** | Full signal path — both pairs, cross-mod, mixer | Pending |
| **M4** | DaisySeed HAL + cycled knobs + soft takeover | Pending |
| **M5** | LED feedback system | **Blocked** (I2C defect) |

Each milestone produces a deployable binary testable on hardware. M2 is the **artistic validation milestone** — the sound must be recognizably Radigue (slow beating drones from near-unison sine ring modulation) before proceeding further.

## Planning Documents

- [Signal Path Design](signal-path-design.md) — full signal flow, control mapping, LED spec
- [Architecture Decisions](architecture-decisions.md) — ADR-001 through ADR-003
- [Implementation Plan](implementation-plan.md) — component design, milestone details, code templates
- [Project Context](project-context.md) — hardware targets, tech stack, team
- [Open Questions](open-questions.md) — resolved and remaining design questions

## License

TBD
