#pragma once

#include "ComPtr.h"
#include "D3DDevice.h"
#include <d3d11.h>
#include <filesystem>
#include <unordered_map>
#include <string>

enum class ShaderType {
    Vertex,
    Geometry,
    Pixel,
    Compute
};

class ShaderManager {
private:
    std::shared_ptr<D3DDevice> device;
    
    std::unordered_map<std::string, ComPtr<ID3D11VertexShader>> vertexShaders;
    std::unordered_map<std::string, ComPtr<ID3D11GeometryShader>> geometryShaders;
    std::unordered_map<std::string, ComPtr<ID3D11PixelShader>> pixelShaders;
    std::unordered_map<std::string, ComPtr<ID3D11ComputeShader>> computeShaders;

public:
    explicit ShaderManager(std::shared_ptr<D3DDevice> device);
    ~ShaderManager() = default;

    bool LoadShader(const std::string& name, const std::filesystem::path& filePath, ShaderType type);
    bool LoadAllShaders(const std::filesystem::path& shaderDirectory);
    
    ID3D11VertexShader* GetVertexShader(const std::string& name);
    ID3D11GeometryShader* GetGeometryShader(const std::string& name);
    ID3D11PixelShader* GetPixelShader(const std::string& name);
    ID3D11ComputeShader* GetComputeShader(const std::string& name);

private:
    std::vector<char> ReadShaderBytecode(const std::filesystem::path& filePath);
    template<typename T>
    bool CreateShader(const std::vector<char>& bytecode, ComPtr<T>& shader);
};
