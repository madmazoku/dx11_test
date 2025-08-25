# Quick Start Guide - DirectX 11 Particle Physics

## 🚀 Running the Simulation

1. **Build the project** in Visual Studio (Debug or Release, x64 recommended)
2. **Run the executable** - the simulation will start immediately
3. **Use the controls** to interact with the particle system

## 🎮 Interactive Controls

### Mouse Controls
- **Left Mouse Button + Drag**: Rotate camera around particle cloud
- **Mouse Wheel**: Zoom in/out (adjusts percentile view from 1% to 100%)

### Keyboard Shortcuts
- **ESC**: Exit application
- **R**: Reset entire simulation and camera
- **C**: Reset camera to default position
- **SPACE**: Center camera on current particle cloud
- **1-9**: Set zoom to 10%, 20%, 30%... 90%
- **P**: Toggle performance statistics overlay
- **L**: Cycle through logging levels
- **H**: Show available presets and help
- **→/← Arrow Keys**: Switch between simulation presets
- **+/-**: Next/Previous preset (alternative keys)

## 📊 Understanding the Display

### Camera Behavior
- **Auto-centering**: Camera always looks at the center of mass of all particles
- **Percentile zoom**: 
  - 10% = Shows only the 10% of particles closest to center
  - 50% = Shows half of all particles 
  - 100% = Shows all particles within the bounding sphere
- **Smooth rotation**: Camera orbits around the particle cloud center

### Performance Stats (Press P to toggle)
When enabled, you'll see:
- **FPS**: Frames per second and frame time
- **Particle Count**: Number of active particles
- **Camera Info**: Position, distance, zoom level
- **Controls reminder**

## 🎯 Built-in Presets

The simulation comes with 5 built-in presets - press **H** to see the full list:

1. **default**: Balanced simulation (256 particles)
2. **performance**: Optimized for smooth 60fps (128 particles) 
3. **spectacular**: High particle count for visual impact (512 particles)
4. **zerog**: Zero gravity floating particles
5. **bouncy**: High energy with lots of bouncing

**Switch between presets using the arrow keys (←/→) or +/- keys**

Each preset optimizes different aspects:
- Particle count for performance vs. visual complexity
- Physics parameters for different behaviors
- Visual settings for different moods
- Boundary sizes for different scales

## 🔧 Customization

Edit `config.json` to customize:

### Simulation Parameters
```json
"simulation": {
    "particleCount": 256,     // More particles = more complex behavior
    "gravity": [0.0, -9.81, 0.0],  // Gravity vector
    "damping": 0.995,         // Energy loss (0.9-0.999)
    "springConstant": 100.0,  // Inter-particle forces
    "boundaryMin/Max": ...    // Simulation boundaries
}
```

### Camera Settings
```json
"camera": {
    "zoomMin": 0.05,          // Minimum zoom (5% of particles)
    "zoomMax": 1.0,           // Maximum zoom (100% of particles)
    "rotationSensitivity": 0.008,  // Mouse sensitivity
    "fov": 50.0               // Field of view in degrees
}
```

### Visual Settings
```json
"rendering": {
    "windowWidth": 1440,      // Window size
    "windowHeight": 900,
    "sphereRadius": 0.08,     // Particle display size
    "clearColor": [0.05, 0.05, 0.15, 1.0],  // Background color
    "lighting": {
        "intensity": 1.2,     // Light brightness
        "ambientIntensity": 0.3  // Ambient light
    }
}
```

## 🔬 Experimentation Ideas

1. **Try different particle counts**: 64, 128, 256, 512, 1024
2. **Adjust gravity**: Try [0, 0, 0] for zero gravity, or [-5, 0, 0] for sideways
3. **Change damping**: 0.99 = more bouncy, 0.999 = more stable
4. **Modify boundaries**: Larger boundaries = more spread out particles
5. **Experiment with spring constants**: Higher values = stiffer connections

## 💡 Tips for Best Experience

- **Start with 256 particles** for good balance of complexity and performance
- **Use mouse wheel frequently** to explore different zoom levels
- **Try the number keys (1-9)** for quick zoom presets
- **Watch the center of mass** - it often moves in interesting patterns
- **Use the stats overlay (P)** to monitor performance
- **Reset (R) when things get chaotic** and try different parameters

## 🐛 Troubleshooting

- **Low FPS**: Reduce particle count in config.json
- **Particles disappear**: Check boundary settings, try resetting (R)
- **Camera feels sluggish**: Increase rotationSensitivity in config
- **Too zoomed in/out**: Use mouse wheel or number keys to adjust

---

**Have fun exploring the particle physics simulation!**
