# Developer Guide - Multi-Type Particle System

This document provides detailed technical information for developers who want to understand, modify, or extend the particle simulation system.

## 🏗️ Architecture Overview

### Core Design Principles
- **Force-Centric**: All interactions defined as force relationships between particle types
- **GPU-First**: Physics simulation runs entirely on GPU using compute shaders
- **Type-Based**: Particles grouped by type with shared properties and behaviors
- **Data-Driven**: All configuration externalized to JSON files
- **Modern C++**: Uses C++20 features, RAII, and smart pointers throughout

### System Components

```
Application (Coordinator)
├── D3DDevice (DirectX 11 Management)
├── ShaderManager (HLSL Compilation & Management)
├── ParticleSystem (Physics & Data Management)
├── Renderer (Visualization Pipeline)
├── InteractiveCamera (3D Navigation)
├── ConfigManager (JSON Configuration)
└── Supporting Systems (Logger, Profiler, Stats, Memory)
```

---

## 🔧 Extending the System

### Adding New Force Types

#### Step 1: Define the Force Type
Add to `Structures.h`:
```cpp
enum class ForceType : uint32_t {
    // ... existing types ...
    MyNewForce = 9    // Add your new type
};
```

#### Step 2: Implement GPU Calculation
Add to `ComputeShader.hlsl` in the force calculation switch:
```hlsl
case 9: // MyNewForce
{
    // Your force calculation here
    // Parameters available in rule.parameters[0-7]
    float3 force = CalculateMyNewForce(particleA, particleB, rule);
    forceSum += force * rule.strength;
    break;
}
```

#### Step 3: Add Configuration Support
Extend `ConfigManager.cpp` to parse your force:
```cpp
ForceType ConfigManager::StringToForceType(const std::string& str) {
    // ... existing mappings ...
    if (str == "MyNewForce") return ForceType::MyNewForce;
    // ...
}
```

#### Step 4: Document Parameters
Update this guide and comments to document parameter usage:
```cpp
// MyNewForce parameters:
//   [0] = primary strength coefficient
//   [1] = secondary parameter
//   [2] = optional modifier
```

### Adding New Particle Properties

#### Step 1: Extend ParticleType Structure
```cpp
struct ParticleType {
    // ... existing properties ...
    float myNewProperty = 1.0f;  // Add new property with default
};
```

#### Step 2: Update GPU Structures
Ensure HLSL structures match C++ structures in compute shaders.

#### Step 3: Add JSON Configuration
Extend configuration parsing in `ConfigManager.cpp`:
```cpp
// In LoadParticleTypes()
if (typeObj.contains("myNewProperty")) {
    type.myNewProperty = typeObj["myNewProperty"].get<float>();
}
```

#### Step 4: Update GPU Buffers
Modify buffer creation code to include new property data.

---

## 🖥️ GPU Programming Details

### Compute Shader Architecture

The main physics simulation runs in `ComputeShader.hlsl`:

```hlsl
[numthreads(256, 1, 1)]  // Process 256 particles per workgroup
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint particleIndex = id.x;
    
    // 1. Load particle data from GPU buffers
    // 2. Calculate all forces affecting this particle
    // 3. Integrate physics (position, velocity updates)
    // 4. Handle boundary conditions
    // 5. Write updated state back to buffers
}
```

### Buffer Management

The system uses several GPU buffers:
- **Particle Buffer**: Current particle state (position, velocity, etc.)
- **ParticleType Buffer**: Type definitions and properties
- **InteractionRule Buffer**: Force interaction specifications
- **Constants Buffer**: Per-frame constants (time step, boundaries)

### Memory Layout Considerations
- All GPU structures use 16-byte alignment for optimal performance
- Particle data is stored in AoS (Array of Structures) format for cache efficiency
- Buffer sizes are computed based on particle count and padded as needed

---

## 🎨 Rendering Pipeline

### Shader Stages

1. **Vertex Shader** (`VertexShader.hlsl`)
   - Processes raw particle data
   - Prepares data for geometry shader
   - Applies global transformations

2. **Geometry Shader** (`GeometryShaderIcosphere.hlsl`)
   - Generates icosphere geometry from point particles
   - Implements Level-of-Detail (LOD) based on distance
   - Performs frustum culling for performance

3. **Pixel Shader** (`PixelShaderPhong.hlsl`)
   - Implements physically-based Phong lighting
   - Supports material properties per particle type
   - Applies tone mapping and gamma correction

### Rendering Optimizations

- **Frustum Culling**: GPU-based visibility culling using `FrustumCulling.hlsl`
- **LOD System**: Adaptive geometry detail based on camera distance
- **Instanced Rendering**: Single draw call for all particles of same type
- **Buffer Pooling**: Reuses GPU memory allocations to reduce overhead

---

## 🧪 Testing and Debugging

### Debug Features

Enable debugging in `config.json`:
```json
{
    "simulation": {
        "enableDebugOutput": true,
        "debugOutputInterval": 60
    }
}
```

### Performance Profiling

The system includes built-in profiling:
```cpp
PROFILE_FUNCTION();  // Add to any function for timing
```

View results in debug output or stats overlay (F1 key).

### GPU Debugging

For shader debugging:
1. Install Graphics Debugger (Visual Studio Graphics Tools)
2. Capture frame during simulation
3. Inspect compute shader execution and buffer contents
4. Use PIX for advanced GPU profiling

### Common Debug Scenarios

- **Physics Issues**: Check force parameter values and ranges
- **Rendering Issues**: Verify buffer data and shader inputs
- **Performance Issues**: Use profiler to identify bottlenecks
- **Memory Issues**: Monitor GPU memory usage through D3D debug layer

---

## 📊 Performance Guidelines

### CPU Performance
- Main thread handles Windows messages and coordination
- Physics simulation is GPU-only (no CPU particle updates)
- Configuration loading and parsing happens once at startup
- Rendering commands are batched for efficiency

### GPU Performance
- Compute shader workgroup size is optimized for modern GPUs (256 threads)
- Memory access patterns are designed for cache efficiency
- Buffer updates use dynamic buffers with MAP_WRITE_DISCARD
- Rendering uses hardware instancing where possible

### Memory Management
- CPU uses smart pointers (shared_ptr, unique_ptr) for automatic cleanup
- GPU memory is managed through DirectX 11 buffer objects
- Memory pooling reduces allocation overhead
- Buffers are sized based on maximum particle count

### Optimization Checklist
- [ ] Particle count appropriate for target hardware
- [ ] Frustum culling enabled for large simulations
- [ ] LOD levels configured based on scene scale
- [ ] VSync settings match display requirements
- [ ] Debug output disabled in release builds

---

## 🔍 Code Organization

### Header Files
- **Structures.h**: Core data structures and enums
- **Application.h**: Main application coordinator
- **ParticleSystem.h**: Physics simulation management
- **Renderer.h**: Graphics rendering pipeline
- **ConfigManager.h**: JSON configuration parsing
- **[Component].h**: Individual system interfaces

### Implementation Files
- Corresponding .cpp files implement the functionality
- Each system is self-contained with minimal dependencies
- COM smart pointers (ComPtr<>) manage DirectX resources
- Error handling uses exceptions for initialization, logging for runtime

### Shader Files
- **ComputeShader.hlsl**: Main physics simulation
- **FrustumCulling.hlsl**: Visibility optimization
- **VertexShader.hlsl**: Vertex processing
- **GeometryShaderIcosphere.hlsl**: Geometry generation with LOD
- **PixelShaderPhong.hlsl**: Material-based lighting

---

## 🚀 Build System

### Project Configuration
- **Target Platform**: x64 (required for large particle counts)
- **C++ Standard**: C++20 (for modern language features)
- **DirectX SDK**: Windows 10 SDK (10.0.19041 or later)
- **Runtime**: Visual C++ Redistributable 2019+

### Preprocessor Definitions
- `WIN32_LEAN_AND_MEAN`: Reduces Windows header bloat
- `NOMINMAX`: Prevents Windows min/max macros from interfering
- Debug builds include additional validation and logging

### Linker Settings
- Links against: d3d11.lib, dxgi.lib, d3dcompiler.lib
- Subsystem: Windows (GUI application)
- Entry point: WinMain (Windows application entry)

---

## 🔮 Future Architecture Considerations

### Planned Enhancements
- **Multi-GPU Support**: Distribute simulation across multiple GPUs
- **Compute Shader 6.0**: Leverage newer DirectX 12 features
- **CPU Fallback**: Software implementation for older hardware
- **Vulkan Backend**: Cross-platform graphics API support

### Extensibility Points
- Force calculation system designed for easy extension
- Rendering pipeline supports multiple geometry types
- Configuration system handles arbitrary JSON structures
- Plugin architecture for custom force implementations

### Compatibility
- Current code targets DirectX 11 for broad hardware support
- Architecture designed to support future DirectX 12 migration
- Force calculation interface abstracts GPU implementation details
- JSON configuration ensures forward/backward compatibility

---

This developer guide covers the essential architecture and extension points. For specific implementation details, refer to the comprehensive inline documentation in the source code files.
