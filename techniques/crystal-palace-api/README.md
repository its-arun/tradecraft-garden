# Crystal Palace API - Programmatic Usage

## Overview

Crystal Palace provides a Java API for programmatic capability generation, enabling dynamic specification creation and PIC building without pre-existing specification files.

## Core API

### LinkSpec Class

**Primary interface** for working with specification files:

```java
import crystalpalace.LinkSpec;
import crystalpalace.CrystalUtils;
import java.util.HashMap;

// Parse specification file
var spec = LinkSpec.Parse("/path/to/spec.txt");

// Load capability
var capability = CrystalUtils.readFromFile("/path/to/capability.dll");

// Build PIC
var args = new HashMap<String, Object>();
var pic = spec.run(capability, args);

// Write output
CrystalUtils.writeToFile(pic, "/path/to/output.bin");
```

**Automatic Detection**:
- Detects DLL vs COFF automatically
- Populates HashMap with `$DLL` or `$OBJECT`
- Runs appropriate processing method

## Advanced API

### SpecParser Class

**Undocumented but public** - accepts raw specification content:

```java
import crystalpalace.SpecParser;

// Create parser
var parser = new SpecParser();

// Parse raw specification string
String specContent = """
x64:
  push $LOADER
  make pic
  push $COFF0
  merge
  make object +optimize +disco +mutate
""";

parser.parse(specContent, "dynamicSpec.txt");

// Get compiled spec
var spec = parser.getSpec();

// Build for specific architecture
var args = new HashMap<String, Object>();
args.put("$LOADER", loaderBytes);
args.put("$COFF0", coffBytes);

var pic = spec.buildPic("x64", args);
```

## Programmatic Specification Generation

### Dynamic COFF Merger Example

```java
public class CoffMerger {
    public byte[] mergeCoffs(byte[] loader, List<byte[]> coffs) {
        // Build specification dynamically
        StringBuilder sb = new StringBuilder();
        sb.append("x64:\n");
        sb.append("push $LOADER\n");
        sb.append("make pic\n");

        // Add each COFF
        for (int i = 0; i < coffs.size(); i++) {
            sb.append("push $COFF").append(i).append("\n");
            sb.append("make coff\n");
            sb.append("merge\n");
        }

        // Final processing
        sb.append("make object +optimize +disco +mutate\n");

        // Parse specification
        var parser = new SpecParser();
        parser.parse(sb.toString(), "merger.spec");
        var spec = parser.getSpec();

        // Populate arguments
        var args = new HashMap<String, Object>();
        args.put("$LOADER", loader);

        for (int i = 0; i < coffs.size(); i++) {
            args.put("$COFF" + i, coffs.get(i));
        }

        // Build
        return spec.buildPic("x64", args);
    }
}

// Usage
CoffMerger merger = new CoffMerger();
byte[] loader = Files.readAllBytes(Path.of("loader.o"));
List<byte[]> coffs = List.of(
    Files.readAllBytes(Path.of("module1.o")),
    Files.readAllBytes(Path.of("module2.o")),
    Files.readAllBytes(Path.of("module3.o"))
);

byte[] result = merger.mergeCoffs(loader, coffs);
Files.write(Path.of("merged.bin"), result);
```

### Template-Based Generation

```java
public class CapabilityBuilder {
    private String template = """
        x64:
          push $LOADER
          make pic +{OPTIMIZATIONS}
          push $PAYLOAD
          append $PIC
          {ADDITIONAL_RESOURCES}
        """;

    public byte[] build(BuildConfig config) {
        // Substitute template variables
        String spec = template
            .replace("{OPTIMIZATIONS}", config.optimizations)
            .replace("{ADDITIONAL_RESOURCES}", config.resources);

        var parser = new SpecParser();
        parser.parse(spec, "generated.spec");
        var compiled = parser.getSpec();

        var args = new HashMap<String, Object>();
        args.put("$LOADER", config.loader);
        args.put("$PAYLOAD", config.payload);

        return compiled.buildPic("x64", args);
    }
}

// Usage
var config = new BuildConfig();
config.loader = loadFile("loader.o");
config.payload = loadFile("payload.bin");
config.optimizations = "optimize disco mutate";
config.resources = "push $CONFIG\nappend $PIC";

var builder = new CapabilityBuilder();
byte[] capability = builder.build(config);
```

## Use Cases

### 1. C2 Server Integration

```java
// Dynamically build payloads on C2 server
public byte[] buildPayloadForTarget(TargetInfo target) {
    String spec = String.format("""
        x64:
          push $LOADER
          dfr "resolve" "ror13"
          make pic +optimize +mutate
          push $CONFIG
          append $PIC
        """);

    // Generate target-specific config
    byte[] config = generateConfig(target.ip, target.port, target.key);

    var parser = new SpecParser();
    parser.parse(spec, "target-" + target.id + ".spec");
    var compiled = parser.getSpec();

    var args = new HashMap<String, Object>();
    args.put("$LOADER", standardLoader);
    args.put("$CONFIG", config);

    return compiled.buildPic("x64", args);
}
```

### 2. Automated Testing

```java
@Test
public void testCapabilityVariants() {
    String[] optimizations = {
        "optimize",
        "disco",
        "mutate",
        "optimize disco",
        "optimize mutate",
        "optimize disco mutate"
    };

    for (String opts : optimizations) {
        byte[] pic = buildWithOptimizations(loader, payload, opts);
        assertNotNull(pic);
        assertTrue(pic.length > 0);
        // Test execution...
    }
}
```

### 3. Build Pipeline Integration

```java
// Maven/Gradle plugin for capability building
public class CrystalPalaceMojo extends AbstractMojo {
    @Parameter
    private File specFile;

    @Parameter
    private File[] inputs;

    @Parameter
    private File output;

    public void execute() throws MojoExecutionException {
        try {
            var spec = LinkSpec.Parse(specFile.getPath());

            var capability = Files.readAllBytes(inputs[0].toPath());
            var args = new HashMap<String, Object>();

            var pic = spec.run(capability, args);

            Files.write(output.toPath(), pic);
        } catch (Exception e) {
            throw new MojoExecutionException("Build failed", e);
        }
    }
}
```

## API Advantages

### 1. No File I/O Required
- Generate specs in memory
- Process byte arrays directly
- No temporary files

### 2. Dynamic Configuration
- Change specs based on runtime conditions
- A/B test different transformations
- Environment-specific builds

### 3. Automation
- CI/CD integration
- Batch processing
- Parameterized builds

### 4. Flexibility
- Template systems
- Configuration-driven builds
- Programmatic control

## HashMap Variables

### Common Variables

```java
var args = new HashMap<String, Object>();

// Automatically populated by Crystal Palace
args.put("$DLL", dllBytes);        // When input is DLL
args.put("$OBJECT", coffBytes);    // When input is COFF
args.put("$PIC", picBytes);        // After make pic
args.put("$PICO", picoBytes);      // After make pico

// Custom variables (for your specs)
args.put("$LOADER", loaderBytes);
args.put("$PAYLOAD", payloadBytes);
args.put("$CONFIG", configBytes);
args.put("$RESOURCE1", resource1Bytes);
// etc.
```

### Variable Usage in Specs

```java
String spec = """
    x64:
      push $LOADER          # Reference custom variable
      make pic
      push $PAYLOAD         # Reference custom variable
      append $PIC           # Reference auto-populated variable
    """;
```

## CLI vs API

### CLI (./link tool)
```bash
./link capability.spec capability.dll output.bin
```
- Simple, file-based
- Good for manual builds
- Limited automation

### API
```java
var spec = LinkSpec.Parse("capability.spec");
var pic = spec.run(dllBytes, args);
```
- Programmatic control
- Dynamic generation
- Full automation capability

## Integration Examples

### Example 1: Web Service

```java
@RestController
public class PayloadController {
    @PostMapping("/build")
    public ResponseEntity<byte[]> buildPayload(@RequestBody BuildRequest req) {
        try {
            byte[] pic = buildCapability(req.getLoader(), req.getPayload(), req.getOptions());
            return ResponseEntity.ok(pic);
        } catch (Exception e) {
            return ResponseEntity.status(500).build();
        }
    }

    private byte[] buildCapability(byte[] loader, byte[] payload, String opts) {
        String spec = String.format("x64:\npush $LOADER\nmake pic +%s\npush $PAYLOAD\nappend $PIC", opts);

        var parser = new SpecParser();
        parser.parse(spec, "api.spec");
        var compiled = parser.getSpec();

        var args = new HashMap<String, Object>();
        args.put("$LOADER", loader);
        args.put("$PAYLOAD", payload);

        return compiled.buildPic("x64", args);
    }
}
```

### Example 2: CLI Tool

```java
public class CapabilityTool {
    public static void main(String[] args) {
        if (args.length < 3) {
            System.err.println("Usage: tool <loader> <payload> <output>");
            System.exit(1);
        }

        try {
            byte[] loader = Files.readAllBytes(Path.of(args[0]));
            byte[] payload = Files.readAllBytes(Path.of(args[1]));

            String spec = "x64:\npush $LOADER\nmake pic +optimize +mutate\npush $PAYLOAD\nappend $PIC";

            var parser = new SpecParser();
            parser.parse(spec, "cli.spec");
            var compiled = parser.getSpec();

            var hashMap = new HashMap<String, Object>();
            hashMap.put("$LOADER", loader);
            hashMap.put("$PAYLOAD", payload);

            byte[] result = compiled.buildPic("x64", hashMap);
            Files.write(Path.of(args[2]), result);

            System.out.println("Built: " + args[2]);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }
}
```

## Related Techniques

- [Crystal Palace](../crystal-palace/) - Core linker framework
- [Position Independent Code](../position-independent-code/) - PIC fundamentals

## Resources

- [Crystal Palace API](https://rastamouse.me/crystal-palace-api/) - Rasta Mouse
- [Tradecraft Garden](https://tradecraftgarden.org/) - Official documentation

## Next Steps

1. Experiment with SpecParser for dynamic specs
2. Build template-based generation systems
3. Integrate into CI/CD pipelines
4. Create custom capability builders
