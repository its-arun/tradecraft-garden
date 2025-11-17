# Tradecraft Garden - Knowledge Repository

A comprehensive documentation of Raphael Mudge's Tradecraft Garden research, focusing on position-independent code loaders, in-memory evasion, and offensive security tradecraft.

## 📚 About This Repository

This repository documents the groundbreaking research by Raphael Mudge (creator of Cobalt Strike) on advanced red team techniques, particularly focusing on position-independent capability loaders for in-memory evasion.

**Source Material:**
- [Tradecraft Garden](https://tradecraftgarden.org/)
- [AFF-WG Blog Posts](https://aff-wg.org/)

## 🎯 Core Philosophy

The Tradecraft Garden project embodies the principle of **"leverage"** - developing technologies that grant individual researchers productivity advantages for rapid hypothesis testing and adaptation, rather than secretive proprietary approaches.

## 📖 Source Material Timeline

### Tradecraft Garden (Raphael Mudge)
1. **June 4, 2025** - [Planting a Tradecraft Garden](https://aff-wg.org/2025/06/04/planting-a-tradecraft-garden/) - Initial project release
2. **July 9, 2025** - [Tilling the Soil](https://aff-wg.org/2025/07/09/tradecraft-garden-tilling-the-soil/) - Binary Transform and COFF support
3. **September 10, 2025** - [COFFing Out the Night Soil](https://aff-wg.org/2025/09/10/coffing-out-the-night-soil) - COFF normalization, merge, export
4. **October 13, 2025** - [Weeding the Tradecraft Garden](https://aff-wg.org/2025/10/13/weeding-the-tradecraft-garden/) - PIC ergonomics and shared libraries
5. **October 27, 2025** - [PIC Parterre](https://aff-wg.org/2025/10/27/tradecraft-gardens-pic-parterre/) - DFR revisited, fixbss, remap
6. **November 10, 2025** - [Aspect-Oriented Programming](https://aff-wg.org/2025/11/10/tradecraft-engineering-with-aspect-oriented-programming/) - PIC/PICO instrumentation, exports

### Community Research (Rasta Mouse)
7. **October 28, 2025** - [Arranging the PIC Parterre](https://rastamouse.me/arranging-the-pic-parterre/) - Services PICO pattern
8. **October 12, 2025** - [Crystal Kit](https://rastamouse.me/crystal-kit/) - Crystal Palace toolkit
9. **September 14, 2025** - [Crystal Palace API](https://rastamouse.me/crystal-palace-api/) - Programmatic usage
10. **July 25, 2025** - [Debugging the Tradecraft Garden](https://rastamouse.me/debugging-the-tradecraft-garden/) - Development techniques
11. **December 2024** - [UDRL, SleepMask, and BeaconGate](https://rastamouse.me/udrl-sleepmask-and-beacongate/) - Cobalt Strike evasion

### EDR Evasion Research (Offsec Almond)
12. [Evading Elastic Callstack Signatures](https://offsec.almond.consulting/evading-elastic-callstack-signatures.html) - Call stack spoofing

## 🔧 Major Techniques Covered

### [Crystal Palace](./techniques/crystal-palace/)
A specialized linker and linker script language designed for PIC DLL loaders. Enables sophisticated binary transformation, resource linking, and capability assembly.

### [Position Independent Code (PIC)](./techniques/position-independent-code/)
Techniques for creating code that runs regardless of where it's loaded in memory - essential for in-memory evasion and reflective loading.

### [PICO (Position Independent Capability Objects)](./techniques/pico/)
A convention similar to BOFs (Beacon Object Files) for executing one-time or persistent COFFs from position-independent code.

### [COFF Operations](./techniques/coff-operations/)
Working with Common Object File Format - normalization, merging, and exporting capabilities for smaller, more flexible payloads.

### [Dynamic Function Resolution (DFR)](./techniques/dynamic-function-resolution/)
Advanced techniques for resolving Windows API functions at runtime without import tables, using hash-based and string-based approaches.

### [Binary Transformation Framework](./techniques/binary-transformation/)
Disassemble, modify, and reassemble programs with features like optimization, function randomization, and code mutation.

### [Aspect-Oriented Programming](./techniques/aspect-oriented-tradecraft/)
Applying AOP principles to separate tradecraft from capability code through instrumentation, hooks, and pointcuts.

### [Shared Libraries](./techniques/shared-libraries/)
ZIP-based shared library architecture for PIC/PICO with conventions for code reuse and link-time optimization.

### [UDRL and Sleep Mask](./techniques/udrl-sleepmask/)
User-Defined Reflective Loaders, Sleep Masks, and BeaconGate for Cobalt Strike - advanced in-memory evasion and custom PE loading.

### [EDR Evasion - Call Stack Spoofing](./techniques/edr-evasion/call-stack-spoofing/)
Gadget-based call stack injection to evade EDR signature detection, specifically targeting Elastic Security patterns.

### [Services PICO Pattern](./techniques/pic-organization/services-pattern/)
Architectural pattern for organizing PIC code with centralized shared functionality and dual-resolver design.

### [Crystal Palace API](./techniques/crystal-palace-api/)
Programmatic usage of Crystal Palace for dynamic specification generation and automated capability building.

### [Tradecraft Debugging](./techniques/tradecraft-debugging/)
Development and debugging techniques for Tradecraft Garden projects using WSL, VS Code, and WinDbg.

## 🎓 Learning Path

**For Beginners:**
1. Start with [Position Independent Code basics](./techniques/position-independent-code/)
2. Understand [Dynamic Function Resolution](./techniques/dynamic-function-resolution/)
3. Learn about [COFF format and operations](./techniques/coff-operations/)

**For Intermediate:**
1. Deep dive into [Crystal Palace](./techniques/crystal-palace/)
2. Explore [PICOs](./techniques/pico/) and their use cases
3. Study [Binary Transformation techniques](./techniques/binary-transformation/)

**For Advanced:**
1. Master [Aspect-Oriented Tradecraft](./techniques/aspect-oriented-tradecraft/)
2. Study [UDRL and Sleep Mask](./techniques/udrl-sleepmask/) for Beacon evasion
3. Learn [EDR Evasion techniques](./techniques/edr-evasion/call-stack-spoofing/)
4. Build custom [Shared Libraries](./techniques/shared-libraries/)
5. Apply [Services PICO Pattern](./techniques/pic-organization/services-pattern/)
6. Use [Crystal Palace API](./techniques/crystal-palace-api/) for automation
7. Master [Debugging techniques](./techniques/tradecraft-debugging/)

## 🛠️ Repository Structure

Each technique folder contains:
- `README.md` - Overview and quick reference
- `NOTES.md` - Bite-sized, quick reference notes
- `DETAILED_EXPLANATION.md` - Comprehensive technical explanation
- `POC/` - Proof of concept code and examples
- `ADVANCEMENTS.md` - Potential improvements and future research
- `RED_TEAM_USAGE.md` - Practical red team applications

## ⚠️ Disclaimer

This repository is for educational purposes, security research, authorized penetration testing, and defensive security work only. All techniques documented here should be used in accordance with applicable laws and with proper authorization.

## 🙏 Credits

### Primary Research
- **Raphael Mudge** - Creator of Tradecraft Garden, Crystal Palace, and Cobalt Strike
- **Tradecraft Garden Project** - Core research and tooling

### Community Contributors
- **Rasta Mouse** - Services PICO pattern, Crystal Palace API usage, debugging techniques, UDRL/Sleep Mask integration
- **Offsec Almond Consulting** - EDR evasion research (call stack spoofing)

This repository serves as an educational companion to help security researchers understand and apply these advanced techniques.

## 📚 Additional Resources

### Official Sites
- [Tradecraft Garden Official Site](https://tradecraftgarden.org/)
- [Tradecraft Garden Videos](https://tradecraftgarden.org/videos.html)
- [Release Notes](https://tradecraftgarden.org/releasenotes.txt)
- [AFF-WG Blog](https://aff-wg.org/) - Raphael Mudge's blog

### Community Resources
- [Rasta Mouse Blog](https://rastamouse.me/) - Advanced tradecraft techniques
- [Offsec Almond Consulting](https://offsec.almond.consulting/) - EDR evasion research
- [RESOURCES.md](./RESOURCES.md) - Comprehensive resource list with community projects

### Community Projects
- [LibTP](https://github.com/rasta-mouse/LibTP) - Thread Pool API proxying
- [execute-assembly-pico](https://github.com/ofasgard/execute-assembly-pico) - CLR hosting PICO

---

## 📊 Repository Statistics

- **14 Major Techniques** documented
- **12 Source Blog Posts** analyzed
- **50+ Documentation Files** created
- **15,000+ Lines** of comprehensive documentation
- **Multiple POC Examples** with working code
- **Complete Coverage** of Tradecraft Garden ecosystem

---

**Last Updated:** 2025-11-17
**Curator:** Community-driven documentation project
**Total Commits:** 5 comprehensive documentation commits
