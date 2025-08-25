#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "dxgi.lib")

#include <Windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>
#include <array>
#include <cmath>

struct float3 {
	float x, y, z;
};

struct Point
{
	float position[3];
	float oldPosition[3];
	float acceleration[3];
};

struct Vertex
{
	float position[4];
};

struct TransformBuffer
{
	float viewProjectionMatrix[16];
	float worldMatrix[16];
	float cameraPos[3];
	float sphereRadius;
};

struct LightBuffer
{
	float lightDirection[3];
	float lightIntensity;
	float lightColor[3];
	float ambientIntensity;
	float cameraPos[3];
	float specularPower;
};

const size_t POINTS_COUNT = 64;

ID3D11Device* device = nullptr;                  // Direct3D device
ID3D11DeviceContext* context = nullptr;          // Device context for executing commands
IDXGISwapChain* swapChain = nullptr;             // Swap chain for presenting
ID3D11RenderTargetView* renderTargetView = nullptr; // Render target view
ID3D11DepthStencilView* depthStencilView = nullptr;  // Depth stencil view
ID3D11Texture2D* depthStencilBuffer = nullptr;      // Depth stencil buffer

ID3D11ComputeShader* computeShader = nullptr;    // Compute shader
ID3D11Buffer* pointsBufferA = nullptr;           // Buffer A with point data
ID3D11Buffer* pointsBufferB = nullptr;           // Buffer B with point data
ID3D11ShaderResourceView* pointsSRVA = nullptr;  // Resource View A for reading the buffer
ID3D11ShaderResourceView* pointsSRVB = nullptr;  // Resource View B for reading the buffer
ID3D11UnorderedAccessView* pointsUAVA = nullptr; // Unordered Access View A for writing to the buffer
ID3D11UnorderedAccessView* pointsUAVB = nullptr; // Unordered Access View B for writing to the buffer

ID3D11VertexShader* vertexShader = nullptr;      // Vertex shader
ID3D11GeometryShader* geometryShader = nullptr;  // Geometry shader
ID3D11PixelShader* pixelShader = nullptr;        // Pixel shader

ID3D11Buffer* transformBuffer = nullptr;         // Transform constant buffer
ID3D11Buffer* lightBuffer = nullptr;             // Light constant buffer
ID3D11RasterizerState* rasterizerState = nullptr; // Rasterizer state
ID3D11DepthStencilState* depthStencilState = nullptr; // Depth stencil state

std::string HumanReadableSize(uint64_t size)
{
	constexpr std::array<std::pair<const char*, uint64_t>, 5> units = { {
		{"B", 1},
		{"KB", 1024},
		{"MB", 1024 * 1024},
		{"GB", 1024 * 1024 * 1024},
		{"TB", 1024ull * 1024 * 1024 * 1024}
	} };

	for (auto it = units.rbegin(); it != units.rend(); ++it)
	{
		if (size >= it->second)
		{
			std::string result = std::format("{:.2f}{}", static_cast<double>(size) / it->second, it->first);
			return result;
		}
	}

	return std::format("{:.2f}B", static_cast<double>(size)); // Fallback for sizes less than 1B
}

std::string MakeFailureMessage(HRESULT hr)
{
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&messageBuffer,
		0,
		nullptr
	);

	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);

	return message;
}

void ThrowIfFailure(HRESULT hr, const std::string& message)
{
	if (!FAILED(hr)) return;

	std::string failure_message = MakeFailureMessage(hr);

	// Remove trailing space-like characters
	failure_message.erase(failure_message.find_last_not_of(" \t\n\r\f\v") + 1);

	// Make full error message
	std::string fullErrorMessage = std::format(
		"{} {} HRESULT: 0x{:08X}L",
		message,
		failure_message,
		static_cast<unsigned long>(hr)
	);

	throw std::runtime_error(fullErrorMessage);
}

std::string ConvertWideToNarrow(const std::wstring& wideString)
{
	if (wideString.empty()) return std::string();

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wideString[0], (int)wideString.size(), NULL, 0, NULL, NULL);
	std::string narrowString(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wideString[0], (int)wideString.size(), &narrowString[0], size_needed, NULL, NULL);
	return narrowString;
}

inline UINT SafeSizeTToUINT(size_t sz)
{
	static const size_t szMaxUint = static_cast<size_t>(UINT_MAX);
	if (sz > szMaxUint)
	{
		std::string err_msg = std::format("size_t to UINT cast overflow: {} exceeded max UINT value {}", sz, szMaxUint);
		throw std::out_of_range(err_msg);
	}
	return static_cast<UINT>(sz);
}

std::filesystem::path ExecutableDirectory()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	return std::filesystem::path(buffer).parent_path();
}

template <typename T>
size_t GetBufferSize(ID3D11Buffer* buffer)
{
	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);
	return desc.ByteWidth / sizeof(T);
}

std::vector<char> ReadFileToByteVector(const std::filesystem::path& filePath)
{
	// Check if the file exists
	if (!std::filesystem::exists(filePath))
	{
		throw std::runtime_error("Shader file does not exist: " + filePath.string());
	}

	// Open the file
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open shader file: " + filePath.string());
	}

	// Determine the file size
	auto fileSize = std::filesystem::file_size(filePath);

	// Read the file into memory
	std::vector<char> shaderData(fileSize);
	file.read(shaderData.data(), fileSize);
	file.close();

	return shaderData; // Move semantics are applied here
}

ID3D11ComputeShader* LoadComputeShader(ID3D11Device* device, const std::filesystem::path& filePath)
{
	std::cout << "Read compute shader: " << filePath << std::endl;

	std::vector<char> shaderData = ReadFileToByteVector(filePath);

	// Create the shader from the bytecode
	ID3D11ComputeShader* shader = nullptr;
	HRESULT hr = device->CreateComputeShader(shaderData.data(), shaderData.size(), nullptr, &shader);
	ThrowIfFailure(hr, "Failed to create compute shader from .cso file!");

	return shader;
}

ID3D11VertexShader* LoadVertexShader(ID3D11Device* device, const std::filesystem::path& filePath)
{
	std::cout << "Read vertex shader: " << filePath << std::endl;

	std::vector<char> shaderData = ReadFileToByteVector(filePath);

	// Create the vertex shader from the bytecode
	ID3D11VertexShader* shader = nullptr;
	HRESULT hr = device->CreateVertexShader(shaderData.data(), shaderData.size(), nullptr, &shader);
	ThrowIfFailure(hr, "Failed to create vertex shader from .cso file!");

	return shader;
}

ID3D11GeometryShader* LoadGeometryShader(ID3D11Device* device, const std::filesystem::path& filePath)
{
	std::cout << "Read geometry shader: " << filePath << std::endl;

	std::vector<char> shaderData = ReadFileToByteVector(filePath);

	// Create the vertex shader from the bytecode
	ID3D11GeometryShader* shader = nullptr;
	HRESULT hr = device->CreateGeometryShader(shaderData.data(), shaderData.size(), nullptr, &shader);
	ThrowIfFailure(hr, "Failed to create vertex shader from .cso file!");

	return shader;
}

ID3D11PixelShader* LoadPixelShader(ID3D11Device* device, const std::filesystem::path& filePath)
{
	std::cout << "Read pixel shader: " << filePath << std::endl;

	std::vector<char> shaderData = ReadFileToByteVector(filePath);

	// Create the pixel shader from the bytecode
	ID3D11PixelShader* shader = nullptr;
	HRESULT hr = device->CreatePixelShader(shaderData.data(), shaderData.size(), nullptr, &shader);
	ThrowIfFailure(hr, "Failed to create pixel shader from .cso file!");

	return shader;
}

void DumpAdapterDesc(const std::string& name, IDXGIAdapter* adapter, const std::string& ident)
{
	DXGI_ADAPTER_DESC desc;
	ThrowIfFailure(adapter->GetDesc(&desc), "Failed to get adapter's desc");
	std::cout << ident << name << ": " << ConvertWideToNarrow(desc.Description) << std::endl;
	std::cout << ident << "\tVendor ID: " << std::format("0x{:X}", desc.VendorId) << std::endl;
	std::cout << ident << "\tDevice ID: " << std::format("0x{:X}", desc.DeviceId) << std::endl;
	std::cout << ident << "\tSubSys ID: " << std::format("0x{:X}", desc.SubSysId) << std::endl;
	std::cout << ident << "\tRevision: " << desc.Revision << std::endl;
	std::cout << ident << "\tDedicated Video Memory: " << HumanReadableSize(desc.DedicatedVideoMemory) << std::endl;
	std::cout << ident << "\tDedicated System Memory: " << HumanReadableSize(desc.DedicatedSystemMemory) << std::endl;
	std::cout << ident << "\tShared System Memory: " << HumanReadableSize(desc.SharedSystemMemory) << std::endl;
}

std::pair<IDXGIAdapter*, D3D_DRIVER_TYPE> DetermineBestAdapter()
{
	IDXGIFactory* factory = nullptr;
	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));
	ThrowIfFailure(hr, "Failed to create DXGIFactory.");

	IDXGIAdapter* bestAdapter = nullptr;
	SIZE_T maxDedicatedVideoMemory = 0;

	std::cout << "Adapters" << std::endl;

	for (UINT i = 0; ; ++i)
	{
		IDXGIAdapter* adapter = nullptr;
		hr = factory->EnumAdapters(i, &adapter);
		if (hr == DXGI_ERROR_NOT_FOUND) { break; } // No more adapters to enumerate
		ThrowIfFailure(hr, "Failed to enumerate adapters.");

		DumpAdapterDesc("Adapter", adapter, "\t");

		DXGI_ADAPTER_DESC desc;
		ThrowIfFailure(adapter->GetDesc(&desc), "Failed to get adapter's desc");

		// Omit software adapter
		//   0x1414 : This is the Vendor ID for Microsoft.
		//   0x8c   : This is the Device ID for the Microsoft Basic Render Driver.
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
		{
			std::cout << "\tSoftware Adapter. Skip." << std::endl;
			adapter->Release();
			continue;
		}

		if (desc.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			if (bestAdapter)
			{
				bestAdapter->Release();
			}
			bestAdapter = adapter;
			maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
		}
		else
		{
			adapter->Release();
		}
	}

	factory->Release();

	D3D_DRIVER_TYPE driverType = bestAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
	return { bestAdapter, driverType };
}

void InitD3D(HWND hWnd)
{
	// Determine the best adapter
	auto [bestAdapter, driverType] = DetermineBestAdapter();

	if (bestAdapter)
	{
		DumpAdapterDesc("Best Adapter", bestAdapter, "");
	}

	// Create swap chain description
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = 1024;
	swapChainDesc.BufferDesc.Height = 768;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

	// Description structure for creating a Direct3D device
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevel;

	// Create the device, context, and swap chain
	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		bestAdapter,                // Best adapter device
		driverType,				    // Use GPU driver
		nullptr,                    // No software driver
		0,                          // No special flags
		featureLevels,              // Feature levels
		1,                          // Number of feature levels
		D3D11_SDK_VERSION,          // SDK version
		&swapChainDesc,             // Swap chain description
		&swapChain,                 // Swap chain
		&device,                    // Direct3D device
		&featureLevel,              // Returned feature level
		&context                    // Device context
	);
	ThrowIfFailure(hr, "Failed to create D3D11 device and swap chain!");

	// Create render target view
	ID3D11Texture2D* backBuffer;
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	ThrowIfFailure(hr, "Failed to get back buffer!");

	hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
	ThrowIfFailure(hr, "Failed to create render target view!");
	backBuffer->Release();

	// Create depth stencil buffer
	D3D11_TEXTURE2D_DESC depthBufferDesc = {};
	depthBufferDesc.Width = 1024;
	depthBufferDesc.Height = 768;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	hr = device->CreateTexture2D(&depthBufferDesc, nullptr, &depthStencilBuffer);
	ThrowIfFailure(hr, "Failed to create depth stencil buffer!");

	hr = device->CreateDepthStencilView(depthStencilBuffer, nullptr, &depthStencilView);
	ThrowIfFailure(hr, "Failed to create depth stencil view!");

	// Set render targets
	context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

	// Set viewport
	D3D11_VIEWPORT viewport = {};
	viewport.Width = 1024.0f;
	viewport.Height = 768.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	context->RSSetViewports(1, &viewport);

	// Release the adapter if it was used
	if (bestAdapter)
	{
		bestAdapter->Release();
	}

	// Create the shaders
	computeShader = LoadComputeShader(device, ExecutableDirectory() / "ComputeShader.cso");
	vertexShader = LoadVertexShader(device, ExecutableDirectory() / "VertexShader.vso");
	geometryShader = LoadGeometryShader(device, ExecutableDirectory() / "GeometryShader.gso");
	pixelShader = LoadPixelShader(device, ExecutableDirectory() / "PixelShader.pso");
}

void DumpBufferDesc(const std::string& name, ID3D11Buffer* buffer)
{
	D3D11_BUFFER_DESC desc;
	buffer->GetDesc(&desc);
	std::cout << "Buffer " << name << " description:" << std::endl;
	std::cout << "\tUsage: " << desc.Usage << std::endl;
	std::cout << "\tByteWidth: " << desc.ByteWidth << std::endl;
	std::cout << "\tStructureByteStride: " << desc.StructureByteStride << std::endl;
	std::cout << "\tBindFlags: " << desc.BindFlags << std::endl;
	std::cout << "\tCPUAccessFlags: " << desc.CPUAccessFlags << std::endl;
	std::cout << "\tMiscFlags: " << desc.MiscFlags << std::endl;
}

void CreateComputeBuffers(std::vector<Point>& points)
{
	// Create the buffers for read/write position+velocity data
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = SafeSizeTToUINT(sizeof(Point) * points.size());
	bufferDesc.StructureByteStride = sizeof(Point);
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = points.data();  // Initial point data

	HRESULT hr;
	hr = device->CreateBuffer(&bufferDesc, &initData, &pointsBufferA);
	ThrowIfFailure(hr, "Failed to create buffer A");
	DumpBufferDesc("Buffer A", pointsBufferA);

	hr = device->CreateBuffer(&bufferDesc, &initData, &pointsBufferB);
	ThrowIfFailure(hr, "Failed to create buffer B");
	DumpBufferDesc("Buffer B", pointsBufferB);

	// Create Shader Resource View for the input buffer
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = SafeSizeTToUINT(points.size());
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;

	hr = device->CreateShaderResourceView(pointsBufferA, &srvDesc, &pointsSRVA);
	ThrowIfFailure(hr, "Failed to create SRV A");
	hr = device->CreateShaderResourceView(pointsBufferB, &srvDesc, &pointsSRVB);
	ThrowIfFailure(hr, "Failed to create SRV B");

	// Create Unordered Access View for the output buffer
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = SafeSizeTToUINT(points.size());
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;

	hr = device->CreateUnorderedAccessView(pointsBufferA, &uavDesc, &pointsUAVA);
	ThrowIfFailure(hr, "Failed to create UAV A");
	hr = device->CreateUnorderedAccessView(pointsBufferB, &uavDesc, &pointsUAVB);
	ThrowIfFailure(hr, "Failed to create UAV B");
}

void CreateConstantBuffers()
{
	// Create transform constant buffer
	D3D11_BUFFER_DESC transformBufferDesc = {};
	transformBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	transformBufferDesc.ByteWidth = sizeof(TransformBuffer);
	transformBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	transformBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = device->CreateBuffer(&transformBufferDesc, nullptr, &transformBuffer);
	ThrowIfFailure(hr, "Failed to create transform constant buffer!");

	// Create light constant buffer
	D3D11_BUFFER_DESC lightBufferDesc = {};
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBuffer);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = device->CreateBuffer(&lightBufferDesc, nullptr, &lightBuffer);
	ThrowIfFailure(hr, "Failed to create light constant buffer!");
}

void CreateRenderStates()
{
	// Create rasterizer state
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.AntialiasedLineEnable = false;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;

	HRESULT hr = device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
	ThrowIfFailure(hr, "Failed to create rasterizer state!");

	// Create depth stencil state
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = false;

	hr = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
	ThrowIfFailure(hr, "Failed to create depth stencil state!");
}

void RunComputeShader(ID3D11ShaderResourceView* readSRV, ID3D11UnorderedAccessView* writeUAV)
{
	// Set the shader
	context->CSSetShader(computeShader, nullptr, 0);

	// Set the resources
	context->CSSetShaderResources(0, 1, &readSRV);
	context->CSSetUnorderedAccessViews(0, 1, &writeUAV, nullptr);

	// Run the shader: dispatch with 64 threads per group (matches numthreads)
	UINT numGroups = (POINTS_COUNT + 63) / 64; // Round up division
	context->Dispatch(numGroups, 1, 1);

	// Unset the resources
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	context->CSSetShaderResources(0, 1, &nullSRV);
	context->CSSetShader(nullptr, nullptr, 0);
}

void UpdateTransformBuffer(float time)
{
	// Simple camera setup
	float radius = 10.0f;
	float3 cameraPos = { radius * sin(time), 5.0f, radius * cos(time) };
	float3 target = { 0.0f, 0.0f, 0.0f };
	float3 up = { 0.0f, 1.0f, 0.0f };

	// View matrix (simple lookAt)
	// This is simplified - in real projects use proper matrix libraries
	float viewMatrix[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		-cameraPos.x, -cameraPos.y, -cameraPos.z, 1
	};

	// Projection matrix (simple perspective)
	float fov = 45.0f * 3.14159f / 180.0f;
	float aspect = 1024.0f / 768.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;
	
	float projMatrix[16] = {
		1.0f / (aspect * tan(fov / 2)), 0, 0, 0,
		0, 1.0f / tan(fov / 2), 0, 0,
		0, 0, -(farPlane + nearPlane) / (farPlane - nearPlane), -1,
		0, 0, -(2 * farPlane * nearPlane) / (farPlane - nearPlane), 0
	};

	// World matrix (identity)
	float worldMatrix[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = context->Map(transformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	ThrowIfFailure(hr, "Failed to map transform buffer!");

	TransformBuffer* transformData = (TransformBuffer*)mappedResource.pData;
	memcpy(transformData->viewProjectionMatrix, projMatrix, sizeof(float) * 16);
	memcpy(transformData->worldMatrix, worldMatrix, sizeof(float) * 16);
	transformData->cameraPos[0] = cameraPos.x;
	transformData->cameraPos[1] = cameraPos.y;
	transformData->cameraPos[2] = cameraPos.z;
	transformData->sphereRadius = 0.1f;

	context->Unmap(transformBuffer, 0);
}

void UpdateLightBuffer()
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = context->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	ThrowIfFailure(hr, "Failed to map light buffer!");

	LightBuffer* lightData = (LightBuffer*)mappedResource.pData;
	lightData->lightDirection[0] = -1.0f;
	lightData->lightDirection[1] = -1.0f;
	lightData->lightDirection[2] = -1.0f;
	lightData->lightIntensity = 1.0f;
	lightData->lightColor[0] = 1.0f;
	lightData->lightColor[1] = 1.0f;
	lightData->lightColor[2] = 1.0f;
	lightData->ambientIntensity = 0.2f;
	lightData->cameraPos[0] = 0.0f; // Will be updated from transform buffer
	lightData->cameraPos[1] = 5.0f;
	lightData->cameraPos[2] = 10.0f;
	lightData->specularPower = 32.0f;

	context->Unmap(lightBuffer, 0);
}

void RenderFrame(ID3D11ShaderResourceView* pointsSRV, float time)
{
	// Clear render target
	float clearColor[4] = { 0.1f, 0.1f, 0.2f, 1.0f };
	context->ClearRenderTargetView(renderTargetView, clearColor);
	context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Update constant buffers
	UpdateTransformBuffer(time);
	UpdateLightBuffer();

	// Set shaders
	context->VSSetShader(vertexShader, nullptr, 0);
	context->GSSetShader(geometryShader, nullptr, 0);
	context->PSSetShader(pixelShader, nullptr, 0);

	// Set constant buffers
	context->VSSetConstantBuffers(0, 1, &transformBuffer);
	context->GSSetConstantBuffers(0, 1, &transformBuffer);
	context->PSSetConstantBuffers(0, 1, &lightBuffer);

	// Set shader resources
	context->VSSetShaderResources(0, 1, &pointsSRV);

	// Set render states
	context->RSSetState(rasterizerState);
	context->OMSetDepthStencilState(depthStencilState, 1);

	// Set primitive topology
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// Draw points (will be expanded to spheres by geometry shader)
	context->Draw(POINTS_COUNT, 0);

	// Present the frame
	swapChain->Present(0, 0);

	// Unset resources
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->VSSetShaderResources(0, 1, &nullSRV);
	context->VSSetShader(nullptr, nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

void ReadBackComputeResults(ID3D11Buffer* buffer, std::vector<Point>& points)
{
	DumpBufferDesc("Compute", buffer);

	// Description of the buffer for reading data back to the CPU
	D3D11_BUFFER_DESC readBackBufferDesc = {};
	readBackBufferDesc.Usage = D3D11_USAGE_STAGING;
	readBackBufferDesc.ByteWidth = SafeSizeTToUINT(sizeof(Point) * points.size());
	readBackBufferDesc.StructureByteStride = sizeof(Point);
	readBackBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	readBackBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	ID3D11Buffer* readBackBuffer;
	ThrowIfFailure(
		device->CreateBuffer(&readBackBufferDesc, nullptr, &readBackBuffer),
		"Failed to create compute read back buffer"
	);

	// Copy data from the output buffer
	context->CopyResource(readBackBuffer, buffer);

	// Map the data for reading
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ThrowIfFailure(
		context->Map(readBackBuffer, 0, D3D11_MAP_READ, 0, &mappedResource),
		"Failed to map compute read back buffer"
	);

	memcpy(points.data(), mappedResource.pData, sizeof(Point) * points.size());

	// Unmap the resource
	context->Unmap(readBackBuffer, 0);

	readBackBuffer->Release();
}

void CleanupMain()
{
	if (swapChain) swapChain->Release();
	if (renderTargetView) renderTargetView->Release();
	if (depthStencilView) depthStencilView->Release();
	if (depthStencilBuffer) depthStencilBuffer->Release();
	if (device) device->Release();
	if (context) context->Release();
}

void CleanupRender()
{
	if (pixelShader) pixelShader->Release();
	if (transformBuffer) transformBuffer->Release();
	if (lightBuffer) lightBuffer->Release();
	if (rasterizerState) rasterizerState->Release();
	if (depthStencilState) depthStencilState->Release();
}

void CleanupCompute()
{
	if (computeShader) computeShader->Release();
	if (pointsBufferA) pointsBufferA->Release();
	if (pointsBufferB) pointsBufferB->Release();
	if (pointsSRVA) pointsSRVA->Release();
	if (pointsSRVB) pointsSRVB->Release();
	if (pointsUAVA) pointsUAVA->Release();
	if (pointsUAVB) pointsUAVB->Release();
}

void CleanupVertex()
{
	if (vertexShader) vertexShader->Release();
	if (geometryShader) geometryShader->Release();
}

void Cleanup()
{
	CleanupRender();
	CleanupVertex();
	CleanupCompute();
	CleanupMain();
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND CreateApplicationWindow()
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = L"ParticleSimulation";

	RegisterClassEx(&wc);

	HWND hWnd = CreateWindowEx(
		0,
		L"ParticleSimulation",
		L"Particle Simulation - Verlet Integration",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1024 + 16, 768 + 39, // Account for window borders
		nullptr,
		nullptr,
		GetModuleHandle(nullptr),
		nullptr
	);

	if (hWnd)
	{
		ShowWindow(hWnd, SW_SHOWDEFAULT);
		UpdateWindow(hWnd);
	}

	return hWnd;
}

void run()
{
	// Create window
	HWND hWnd = CreateApplicationWindow();
	if (!hWnd)
	{
		throw std::runtime_error("Failed to create window!");
	}

	// Initialize Direct3D
	InitD3D(hWnd);

	// Create initial point data with Verlet integration setup
	std::vector<Point> points(POINTS_COUNT);
	for (size_t idx = 0; idx < POINTS_COUNT; ++idx)
	{
		auto& point = points[idx];
		// Random initial positions in a cube
		point.position[0] = (rand() % 200 - 100) / 100.0f; // -1 to 1
		point.position[1] = (rand() % 200 - 100) / 100.0f; 
		point.position[2] = (rand() % 200 - 100) / 100.0f;
		
		// For Verlet integration, set old position same as current (no initial velocity)
		point.oldPosition[0] = point.position[0];
		point.oldPosition[1] = point.position[1];
		point.oldPosition[2] = point.position[2];
		
		// Start with zero acceleration
		point.acceleration[0] = point.acceleration[1] = point.acceleration[2] = 0.0f;
	}

	// Create buffers and resources
	CreateComputeBuffers(points);
	CreateConstantBuffers();
	CreateRenderStates();

	// Message loop with simulation
	MSG msg = {};
	bool running = true;
	int frameCount = 0;
	
	while (running && frameCount < 3600) // Run for ~60 seconds at 60fps
	{
		// Process Windows messages
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (running)
		{
			// Run one simulation step
			ID3D11Buffer* currentReadBuffer = (frameCount % 2 == 0) ? pointsBufferA : pointsBufferB;
			ID3D11Buffer* currentWriteBuffer = (frameCount % 2 == 0) ? pointsBufferB : pointsBufferA;
			ID3D11ShaderResourceView* currentReadSRV = (frameCount % 2 == 0) ? pointsSRVA : pointsSRVB;
			ID3D11ShaderResourceView* currentWriteSRV = (frameCount % 2 == 0) ? pointsSRVB : pointsSRVA;
			ID3D11UnorderedAccessView* currentWriteUAV = (frameCount % 2 == 0) ? pointsUAVB : pointsUAVA;

			// Run compute shader for physics
			RunComputeShader(currentReadSRV, currentWriteUAV);

			// Render the frame
			float time = frameCount / 60.0f;
			RenderFrame(currentWriteSRV, time);

			frameCount++;

			// Debug output every 60 frames
			if (frameCount % 60 == 0)
			{
				ReadBackComputeResults(currentWriteBuffer, points);
				std::cout << "Frame " << frameCount << " - First particle pos: (" 
					<< points[0].position[0] << ", " << points[0].position[1] << ", " << points[0].position[2] << ")" << std::endl;
			}
		}
	}

	// Cleanup
	Cleanup();
}

int main()
{
	std::cout << "Hello World" << std::endl;
	std::cout << "Working in: " << std::filesystem::current_path() << std::endl;

	try
	{
		run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}