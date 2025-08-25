#include "MemoryManager.h"
#include "Logger.h"
#include <algorithm>

MemoryProfiler g_memoryProfiler;

ComPtr<ID3D11Buffer> GPUMemoryManager::GetBuffer(size_t size, D3D11_USAGE usage, UINT bindFlags) {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    auto now = std::chrono::steady_clock::now();
    
    // Поиск подходящего буфера в пуле
    auto it = std::find_if(bufferPool.begin(), bufferPool.end(),
        [size, usage, bindFlags](const auto& pair) {
            const auto& info = pair.second;
            return info.size >= size && info.usage == usage && info.bindFlags == bindFlags;
        });
    
    if (it != bufferPool.end()) {
        auto buffer = std::move(it->first);
        bufferPool.erase(it);
        g_memoryProfiler.RecordAllocation(size, true);
        return buffer;
    }
    
    // Создаем новый буфер, если не найден подходящий
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(size);
    desc.Usage = usage;
    desc.BindFlags = bindFlags;
    desc.CPUAccessFlags = (usage == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
    
    ComPtr<ID3D11Buffer> buffer;
    HRESULT hr = device->CreateBuffer(&desc, nullptr, buffer.getAddressOf());
    
    if (SUCCEEDED(hr)) {
        g_memoryProfiler.RecordAllocation(size, true);
        LOG_DEBUG("Created new GPU buffer: {} bytes", size);
    } else {
        LOG_ERROR("Failed to create GPU buffer: {} bytes, HRESULT: {}", size, hr);
    }
    
    return buffer;
}

void GPUMemoryManager::ReturnBuffer(ComPtr<ID3D11Buffer> buffer) {
    if (!buffer) return;
    
    std::lock_guard<std::mutex> lock(poolMutex);
    
    D3D11_BUFFER_DESC desc;
    buffer->GetDesc(&desc);
    
    BufferInfo info;
    info.size = desc.ByteWidth;
    info.usage = desc.Usage;
    info.bindFlags = desc.BindFlags;
    info.lastUsed = std::chrono::steady_clock::now();
    
    bufferPool.emplace_back(std::move(buffer), info);
    
    // Ограничиваем размер пула
    if (bufferPool.size() > 100) {
        // Удаляем самые старые буферы
        std::sort(bufferPool.begin(), bufferPool.end(),
            [](const auto& a, const auto& b) {
                return a.second.lastUsed < b.second.lastUsed;
            });
        
        auto oldSize = bufferPool.front().second.size;
        bufferPool.erase(bufferPool.begin());
        g_memoryProfiler.RecordDeallocation(oldSize, true);
    }
}

void GPUMemoryManager::CleanupOldBuffers(std::chrono::minutes maxAge) {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    auto cutoffTime = std::chrono::steady_clock::now() - maxAge;
    
    auto removedCount = std::erase_if(bufferPool, [cutoffTime](const auto& pair) {
        bool shouldRemove = pair.second.lastUsed < cutoffTime;
        if (shouldRemove) {
            g_memoryProfiler.RecordDeallocation(pair.second.size, true);
        }
        return shouldRemove;
    });
    
    if (removedCount > 0) {
        LOG_INFO("Cleaned up {} old GPU buffers from memory pool", removedCount);
    }
}

size_t GPUMemoryManager::GetTotalAllocatedMemory() const {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    size_t total = 0;
    for (const auto& pair : bufferPool) {
        total += pair.second.size;
    }
    return total;
}

void GPUMemoryManager::PrintMemoryStats() const {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    size_t totalMemory = GetTotalAllocatedMemory();
    LOG_INFO("GPU Memory Pool Stats:");
    LOG_INFO("  Buffer count: {}", bufferPool.size());
    LOG_INFO("  Total memory: {} MB", totalMemory / (1024 * 1024));
    
    // Группировка по размерам
    std::map<size_t, int> sizeGroups;
    for (const auto& pair : bufferPool) {
        size_t sizeCategory = (pair.second.size / 1024) * 1024; // Round to KB
        sizeGroups[sizeCategory]++;
    }
    
    LOG_INFO("  Size distribution:");
    for (const auto& group : sizeGroups) {
        LOG_INFO("    {} KB: {} buffers", group.first / 1024, group.second);
    }
}

void MemoryProfiler::RecordAllocation(size_t size, bool isGPU) {
    std::lock_guard<std::mutex> lock(statsMutex);
    
    stats.allocations++;
    if (isGPU) {
        stats.totalGPUMemory += size;
        stats.peakGPUMemory = std::max(stats.peakGPUMemory, stats.totalGPUMemory);
    } else {
        stats.totalCPUMemory += size;
        stats.peakCPUMemory = std::max(stats.peakCPUMemory, stats.totalCPUMemory);
    }
}

void MemoryProfiler::RecordDeallocation(size_t size, bool isGPU) {
    std::lock_guard<std::mutex> lock(statsMutex);
    
    stats.deallocations++;
    if (isGPU) {
        stats.totalGPUMemory = (stats.totalGPUMemory >= size) ? stats.totalGPUMemory - size : 0;
    } else {
        stats.totalCPUMemory = (stats.totalCPUMemory >= size) ? stats.totalCPUMemory - size : 0;
    }
}

MemoryProfiler::MemoryStats MemoryProfiler::GetStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

void MemoryProfiler::PrintStats() const {
    auto currentStats = GetStats();
    
    LOG_INFO("Memory Profiler Stats:");
    LOG_INFO("  Allocations: {}", currentStats.allocations);
    LOG_INFO("  Deallocations: {}", currentStats.deallocations);
    LOG_INFO("  Active allocations: {}", currentStats.allocations - currentStats.deallocations);
    LOG_INFO("  Current CPU memory: {} MB", currentStats.totalCPUMemory / (1024 * 1024));
    LOG_INFO("  Peak CPU memory: {} MB", currentStats.peakCPUMemory / (1024 * 1024));
    LOG_INFO("  Current GPU memory: {} MB", currentStats.totalGPUMemory / (1024 * 1024));
    LOG_INFO("  Peak GPU memory: {} MB", currentStats.peakGPUMemory / (1024 * 1024));
}

void MemoryProfiler::Reset() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats = MemoryStats{};
}
