# Multi-Type Particle Physics Simulation

A comprehensive DirectX 11 particle simulation system featuring multiple particle types, force-based interactions, and advanced GPU-accelerated physics. This modern C++ application demonstrates real-time particle physics with sophisticated rendering and interactive controls.

## 🌟 Key Features

### Advanced Physics System
- **Multi-type particles** with distinct physical properties (mass, radius, charge, temperature)
- **Force-centric architecture** supporting multiple simultaneous force interactions
- **GPU-accelerated simulation** using DirectX 11 compute shaders for high performance
- **Configurable interactions** between different particle types via JSON configuration

### Supported Force Types
- **Spring Forces** - Elastic connections with stiffness and damping parameters
- **Lennard-Jones Potential** - Molecular interaction forces with epsilon/sigma parameters
- **Electromagnetic Forces** - Coulomb interactions based on particle charges
- **Gravitational Forces** - Newtonian attraction between particles with mass
- **Viscous Forces** - Drag and friction effects in fluid environments
- **Environmental Fields** - Constant and radial force fields affecting particle motion

### Visual Rendering System
- **Icosphere-based geometry** - Smooth spherical particles with configurable subdivision levels
- **Phong lighting model** - Realistic shading with specular highlights and material properties
- **Level-of-Detail (LOD)** - Adaptive geometry detail based on distance from camera
- **Frustum culling** - GPU-based visibility optimization for large particle counts
- **Interactive camera** - Mouse-controlled 3D navigation with auto-centering

### Performance & Monitoring
- **Real-time statistics** - FPS, particle count, culling efficiency, memory usage
- **GPU memory management** - Efficient buffer pooling and automatic cleanup
- **Performance profiling** - Built-in timing for optimization and debugging
- **Configurable quality** - Adjustable particle counts and rendering detail

## 🛠️ Technical Architecture

### Core Components
- **Application** - Main coordinator managing all subsystems and Windows message loop
- **ParticleSystem** - Multi-type particle management with GPU buffer handling
- **Renderer** - DirectX 11 rendering with icosphere geometry and Phong shading
- **ConfigManager** - JSON-based configuration loading and management
- **ShaderManager** - HLSL shader compilation and resource management
- **InteractiveCamera** - 3D camera with mouse/keyboard controls and particle tracking

### Shader Pipeline
- **ComputeShader.hlsl** - Main particle physics simulation with force calculations
- **FrustumCulling.hlsl** - GPU-based visibility culling for performance optimization
- **VertexShader.hlsl** - Particle data preparation for geometry generation
- **GeometryShaderIcosphere.hlsl** - Dynamic icosphere generation with LOD support
- **PixelShaderPhong.hlsl** - Material-based Phong lighting with realistic shading

## 🚀 Quick Start

### Prerequisites
- Windows 10 or later
- Visual Studio 2019 or later with C++20 support
- DirectX 11 compatible graphics card
- Windows SDK 10.0 or later

### Building and Running
1. Clone the repository and open `dx11_test.sln` in Visual Studio
2. Set configuration to `Debug|x64` or `Release|x64`
3. Build the solution (Ctrl+Shift+B)
4. Run the executable - configuration loads automatically from `config.json`

### Controls
- **Mouse drag** - Rotate camera around particle center
- **Mouse wheel** - Zoom in/out
- **ESC** - Exit application
- **F1** - Toggle statistics overlay
- **F2** - Toggle frustum culling
- **F3** - Toggle camera auto-centering
- **R** - Reset simulation to initial state
- **P** - Pause/resume physics simulation
- **1-5** - Load predefined simulation presets

## ⚙️ Configuration

The simulation is highly configurable via `config.json`. Key sections include:

### Particle Types
```json
"particleTypes": [
    {
        "id": 0,
        "name": "Water",
        "mass": 1.0,
        "radius": 0.1,
        "charge": 0.0,
        "material": {
            "diffuseColor": [0.2, 0.6, 1.0],
            "specularColor": [1.0, 1.0, 1.0],
            "shininess": 64.0
        }
    }
]
```

### Interaction Rules
```json
"interactionRules": [
    {
        "typeA": 0,
        "typeB": 0,
        "forces": [
            {
                "type": "Spring",
                "strength": 1.0,
                "range": 0.5,
                "parameters": [10.0, 0.5, 0.16]
            }
        ]
    }
]
```

### Rendering Settings
```json
"rendering": {
    "windowWidth": 1440,
    "windowHeight": 900,
    "enableVSync": true,
    "culling": {
        "enableFrustumCulling": true,
        "maxRenderDistance": 100.0,
        "enableLOD": true
    }
}
```

## 📊 Performance

The system is optimized for real-time simulation with the following capabilities:
- **512+ particles** at 60+ FPS on modern hardware
- **GPU compute shaders** for parallel force calculations
- **Frustum culling** reducing rendered particles by 60-80%
- **LOD system** adapting geometry detail based on distance
- **Memory pooling** for efficient GPU buffer management

## 🔧 Development

### Adding New Force Types
1. Add force type enum to `Structures.h`
2. Implement force calculation in `ComputeShader.hlsl`
3. Add configuration parsing in `ConfigManager.cpp`
4. Update documentation and examples

### Extending Particle Properties
1. Modify `Particle` structure in `Structures.h`
2. Update shader structures in all HLSL files
3. Add configuration support in JSON parsing
4. Update GPU buffer creation code

## 📁 Project Structure

```
dx11_test/
├── Source Files (.cpp)
│   ├── Application.cpp          # Main application coordinator
│   ├── ParticleSystem.cpp       # Multi-type particle management
│   ├── Renderer.cpp             # DirectX rendering system
│   ├── ConfigManager.cpp        # JSON configuration handling
│   └── [Other system files]
├── Headers (.h)
│   ├── Application.h            # Application class definition
│   ├── Structures.h             # Core data structures
│   └── [Other headers]
├── Shaders (.hlsl)
│   ├── ComputeShader.hlsl       # Main physics simulation
│   ├── FrustumCulling.hlsl      # Visibility optimization
│   └── [Rendering shaders]
├── Configuration
│   ├── config.json              # Main configuration file
│   └── dx11_test.sln           # Visual Studio solution
└── Documentation
    ├── README.md                # This file
    └── QUICKSTART.md           # Quick setup guide
```

## 🎯 Future Enhancements

- **Spatial partitioning** for O(n log n) collision detection
- **Multi-threading** for CPU-based force calculations
- **Fluid simulation** with density-based interactions
- **Collision detection** with realistic bounce physics
- **Save/load** simulation states and replay functionality
- **VR support** for immersive particle interaction

---

Built with modern C++20, DirectX 11, and high-performance computing principles for real-time particle physics simulation.

## 📄 Additional Documentation

- **[QUICKSTART.md](QUICKSTART.md)** - Detailed setup guide with configuration examples
- **[DEVELOPER.md](DEVELOPER.md)** - Technical architecture and extension guide for developers  
- **[CONFIG_EXAMPLES.md](CONFIG_EXAMPLES.md)** - Ready-to-use configurations for different simulation types
- **Source Code** - Fully documented with professional inline comments throughout all files

## 💡 Tips and Troubleshooting

### Performance Optimization
- Reduce `particleCount` in config.json for better performance on older hardware
- Disable `enableVSync` for maximum frame rate
- Set `enableFrustumCulling: true` to improve performance with large particle counts
- Lower LOD levels for distant particles to reduce geometry complexity

### Common Issues
- **Black screen**: Check that DirectX 11 is supported on your graphics card
- **Low FPS**: Reduce particle count or disable advanced features in config.json
- **Compilation errors**: Ensure Windows SDK 10.0+ and Visual Studio 2019+ are installed
- **Config not loading**: Verify config.json syntax using a JSON validator

### System Requirements
- **Minimum**: Intel HD 4000 / AMD Radeon 6000+ / NVIDIA GTX 400+, 4GB RAM
- **Recommended**: Dedicated GPU with 2GB+ VRAM, 8GB+ RAM, quad-core CPU
- **Optimal**: Modern gaming GPU (GTX 1060+ / RX 580+), 16GB+ RAM, fast SSD

## 🤝 Contributing

When contributing to this project:
1. Follow the existing code documentation standards
2. Update relevant .md files for any new features
3. Test changes with different configuration files
4. Maintain backward compatibility with existing configs

## 📝 License

This project is provided as educational example code for DirectX 11 and particle physics simulation development.
