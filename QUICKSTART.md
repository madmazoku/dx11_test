# Multi-Type Particle System - Quick Start Guide

## 🚀 Build and Run (5 minutes)

### Prerequisites
- **Windows 10/11** (64-bit)
- **Visual Studio 2019+** with C++ desktop development
- **DirectX 11** compatible graphics card
- **Windows SDK 10.0+**

### Build Steps
1. **Clone/Download** the project to your local machine
2. **Open** `dx11_test.sln` in Visual Studio
3. **Set Platform** to `x64` (Debug or Release)
4. **Build Solution** (Ctrl+Shift+B) - should complete without errors
5. **Run** (F5) - application will start with default configuration

### First Run
- Application loads `config.json` automatically
- **512 particles** of 4 different types will appear
- Use **mouse** to rotate camera, **wheel** to zoom
- Press **F1** for performance statistics
- Press **ESC** to exit

---

## ⚙️ Configuration Guide

The system is fully configurable via `config.json`. Here's what each section does:

### Basic Simulation Settings
```json
"simulation": {
    "particleCount": 512,        // Total particles (affects performance)
    "timeStep": 0.016,          // Physics time step (0.016 = 60fps)
    "enableDebugOutput": true,   // Console debug info
    "globalDamping": 0.01,      // Air resistance/friction
    "boundaryMin": [-10, -10, -10],  // Simulation box boundaries
    "boundaryMax": [10, 10, 10]
}
```

### Defining Particle Types
```json
"particleTypes": [
    {
        "id": 0,                    // Unique identifier
        "name": "Water",            // Display name
        "mass": 1.0,               // Physical mass (affects gravity, inertia)
        "radius": 0.1,             // Size (affects collisions, rendering)
        "charge": 0.0,             // Electric charge (+ or -, 0 = neutral)
        "material": {              // Visual appearance
            "diffuseColor": [0.2, 0.6, 1.0],    // RGB color
            "specularColor": [1.0, 1.0, 1.0],   // Shine color
            "shininess": 64.0,                   // Shininess factor
            "metallic": 0.0,                     // 0=plastic, 1=metal
            "roughness": 0.1,                    // 0=mirror, 1=rough
            "emissive": 0.0,                     // Glow intensity
            "emissiveColor": [0.0, 0.0, 0.0]     // Glow color
        },
        "lodLevels": 2             // Detail levels (1-3, higher = more detailed)
    }
]
```

### Interaction Rules (Forces Between Particles)
```json
"interactionRules": [
    {
        "typeA": 0,                // First particle type
        "typeB": 0,                // Second particle type  
        "enabled": true,           // Turn this interaction on/off
        "forces": [
            {
                "type": "LennardJones",        // Force type
                "strength": 1.0,               // Force strength multiplier
                "range": 0.5,                  // Maximum interaction distance
                "parameters": [0.1, 0.2]       // Force-specific parameters
            }
        ]
    }
]
```

### Available Force Types & Parameters

#### Spring Force (Elastic connections)
```json
{
    "type": "Spring",
    "parameters": [
        10.0,    // Stiffness (higher = stiffer spring)
        0.5,     // Damping (higher = more energy loss)
        0.16     // Rest length (natural spring length)
    ]
}
```

#### Lennard-Jones (Molecular forces)
```json
{
    "type": "LennardJones", 
    "parameters": [
        0.1,     // Epsilon (attraction strength)
        0.2      // Sigma (particle size parameter)
    ]
}
```

#### Electromagnetic (Charged particles)
```json
{
    "type": "Electromagnetic",
    "parameters": [
        8.99e9   // Coulomb constant (standard: 8.99e9)
    ]
}
```

#### Gravitational (Mass attraction)
```json
{
    "type": "Gravitational",
    "parameters": [
        6.67e-11 // Gravitational constant
    ]
}
```

### Environmental Forces (Global effects)
```json
"environmentalForces": [
    {
        "targetTypeId": 4294967295,  // All particle types (0xFFFFFFFF)
        "enabled": true,
        "force": {
            "type": "ConstantField",     // Uniform force field
            "strength": 1.0,
            "parameters": [
                0.0, -9.81, 0.0, 1.0    // [x, y, z, mass_multiplier]
            ]                            // This creates gravity
        }
    }
]
```

### Rendering Settings
```json
"rendering": {
    "windowWidth": 1440,           // Window width in pixels
    "windowHeight": 900,           // Window height in pixels
    "enableVSync": true,           // Limit FPS to monitor refresh rate
    "enableInteractiveCamera": true, // Mouse camera controls
    "culling": {
        "enableFrustumCulling": true,    // Don't render off-screen particles
        "maxRenderDistance": 100.0,      // Don't render very distant particles
        "enableLOD": true,               // Use less detail for distant particles
        "lodDistance1": 10.0,            // Distance thresholds for detail levels
        "lodDistance2": 25.0,
        "lodDistance3": 50.0
    },
    "lighting": {
        "lightDirection": [-0.5, -1.0, -0.3],   // Main light direction
        "lightColor": [1.0, 1.0, 1.0],          // Light color (RGB)
        "lightIntensity": 1.0,                   // Light brightness
        "ambientIntensity": 0.2                  // Background lighting
    }
}
```

---

## 🎮 Controls Reference

### Camera Controls
- **Mouse drag** → Rotate camera around particles
- **Mouse wheel** → Zoom in/out
- **Right click + drag** → Pan camera (if implemented)

### Keyboard Shortcuts
- **ESC** → Exit application
- **F1** → Toggle performance overlay (FPS, particle count, etc.)
- **F2** → Toggle frustum culling (performance optimization)
- **F3** → Toggle automatic camera centering
- **R** → Reset simulation to initial state
- **P** → Pause/resume physics (rendering continues)
- **1-5** → Load preset configurations (if available)

### Debug Features
- **Spacebar** → Single-step physics (when paused)
- **F4** → Toggle wireframe mode (if implemented)
- **F5** → Dump current state to log file

---

## 🛠️ Common Configuration Examples

### Example 1: Simple Gravity Simulation
```json
{
    "simulation": { "particleCount": 128 },
    "particleTypes": [
        { "id": 0, "name": "Ball", "mass": 1.0, "radius": 0.1 }
    ],
    "interactionRules": [],
    "environmentalForces": [
        {
            "targetTypeId": 4294967295,
            "force": {
                "type": "ConstantField",
                "strength": 1.0,
                "parameters": [0.0, -9.81, 0.0, 1.0]
            }
        }
    ]
}
```

### Example 2: Gas Simulation  
```json
{
    "particleTypes": [
        { "id": 0, "name": "Gas", "mass": 0.1, "radius": 0.05 }
    ],
    "interactionRules": [
        {
            "typeA": 0, "typeB": 0,
            "forces": [
                {
                    "type": "LennardJones",
                    "strength": 0.1,
                    "range": 0.3,
                    "parameters": [0.01, 0.1]
                }
            ]
        }
    ]
}
```

### Example 3: Charged Particles
```json
{
    "particleTypes": [
        { "id": 0, "name": "Proton", "charge": 1.0, "mass": 1.0 },
        { "id": 1, "name": "Electron", "charge": -1.0, "mass": 0.001 }
    ],
    "interactionRules": [
        {
            "typeA": 0, "typeB": 1,
            "forces": [
                {
                    "type": "Electromagnetic", 
                    "strength": 1.0,
                    "parameters": [8.99e9]
                }
            ]
        }
    ]
}
```

---

## 🚨 Troubleshooting

### Application Won't Start
- **Check DirectX 11**: Run `dxdiag` to verify DirectX 11 support
- **Update drivers**: Ensure latest graphics drivers are installed
- **Check files**: Verify `config.json` exists and is valid JSON

### Performance Issues
- **Reduce particle count**: Start with `"particleCount": 64` and increase
- **Disable VSync**: Set `"enableVSync": false` 
- **Enable culling**: Ensure all culling options are `true`
- **Lower LOD**: Reduce `lodLevels` in particle types

### Visual Issues
- **Black screen**: Graphics card may not support required shaders
- **Wrong colors**: Check `diffuseColor` values in particle materials
- **No lighting**: Verify lighting configuration in render settings

### Configuration Errors
- **JSON syntax**: Use a JSON validator to check config.json
- **Missing parameters**: Ensure all force parameter arrays are complete
- **Type ID conflicts**: Each particle type needs unique `id` value

### Getting Help
- Check console output for error messages
- Enable `"enableDebugOutput": true` for detailed logging
- Verify Visual Studio C++ redistributables are installed

---

## 🔧 Performance Tuning

### For Low-End Hardware
```json
{
    "simulation": { "particleCount": 64 },
    "rendering": {
        "enableVSync": false,
        "culling": { "maxRenderDistance": 50.0 }
    }
}
```

### For High-End Hardware  
```json
{
    "simulation": { "particleCount": 2048 },
    "rendering": {
        "culling": { "maxRenderDistance": 200.0 },
        "lighting": { "lightIntensity": 1.5 }
    }
}
```

### For Development/Debugging
```json
{
    "simulation": { 
        "particleCount": 32,
        "enableDebugOutput": true,
        "debugOutputInterval": 10
    }
}
```
