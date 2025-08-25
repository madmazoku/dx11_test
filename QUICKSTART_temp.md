# Quick Start Guide

## Build and Run

1. Open `dx11_test.sln` in Visual Studio
2. Build in Debug or Release mode  
3. Run the executable

## Configuration

The system loads configuration from `config.json`. Key sections:

### Particle Types
```json
"particleTypes": [
    {
        "id": 0,
        "name": "Water",
        "mass": 1.0,
        "radius": 0.1,
        "charge": 0.0,
        "material": { /* visual properties */ }
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
                "type": "LennardJones",
                "strength": 1.0,
                "range": 0.5,
                "parameters": [0.1, 0.2]  // [epsilon, sigma]
            }
        ]
    }
]
```

### Environmental Forces
```json
"environmentalForces": [
    {
        "targetTypeId": 4294967295,  // All types
        "force": {
            "type": "ConstantField", 
            "strength": 1.0,
            "parameters": [0.0, -9.81, 0.0, 1.0]  // [x, y, z, mass_mult]
        }
    }
]
```

## Controls

- **Mouse**: Rotate camera
- **Mouse wheel**: Zoom
- **F1**: Toggle stats
- **Escape**: Exit
