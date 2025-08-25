#pragma once

#include "ComPtr.h"
#include "Structures.h"
#include <d3d11.h>
#include <string>
#include <filesystem>

/**
 * @file D3DDevice.h
 * @brief DirectX 11 device management and initialization
 * 
 * This file provides a comprehensive wrapper for DirectX 11 device creation,
 * initialization, and management. It handles all the complexity of setting up
 * a DirectX rendering context including device creation, swap chain setup,
 * render target configuration, and depth buffer management.
 * 
 * Key features:
 * - Automatic hardware adapter detection and selection
 * - Comprehensive error handling during device creation
 * - Render target and depth buffer management
 * - Viewport configuration
 * - Present and clear operations
 * - Resource lifetime management through ComPtr
 * 
 * The class abstracts away the boilerplate DirectX initialization code and
 * provides a clean interface for the rendering system.
 */

/**
 * @class D3DDevice
 * @brief DirectX 11 device wrapper and management class
 * 
 * Manages the core DirectX 11 rendering objects including device, device context,
 * swap chain, render targets, and depth buffers. Provides a simplified interface
 * for DirectX initialization and common rendering operations.
 * 
 * Initialization process:
 * 1. Determine best available graphics adapter
 * 2. Create D3D11 device and device context
 * 3. Create swap chain for window presentation
 * 4. Create render target view from back buffer
 * 5. Create depth/stencil buffer and view
 * 6. Configure viewport dimensions
 * 
 * The class ensures proper resource cleanup through RAII and ComPtr usage,
 * preventing DirectX resource leaks.
 */
class D3DDevice {
private:
    // === Core DirectX Objects ===
    ComPtr<ID3D11Device> device;                   ///< D3D11 device for resource creation
    ComPtr<ID3D11DeviceContext> context;           ///< Device context for rendering commands
    ComPtr<IDXGISwapChain> swapChain;              ///< Swap chain for presentation
    ComPtr<ID3D11RenderTargetView> renderTargetView; ///< Render target view for back buffer
    ComPtr<ID3D11DepthStencilView> depthStencilView; ///< Depth/stencil view for depth testing
    ComPtr<ID3D11Texture2D> depthStencilBuffer;    ///< Depth/stencil buffer texture
    
    RenderConfig config;                           ///< Render configuration settings

public:
    // === Construction and Lifecycle ===
    
    /**
     * @brief Construct D3DDevice with render configuration
     * 
     * @param renderConfig Configuration settings for rendering system
     */
    explicit D3DDevice(const RenderConfig& renderConfig);
    
    /**
     * @brief Default destructor
     * 
     * Resources are automatically cleaned up by ComPtr destructors
     */
    ~D3DDevice() = default;

    // === Core Operations ===
    
    /**
     * @brief Initialize DirectX device and rendering context
     * 
     * Performs complete DirectX initialization including device creation,
     * swap chain setup, render target configuration, and viewport setup.
     * 
     * @param hWnd Window handle for swap chain creation
     * @return true if initialization successful, false on error
     */
    bool Initialize(HWND hWnd);
    
    /**
     * @brief Present the back buffer to the screen
     * 
     * Swaps front and back buffers to display the rendered frame.
     * Respects VSync setting from configuration.
     */
    void Present();
    
    /**
     * @brief Clear render target and depth buffer
     * 
     * Clears the back buffer to the configured clear color and resets
     * the depth buffer for a new frame.
     */
    void ClearRenderTarget();
    
    // === Resource Access ===
    
    /**
     * @brief Get the DirectX device
     * @return Pointer to ID3D11Device interface
     */
    ID3D11Device* GetDevice() const { return device.get(); }
    
    /**
     * @brief Get the device context
     * @return Pointer to ID3D11DeviceContext interface
     */
    ID3D11DeviceContext* GetContext() const { return context.get(); }
    
    /**
     * @brief Get the render target view
     * @return Pointer to ID3D11RenderTargetView interface
     */
    ID3D11RenderTargetView* GetRenderTargetView() const { return renderTargetView.get(); }
    
    /**
     * @brief Get the depth stencil view
     * @return Pointer to ID3D11DepthStencilView interface
     */
    ID3D11DepthStencilView* GetDepthStencilView() const { return depthStencilView.get(); }
    
    /**
     * @brief Get the render configuration
     * @return Reference to RenderConfig structure
     */
    const RenderConfig& GetConfig() const { return config; }

private:
    // === Internal Initialization Methods ===
    
    /**
     * @brief Create DirectX device and swap chain
     * 
     * Handles the complex process of creating a DirectX device with
     * appropriate feature levels and creating a swap chain for the window.
     * 
     * @param hWnd Window handle for swap chain
     * @return true if successful, false on error
     */
    bool CreateDeviceAndSwapChain(HWND hWnd);
    
    /**
     * @brief Create render target view from swap chain back buffer
     * 
     * Retrieves the back buffer from the swap chain and creates a
     * render target view for rendering operations.
     * 
     * @return true if successful, false on error
     */
    bool CreateRenderTargetView();
    
    /**
     * @brief Create depth/stencil buffer and view
     * 
     * Creates a depth buffer texture and associated depth/stencil view
     * for depth testing and stencil operations.
     * 
     * @return true if successful, false on error
     */
    bool CreateDepthStencilBuffer();
    
    /**
     * @brief Configure rendering viewport
     * 
     * Sets up the viewport transformation from normalized device
     * coordinates to screen coordinates based on window dimensions.
     */
    void SetupViewport();
    
    /**
     * @brief Determine the best available graphics adapter
     * 
     * Enumerates available graphics adapters and selects the most
     * appropriate one for DirectX rendering. Prefers hardware adapters
     * with the most video memory.
     * 
     * @return Pair containing the selected adapter and driver type
     */
    std::pair<ComPtr<IDXGIAdapter>, D3D_DRIVER_TYPE> DetermineBestAdapter();
    
    /**
     * @brief Log information about a graphics adapter
     * 
     * Outputs detailed information about a graphics adapter to the log
     * for debugging and system information purposes.
     * 
     * @param adapter Adapter to query information from
     * @param name Descriptive name for the adapter
     */
    void DumpAdapterInfo(IDXGIAdapter* adapter, const std::string& name);
};
