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

const size_t POINTS_COUNT = 10;

ID3D11Device* device = nullptr;                // Direct3D device
ID3D11DeviceContext* context = nullptr;        // Device context for executing commands
ID3D11ComputeShader* computeShader = nullptr;  // Compute shader

ID3D11Buffer* pointsBufferA = nullptr;        // Buffer A with point data
ID3D11Buffer* pointsBufferB = nullptr;        // Buffer B with point data
ID3D11ShaderResourceView* pointsSRVA = nullptr; // Resource View A for reading the buffer
ID3D11ShaderResourceView* pointsSRVB = nullptr; // Resource View B for reading the buffer
ID3D11UnorderedAccessView* pointsUAVA = nullptr; // Unordered Access View A for writing to the buffer
ID3D11UnorderedAccessView* pointsUAVB = nullptr; // Unordered Access View B for writing to the buffer

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

inline UINT safeSizeTToUINT(size_t sz)
{
	static const size_t szMaxUint = static_cast<size_t>(UINT_MAX);
	if (sz > szMaxUint)
	{
		std::string err_msg = std::format("size_t to UINT cast overflow: {} exceeded max UINT value {}", sz, szMaxUint);
		throw std::out_of_range(err_msg);
	}
	return static_cast<UINT>(sz);
}

std::filesystem::path executable_directory()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	return std::filesystem::path(buffer).parent_path();
}

ID3D11ComputeShader* CompileComputeShader(ID3D11Device* device, const std::filesystem::path& filePath)
{
	std::cout << "Compile compute shader: " << filePath << std::endl;

	// Check if the file exists
	if (!std::filesystem::exists(filePath))
	{
		throw std::runtime_error("Shader file does not exist: " + filePath.string());
	}

	// Convert path to LPCWSTR
	const std::wstring wstrFilename = filePath.wstring();
	LPCWSTR lpcwstrFilename = wstrFilename.c_str();

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	// Compile the shader from the file
	HRESULT hr = D3DCompileFromFile(
		lpcwstrFilename,   // Path to the shader file
		nullptr,           // Macros
		nullptr,           // Include
		"CSMain",          // Name of the main function
		"cs_5_0",          // Shader level (compute shader 5.0)
		0, 0,              // Compilation flags
		&shaderBlob,       // Output blob with bytecode
		&errorBlob         // Compilation errors
	);

	if (FAILED(hr))
	{
		std::string errorMsg = "Failed to compile shader from file: '"+ filePath.string()+"'. Shader compilation error: ";
		if (errorBlob)
		{
			errorMsg += std::string((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		else
		{
			errorMsg += "Unknown";
		}
		ThrowIfFailure(hr, errorMsg);
	}

	// Create the shader from the bytecode
	ID3D11ComputeShader* shader = nullptr;
	device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &shader);

	shaderBlob->Release();
	return shader;
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

	if(bestAdapter) {
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

	// Create the shader (see the compilation function below)
	computeShader = LoadComputeShader(device, executable_directory() / "ComputeShader.cso");
}

void CreateBuffers(std::vector<Point>& points)
{
	// Description structure for the buffer
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = safeSizeTToUINT(sizeof(Point) * points.size());
	bufferDesc.StructureByteStride = sizeof(Point);
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = points.data();  // Initial point data

	// Create the input buffer for reading points
	device->CreateBuffer(&bufferDesc, &initData, &pointsBufferA);
	device->CreateBuffer(&bufferDesc, &initData, &pointsBufferB);

	// Create Shader Resource View for the input buffer
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = safeSizeTToUINT(points.size());
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;

	device->CreateShaderResourceView(pointsBufferA, &srvDesc, &pointsSRVA);
	device->CreateShaderResourceView(pointsBufferB, &srvDesc, &pointsSRVB);

	// Create Unordered Access View for the output buffer
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = safeSizeTToUINT(points.size());
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;

	device->CreateUnorderedAccessView(pointsBufferA, &uavDesc, &pointsUAVA);
	device->CreateUnorderedAccessView(pointsBufferB, &uavDesc, &pointsUAVB);
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

	// Reset the resources
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	context->CSSetShaderResources(0, 1, &nullSRV);
	context->CSSetShader(nullptr, nullptr, 0);
}

void ReadBackResults(ID3D11Buffer* writeBuffer, std::vector<Point>& points)
{
	// Description of the buffer for reading data back to the CPU
	D3D11_BUFFER_DESC readBackBufferDesc = {};
	readBackBufferDesc.Usage = D3D11_USAGE_STAGING;
	readBackBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	readBackBufferDesc.ByteWidth = safeSizeTToUINT(sizeof(Point) * points.size());
	readBackBufferDesc.StructureByteStride = sizeof(Point);
	readBackBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	ID3D11Buffer* readBackBuffer;
	device->CreateBuffer(&readBackBufferDesc, nullptr, &readBackBuffer);

	// Copy data from the output buffer
	context->CopyResource(readBackBuffer, writeBuffer);

	// Map the data for reading
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(readBackBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);

	memcpy(points.data(), mappedResource.pData, sizeof(Point) * points.size());

	// Unmap the resource
	context->Unmap(readBackBuffer, 0);

	readBackBuffer->Release();
}

void ComputeLoop(std::vector<Point>& points, int numIterations)
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

		// Run the compute shader
		RunComputeShader(currentReadSRV, currentWriteUAV);

		// Read back the results
		ReadBackResults(currentWriteBuffer, points);

		// Swap the buffers
		std::swap(currentReadBuffer, currentWriteBuffer);
		std::swap(currentReadSRV, currentWriteSRV);
		std::swap(currentReadUAV, currentWriteUAV);

		// Output the results (for debugging)
		size_t idx = 0;
		for (const auto& point : points)
		{
			++idx;
			std::cout << std::format(
				"[{}] Position: ({:.6f}, {:.6f}, {:.6f}); Velocity: ({:.6f}, {:.6f}, {:.6f})\n",
				idx,
				point.position[0], point.position[1], point.position[2],
				point.velocity[0], point.velocity[1], point.velocity[2]
			);
		}
		std::cout << std::endl;
	}
}

void Cleanup()
{
	if (computeShader) computeShader->Release();
	if (pointsBufferA) pointsBufferA->Release();
	if (pointsBufferB) pointsBufferB->Release();
	if (pointsSRVA) pointsSRVA->Release();
	if (pointsSRVB) pointsSRVB->Release();
	if (pointsUAVA) pointsUAVA->Release();
	if (pointsUAVB) pointsUAVB->Release();
	if (device) device->Release();
	if (context) context->Release();
}

void run()
{
	HWND hWnd = nullptr;

	// Initialize Direct3D
	InitD3D(hWnd);

	// Create initial point data
	std::vector<Point> points(POINTS_COUNT);
	for (auto& point : points)
	{
		point.position[0] = rand() % 100 / 100.0f;
		point.position[1] = rand() % 100 / 100.0f;
		point.position[2] = rand() % 100 / 100.0f;
		point.velocity[0] = point.velocity[1] = point.velocity[2] = 0.0f;
	}

	// Create buffers for point data
	CreateBuffers(points);

	// Run the compute shader loop
	ComputeLoop(points, 10);

	// Cleanup (assuming Cleanup is already defined)
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