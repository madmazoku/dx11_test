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

struct Point
{
	float position[3];
	float velocity[3];
};

struct Vertex
{
	float position[4];
};

const size_t POINTS_COUNT = 10;

ID3D11Device* device = nullptr;                  // Direct3D device
ID3D11DeviceContext* context = nullptr;          // Device context for executing commands

ID3D11ComputeShader* computeShader = nullptr;    // Compute shader
ID3D11Buffer* pointsBufferA = nullptr;           // Buffer A with point data
ID3D11Buffer* pointsBufferB = nullptr;           // Buffer B with point data
ID3D11ShaderResourceView* pointsSRVA = nullptr;  // Resource View A for reading the buffer
ID3D11ShaderResourceView* pointsSRVB = nullptr;  // Resource View B for reading the buffer
ID3D11UnorderedAccessView* pointsUAVA = nullptr; // Unordered Access View A for writing to the buffer
ID3D11UnorderedAccessView* pointsUAVB = nullptr; // Unordered Access View B for writing to the buffer

ID3D11VertexShader* vertexShader = nullptr;      // Vertex shader
ID3D11Buffer* vertexOutputBuffer = nullptr;      // Buffer for the vertex shader output

ID3D11GeometryShader* geometryShader = nullptr;     // Geometry shader
ID3D11Buffer* geometryOutputBuffer = nullptr;     // Buffer to capture shader output

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

	// Description structure for creating a Direct3D device
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevel;

	// Create the device and context
	HRESULT hr = D3D11CreateDevice(
		bestAdapter,                // Best adapter device
		driverType,				    // Use GPU driver
		nullptr,                    // No software driver
		0,                          // No special flags
		featureLevels,              // Feature levels
		1,                          // Number of feature levels
		D3D11_SDK_VERSION,          // SDK version
		&device,                    // Direct3D device
		&featureLevel,              // Returned feature level
		&context                    // Device context
	);
	ThrowIfFailure(hr, "Failed to create D3D11 device!");

	// Release the adapter if it was used
	if (bestAdapter)
	{
		bestAdapter->Release();
	}

	// Create the shaders
	computeShader = LoadComputeShader(device, ExecutableDirectory() / "ComputeShader.cso");
	vertexShader = LoadVertexShader(device, ExecutableDirectory() / "VertexShader.vso");
	geometryShader = LoadGeometryShader(device, ExecutableDirectory() / "GeometryShader.gso");
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

void CreateVertexBuffers(std::vector<Vertex>& vertexes)
{
	// Create the buffer for the vertex shader output
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = SafeSizeTToUINT(sizeof(Vertex) * vertexes.size());
	bufferDesc.StructureByteStride = sizeof(Vertex);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertexes.data();  // Initial point data

	HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &vertexOutputBuffer);
	ThrowIfFailure(hr, "Failed to create vertex output buffer");
	DumpBufferDesc("Vertex Output", vertexOutputBuffer);
}

void RunComputeShader(ID3D11ShaderResourceView* readSRV, ID3D11UnorderedAccessView* writeUAV)
{
	// Set the shader
	context->CSSetShader(computeShader, nullptr, 0);

	// Set the resources
	context->CSSetShaderResources(0, 1, &readSRV);
	context->CSSetUnorderedAccessViews(0, 1, &writeUAV, nullptr);

	// Run the shader: for example, POINTS_COUNT points -> POINTS_COUNT dispatches
	context->Dispatch(POINTS_COUNT, 1, 1);

	// Unset the resources
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	context->CSSetShaderResources(0, 1, &nullSRV);
	context->CSSetShader(nullptr, nullptr, 0);
}

void RunVertexShader(ID3D11ShaderResourceView* readSRV)
{
	// Set the shader
	context->VSSetShader(vertexShader, nullptr, 0);

	// Set the resources
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->VSSetShaderResources(0, 1, &readSRV);
	context->SOSetTargets(1, &vertexOutputBuffer, &offset);

	// Draw call to process the data with the vertex shader
	context->Draw(POINTS_COUNT, 0);

	// Unset the resources
	ID3D11ShaderResourceView* nullSRV = nullptr;
	ID3D11Buffer* nullBuffer = nullptr;
	context->VSSetShaderResources(0, 1, &nullSRV);
	context->SOSetTargets(1, &nullBuffer, &offset);
	context->VSSetShader(nullptr, nullptr, 0);
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

void ReadBackVertexResults(std::vector<Vertex>& vertexes)
{
	DumpBufferDesc("Vertex", vertexOutputBuffer);

	// Create a staging buffer for reading back data
	D3D11_BUFFER_DESC readBackBufferDesc = {};
	readBackBufferDesc.Usage = D3D11_USAGE_STAGING;
	readBackBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	readBackBufferDesc.ByteWidth = SafeSizeTToUINT(sizeof(Vertex) * vertexes.size());
	readBackBufferDesc.StructureByteStride = sizeof(Vertex);
	readBackBufferDesc.BindFlags = 0;
	readBackBufferDesc.MiscFlags = 0;

	ID3D11Buffer* readBackBuffer;
	ThrowIfFailure(
		device->CreateBuffer(&readBackBufferDesc, nullptr, &readBackBuffer),
		"Failed to create vertex read back buffer"
	);

	// Copy data from the output buffer
	context->CopyResource(readBackBuffer, vertexOutputBuffer);

	// Map the data for reading
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ThrowIfFailure(
		context->Map(readBackBuffer, 0, D3D11_MAP_READ, 0, &mappedResource),
		"Failed to map vertex read back buffer"
	);
	memcpy(vertexes.data(), mappedResource.pData, sizeof(Vertex) * vertexes.size());

	context->Unmap(readBackBuffer, 0);

	readBackBuffer->Release();
}

void ComputeLoop(std::vector<Point>& points, std::vector<Vertex>& vertexes, int numIterations)
{
	ID3D11Buffer* currentReadBuffer = pointsBufferA;
	ID3D11Buffer* currentWriteBuffer = pointsBufferB;
	ID3D11ShaderResourceView* currentReadSRV = pointsSRVA;
	ID3D11ShaderResourceView* currentWriteSRV = pointsSRVB;
	ID3D11UnorderedAccessView* currentReadUAV = pointsUAVA;
	ID3D11UnorderedAccessView* currentWriteUAV = pointsUAVB;

	for (int i = 0; i < numIterations; ++i)
	{
		std::cout << "Iteration " << i << std::endl;

		// Run shaders
		RunComputeShader(currentReadSRV, currentWriteUAV);
		RunVertexShader(currentWriteSRV);

		// Read back the results
		ReadBackComputeResults(currentWriteBuffer, points);
		ReadBackVertexResults(vertexes);

		// Swap the buffers
		std::swap(currentReadBuffer, currentWriteBuffer);
		std::swap(currentReadSRV, currentWriteSRV);
		std::swap(currentReadUAV, currentWriteUAV);

		// Output the results (for debugging)
		size_t idx = 0;
		for (size_t idx = 0; idx < POINTS_COUNT; ++idx)
		{
			auto& point = points[idx];

			std::cout << std::format(
				"[{}] Position: ({:.6f}, {:.6f}, {:.6f}); Velocity: ({:.6f}, {:.6f}, {:.6f})",
				idx,
				point.position[0], point.position[1], point.position[2],
				point.velocity[0], point.velocity[1], point.velocity[2]
			) << std::endl;

			auto& vertex = vertexes[idx];
			std::cout << std::format(
				"[{}] Vertex: ({:.6f}, {:.6f}, {:.6f}, {:.6f})",
				idx,
				vertex.position[0], vertex.position[1], vertex.position[2], vertex.position[3]
			) << std::endl;
		}
		std::cout << std::endl;
	}
}

void CleanupMain()
{
	if (device) device->Release();
	if (context) context->Release();
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
	if (vertexOutputBuffer) vertexOutputBuffer->Release();
}

void Cleanup()
{
	CleanupVertex();
	CleanupCompute();
	CleanupMain();
}

void run()
{
	HWND hWnd = nullptr;

	// Initialize Direct3D
	InitD3D(hWnd);

	// Create initial point data
	std::vector<Point> points(POINTS_COUNT);
	std::vector<Vertex> vertexes(POINTS_COUNT);
	for (size_t idx = 0; idx < POINTS_COUNT; ++idx)
	{
		auto& point = points[idx];
		point.position[0] = rand() % 100 / 100.0f;
		point.position[1] = rand() % 100 / 100.0f;
		point.position[2] = rand() % 100 / 100.0f;
		point.velocity[0] = points[idx].velocity[1] = points[idx].velocity[2] = 0.0f;

		auto& vertex = vertexes[idx];
		vertex.position[0] = point.position[0];
		vertex.position[1] = point.position[0];
		vertex.position[2] = point.position[0];
		vertex.position[3] = 1.0f;
	}

	// Create buffers for point data
	CreateComputeBuffers(points);
	CreateVertexBuffers(vertexes);

	// Run the compute shader loop
	ComputeLoop(points, vertexes, 5);

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