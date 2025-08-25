/**
 * @file ShaderManager.h
 * @brief Header for the ShaderManager class - handles HLSL shader compilation and management
 * 
 * The ShaderManager provides centralized loading, compilation, and management of all
 * HLSL shaders used by the particle system. It supports all DirectX 11 shader types
 * (vertex, geometry, pixel, compute) and maintains shader caches for efficient access.
 * 
 * The system handles shader bytecode loading, compilation error reporting, and provides
 * type-safe retrieval of compiled shader objects. It can load shaders individually or
 * batch-load entire shader directories with automatic type detection.
 * 
 * @author DirectX 11 Particle System  
 * @date 2024
 */

#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include <d3d11.h>
#include <filesystem>
#include <unordered_map>
#include <string>

/**
 * @enum ShaderType
 * @brief Enumeration of supported HLSL shader types
 * 
 * Defines the different shader pipeline stages supported by the system:
 * - Vertex: Transform vertices and prepare for geometry processing
 * - Geometry: Generate icosphere geometry from point particles  
 * - Pixel: Calculate final pixel colors with Phong lighting
 * - Compute: Perform GPU-based frustum culling and other computations
 */
enum class ShaderType {
    Vertex,     ///< Vertex shader for vertex transformation and preparation
    Geometry,   ///< Geometry shader for icosphere generation from points
    Pixel,      ///< Pixel shader for Phong lighting and final color calculation
    Compute     ///< Compute shader for frustum culling and GPU computations
};

/**
 * @class ShaderManager
 * @brief Centralized management system for all HLSL shaders
 * 
 * Manages the complete lifecycle of HLSL shaders including:
 * - Loading and compilation of shader bytecode files
 * - Caching compiled shader objects for efficient access
 * - Type-safe retrieval of vertex, geometry, pixel, and compute shaders
 * - Batch loading of shader directories with automatic type detection
 * - Error handling and reporting for compilation failures
 * 
 * The shader manager expects pre-compiled bytecode files (typically .cso) rather
 * than source HLSL files, allowing for faster loading and better deployment.
 */
class ShaderManager {
private:
    std::shared_ptr<D3DDevice> device;  ///< DirectX device wrapper for shader creation
    
    // Shader caches organized by type for efficient lookup
    std::unordered_map<std::string, ComPtr<ID3D11VertexShader>> vertexShaders;     ///< Cache of compiled vertex shaders
    std::unordered_map<std::string, ComPtr<ID3D11GeometryShader>> geometryShaders; ///< Cache of compiled geometry shaders  
    std::unordered_map<std::string, ComPtr<ID3D11PixelShader>> pixelShaders;       ///< Cache of compiled pixel shaders
    std::unordered_map<std::string, ComPtr<ID3D11ComputeShader>> computeShaders;   ///< Cache of compiled compute shaders

public:
    /**
     * @brief Constructs a new ShaderManager with DirectX device dependency
     * @param device DirectX device wrapper for shader compilation operations
     */
    explicit ShaderManager(std::shared_ptr<D3DDevice> device);
    ~ShaderManager() = default;

    /**
     * @brief Loads and compiles a single shader from bytecode file
     * 
     * Loads a pre-compiled shader bytecode file and creates the appropriate
     * DirectX shader object based on the specified type.
     * 
     * @param name Unique identifier for the shader (used for later retrieval)
     * @param filePath Path to the compiled shader bytecode file (.cso)
     * @param type Type of shader to create (Vertex, Geometry, Pixel, Compute)
     * @return true if shader loaded and compiled successfully, false otherwise
     */
    bool LoadShader(const std::string& name, const std::filesystem::path& filePath, ShaderType type);
    
    /**
     * @brief Batch loads all shader files from a directory
     * 
     * Scans the specified directory for shader bytecode files and automatically
     * loads them with type detection based on filename patterns or content.
     * 
     * @param shaderDirectory Directory containing shader bytecode files
     * @return true if all shaders loaded successfully, false if any failed
     */
    bool LoadAllShaders(const std::filesystem::path& shaderDirectory);
    
    // Type-safe shader retrieval methods
    /**
     * @brief Retrieves a compiled vertex shader by name
     * @param name Name of the shader to retrieve
     * @return Pointer to vertex shader, or nullptr if not found
     */
    ID3D11VertexShader* GetVertexShader(const std::string& name);
    
    /**
     * @brief Retrieves a compiled geometry shader by name  
     * @param name Name of the shader to retrieve
     * @return Pointer to geometry shader, or nullptr if not found
     */
    ID3D11GeometryShader* GetGeometryShader(const std::string& name);
    
    /**
     * @brief Retrieves a compiled pixel shader by name
     * @param name Name of the shader to retrieve  
     * @return Pointer to pixel shader, or nullptr if not found
     */
    ID3D11PixelShader* GetPixelShader(const std::string& name);
    
    /**
     * @brief Retrieves a compiled compute shader by name
     * @param name Name of the shader to retrieve
     * @return Pointer to compute shader, or nullptr if not found
     */
    ID3D11ComputeShader* GetComputeShader(const std::string& name);

private:
    /**
     * @brief Reads shader bytecode from file into memory
     * @param filePath Path to the bytecode file
     * @return Vector containing the bytecode data, empty if read failed
     */
    std::vector<char> ReadShaderBytecode(const std::filesystem::path& filePath);
    
    /**
     * @brief Template function to create DirectX shader objects from bytecode
     * @tparam T Type of shader interface to create (ID3D11VertexShader, etc.)
     * @param bytecode Vector containing the compiled shader bytecode
     * @param shader Reference to ComPtr that will hold the created shader
     * @return true if shader creation succeeded, false otherwise
     */
    template<typename T>
    bool CreateShader(const std::vector<char>& bytecode, ComPtr<T>& shader);
};
