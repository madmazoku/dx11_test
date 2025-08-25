# DirectX 11 Particle Physics Simulation with Verlet Integration

A high-performance particle physics simulation using DirectX 11 compute shaders for real-time Verlet integration and geometry shaders for sphere rendering. Features interactive camera controls with automatic centering and percentile-based scaling.

## 🚀 Features

### Core Simulation
- **Verlet Integration**: Physics-accurate particle simulation using Verlet integration in compute shaders
- **GPU Acceleration**: All physics calculations performed on GPU for optimal performance
- **Configurable Parameters**: Real-time adjustable gravity, damping, spring constants, and boundaries
- **Boundary Handling**: Particles bounce within configurable 3D boundaries

### Advanced Rendering
- **Geometry Shader Spheres**: Particles rendered as 3D spheres using geometry shaders
- **Modern DirectX 11**: Utilizes compute shaders, constant buffers, and modern rendering pipeline
- **Configurable Lighting**: Directional lighting with ambient and specular components
- **Performance Monitoring**: Built-in profiling and frame time logging

### Interactive Camera System
- **Auto-Centering**: Camera automatically centers on the particle cloud's center of mass
- **Percentile Scaling**: Zoom level based on 95th percentile of particle distances (configurable 1%-100%)
- **Mouse Controls**: 
  - **Left Mouse Button + Drag**: Rotate camera around center of mass
  - **Mouse Wheel**: Adjust zoom percentile (1% to 100%)
- **Real-time Updates**: Camera updates smoothly as particle cloud evolves

### Modern C++ Architecture
- **RAII**: Automatic resource management with smart pointers
- **Exception Safety**: Comprehensive error handling and logging
- **Modular Design**: Clean separation of concerns across multiple classes
- **Configuration Management**: JSON-based configuration with runtime reloading
- **Comprehensive Logging**: Multi-level logging with file and console output

## 🏗️ Architecture

### Class Structure

```
Application (main.cpp)
├── D3DDevice          - DirectX 11 device and context management
├── ShaderManager      - Shader compilation and management
├── ParticleSystem     - Particle data and physics simulation
├── Renderer          - Rendering pipeline and frame management
├── InteractiveCamera - Camera controls and view transformations
├── ConfigManager     - JSON configuration management
├── Logger            - Multi-level logging system
├── Profiler          - Performance monitoring and timing
└── Utilities         - Helper functions and common utilities
```

### Key Components

#### ParticleSystem
- Manages particle data using structured buffers
- Implements Verlet integration in compute shader
- Handles spring connections between particles
- Provides particle position data for camera calculations

#### InteractiveCamera
- Calculates particle cloud center of mass and bounding sphere
- Implements percentile-based zoom scaling
- Handles smooth mouse-controlled camera rotation
- Updates view and projection matrices in real-time

#### Renderer
- Orchestrates the complete rendering pipeline
- Manages compute shader dispatch for physics
- Handles geometry shader sphere generation
- Updates constant buffers for camera and lighting

## 🎮 Controls

### Mouse Controls
- **Left Mouse Button + Drag**: Rotate camera around particle cloud center
- **Mouse Wheel Up**: Increase zoom percentile (show more particles)
- **Mouse Wheel Down**: Decrease zoom percentile (focus on central particles)

### Keyboard Shortcuts
- **ESC**: Exit application
- **R**: Reset camera to default position
- **C**: Toggle auto-centering on/off
- **P**: Toggle performance profiling display
- **L**: Toggle logging level (Info/Debug/Verbose)
- **H**: Show available presets and help
- **SPACE**: Center camera on particle cloud
- **→ (Right Arrow) / + / =**: Load next preset
- **← (Left Arrow) / - / _**: Load previous preset
- **1-9**: Quick zoom to 10%-90% percentile

### Advanced Features
- **Preset System**: Built-in presets for different simulation types:
  - `default`: Balanced 256 particles
  - `performance`: Optimized 128 particles for smooth 60fps
  - `spectacular`: 512 particles for visual impact
  - `zerog`: Zero gravity floating particles
  - `bouncy`: High energy with lots of bouncing
- **Real-time Statistics**: Toggle performance overlay with detailed info
- **Smart Camera**: Auto-centering with percentile-based zoom
- **Hot Configuration Reload**: Switch presets without restarting
- **Center**: Always focused on particle cloud's center of mass
- **Zoom**: Based on percentile of particle distances from center
  - 1% = Very close, shows only central particles
  - 50% = Shows half of all particles
  - 100% = Shows all particles within bounding sphere
- **Rotation**: Smooth orbital rotation around center point

## ⚙️ Configuration

The simulation is configured through `config.json`:

```json
{
    "simulation": {
        "particleCount": 128,
        "timeStep": 0.016,
        "gravity": [0.0, -9.81, 0.0],
        "damping": 0.995,
        "springConstant": 50.0,
        "restLength": 0.5,
        "boundaryMin": [-5.0, -5.0, -5.0],
        "boundaryMax": [5.0, 5.0, 5.0]
    },
    "rendering": {
        "windowWidth": 1280,
        "windowHeight": 720,
        "enableVSync": true,
        "clearColor": [0.1, 0.1, 0.2, 1.0],
        "sphereRadius": 0.1,
        "camera": {
            "radius": 12.0,
            "rotationSpeed": 0.5,
            "fov": 45.0,
            "zoomMin": 0.01,
            "zoomMax": 1.0,
            "zoomStep": 0.05,
            "rotationSensitivity": 0.005,
            "enableAutoCentering": true
        }
    }
}
```

### Key Parameters
- `particleCount`: Number of particles in simulation (powers of 2 recommended)
- `timeStep`: Physics timestep in seconds (0.016 = ~60 FPS)
- `camera.zoomMin/Max`: Percentile range for zoom control (0.01 = 1%, 1.0 = 100%)
- `camera.rotationSensitivity`: Mouse sensitivity for camera rotation

## 🔧 Building

### Prerequisites
- Visual Studio 2019/2022 with C++ desktop development
- Windows 10/11 SDK
- DirectX 11 runtime

### Build Steps
1. Open `dx11_test.sln` in Visual Studio
2. Set platform to x64 (recommended)
3. Build solution (Ctrl+Shift+B)
4. Run (F5 or Ctrl+F5)

### Dependencies
All dependencies are included in Windows SDK:
- `d3d11.lib` - DirectX 11 graphics
- `d3dcompiler.lib` - Shader compilation
- `dxguid.lib` - DirectX GUIDs

## 📊 Performance

### Optimization Features
- **GPU Compute**: All physics on GPU via compute shaders
- **Minimal CPU-GPU Sync**: Only when camera needs particle positions
- **Efficient Rendering**: Geometry shaders for sphere generation
- **Smart Updates**: Camera only updates when particles move significantly

### Performance Monitoring
- Built-in frame time profiling
- GPU/CPU timing measurements
- Memory usage tracking
- Configurable performance logging

### Typical Performance
- **1024 particles**: 60+ FPS on GTX 1060/RX 580
- **4096 particles**: 30+ FPS on GTX 1060/RX 580
- **Scale**: O(n) complexity for physics, O(1) for rendering

## 🔬 Technical Details

### Shader Pipeline
1. **ComputeShader.hlsl**: Verlet integration physics
2. **VertexShader.hlsl**: Particle position processing
3. **GeometryShader.hlsl**: Sphere generation from points
4. **PixelShader.hlsl**: Lighting and shading

### Data Flow
```
CPU (Particle Management) → GPU Compute (Physics) → GPU Render (Spheres) → Display
                           ↑                        ↓
                    Camera Update ← CPU (Position Readback)
```

### Memory Layout
- **Structured Buffers**: GPU-optimized particle storage
- **Constant Buffers**: Per-frame uniform data
- **Staging Buffers**: CPU-GPU data exchange for camera

## 🔮 Future Enhancements

### Planned Features
- **ImGui Integration**: Real-time parameter adjustment
- **Multiple Presets**: Predefined simulation configurations
- **Export Functionality**: Save animation sequences
- **Advanced Lighting**: Shadow mapping and multiple light sources
- **Collision Detection**: Inter-particle collision handling

### Advanced Camera Features
- **Smooth Transitions**: Animated camera position changes
- **Bookmarked Views**: Save and restore camera positions
- **Follow Mode**: Camera tracks specific particles
- **Cinematic Mode**: Automated camera movements

### Performance Improvements
- **LOD System**: Level-of-detail for distant particles
- **Frustum Culling**: Skip off-screen particle processing
- **Temporal Upsampling**: Improve visual quality at high particle counts

## 🐛 Troubleshooting

### Common Issues
1. **Black Screen**: Check DirectX 11 support and update drivers
2. **Low Performance**: Reduce `particleCount` in config.json
3. **Compilation Errors**: Ensure Windows SDK is installed
4. **Config Not Loading**: Check config.json syntax and file permissions

### Debug Features
- Comprehensive logging to `particle_simulation.log`
- Debug output for camera and physics state
- Performance counters for bottleneck identification
- Error reporting with stack traces

## 📄 License

This project is provided as-is for educational and research purposes.

---

**Developed with modern C++17, DirectX 11, and HLSL 5.0**
