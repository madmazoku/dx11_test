#pragma once

#include "ComPtr.h"
#include "Structures.h"
#include <d3d11.h>
#include <string>
#include <filesystem>

class D3DDevice {
private:
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11RenderTargetView> renderTargetView;
    ComPtr<ID3D11DepthStencilView> depthStencilView;
    ComPtr<ID3D11Texture2D> depthStencilBuffer;
    
    RenderConfig config;

public:
    explicit D3DDevice(const RenderConfig& renderConfig);
    ~D3DDevice() = default;

    bool Initialize(HWND hWnd);
    void Present();
    void ClearRenderTarget();
    
    // Getters
    ID3D11Device* GetDevice() const { return device.get(); }
    ID3D11DeviceContext* GetContext() const { return context.get(); }
    ID3D11RenderTargetView* GetRenderTargetView() const { return renderTargetView.get(); }
    ID3D11DepthStencilView* GetDepthStencilView() const { return depthStencilView.get(); }
    const RenderConfig& GetConfig() const { return config; }

private:
    bool CreateDeviceAndSwapChain(HWND hWnd);
    bool CreateRenderTargetView();
    bool CreateDepthStencilBuffer();
    void SetupViewport();
    
    std::pair<ComPtr<IDXGIAdapter>, D3D_DRIVER_TYPE> DetermineBestAdapter();
    void DumpAdapterInfo(IDXGIAdapter* adapter, const std::string& name);
};
