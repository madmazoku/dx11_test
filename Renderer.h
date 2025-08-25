/**
 * @file Renderer.h
 * @brief Header for the Renderer class - handles DirectX 11 particle rendering pipeline
 * 
 * The Renderer class manages the complete rendering pipeline for multi-type particle
 * systems using DirectX 11. It supports:
 * - Icosphere geometry generation via geometry shaders
 * - Phong shading with per-particle-type materials  
 * - Frustum culling for performance optimization
 * - Dynamic camera controls and projection
 * - Instanced rendering for efficient particle display
 * 
 * The rendering pipeline uses compute shaders for culling, geometry shaders for
 * icosphere generation, and pixel shaders for realistic Phong lighting. Materials
 * are managed per particle type with support for different visual properties.
 * 
 * @author DirectX 11 Particle System
 * @date 2024
 */

#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include "ShaderManager.h"
#include "ConfigManager.h"
#include "FrustumCuller.h"
#include "Structures.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

using namespace DirectX;

/**
 * @class Renderer
 * @brief Main rendering engine for the particle simulation system
 * 
 * Manages the complete DirectX 11 rendering pipeline including:
 * - Particle instancing and geometry generation
 * - Material property management for different particle types
 * - Frustum culling integration for performance
 * - Camera and projection matrix management  
 * - Lighting calculations and Phong shading
 * - Render state management (rasterizer, depth, blending)
 * 
 * The renderer is designed to handle thousands of particles efficiently using
 * modern GPU rendering techniques including geometry shaders and compute culling.
 */
class Renderer {
private:
    // Core DirectX and shader management systems
    std::shared_ptr<D3DDevice> device;           ///< DirectX device wrapper for GPU operations
    std::shared_ptr<ShaderManager> shaderManager; ///< Shader compilation and management system  
    std::shared_ptr<ConfigManager> configManager; ///< Configuration parameter access
    std::unique_ptr<FrustumCuller> frustumCuller; ///< GPU-based frustum culling system
    
    // Material data for all particle types
    std::vector<MaterialProperties> materials;   ///< Per-particle-type material properties
    
    // Constant buffers for GPU data transfer
    ComPtr<ID3D11Buffer> transformBuffer;        ///< View/projection matrices and camera position
    ComPtr<ID3D11Buffer> lightBuffer;           ///< Light position, color, and parameters  
    ComPtr<ID3D11Buffer> simulationConstantsBuffer; ///< Simulation parameters like sphere radius
    ComPtr<ID3D11Buffer> materialBuffer;        ///< Material properties for all particle types
    ComPtr<ID3D11Buffer> cullingBuffer;         ///< Frustum planes for GPU culling
    
    // Shader resource views for GPU data access
    ComPtr<ID3D11ShaderResourceView> materialBufferSRV; ///< Shader access to material data
    
    // Render states for consistent rendering configuration  
    ComPtr<ID3D11RasterizerState> rasterizerState;    ///< Backface culling and fill mode
    ComPtr<ID3D11DepthStencilState> depthStencilState; ///< Z-buffer testing configuration
    ComPtr<ID3D11BlendState> blendState;              ///< Alpha blending settings
    
    // Camera parameters for view calculations
    XMFLOAT3 cameraPosition;     ///< Current camera world position
    float cameraDistance = 10.0f; ///< Distance from camera target
    float cameraAngle = 0.0f;    ///< Rotation angle around target
    
    // Culling settings for performance optimization
    bool enableFrustumCulling = true; ///< Whether to enable GPU frustum culling

    // Helper methods for initialization and updates
    bool CreateConstantBuffers();     ///< Creates all constant buffers for GPU data transfer
    bool CreateMaterialBuffer();      ///< Creates and initializes material property buffer
    bool CreateRenderStates();        ///< Sets up rasterizer, depth, and blend states
    bool CreateCullingBuffer();       ///< Creates buffer for frustum culling planes
    
    /**
     * @brief Updates transform matrices and camera position on GPU
     * @param viewMatrix Camera view transformation matrix  
     * @param projectionMatrix Camera projection matrix
     * @param cameraPos Camera position in world space
     */
    void UpdateTransformBuffer(const XMMATRIX& viewMatrix, 
                              const XMMATRIX& projectionMatrix, 
                              const XMFLOAT3& cameraPos);
    
    /**
     * @brief Updates lighting parameters for Phong shading
     * @param cameraPos Camera position for lighting calculations
     */                          
    void UpdateLightBuffer(const XMFLOAT3& cameraPos);
    
    /**
     * @brief Gets the culling buffer for frustum culling operations
     * @return Pointer to culling constant buffer
     */
    ID3D11Buffer** GetCullingBuffer();

public:
    /**
     * @brief Constructs a new Renderer with required dependencies
     * @param device DirectX device wrapper for GPU operations
     * @param shaderManager Shader compilation and management system
     * @param configManager Configuration parameter access
     */
    Renderer(std::shared_ptr<D3DDevice> device, 
             std::shared_ptr<ShaderManager> shaderManager,
             std::shared_ptr<ConfigManager> configManager);
    ~Renderer() = default;

    /**
     * @brief Initializes the renderer with default settings
     * @return true if initialization succeeded, false otherwise
     */
    bool Initialize();
    
    /**
     * @brief Initializes renderer with specified maximum particle count
     * @param maxParticles Maximum number of particles to support
     * @return true if initialization succeeded, false otherwise  
     */
    bool InitializeWithMaxParticles(size_t maxParticles);
    
    /**
     * @brief Renders a complete frame with camera-based perspective
     * 
     * Performs the full rendering pipeline including frustum culling,
     * geometry generation, material application, and Phong shading.
     * 
     * @param particles Vector of all particles to potentially render
     * @param viewMatrix Camera view transformation matrix
     * @param projectionMatrix Camera projection matrix  
     * @param cameraPos Camera position in world space
     */
    void RenderFrameWithCamera(const std::vector<Particle>& particles, 
                              const XMMATRIX& viewMatrix, 
                              const XMMATRIX& projectionMatrix,
                              const XMFLOAT3& cameraPos);
    
    /**
     * @brief Updates simulation constants used by shaders
     * @param particles Current particle data for radius calculations
     */
    void UpdateSimulationConstants(const std::vector<Particle>& particles);
    
    // Camera control methods
    /**
     * @brief Sets the camera position in world space
     * @param position New camera position
     */
    void SetCameraPosition(const XMFLOAT3& position);
    
    /**
     * @brief Gets the current camera position
     * @return Current camera position in world space
     */
    XMFLOAT3 GetCameraPosition() const;
    
    // Culling control methods
    /**
     * @brief Enables or disables frustum culling optimization
     * @param enable Whether to enable frustum culling
     */
    void EnableFrustumCulling(bool enable);
    
    /**
     * @brief Checks if frustum culling is currently enabled
     * @return true if frustum culling is enabled
     */
    bool IsFrustumCullingEnabled() const { return enableFrustumCulling; }
    
    // Material system methods  
    /**
     * @brief Gets the current material properties for all particle types
     * @return Reference to material properties vector
     */
    const std::vector<MaterialProperties>& GetMaterials() const { return materials; }
    
    /**
     * @brief Updates material properties for all particle types
     * @param newMaterials New material properties to apply
     */
    void UpdateMaterials(const std::vector<MaterialProperties>& newMaterials);
};
