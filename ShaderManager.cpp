#include "ShaderManager.h"
#include "Utilities.h"
#include <iostream>
#include <fstream>

ShaderManager::ShaderManager(std::shared_ptr<D3DDevice> device) : device(device) {}

bool ShaderManager::LoadAllShaders(const std::filesystem::path& shaderDirectory) {
    try {
        LoadShader("compute", shaderDirectory / "ComputeShader.cso", ShaderType::Compute);
        LoadShader("vertex", shaderDirectory / "VertexShader.vso", ShaderType::Vertex);
        LoadShader("geometry", shaderDirectory / "GeometryShader.gso", ShaderType::Geometry);
        LoadShader("pixel", shaderDirectory / "PixelShader.pso", ShaderType::Pixel);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load shaders: " << e.what() << std::endl;
        return false;
    }
}

bool ShaderManager::LoadShader(const std::string& name, const std::filesystem::path& filePath, ShaderType type) {
    try {
        std::cout << "Loading shader: " << filePath << std::endl;
        auto bytecode = ReadShaderBytecode(filePath);
        
        switch (type) {
            case ShaderType::Vertex: {
                ComPtr<ID3D11VertexShader> shader;
                if (CreateShader(bytecode, shader)) {
                    vertexShaders[name] = std::move(shader);
                    return true;
                }
                break;
            }
            case ShaderType::Geometry: {
                ComPtr<ID3D11GeometryShader> shader;
                if (CreateShader(bytecode, shader)) {
                    geometryShaders[name] = std::move(shader);
                    return true;
                }
                break;
            }
            case ShaderType::Pixel: {
                ComPtr<ID3D11PixelShader> shader;
                if (CreateShader(bytecode, shader)) {
                    pixelShaders[name] = std::move(shader);
                    return true;
                }
                break;
            }
            case ShaderType::Compute: {
                ComPtr<ID3D11ComputeShader> shader;
                if (CreateShader(bytecode, shader)) {
                    computeShaders[name] = std::move(shader);
                    return true;
                }
                break;
            }
        }
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load shader " << name << ": " << e.what() << std::endl;
        return false;
    }
}

std::vector<char> ShaderManager::ReadShaderBytecode(const std::filesystem::path& filePath) {
    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("Shader file does not exist: " + filePath.string());
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filePath.string());
    }

    auto fileSize = std::filesystem::file_size(filePath);
    std::vector<char> bytecode(fileSize);
    file.read(bytecode.data(), fileSize);
    
    return bytecode;
}

template<typename T>
bool ShaderManager::CreateShader(const std::vector<char>& bytecode, ComPtr<T>& shader) {
    if constexpr (std::is_same_v<T, ID3D11VertexShader>) {
        HRESULT hr = device->GetDevice()->CreateVertexShader(
            bytecode.data(), bytecode.size(), nullptr, shader.getAddressOf()
        );
        return SUCCEEDED(hr);
    }
    else if constexpr (std::is_same_v<T, ID3D11GeometryShader>) {
        HRESULT hr = device->GetDevice()->CreateGeometryShader(
            bytecode.data(), bytecode.size(), nullptr, shader.getAddressOf()
        );
        return SUCCEEDED(hr);
    }
    else if constexpr (std::is_same_v<T, ID3D11PixelShader>) {
        HRESULT hr = device->GetDevice()->CreatePixelShader(
            bytecode.data(), bytecode.size(), nullptr, shader.getAddressOf()
        );
        return SUCCEEDED(hr);
    }
    else if constexpr (std::is_same_v<T, ID3D11ComputeShader>) {
        HRESULT hr = device->GetDevice()->CreateComputeShader(
            bytecode.data(), bytecode.size(), nullptr, shader.getAddressOf()
        );
        return SUCCEEDED(hr);
    }
    return false;
}

ID3D11VertexShader* ShaderManager::GetVertexShader(const std::string& name) {
    auto it = vertexShaders.find(name);
    return it != vertexShaders.end() ? it->second.get() : nullptr;
}

ID3D11GeometryShader* ShaderManager::GetGeometryShader(const std::string& name) {
    auto it = geometryShaders.find(name);
    return it != geometryShaders.end() ? it->second.get() : nullptr;
}

ID3D11PixelShader* ShaderManager::GetPixelShader(const std::string& name) {
    auto it = pixelShaders.find(name);
    return it != pixelShaders.end() ? it->second.get() : nullptr;
}

ID3D11ComputeShader* ShaderManager::GetComputeShader(const std::string& name) {
    auto it = computeShaders.find(name);
    return it != computeShaders.end() ? it->second.get() : nullptr;
}
