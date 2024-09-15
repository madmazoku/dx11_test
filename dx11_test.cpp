#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <format>
#include <exception>

struct Point
{
	float position[3];
	float velocity[3];
};

const size_t POINTS_COUNT = 10;

ID3D11Device* device = nullptr;                // Устройство Direct3D
ID3D11DeviceContext* context = nullptr;        // Контекст устройства для выполнения команд
ID3D11ComputeShader* computeShader = nullptr;  // Компьютный шейдер

ID3D11Buffer* pointsBufferA = nullptr;        // Буфер A с данными точек
ID3D11Buffer* pointsBufferB = nullptr;        // Буфер B с данными точек
ID3D11ShaderResourceView* pointsSRVA = nullptr; // Resource View A для чтения буфера
ID3D11ShaderResourceView* pointsSRVB = nullptr; // Resource View B для чтения буфера
ID3D11UnorderedAccessView* pointsUAVA = nullptr; // Unordered Access View A для записи в буфер
ID3D11UnorderedAccessView* pointsUAVB = nullptr; // Unordered Access View B для записи в буфер

inline UINT safeSizeTToUINT(size_t sz) {
	static const size_t szMaxUint = static_cast<size_t>(UINT_MAX);
	if (sz > szMaxUint) {
		std::string err_msg = std::format("size_t to UINT cast overflow: {} exceeded max UINT value {}", sz, szMaxUint);
		throw std::out_of_range(err_msg);
	}
	return static_cast<UINT>(sz);
}

std::filesystem::path executable_directory() {
	char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	return std::filesystem::path(buffer).parent_path();
}

ID3D11ComputeShader* CompileComputeShader(ID3D11Device* device, const std::filesystem::path& filePath)
{
	std::cout << "Compile compute shader: " << filePath << std::endl;

	// Проверка существования файла
	if (!std::filesystem::exists(filePath))
	{
		std::cerr << "Shader file does not exist: " << filePath << std::endl;
		return nullptr;
	}

	// Преобразование пути в LPCWSTR
	const std::wstring wstrFilename = filePath.wstring();
	LPCWSTR lpcwstrFilename = wstrFilename.c_str();

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	// Компилируем шейдер из файла
	HRESULT hr = D3DCompileFromFile(
		lpcwstrFilename,   // Путь к файлу шейдера
		nullptr,           // Макросы
		nullptr,           // Include
		"CSMain",          // Имя основной функции
		"cs_5_0",          // Уровень шейдера (compute shader 5.0)
		0, 0,              // Флаги компиляции
		&shaderBlob,       // Выходной blob с байт-кодом
		&errorBlob         // Ошибки компиляции
	);

	if (FAILED(hr)) {
		std::cerr << "Failed to compile compute shader from file: " << filePath << std::endl;
		if (errorBlob) {
			std::cerr << "Shader compilation error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
			errorBlob->Release();
		}
		else {
			std::cerr << "Shader compilation error: " << "Unknown" << std::endl;
		}
		return nullptr;
	}

	// Создаем шейдер на основе байт-кода
	ID3D11ComputeShader* shader = nullptr;
	device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &shader);

	shaderBlob->Release();
	return shader;
}

ID3D11ComputeShader* LoadComputeShader(ID3D11Device* device, const std::filesystem::path& filePath)
{
	std::cout << "Read compute shader: " << filePath << std::endl;

	// Проверка существования файла
	if (!std::filesystem::exists(filePath))
	{
		std::cerr << "Shader file does not exist: " << filePath << std::endl;
		return nullptr;
	}

	// Открытие файла
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Failed to open shader file: " << filePath << std::endl;
		return nullptr;
	}

	// Определение размера файла
	auto fileSize = std::filesystem::file_size(filePath);

	// Чтение файла в память
	std::vector<char> shaderData(fileSize);
	file.read(shaderData.data(), fileSize);
	file.close();

	// Создание шейдера из прочитанного байт-кода
	ID3D11ComputeShader* shader = nullptr;
	HRESULT hr = device->CreateComputeShader(shaderData.data(), shaderData.size(), nullptr, &shader);
	if (FAILED(hr))
	{
		std::cerr << "Failed to create compute shader from .cso file!" << std::endl;
	}

	return shader;
}

void InitD3D(HWND hWnd)
{
	// Описание структуры для создания устройства Direct3D
	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevel;

	// Создание устройства и контекста
	HRESULT hr = D3D11CreateDevice(
		nullptr,                    // Устройство адаптера по умолчанию
		D3D_DRIVER_TYPE_HARDWARE,   // Используем драйвер GPU
		nullptr,                    // Нет программного драйвера
		0,                          // Нет специальных флагов
		featureLevels,              // Уровни возможностей
		1,                          // Количество уровней возможностей
		D3D11_SDK_VERSION,          // Версия SDK
		&device,                    // Устройство Direct3D
		&featureLevel,              // Возвращаемый уровень возможностей
		&context                    // Контекст устройства
	);

	if (FAILED(hr)) {
		std::cerr << "Failed to create D3D11 device!" << std::endl;
		return;
	}

	// Создание шейдера (см. ниже функцию компиляции)
	computeShader = LoadComputeShader(device, executable_directory() / "ComputeShader.cso");
}

void CreateBuffers(std::vector<Point>& points)
{
	// Описание структуры буфера
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = safeSizeTToUINT(sizeof(Point) * points.size());
	bufferDesc.StructureByteStride = sizeof(Point);
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = points.data();  // Исходные данные точек

	// Создание входного буфера для чтения точек
	device->CreateBuffer(&bufferDesc, &initData, &pointsBufferA);
	device->CreateBuffer(&bufferDesc, &initData, &pointsBufferB);

	// Создание Shader Resource View для входного буфера
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = safeSizeTToUINT(points.size());
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;

	device->CreateShaderResourceView(pointsBufferA, &srvDesc, &pointsSRVA);
	device->CreateShaderResourceView(pointsBufferB, &srvDesc, &pointsSRVB);

	// Создание Unordered Access View для выходного буфера
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
	// Устанавливаем шейдер
	context->CSSetShader(computeShader, nullptr, 0);

	std::cout << "Set resources: read: " << static_cast<void*>(readSRV) << "; write: " << static_cast<void*>(writeUAV) << std::endl;

	// Устанавливаем ресурсы
	context->CSSetShaderResources(0, 1, &readSRV);
	context->CSSetUnorderedAccessViews(0, 1, &writeUAV, nullptr);

	// Запуск шейдера: например, POINTS_COUNT точек -> POINTS_COUNT диспатчей
	context->Dispatch(POINTS_COUNT, 1, 1);

	// Сбрасываем ресурсы
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	context->CSSetShaderResources(0, 1, &nullSRV);
	context->CSSetShader(nullptr, nullptr, 0);
}

void ReadBackResults(ID3D11Buffer* writeBuffer, std::vector<Point>& points)
{
	// Описание буфера для чтения данных обратно на CPU
	D3D11_BUFFER_DESC readBackBufferDesc = {};
	readBackBufferDesc.Usage = D3D11_USAGE_STAGING;
	readBackBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	readBackBufferDesc.ByteWidth = safeSizeTToUINT(sizeof(Point) * points.size());
	readBackBufferDesc.StructureByteStride = sizeof(Point);
	readBackBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	ID3D11Buffer* readBackBuffer;
	device->CreateBuffer(&readBackBufferDesc, nullptr, &readBackBuffer);

	// Копируем данные из выходного буфера
	context->CopyResource(readBackBuffer, writeBuffer);

	// Маппинг данных для чтения
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(readBackBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);

	memcpy(points.data(), mappedResource.pData, sizeof(Point) * points.size());

	// Размаппинг ресурса
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
		// Запускаем шейдер
		RunComputeShader(currentReadSRV, currentWriteUAV);

		// Считываем результаты
		ReadBackResults(currentWriteBuffer, points);

		// Переключаем буферы
		std::swap(currentReadBuffer, currentWriteBuffer);
		std::swap(currentReadSRV, currentWriteSRV);
		std::swap(currentReadUAV, currentWriteUAV);

		// Выводим результаты (например, для отладки)
		size_t idx = 0;
		for (const auto& point : points)
		{
			++idx;
			std::cout << "[" << idx << "] " << std::fixed << std::setprecision(4);
			std::cout << "Position: (" << point.position[0] << ", " << point.position[1] << ", " << point.position[2] << "); ";
			std::cout << "Velocity: (" << point.velocity[0] << ", " << point.velocity[1] << ", " << point.velocity[2] << ")";
			std::cout << std::defaultfloat << std::endl;
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

int main()
{
	std::cout << "Hello World" << std::endl;

	std::cout << "Working in: " << std::filesystem::current_path() << std::endl;

	HWND hWnd = nullptr;

	// Инициализируем Direct3D
	InitD3D(hWnd);

	// Инициализируем точки
	std::vector<Point> points(POINTS_COUNT);
	for (auto& point : points)
	{
		point.position[0] = rand() % 100 / 100.0f;
		point.position[1] = rand() % 100 / 100.0f;
		point.position[2] = rand() % 100 / 100.0f;
		point.velocity[0] = point.velocity[1] = point.velocity[2] = 0.0f;
	}

	// Создаем буферы
	CreateBuffers(points);

	// Выполняем вычисления в цикле
	ComputeLoop(points, 10);

	// Очистка ресурсов
	Cleanup();

	return 0;
}