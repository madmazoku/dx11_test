#pragma once

#include <memory>
#include <vector>
#include <stack>
#include <mutex>
#include <type_traits>

// Современная система управления памятью для высокопроизводительных сценариев

template<typename T>
class ObjectPool {
private:
    std::stack<std::unique_ptr<T>> pool;
    std::mutex poolMutex;
    size_t maxSize;

public:
    explicit ObjectPool(size_t maxSize = 1000) : maxSize(maxSize) {}
    
    template<typename... Args>
    std::unique_ptr<T> acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(poolMutex);
        
        if (!pool.empty()) {
            auto obj = std::move(pool.top());
            pool.pop();
            // Reinitialize object if needed
            if constexpr (std::is_constructible_v<T, Args...>) {
                *obj = T(std::forward<Args>(args)...);
            }
            return obj;
        }
        
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
    void release(std::unique_ptr<T> obj) {
        if (!obj) return;
        
        std::lock_guard<std::mutex> lock(poolMutex);
        if (pool.size() < maxSize) {
            pool.push(std::move(obj));
        }
        // If pool is full, object will be automatically destroyed
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(poolMutex);
        return pool.size();
    }
};

// GPU Memory Manager для оптимизации DirectX буферов
class GPUMemoryManager {
private:
    struct BufferInfo {
        size_t size;
        D3D11_USAGE usage;
        UINT bindFlags;
        std::chrono::steady_clock::time_point lastUsed;
    };
    
    std::vector<std::pair<ComPtr<ID3D11Buffer>, BufferInfo>> bufferPool;
    std::mutex poolMutex;
    ID3D11Device* device;

public:
    explicit GPUMemoryManager(ID3D11Device* device) : device(device) {}
    
    ComPtr<ID3D11Buffer> GetBuffer(size_t size, D3D11_USAGE usage, UINT bindFlags);
    void ReturnBuffer(ComPtr<ID3D11Buffer> buffer);
    void CleanupOldBuffers(std::chrono::minutes maxAge = std::chrono::minutes(5));
    
    size_t GetTotalAllocatedMemory() const;
    void PrintMemoryStats() const;
};

// RAII wrapper для временных GPU ресурсов
template<typename T>
class ScopedGPUResource {
private:
    ComPtr<T> resource;
    GPUMemoryManager* manager;

public:
    ScopedGPUResource(ComPtr<T> res, GPUMemoryManager* mgr) 
        : resource(std::move(res)), manager(mgr) {}
    
    ~ScopedGPUResource() {
        if constexpr (std::is_same_v<T, ID3D11Buffer>) {
            if (manager && resource) {
                manager->ReturnBuffer(resource);
            }
        }
    }
    
    T* get() const { return resource.get(); }
    T* operator->() const { return resource.get(); }
    T** getAddressOf() { return resource.getAddressOf(); }
};

// Memory profiler для отслеживания использования памяти
class MemoryProfiler {
public:
    struct MemoryStats {
        size_t totalCPUMemory = 0;
        size_t totalGPUMemory = 0;
        size_t peakCPUMemory = 0;
        size_t peakGPUMemory = 0;
        size_t allocations = 0;
        size_t deallocations = 0;
    };

private:
    MemoryStats stats;
    std::mutex statsMutex;

public:
    void RecordAllocation(size_t size, bool isGPU = false);
    void RecordDeallocation(size_t size, bool isGPU = false);
    MemoryStats GetStats() const;
    void PrintStats() const;
    void Reset();
};

extern MemoryProfiler g_memoryProfiler;
