# Tradecraft Garden - Knowledge Repository

A comprehensive documentation of Raphael Mudge's Tradecraft Garden research, focusing on position-independent code loaders, in-memory evasion, and offensive security tradecraft.

## 📚 About This Repository

This repository documents the groundbreaking research by Raphael Mudge (creator of Cobalt Strike) on advanced red team techniques, particularly focusing on position-independent capability loaders for in-memory evasion.

**Source Material:**
- [Tradecraft Garden](https://tradecraftgarden.org/)
- [AFF-WG Blog Posts](https://aff-wg.org/)

## 🎯 Core Philosophy

The Tradecraft Garden project embodies the principle of **"leverage"** - developing technologies that grant individual researchers productivity advantages for rapid hypothesis testing and adaptation, rather than secretive proprietary approaches.

## 📖 Blog Post Timeline

1. **June 4, 2025** - [Planting a Tradecraft Garden](https://aff-wg.org/2025/06/04/planting-a-tradecraft-garden/) - Initial project release
2. **July 9, 2025** - [Tilling the Soil](https://aff-wg.org/2025/07/09/tradecraft-garden-tilling-the-soil/) - Binary Transform and COFF support
3. **September 10, 2025** - [COFFing Out the Night Soil](https://aff-wg.org/2025/09/10/coffing-out-the-night-soil) - COFF normalization, merge, export
4. **October 13, 2025** - [Weeding the Tradecraft Garden](https://aff-wg.org/2025/10/13/weeding-the-tradecraft-garden/) - PIC ergonomics and shared libraries
5. **October 27, 2025** - [PIC Parterre](https://aff-wg.org/2025/10/27/tradecraft-gardens-pic-parterre/) - DFR revisited, fixbss, remap
6. **November 10, 2025** - [Aspect-Oriented Programming](https://aff-wg.org/2025/11/10/tradecraft-engineering-with-aspect-oriented-programming/) - PIC/PICO instrumentation, exports

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
2. Build custom [Shared Libraries](./techniques/shared-libraries/)
3. Combine techniques for complete evasion solutions

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

All research and techniques documented here are based on the work of **Raphael Mudge** and the Tradecraft Garden project. This repository serves as an educational companion to help security researchers understand and apply these advanced techniques.

## 📚 Additional Resources

- [Tradecraft Garden Official Site](https://tradecraftgarden.org/)
- [Tradecraft Garden Videos](https://tradecraftgarden.org/videos.html)
- [Release Notes](https://tradecraftgarden.org/releasenotes.txt)

---

**Last Updated:** 2025-11-17
**Curator:** Community-driven documentation project
