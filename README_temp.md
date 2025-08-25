# Multi-Type Particle Physics Simulation

A clean, modern DirectX 11 particle simulation system with support for multiple particle types and force-based interactions.

## Core Architecture

This system implements a **force-centric** architecture where:
- **Particle types** define only essential physical properties: mass, radius, charge
- **Forces** are described as flexible parameter arrays supporting any combination
- **Environmental forces** act on specific particle types or all particles
- **Interactions** between types support multiple simultaneous forces

### Key Features

- **Multi-type particles** with clean property definitions
- **Flexible force system** - spring, Lennard-Jones, electromagnetic, gravitational, viscous, field forces
- **GPU compute shader** simulation for high performance
- **Icosphere-based rendering** with LOD support
- **Interactive camera** system
- **Configuration-driven** setup via JSON

## Essential Properties

### Particle Types
Only essential physical properties are stored:
- `mass` - for gravitational and inertial forces
- `radius` - for collision detection and viscous forces  
- `charge` - for electromagnetic interactions

### Forces
All forces use a flexible parameter array:
- **Spring**: [stiffness, damping, rest_length]
- **Lennard-Jones**: [epsilon, sigma]
- **Gravitational**: [gravitational_constant]
- **Electromagnetic**: [coulomb_constant]
- **Viscous**: [viscosity_coeff, drag_coeff]
- **Environmental fields**: [field_vector_x, field_vector_y, field_vector_z, mass_multiplier]

## Build and Run

1. Open `dx11_test.sln` in Visual Studio
2. Build in Debug or Release mode
3. Run the executable

Configuration is loaded from `config.json` - modify particle types, forces, and rendering settings there.

## Core Files

- `ComputeShader.hlsl` - GPU particle simulation
- `config.json` - System configuration
- `Structures.h` - Core data structures
- `Application.*` - Main application class
- `ParticleSystem.*` - Particle simulation management
- `Renderer.*` - Visualization system

## Controls

- **Mouse**: Camera rotation
- **Mouse wheel**: Zoom in/out
- **F1**: Toggle stats overlay
- **Escape**: Exit application

## Configuration

Edit `config.json` to:
- Define particle types with mass, radius, charge
- Configure interaction rules between types
- Set environmental forces (gravity, viscosity, etc.)
- Adjust rendering and camera settings

## Architecture Benefits

- **Minimal**: Only essential properties, no redundant parameters
- **Flexible**: Any force combinations between particle types
- **Extensible**: Easy to add new force types
- **Performance**: Optimized GPU compute shader implementation
- **Clean**: Force-centric design eliminates parameter duplication
