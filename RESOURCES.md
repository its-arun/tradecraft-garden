# Tradecraft Garden - Additional Resources

## Official Resources

### Primary Sources
- **Tradecraft Garden Website**: [tradecraftgarden.org](https://tradecraftgarden.org/)
- **AFF-WG Blog**: [aff-wg.org](https://aff-wg.org/) - Raphael Mudge's blog with detailed technical posts
- **Video Tutorials**: [tradecraftgarden.org/videos.html](https://tradecraftgarden.org/videos.html)
- **Release Notes**: [tradecraftgarden.org/releasenotes.txt](https://tradecraftgarden.org/releasenotes.txt)

### Blog Posts (Chronological)

1. **June 4, 2025** - [Planting a Tradecraft Garden](https://aff-wg.org/2025/06/04/planting-a-tradecraft-garden/)
   - Initial project announcement
   - Crystal Palace introduction
   - PICO convention overview

2. **July 9, 2025** - [Tilling the Soil](https://aff-wg.org/2025/07/09/tradecraft-garden-tilling-the-soil/)
   - Binary Transform Framework
   - COFF as-a-capability support
   - +optimize, +disco, +mutate transformations

3. **September 10, 2025** - [COFFing Out the Night Soil](https://aff-wg.org/2025/09/10/coffing-out-the-night-soil)
   - COFF normalization
   - COFF merge operations
   - COFF export functionality

4. **October 13, 2025** - [Weeding the Tradecraft Garden](https://aff-wg.org/2025/10/13/weeding-the-tradecraft-garden/)
   - PIC ergonomics improvements
   - Shared libraries architecture
   - DFR enhancements
   - fixptrs for x86

5. **October 27, 2025** - [PIC Parterre](https://aff-wg.org/2025/10/27/tradecraft-gardens-pic-parterre/)
   - DFR revisited (multi-resolver pattern)
   - fixbss for global variables
   - Symbol remapping

6. **November 10, 2025** - [Aspect-Oriented Programming](https://aff-wg.org/2025/11/10/tradecraft-engineering-with-aspect-oriented-programming/)
   - PIC/PICO instrumentation
   - attach and redirect commands
   - PICO exports
   - Hook protection mechanisms

## Community Projects

### Crystal Palace-Based Projects

**LibTP** - Threadpool API Proxying Library
- **Repository**: [github.com/rasta-mouse/LibTP](https://github.com/rasta-mouse/LibTP)
- **Description**: Crystal Palace library for proxying Nt API calls via the Threadpool
- **Author**: Rasta Mouse
- **Use Case**: EDR evasion through syscall proxying

**execute-assembly-pico** - CLR Hosting PICO
- **Repository**: [github.com/ofasgard/execute-assembly-pico](https://github.com/ofasgard/execute-assembly-pico)
- **Description**: PICO that implements CLR hosting to execute .NET assemblies in memory
- **Author**: ofasgard
- **Use Case**: Execute .NET payloads from unmanaged code

### Related Research

**Harvesting the Tradecraft Garden - Part 1**
- **URL**: [rastamouse.me/harvesting-the-tradecraft-garden/](https://rastamouse.me/harvesting-the-tradecraft-garden/)
- **Author**: Rasta Mouse
- **Description**: Practical walkthrough of using Crystal Palace and Tradecraft Garden
- **Topics**: Building PIC loaders, PICO capabilities, practical examples

## Learning Resources

### Video Courses
- **Tradecraft Garden Amphitheater**: Official video course on Win32 evasion tradecraft
  - Location: [tradecraftgarden.org/videos.html](https://tradecraftgarden.org/videos.html)
  - Topics: PIC development, Crystal Palace usage, advanced techniques

### Related Courses (Community)
- **SEKTOR7 Windows Malware Development**: Covers PIC, reflective loading, and evasion
  - Mentions Crystal Palace in recent updates
  - Focuses on practical malware development

## Background Knowledge

### Position Independent Code
- **Intel Manual**: Software Developer's Manual (Volume 1-3)
- **Microsoft PE Format**: [Microsoft PE/COFF Specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)
- **PEB Walking**: Understanding Process Environment Block structure

### Dynamic Function Resolution
- **ROR13 Hash**: Originally from Metasploit's windows/exec payload
- **Export Address Table**: Windows PE export directory structure
- **PEB/LDR Structures**: Undocumented Windows structures for module enumeration

### Reflective Loading
- **Stephen Fewer's Reflective DLL Injection**: Classic research on reflective loading
- **User-Defined Reflective Loaders (UDRLs)**: Cobalt Strike feature that inspired aspects of Tradecraft Garden

### Beacon Object Files (BOFs)
- **Cobalt Strike BOF Guide**: Understanding the BOF convention
- **COFF Loader**: How BOFs are loaded and executed
- **Tradecraft Garden's PICO**: Evolution of the BOF concept for universal use

## Tools and Utilities

### Compilers
- **MinGW-w64**: Cross-platform Windows compiler
  - x86_64-w64-mingw32-gcc (x64 targets)
  - i686-w64-mingw32-gcc (x86 targets)
- **MSVC**: Microsoft Visual C++ compiler
- **Clang**: Alternative compiler with good PIC support

### Analysis Tools
- **objdump**: View COFF/PE structure, symbols, disassembly
- **dumpbin**: Windows tool for PE/COFF analysis
- **x64dbg/WinDbg**: Debugging PIC and analyzing execution
- **IDA Pro/Ghidra**: Reverse engineering for understanding techniques

### Hash Generators
- **ROR13 Calculator**: Generate hashes for API resolution
  - Implementation available in POC examples
  - Online calculators available

## Social Media and Updates

### Twitter/X Accounts
- **@SEKTOR7net**: Regular updates on malware development, mentions Crystal Palace
- **@Raph_Mudge**: Raphael Mudge (check for occasional updates)
- **@_RastaMouse**: Rasta Mouse - Active in red team community
- **@JustHackingHQ**: Malware development courses and updates

## Development Environment Setup

### Required Tools
```bash
# Install MinGW-w64
sudo apt install mingw-w64

# Install build tools
sudo apt install build-essential

# Install analysis tools
sudo apt install binutils
```

### Crystal Palace Setup
1. Download from tradecraftgarden.org
2. Extract to preferred location
3. Add to PATH or use full path
4. Test with example specifications

## Code Repositories for Learning

While Crystal Palace itself may not be open-source on GitHub, these repositories demonstrate related techniques:

### Reflective Loading
- Search GitHub for "reflective dll injection"
- Look for implementations of reflective loaders

### PIC Shellcode
- Search for "position independent shellcode"
- Study shellcode development repositories

### API Hashing
- Search for "api hashing windows"
- ROR13 implementations

## Recommended Reading Order

### For Beginners
1. Start with "Planting a Tradecraft Garden" blog post
2. Read Position Independent Code fundamentals
3. Study Dynamic Function Resolution
4. Learn COFF format basics
5. Experiment with simple PIC examples

### For Intermediate
1. Deep dive into Crystal Palace documentation
2. Study Binary Transformation Framework
3. Learn PICO development
4. Explore Aspect-Oriented Programming concepts
5. Build modular capabilities

### For Advanced
1. Study multi-resolver patterns
2. Master link-time optimization
3. Implement custom shared libraries
4. Develop aspect libraries
5. Create full capability frameworks

## Community and Discussion

### Forums and Channels
- **Red Team Discord Servers**: Many discuss advanced techniques
- **Offensive Security Forums**: Discussion of tradecraft
- **MalwareTech Discord**: Malware analysis and development

### Conferences
- **DEF CON**: Red team village, malware development talks
- **Black Hat**: Advanced offensive security research
- **BSides**: Local security conferences with practical content

## Contributing

### To This Repository
This repository is a community documentation project for Tradecraft Garden. Contributions welcome:
- Additional POC examples
- Technique clarifications
- Use case documentation
- Bug fixes and improvements

### To Tradecraft Garden Ecosystem
- Develop shared libraries
- Create PICO capabilities
- Share aspect libraries
- Write tutorials and guides

## Staying Updated

### Follow These Sources
1. **aff-wg.org** - Primary source for updates
2. **tradecraftgarden.org** - Official project site
3. **Twitter/X** - @SEKTOR7net for practical applications
4. **GitHub** - Watch community projects using Crystal Palace

### Release Notes
Check [tradecraftgarden.org/releasenotes.txt](https://tradecraftgarden.org/releasenotes.txt) for:
- New features
- Bug fixes
- Breaking changes
- Example updates

## Legal and Ethical Considerations

### Authorized Use Only
- Security research
- Authorized penetration testing
- Red team engagements
- Defensive security (understanding attacker techniques)
- Educational purposes

### Prohibited Uses
- Unauthorized access to computer systems
- Malicious software distribution
- Criminal activities
- Violation of computer fraud laws

### Compliance
Always ensure compliance with:
- Local and federal laws
- Organizational policies
- Contractual agreements
- Ethical guidelines

## Credits

All techniques and tools documented here are based on research by:
- **Raphael Mudge** - Creator of Tradecraft Garden and Crystal Palace
- **Tradecraft Garden Community** - Contributors and researchers
- **Red Team Community** - Ongoing research and development

---

**Last Updated**: 2025-11-17

For the latest information, always check official sources at tradecraftgarden.org and aff-wg.org.
