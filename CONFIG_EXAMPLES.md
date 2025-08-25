# Configuration Examples - Multi-Type Particle System

This document provides ready-to-use configuration examples for different types of particle simulations. Copy these into your `config.json` to experiment with various physical phenomena.

---

## 🌊 Fluid Simulation (Water-like particles)

Simulates water-like fluid behavior using Lennard-Jones forces and viscosity.

```json
{
    "simulation": {
        "particleCount": 512,
        "timeStep": 0.016,
        "globalDamping": 0.02,
        "boundaryMin": [-8.0, -5.0, -8.0],
        "boundaryMax": [8.0, 5.0, 8.0],
        "boundaryRestitution": 0.3
    },
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
                "shininess": 64.0,
                "metallic": 0.0,
                "roughness": 0.1
            },
            "lodLevels": 2
        }
    ],
    "interactionRules": [
        {
            "typeA": 0,
            "typeB": 0,
            "forces": [
                {
                    "type": "LennardJones",
                    "strength": 1.0,
                    "range": 0.3,
                    "parameters": [0.1, 0.15]
                },
                {
                    "type": "ViscousForce", 
                    "strength": 2.0,
                    "range": 0.25,
                    "parameters": [0.5, 0.1]
                }
            ]
        }
    ],
    "environmentalForces": [
        {
            "targetTypeId": 4294967295,
            "force": {
                "type": "ConstantField",
                "strength": 1.0,
                "parameters": [0.0, -9.81, 0.0, 1.0]
            }
        }
    ],
    "rendering": {
        "windowWidth": 1440,
        "windowHeight": 900,
        "enableVSync": true,
        "culling": {
            "enableFrustumCulling": true,
            "maxRenderDistance": 50.0
        }
    }
}
```

---

## ⚡ Electromagnetic Plasma Simulation

Charged particles with electromagnetic interactions, creating plasma-like behavior.

```json
{
    "simulation": {
        "particleCount": 256,
        "timeStep": 0.008,
        "globalDamping": 0.005,
        "boundaryMin": [-5.0, -5.0, -5.0],
        "boundaryMax": [5.0, 5.0, 5.0],
        "boundaryRestitution": 0.9
    },
    "particleTypes": [
        {
            "id": 0,
            "name": "Proton",
            "mass": 1.0,
            "radius": 0.08,
            "charge": 1.0,
            "material": {
                "diffuseColor": [1.0, 0.3, 0.3],
                "specularColor": [1.0, 0.8, 0.8],
                "shininess": 32.0,
                "emissive": 0.5,
                "emissiveColor": [1.0, 0.2, 0.2]
            }
        },
        {
            "id": 1,
            "name": "Electron",
            "mass": 0.01,
            "radius": 0.06,
            "charge": -1.0,
            "material": {
                "diffuseColor": [0.3, 0.3, 1.0],
                "specularColor": [0.8, 0.8, 1.0],
                "shininess": 32.0,
                "emissive": 0.5,
                "emissiveColor": [0.2, 0.2, 1.0]
            }
        }
    ],
    "interactionRules": [
        {
            "typeA": 0,
            "typeB": 0,
            "forces": [
                {
                    "type": "Electromagnetic",
                    "strength": 0.1,
                    "range": 2.0,
                    "parameters": [8.99e-3]
                }
            ]
        },
        {
            "typeA": 1,
            "typeB": 1,
            "forces": [
                {
                    "type": "Electromagnetic",
                    "strength": 0.1,
                    "range": 2.0,
                    "parameters": [8.99e-3]
                }
            ]
        },
        {
            "typeA": 0,
            "typeB": 1,
            "forces": [
                {
                    "type": "Electromagnetic",
                    "strength": 0.1,
                    "range": 2.0,
                    "parameters": [8.99e-3]
                }
            ]
        }
    ],
    "environmentalForces": [],
    "rendering": {
        "lighting": {
            "ambientIntensity": 0.3
        }
    }
}
```

---

## 🌌 Galaxy Formation Simulation

Large-scale gravitational simulation with different mass scales.

```json
{
    "simulation": {
        "particleCount": 1024,
        "timeStep": 0.02,
        "globalDamping": 0.001,
        "boundaryMin": [-20.0, -20.0, -20.0],
        "boundaryMax": [20.0, 20.0, 20.0],
        "boundaryRestitution": 0.0
    },
    "particleTypes": [
        {
            "id": 0,
            "name": "Star",
            "mass": 10.0,
            "radius": 0.2,
            "charge": 0.0,
            "material": {
                "diffuseColor": [1.0, 1.0, 0.8],
                "emissive": 1.0,
                "emissiveColor": [1.0, 0.9, 0.6]
            },
            "lodLevels": 3
        },
        {
            "id": 1,
            "name": "Planet",
            "mass": 1.0,
            "radius": 0.1,
            "charge": 0.0,
            "material": {
                "diffuseColor": [0.6, 0.8, 0.9],
                "shininess": 16.0
            },
            "lodLevels": 2
        },
        {
            "id": 2,
            "name": "Dust",
            "mass": 0.1,
            "radius": 0.05,
            "charge": 0.0,
            "material": {
                "diffuseColor": [0.7, 0.6, 0.5],
                "roughness": 0.9
            },
            "lodLevels": 1
        }
    ],
    "interactionRules": [
        {
            "typeA": 0,
            "typeB": 0,
            "forces": [
                {
                    "type": "Gravitational",
                    "strength": 0.5,
                    "range": 15.0,
                    "parameters": [6.67e-2]
                }
            ]
        },
        {
            "typeA": 0,
            "typeB": 1,
            "forces": [
                {
                    "type": "Gravitational",
                    "strength": 0.5,
                    "range": 10.0,
                    "parameters": [6.67e-2]
                }
            ]
        },
        {
            "typeA": 0,
            "typeB": 2,
            "forces": [
                {
                    "type": "Gravitational",
                    "strength": 0.3,
                    "range": 8.0,
                    "parameters": [6.67e-2]
                }
            ]
        },
        {
            "typeA": 1,
            "typeB": 1,
            "forces": [
                {
                    "type": "Gravitational",
                    "strength": 0.1,
                    "range": 5.0,
                    "parameters": [6.67e-3]
                }
            ]
        }
    ],
    "environmentalForces": [],
    "rendering": {
        "culling": {
            "maxRenderDistance": 100.0
        }
    }
}
```

---

## 🔗 Elastic Network (Spring-Mass System)

Connected particles forming a flexible mesh structure.

```json
{
    "simulation": {
        "particleCount": 144,
        "timeStep": 0.016,
        "globalDamping": 0.01,
        "boundaryMin": [-6.0, -6.0, -6.0],
        "boundaryMax": [6.0, 6.0, 6.0]
    },
    "particleTypes": [
        {
            "id": 0,
            "name": "Node",
            "mass": 1.0,
            "radius": 0.08,
            "material": {
                "diffuseColor": [0.8, 0.4, 0.2],
                "specularColor": [1.0, 0.8, 0.6],
                "shininess": 32.0
            }
        }
    ],
    "interactionRules": [
        {
            "typeA": 0,
            "typeB": 0,
            "forces": [
                {
                    "type": "Spring",
                    "strength": 1.0,
                    "range": 1.5,
                    "parameters": [25.0, 2.0, 0.8]
                },
                {
                    "type": "LennardJones",
                    "strength": 0.1,
                    "range": 0.4,
                    "parameters": [0.01, 0.2]
                }
            ]
        }
    ],
    "environmentalForces": [
        {
            "targetTypeId": 4294967295,
            "force": {
                "type": "ConstantField",
                "strength": 0.5,
                "parameters": [0.0, -5.0, 0.0, 1.0]
            }
        }
    ]
}
```

---

## 🌪️ Vortex Fluid Dynamics

Particles affected by swirling vortex forces.

```json
{
    "simulation": {
        "particleCount": 512,
        "timeStep": 0.012,
        "globalDamping": 0.02
    },
    "particleTypes": [
        {
            "id": 0,
            "name": "FluidParticle",
            "mass": 1.0,
            "radius": 0.08,
            "material": {
                "diffuseColor": [0.3, 0.7, 0.9],
                "metallic": 0.1,
                "roughness": 0.2
            }
        }
    ],
    "interactionRules": [
        {
            "typeA": 0,
            "typeB": 0,
            "forces": [
                {
                    "type": "LennardJones",
                    "strength": 0.5,
                    "range": 0.25,
                    "parameters": [0.05, 0.12]
                }
            ]
        }
    ],
    "environmentalForces": [
        {
            "targetTypeId": 0,
            "force": {
                "type": "VortexField",
                "strength": 2.0,
                "parameters": [0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 3.0]
            }
        },
        {
            "targetTypeId": 0,
            "force": {
                "type": "ConstantField",
                "strength": 0.2,
                "parameters": [0.0, -2.0, 0.0, 1.0]
            }
        }
    ]
}
```

---

## 🧪 Chemical Reaction Simulation

Multiple particle types with different interaction strengths.

```json
{
    "simulation": {
        "particleCount": 384,
        "timeStep": 0.016,
        "globalDamping": 0.015
    },
    "particleTypes": [
        {
            "id": 0,
            "name": "Reactant_A",
            "mass": 1.0,
            "radius": 0.1,
            "material": {
                "diffuseColor": [1.0, 0.2, 0.2],
                "emissive": 0.2,
                "emissiveColor": [1.0, 0.1, 0.1]
            }
        },
        {
            "id": 1,
            "name": "Reactant_B",
            "mass": 1.2,
            "radius": 0.12,
            "material": {
                "diffuseColor": [0.2, 1.0, 0.2],
                "emissive": 0.2,
                "emissiveColor": [0.1, 1.0, 0.1]
            }
        },
        {
            "id": 2,
            "name": "Product_C",
            "mass": 2.0,
            "radius": 0.15,
            "material": {
                "diffuseColor": [0.2, 0.2, 1.0],
                "emissive": 0.3,
                "emissiveColor": [0.1, 0.1, 1.0]
            }
        },
        {
            "id": 3,
            "name": "Catalyst",
            "mass": 0.5,
            "radius": 0.06,
            "material": {
                "diffuseColor": [1.0, 1.0, 0.2],
                "emissive": 0.8,
                "emissiveColor": [1.0, 0.8, 0.0]
            }
        }
    ],
    "interactionRules": [
        {
            "typeA": 0,
            "typeB": 1,
            "forces": [
                {
                    "type": "LennardJones",
                    "strength": 2.0,
                    "range": 0.4,
                    "parameters": [0.2, 0.18]
                }
            ]
        },
        {
            "typeA": 0,
            "typeB": 3,
            "forces": [
                {
                    "type": "LennardJones",
                    "strength": 1.5,
                    "range": 0.3,
                    "parameters": [0.1, 0.14]
                }
            ]
        },
        {
            "typeA": 1,
            "typeB": 3,
            "forces": [
                {
                    "type": "LennardJones",
                    "strength": 1.5,
                    "range": 0.3,
                    "parameters": [0.1, 0.16]
                }
            ]
        },
        {
            "typeA": 2,
            "typeB": 2,
            "forces": [
                {
                    "type": "LennardJones",
                    "strength": 0.5,
                    "range": 0.5,
                    "parameters": [0.05, 0.25]
                }
            ]
        }
    ],
    "environmentalForces": []
}
```

---

## 🏐 Bouncing Balls (Simple Physics)

Basic collision and gravity simulation for testing.

```json
{
    "simulation": {
        "particleCount": 64,
        "timeStep": 0.016,
        "globalDamping": 0.001,
        "boundaryRestitution": 0.8
    },
    "particleTypes": [
        {
            "id": 0,
            "name": "Ball",
            "mass": 1.0,
            "radius": 0.15,
            "material": {
                "diffuseColor": [0.8, 0.3, 0.8],
                "specularColor": [1.0, 1.0, 1.0],
                "shininess": 64.0
            },
            "lodLevels": 3
        }
    ],
    "interactionRules": [
        {
            "typeA": 0,
            "typeB": 0,
            "forces": [
                {
                    "type": "LennardJones",
                    "strength": 0.1,
                    "range": 0.35,
                    "parameters": [0.01, 0.3]
                }
            ]
        }
    ],
    "environmentalForces": [
        {
            "targetTypeId": 4294967295,
            "force": {
                "type": "ConstantField",
                "strength": 1.0,
                "parameters": [0.0, -9.81, 0.0, 1.0]
            }
        }
    ],
    "rendering": {
        "windowWidth": 1200,
        "windowHeight": 800,
        "enableVSync": true
    }
}
```

---

## 🎯 Performance Test Configuration

Minimal configuration for performance testing and benchmarking.

```json
{
    "simulation": {
        "particleCount": 2048,
        "timeStep": 0.016,
        "maxFrames": 3600,
        "enableDebugOutput": true,
        "debugOutputInterval": 60,
        "globalDamping": 0.01
    },
    "particleTypes": [
        {
            "id": 0,
            "name": "TestParticle",
            "mass": 1.0,
            "radius": 0.1,
            "material": {
                "diffuseColor": [0.5, 0.5, 0.5]
            },
            "lodLevels": 1
        }
    ],
    "interactionRules": [
        {
            "typeA": 0,
            "typeB": 0,
            "forces": [
                {
                    "type": "LennardJones",
                    "strength": 0.5,
                    "range": 0.3,
                    "parameters": [0.1, 0.15]
                }
            ]
        }
    ],
    "environmentalForces": [],
    "rendering": {
        "enableVSync": false,
        "culling": {
            "enableFrustumCulling": true,
            "enableLOD": true,
            "maxRenderDistance": 50.0
        }
    }
}
```

---

## 📝 Usage Tips

### Modifying Configurations
1. **Start Simple**: Begin with fewer particles and forces
2. **Test Changes**: Make one modification at a time
3. **Save Variants**: Keep working configurations in separate files
4. **Performance First**: Optimize for smooth frame rates before adding complexity

### Parameter Guidelines
- **Particle Count**: Start with 64-256, increase gradually
- **Time Step**: Smaller values (0.008-0.02) for stability
- **Force Strength**: Start with 0.1-1.0, adjust based on behavior
- **Interaction Range**: Keep 2-5x particle radius for performance

### Common Issues
- **Explosive Behavior**: Reduce force strengths or increase damping
- **Particles Stuck**: Check boundary settings and force ranges
- **Poor Performance**: Reduce particle count or enable more culling
- **No Movement**: Verify environmental forces or initial conditions

Copy any of these configurations to `config.json` and run the simulation to see different physical phenomena in action!
