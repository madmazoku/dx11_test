#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include "ShaderManager.h"
#include "InteractiveCamera.h"
#include "Structures.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

/**
 * @file FrustumCuller.h
 * @brief GPU-based frustum culling and Level-of-Detail (LOD) system
 * 
 * This file implements a high-performance GPU-based culling system that eliminates
 * particles outside the camera view frustum and assigns appropriate LOD levels based
 * on distance from the camera. The system uses DirectX compute shaders to perform
 * culling operations in parallel on the GPU.
 * 
 * Key features:
 * - GPU-accelerated frustum culling for maximum performance
 * - Distance-based Level-of-Detail assignment
 * - Backface culling for optimization
 * - Comprehensive culling statistics
 * - Real-time parameter adjustment
 * - Memory-efficient GPU buffer management
 * 
 * The culling system significantly improves rendering performance by reducing
 * the number of particles that need to be processed by subsequent rendering stages.
 */

/**
 * @struct CullingData
 * @brief GPU culling result data for each particle
 * 
 * This structure contains the culling results computed by the GPU compute shader.
 * It matches the HLSL structure layout exactly to ensure proper GPU memory access.
 * Each particle gets one CullingData entry with visibility and LOD information.
 */
struct CullingData {
    UINT isVisible;                 ///< 1 if particle is visible, 0 if culled
    float distanceToCamera;         ///< Distance from camera to particle center
    UINT lodLevel;                  ///< Level-of-detail (0=high, 1=medium, 2=low)
    UINT padding;                   ///< Padding for 16-byte alignment
};

/**
 * @struct FrustumConstants
 * @brief GPU constant buffer for frustum culling parameters
 * 
 * This structure is passed to the GPU compute shader and contains all parameters
 * needed for frustum culling calculations. It must maintain 16-byte alignment
 * for DirectX constant buffer requirements.
 * 
 * The frustum planes are in the standard format: ax + by + cz + d = 0,
 * where (a,b,c) is the plane normal and d is the distance from origin.
 */
struct alignas(16) FrustumConstants {
    XMFLOAT4 frustumPlanes[6];      ///< View frustum planes: Left, Right, Top, Bottom, Near, Far
    XMFLOAT3 cameraPosition;        ///< Camera world position
    float maxRenderDistance;        ///< Maximum rendering distance (beyond = culled)
    XMFLOAT3 cameraForward;         ///< Camera forward direction vector
    float nearPlane;                ///< Near clipping plane distance
    XMFLOAT3 cameraUp;              ///< Camera up direction vector
    float farPlane;                 ///< Far clipping plane distance
    XMFLOAT3 cameraRight;           ///< Camera right direction vector
    float fovY;                     ///< Vertical field of view in radians
    float lodDistance1;             ///< Distance threshold for high->medium LOD
    float lodDistance2;             ///< Distance threshold for medium->low LOD
    float lodDistance3;             ///< Distance threshold for low->minimal LOD
    float aspectRatio;              ///< Screen aspect ratio (width/height)
};

/**
 * @class FrustumCuller
 * @brief GPU-based frustum culling and LOD management system
 * 
 * This class implements a high-performance culling system that runs entirely on the GPU
 * using DirectX compute shaders. It performs view frustum culling to eliminate particles
 * outside the camera view and assigns Level-of-Detail levels based on distance.
 * 
 * Architecture:
 * 1. Initialize GPU buffers and compute shader resources
 * 2. Update frustum parameters based on camera state
 * 3. Dispatch compute shader to process all particles in parallel
 * 4. Generate culling results and statistics
 * 5. Provide culling data to rendering pipeline
 * 
 * Performance benefits:
 * - GPU parallelization of culling calculations
 * - Reduced CPU-GPU synchronization
 * - Memory-efficient buffer management
 * - Real-time culling parameter updates
 * 
 * The system integrates seamlessly with the particle rendering pipeline,
 * allowing for efficient rendering of large particle counts.
 */
class FrustumCuller {
private:
    // === Dependencies ===
    std::shared_ptr<D3DDevice> device;         ///< DirectX device for GPU operations
    std::shared_ptr<ShaderManager> shaderManager; ///< Shader management system
    
    // === GPU Resources ===
    ComPtr<ID3D11Buffer> frustumConstantsBuffer;    ///< Constant buffer for frustum parameters
    ComPtr<ID3D11Buffer> cullingDataBuffer;         ///< GPU buffer containing culling results
    ComPtr<ID3D11Buffer> cullingDataStagingBuffer;  ///< CPU-readable staging buffer
    ComPtr<ID3D11ShaderResourceView> cullingDataSRV; ///< Shader resource view for reading
    ComPtr<ID3D11UnorderedAccessView> cullingDataUAV; ///< Unordered access view for writing
    
    /**
     * @struct CullingSettings
     * @brief Configuration parameters for the culling system
     * 
     * These settings control various aspects of the culling behavior and can be
     * adjusted at runtime to fine-tune performance vs. quality trade-offs.
     */
    struct CullingSettings {
        float maxRenderDistance = 100.0f;      ///< Maximum render distance before culling
        float lodDistance1 = 10.0f;            ///< Distance for high detail LOD threshold
        float lodDistance2 = 25.0f;            ///< Distance for medium detail LOD threshold
        float lodDistance3 = 50.0f;            ///< Distance for low detail LOD threshold
        bool enableLOD = true;                 ///< Enable distance-based LOD system
        bool enableBackfaceCulling = true;     ///< Enable backface culling optimization
        bool enableDistanceCulling = true;     ///< Enable maximum distance culling
    } settings;                                ///< Current culling configuration
    
    size_t particleCount = 0;                  ///< Current number of particles being culled
    std::vector<CullingData> cullingResults;   ///< CPU copy of culling results for analysis
    
    /**
     * @struct CullingStats
     * @brief Statistical information about culling performance
     * 
     * Provides detailed metrics about the culling process for performance
     * monitoring and optimization. Updated each frame after culling.
     */
    struct CullingStats {
        UINT totalParticles = 0;        ///< Total particles processed
        UINT visibleParticles = 0;      ///< Particles passing all culling tests
        UINT culledByFrustum = 0;       ///< Particles culled by frustum test
        UINT culledByDistance = 0;      ///< Particles culled by distance test
        UINT culledByBackface = 0;      ///< Particles culled by backface test
        UINT lodLevel0Count = 0;        ///< Particles at high detail LOD
        UINT lodLevel1Count = 0;        ///< Particles at medium detail LOD
        UINT lodLevel2Count = 0;        ///< Particles at low detail LOD
        float cullingTimeMs = 0.0f;     ///< Time spent in culling pass (milliseconds)
    } stats;                           ///< Current frame culling statistics

public:
    // === Construction and Lifecycle ===
    
    /**
     * @brief Construct FrustumCuller with required dependencies
     * 
     * @param device DirectX device for GPU resource creation
     * @param shaderManager Shader manager for compute shader access
     */
    FrustumCuller(std::shared_ptr<D3DDevice> device, 
                  std::shared_ptr<ShaderManager> shaderManager);
    
    /**
     * @brief Default destructor
     * 
     * GPU resources are automatically cleaned up by ComPtr destructors
     */
    ~FrustumCuller() = default;
    
    /**
     * @brief Initialize the frustum culling system
     * 
     * Creates all necessary GPU buffers and resources for culling operations.
     * Must be called before performing any culling operations.
     * 
     * @param maxParticles Maximum number of particles that will be processed
     * @return true if initialization successful, false on error
     */
    bool Initialize(size_t maxParticles);
    
    // === Core Operations ===
    
    /**
     * @brief Perform GPU-based frustum culling on particles
     * 
     * Dispatches a compute shader to process all particles in parallel,
     * determining visibility and LOD level for each particle based on
     * the current camera state and culling settings.
     * 
     * @param camera Interactive camera providing view/projection matrices
     * @param particlesSRV Shader resource view of particle data to cull
     * @param aspectRatio Screen aspect ratio for frustum calculation
     */
    void PerformCulling(const InteractiveCamera& camera, 
                       ID3D11ShaderResourceView* particlesSRV,
                       float aspectRatio);
    
    // === Results Access ===
    
    /**
     * @brief Get shader resource view of culling results
     * 
     * Provides access to the GPU buffer containing culling results for
     * use by subsequent rendering passes.
     * 
     * @return Shader resource view containing CullingData for each particle
     */
    ID3D11ShaderResourceView* GetCullingDataSRV() const { return cullingDataSRV.get(); }
    
    /**
     * @brief Get statistical information about last culling pass
     * 
     * @return Reference to CullingStats structure with performance metrics
     */
    const CullingStats& GetStats() const { return stats; }
    
    // === Configuration Methods ===
    
    /**
     * @brief Set maximum render distance
     * 
     * Particles beyond this distance will be culled regardless of
     * frustum visibility. Useful for performance optimization.
     * 
     * @param distance Maximum render distance in world units
     */
    void SetMaxRenderDistance(float distance) { settings.maxRenderDistance = distance; }
    
    /**
     * @brief Configure Level-of-Detail distance thresholds
     * 
     * Sets the distance thresholds for automatic LOD assignment.
     * Particles closer than lod1 use high detail, between lod1 and lod2
     * use medium detail, etc.
     * 
     * @param lod1 High to medium detail transition distance
     * @param lod2 Medium to low detail transition distance
     * @param lod3 Low to minimal detail transition distance
     */
    void SetLODDistances(float lod1, float lod2, float lod3) {
        settings.lodDistance1 = lod1;
        settings.lodDistance2 = lod2;
        settings.lodDistance3 = lod3;
    }
    
    /**
     * @brief Enable or disable Level-of-Detail system
     * @param enable true to enable LOD, false to use maximum detail for all particles
     */
    void EnableLOD(bool enable) { settings.enableLOD = enable; }
    
    /**
     * @brief Enable or disable backface culling optimization
     * @param enable true to enable backface culling, false to disable
     */
    void EnableBackfaceCulling(bool enable) { settings.enableBackfaceCulling = enable; }
    
    /**
     * @brief Enable or disable distance-based culling
     * @param enable true to enable distance culling, false to disable
     */
    void EnableDistanceCulling(bool enable) { settings.enableDistanceCulling = enable; }
    
    // === Debug and Analysis ===
    
    /**
     * @brief Read culling results back from GPU to CPU
     * 
     * Performs a synchronous read-back of culling results from GPU memory
     * to CPU memory for analysis. This is a costly operation and should
     * only be used for debugging or offline analysis.
     * 
     * @return Vector containing culling results for all particles
     */
    std::vector<CullingData> ReadBackCullingResults();
    
    /**
     * @brief Print culling statistics to debug output
     * 
     * Outputs detailed statistics about the last culling pass including
     * visibility counts, LOD distribution, and timing information.
     */
    void PrintCullingStats() const;
    
private:
    // === Internal Implementation Methods ===
    
    /**
     * @brief Create and initialize GPU buffers for culling
     * 
     * Allocates GPU memory for frustum constants, culling results, and
     * staging buffers. Sets up shader resource views and unordered access
     * views for compute shader access.
     * 
     * @param maxParticles Maximum number of particles to allocate buffer space for
     * @return true if buffer creation successful, false on error
     */
    bool CreateBuffers(size_t maxParticles);
    
    /**
     * @brief Update frustum constant buffer with current camera state
     * 
     * Extracts view frustum planes from camera matrices and updates the
     * GPU constant buffer with current frustum parameters, camera vectors,
     * and LOD distances.
     * 
     * @param camera Interactive camera with current view/projection matrices
     * @param aspectRatio Screen aspect ratio for frustum calculation
     */
    void UpdateFrustumConstants(const InteractiveCamera& camera, float aspectRatio);
    
    /**
     * @brief Extract frustum planes from view-projection matrix
     * 
     * Computes the six frustum planes (left, right, top, bottom, near, far)
     * from the combined view-projection matrix using standard plane extraction
     * algorithms. Planes are normalized for distance calculations.
     * 
     * @param viewProjectionMatrix Combined view-projection transformation matrix
     * @param planes Output array of 6 frustum planes in standard form
     */
    void ExtractFrustumPlanes(const XMMATRIX& viewProjectionMatrix, XMFLOAT4 planes[6]);
    
    /**
     * @brief Update culling statistics from GPU results
     * 
     * Reads back a subset of culling results to compute statistics about
     * visibility, LOD distribution, and culling efficiency. Used for
     * performance monitoring and optimization.
     */
    void UpdateStatistics();
    
    /**
     * @brief Normalize a frustum plane equation
     * 
     * Normalizes a plane equation of the form ax + by + cz + d = 0
     * by dividing by the length of the normal vector (a, b, c).
     * This ensures correct distance calculations.
     * 
     * @param plane Input plane equation to normalize
     * @return Normalized plane equation
     */
    XMFLOAT4 NormalizePlane(const XMFLOAT4& plane);
};
