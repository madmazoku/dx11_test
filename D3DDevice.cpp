#include "D3DDevice.h"
#include "Utilities.h"
#include <dxgi.h>
#include <iostream>

D3DDevice::D3DDevice(const RenderConfig& renderConfig) : config(renderConfig) {}

bool D3DDevice::Initialize(HWND hWnd) {
    try {
        if (!CreateDeviceAndSwapChain(hWnd)) return false;
        if (!CreateRenderTargetView()) return false;
        if (!CreateDepthStencilBuffer()) return false;
        SetupViewport();
        return true;
    }
    catch (const D3DException& e) {
        std::cerr << "D3DDevice initialization failed: " << e.what() << std::endl;
        return false;
    }
}

bool D3DDevice::CreateDeviceAndSwapChain(HWND hWnd) {
    auto [adapter, driverType] = DetermineBestAdapter();
    
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = config.windowWidth;
    swapChainDesc.BufferDesc.Height = config.windowHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;

    THROW_IF_FAILED(
        D3D11CreateDeviceAndSwapChain(
            adapter.get(), driverType, nullptr, 0,
            featureLevels, 1, D3D11_SDK_VERSION,
            &swapChainDesc, swapChain.getAddressOf(),
            device.getAddressOf(), &featureLevel, context.getAddressOf()
        ),
        "Failed to create D3D11 device and swap chain"
    );

    return true;
}

bool D3DDevice::CreateRenderTargetView() {
    ComPtr<ID3D11Texture2D> backBuffer;
    THROW_IF_FAILED(
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), 
                            reinterpret_cast<void**>(backBuffer.getAddressOf())),
        "Failed to get back buffer"
    );

    THROW_IF_FAILED(
        device->CreateRenderTargetView(backBuffer.get(), nullptr, renderTargetView.getAddressOf()),
        "Failed to create render target view"
    );

    return true;
}

bool D3DDevice::CreateDepthStencilBuffer() {
    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    depthBufferDesc.Width = config.windowWidth;
    depthBufferDesc.Height = config.windowHeight;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    THROW_IF_FAILED(
        device->CreateTexture2D(&depthBufferDesc, nullptr, depthStencilBuffer.getAddressOf()),
        "Failed to create depth stencil buffer"
    );

    THROW_IF_FAILED(
        device->CreateDepthStencilView(depthStencilBuffer.get(), nullptr, depthStencilView.getAddressOf()),
        "Failed to create depth stencil view"
    );

    return true;
}

void D3DDevice::SetupViewport() {
    context->OMSetRenderTargets(1, renderTargetView.getAddressOf(), depthStencilView.get());

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(config.windowWidth);
    viewport.Height = static_cast<float>(config.windowHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    context->RSSetViewports(1, &viewport);
}

void D3DDevice::Present() {
    swapChain->Present(0, 0);
}

void D3DDevice::ClearRenderTarget() {
    context->ClearRenderTargetView(renderTargetView.get(), config.clearColor);
    context->ClearDepthStencilView(depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

std::pair<ComPtr<IDXGIAdapter>, D3D_DRIVER_TYPE> D3DDevice::DetermineBestAdapter() {
    ComPtr<IDXGIFactory> factory;
    THROW_IF_FAILED(
        CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(factory.getAddressOf())),
        "Failed to create DXGI factory"
    );

    ComPtr<IDXGIAdapter> bestAdapter;
    SIZE_T maxDedicatedVideoMemory = 0;

    std::cout << "Enumerating adapters:" << std::endl;

    for (UINT i = 0; ; ++i) {
        ComPtr<IDXGIAdapter> adapter;
        HRESULT hr = factory->EnumAdapters(i, adapter.getAddressOf());
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        THROW_IF_FAILED(hr, "Failed to enumerate adapters");

        DumpAdapterInfo(adapter.get(), std::format("Adapter {}", i));

        DXGI_ADAPTER_DESC desc;
        THROW_IF_FAILED(adapter->GetDesc(&desc), "Failed to get adapter description");

        // Skip software adapters
        if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c) {
            std::cout << "  Skipping software adapter" << std::endl;
            continue;
        }

        if (desc.DedicatedVideoMemory > maxDedicatedVideoMemory) {
            bestAdapter = std::move(adapter);
            maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
        }
    }

    if (bestAdapter) {
        DumpAdapterInfo(bestAdapter.get(), "Selected adapter");
    }

    D3D_DRIVER_TYPE driverType = bestAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
    return { std::move(bestAdapter), driverType };
}

void D3DDevice::DumpAdapterInfo(IDXGIAdapter* adapter, const std::string& name) {
    DXGI_ADAPTER_DESC desc;
    THROW_IF_FAILED(adapter->GetDesc(&desc), "Failed to get adapter description");
    
    std::cout << name << ": " << Utils::ConvertWideToNarrow(desc.Description) << std::endl;
    std::cout << "  Vendor ID: 0x" << std::hex << desc.VendorId << std::endl;
    std::cout << "  Device ID: 0x" << desc.DeviceId << std::endl;
    std::cout << "  Dedicated Video Memory: " << Utils::HumanReadableSize(desc.DedicatedVideoMemory) << std::endl;
    std::cout << std::dec; // Reset to decimal
}
