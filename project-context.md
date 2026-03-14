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
- **SDK**: Aurora SDK (https://github.com/qu-bit-electronix/Aurora-SDK)
- **Purpose**: Prove concept viability in hardware
- **Controls**: Knobs, CV inputs, audio I/O
- **Audio**: Stereo in/out with integrated audio codec

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

1. Architecture design - platform abstraction strategy
2. Aurora POC implementation - basic synthesis engine + controls
3. Code review for portability - identify Aurora-specific coupling
4. DPT port - validate architecture on different hardware
5. Custom hardware design & integration

---

## Next Steps

1. Define synthesis engine architecture (oscillators, filters, modulators, feedback loops)
2. Design platform abstraction layer (HAL)
3. Plan Aurora POC MVP scope
4. Set up initial codebase structure

---

**Last Updated**: 2026-03-14
