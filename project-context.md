---
project: Eliane-DSP
status: Active
created: 2026-03-14
---

# Eliane-DSP Project Context

**Project Name**: Eliane-DSP  
**Status**: New codebase - POC phase  
**Code Repository**: Not yet created  
**Repository**: TBD  

---

## Project Overview

Audio DSP synthesis firmware for Daisy-based hardware, implementing a complex audio source patch inspired by Eliane Radigue's works. Multi-platform approach: POC on Aurora hardware, eventual ports to dsp.coffee DPT and custom hardware.

**Key Philosophy**: Platform-agnostic architecture. Code must be portable across Daisy variants from day one - no direct ties to Aurora SDK.

## Hardware Targets

### Phase 1: POC (Aurora)
- **Device**: Qu-Bit Aurora (Daisy Seed-based eurorack reverb platform)
- **Revision**: REV4 (Daisy Seed 2 DFM with built-in codec)
- **SDK**: Aurora SDK (https://github.com/qu-bit-electronix/Aurora-SDK)
- **Purpose**: Prove concept viability in hardware
- **Controls**: Knobs, CV inputs, audio I/O
- **Audio**: Stereo in/out with integrated audio codec
- **⚠️ Hardware Issue**: I2C bus defective — PCA9685 LED drivers lock up during init.
  Using `DaisySeed` directly instead of Aurora `Hardware` class.
  Knobs/switches/gates accessible via direct ADC/GPIO. LEDs inaccessible.
  Qu-Bit contacted for repair/replacement. See ADR-004.
- **Deploy**: DFU via `make program-dfu` (USB stick bootloader not available with `BOOT_NONE`)

### Phase 2: Port to dsp.coffee DPT (optional)
- **Device**: dsp.coffee DPT (Daisy Patch Submodule-based multifunction module)
- **SDK**: dpt SDK (https://github.com/joemisra/dpt)
- **Hardware**: 8 pots + 8 CV inputs, 2 gate in/out, stereo audio, MIDI, SD card, 4x12-bit CV outputs
- **Status**: Beginnings of SDK available

### Phase 3: Custom Hardware
- **Goal**: Design and build custom Daisy-based hardware optimized for Eliane-DSP
- **Platform**: Daisy-based (maintains libDaisy + DaisySP compatibility)
- **Advantage**: Full control over I/O, form factor, and integration while staying within Daisy ecosystem

## Tech Stack

- **Language**: C++
- **Base Platform**: Daisy ecosystem (all three targets)
  - libDaisy (hardware abstraction)
  - DaisySP (DSP library with 1.1k+ stars, comprehensive modules)
- **Build System**: Make/GCC ARM toolchain

### DaisySP Modules Available
**Control & Modulation**: AD/ADSR envelopes, Phasor, LFO  
**Synthesis**: Oscillators, FM synthesis  
**Filters**: One-pole lowpass/highpass, FIR, SOAP, state-variable options  
**Noise**: Whitenoise, Clocked Noise, Dust, Fractal Noise  
**Effects**: Phaser, Wavefolder, Decimate, Overdrive, Crossfade  
**Utilities**: Math functions, Signal conditioning, DCBlocker  

*Perfect foundation for complex modulation synthesis inspired by Eliane Radigue's work.*

## Team

- **Lead**: Adam

## Architectural Constraints

1. **Platform-Agnostic Core**: Audio DSP logic must not depend on Aurora, DPT, or any specific hardware
2. **Hardware Abstraction Layer**: Single HAL for different Daisy boards
3. **Minimal Dependencies**: Avoid framework lock-in; rely primarily on libDaisy + DaisySP
4. **Reusability**: Code structure enables easy porting to new hardware

## Inspiration

- **Artist**: Eliane Radigue
- **Musical Context**: Complex, evolving audio synthesis via modulation and feedback

## Key Milestones

1. ~~Architecture design — platform abstraction strategy~~ ✅
2. Aurora POC implementation — ~~basic synthesis engine + controls~~ M1 complete (DaisySeed direct), M2-M4 pending
3. Code review for portability — identify Aurora-specific coupling
4. DPT port — validate architecture on different hardware
5. Custom hardware design & integration

---

## Next Steps

1. **M2**: Build Engine class with one oscillator pair + ring mod + filter — core sound validation
2. **M3**: Expand Engine to full signal path (both pairs, cross-mod, mixer)
3. **M4**: Build DaisySeedHal — configure ADC/GPIO for all knobs/switches, implement cycled knobs
4. Resolve Qu-Bit I2C repair — unblocks M5 (LED feedback)

---

**Last Updated**: 2026-03-15
