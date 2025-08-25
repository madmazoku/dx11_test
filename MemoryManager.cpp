/**
 * @file MemoryManager.cpp
 * @brief GPU memory management and profiling system
 * 
 * This file implements efficient GPU buffer pooling and memory profiling for DirectX 11.
 * It provides:
 * - Buffer pooling to reduce GPU allocation overhead
 * - Memory usage tracking and profiling
 * - Automatic cleanup of unused buffers
 * - Thread-safe operations for multi-threaded environments
 * 
 * The memory manager is crucial for performance in particle simulation systems
 * where hundreds of GPU buffers may be created and destroyed per frame.
 */

#include "MemoryManager.h"
#include "Logger.h"
#include <algorithm>

// Global memory profiler instance for tracking allocation statistics
MemoryProfiler g_memoryProfiler;

/**
 * Retrieve a GPU buffer from the pool or create a new one
 * 
 * This method implements efficient buffer reuse by searching the pool for
 * a suitable existing buffer before creating a new one. This reduces GPU
 * allocation overhead which can be significant in real-time applications.
 * 
 * @param size      Required buffer size in bytes
 * @param usage     D3D11 usage pattern (DEFAULT, DYNAMIC, etc.)
 * @param bindFlags D3D11 bind flags (VERTEX_BUFFER, CONSTANT_BUFFER, etc.)
 * @return          ComPtr to the allocated buffer, or nullptr on failure
 */
ComPtr<ID3D11Buffer> GPUMemoryManager::GetBuffer(size_t size, D3D11_USAGE usage, UINT bindFlags) {
    // Thread-safe access to the buffer pool
    std::lock_guard<std::mutex> lock(poolMutex);
    
    auto now = std::chrono::steady_clock::now();
    
    // Search for a suitable buffer in the pool that meets our requirements
    // We look for a buffer that is large enough and has matching usage/bind flags
    auto it = std::find_if(bufferPool.begin(), bufferPool.end(),
        [size, usage, bindFlags](const auto& pair) {
            const auto& info = pair.second;
            return info.size >= size && info.usage == usage && info.bindFlags == bindFlags;
        });
    
    // If we found a suitable buffer, reuse it to avoid GPU allocation overhead
    if (it != bufferPool.end()) {
        auto buffer = std::move(it->first);
        bufferPool.erase(it);
        g_memoryProfiler.RecordAllocation(size, true);
        return buffer;
    }
    
    // No suitable buffer found in pool - create a new one
    // Set up D3D11 buffer description with the requested parameters
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = static_cast<UINT>(size);
    desc.Usage = usage;
    desc.BindFlags = bindFlags;
    // Dynamic buffers need CPU write access for updating data
    desc.CPUAccessFlags = (usage == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
    
    ComPtr<ID3D11Buffer> buffer;
    HRESULT hr = device->CreateBuffer(&desc, nullptr, buffer.getAddressOf());
    
    // Log the result and update memory profiling statistics
    if (SUCCEEDED(hr)) {
        g_memoryProfiler.RecordAllocation(size, true);
        LOG_DEBUG("Created new GPU buffer: {} bytes", size);
    } else {
        LOG_ERROR("Failed to create GPU buffer: {} bytes, HRESULT: {}", size, hr);
    }
    
    return buffer;
}

/**
 * Return a GPU buffer to the pool for reuse
 * 
 * This method adds a buffer back to the pool instead of immediately destroying it.
 * This allows for efficient reuse and reduces GPU allocation/deallocation overhead.
 * The pool size is managed to prevent unlimited growth.
 * 
 * @param buffer The buffer to return to the pool (can be nullptr)
 */
void GPUMemoryManager::ReturnBuffer(ComPtr<ID3D11Buffer> buffer) {
    if (!buffer) return;
    
    // Thread-safe access to the buffer pool
    std::lock_guard<std::mutex> lock(poolMutex);
    
    // Extract buffer properties to store with the buffer for future matching
    D3D11_BUFFER_DESC desc;
    buffer->GetDesc(&desc);
    
    // Create buffer info structure with metadata for pool management
    BufferInfo info;
    info.size = desc.ByteWidth;
    info.usage = desc.Usage;
    info.bindFlags = desc.BindFlags;
    info.lastUsed = std::chrono::steady_clock::now();  // For age-based cleanup
    
    // Add buffer to pool for future reuse
    bufferPool.emplace_back(std::move(buffer), info);
    
    // Prevent unlimited pool growth by removing oldest buffers when limit exceeded
    // Limit pool size
    if (bufferPool.size() > 100) {
        // Sort by last used time to identify oldest buffers for removal
        // Remove oldest buffers
        std::sort(bufferPool.begin(), bufferPool.end(),
            [](const auto& a, const auto& b) {
                return a.second.lastUsed < b.second.lastUsed;
            });
        
        // Remove the oldest buffer and update memory profiling statistics
        auto oldSize = bufferPool.front().second.size;
        bufferPool.erase(bufferPool.begin());
        g_memoryProfiler.RecordDeallocation(oldSize, true);
    }
}

/**
 * Clean up old buffers from the pool based on age
 * 
 * This method removes buffers that haven't been used recently to prevent
 * the pool from consuming excessive GPU memory. It's typically called
 * periodically or when memory pressure is detected.
 * 
 * @param maxAge Maximum age for buffers to remain in pool (default: minutes)
 */
void GPUMemoryManager::CleanupOldBuffers(std::chrono::minutes maxAge) {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    // Calculate cutoff time - buffers older than this will be removed
    auto cutoffTime = std::chrono::steady_clock::now() - maxAge;
    
    // Remove all buffers that haven't been used since cutoff time
    // std::erase_if provides efficient removal while updating profiling stats
    auto removedCount = std::erase_if(bufferPool, [cutoffTime](const auto& pair) {
        bool shouldRemove = pair.second.lastUsed < cutoffTime;
        if (shouldRemove) {
            g_memoryProfiler.RecordDeallocation(pair.second.size, true);
        }
        return shouldRemove;
    });
    
    // Log cleanup results for monitoring and debugging
    if (removedCount > 0) {
        LOG_INFO("Cleaned up {} old GPU buffers from memory pool", removedCount);
    }
}

/**
 * Calculate total memory currently held in the buffer pool
 * 
 * @return Total size in bytes of all buffers currently in the pool
 */
size_t GPUMemoryManager::GetTotalAllocatedMemory() const {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    size_t total = 0;
    for (const auto& pair : bufferPool) {
        total += pair.second.size;
    }
    return total;
}

/**
 * Print detailed memory pool statistics for debugging and monitoring
 * 
 * This method outputs comprehensive information about the current state
 * of the GPU memory pool, including buffer counts, total memory usage,
 * and size distribution histogram.
 */
void GPUMemoryManager::PrintMemoryStats() const {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    size_t totalMemory = GetTotalAllocatedMemory();
    LOG_INFO("GPU Memory Pool Stats:");
    LOG_INFO("  Buffer count: {}", bufferPool.size());
    LOG_INFO("  Total memory: {} MB", totalMemory / (1024 * 1024));
    
    // Create histogram of buffer sizes for analysis
    // Grouping by sizes
    std::map<size_t, int> sizeGroups;
    for (const auto& pair : bufferPool) {
        size_t sizeCategory = (pair.second.size / 1024) * 1024; // Round to KB
        sizeGroups[sizeCategory]++;
    }
    
    // Output size distribution for memory usage analysis
    LOG_INFO("  Size distribution:");
    for (const auto& group : sizeGroups) {
        LOG_INFO("    {} KB: {} buffers", group.first / 1024, group.second);
    }
}

// =====================================================================================
// MemoryProfiler Implementation - Memory allocation tracking and profiling
// =====================================================================================

/**
 * Record a memory allocation for profiling statistics
 * 
 * This method tracks both CPU and GPU memory allocations, maintaining
 * running totals and peak usage statistics for performance analysis.
 * 
 * @param size  Size of the allocation in bytes
 * @param isGPU Whether this is a GPU allocation (true) or CPU allocation (false)
 */
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

/**
 * Record a memory deallocation for profiling statistics
 * 
 * Updates the running memory totals when memory is freed, helping track
 * actual memory usage and detect potential memory leaks.
 * 
 * @param size  Size of the deallocation in bytes
 * @param isGPU Whether this is a GPU deallocation (true) or CPU deallocation (false)
 */
void MemoryProfiler::RecordDeallocation(size_t size, bool isGPU) {
    std::lock_guard<std::mutex> lock(statsMutex);
    
    stats.deallocations++;
    if (isGPU) {
        // Prevent underflow in case of accounting errors
        stats.totalGPUMemory = (stats.totalGPUMemory >= size) ? stats.totalGPUMemory - size : 0;
    } else {
        stats.totalCPUMemory = (stats.totalCPUMemory >= size) ? stats.totalCPUMemory - size : 0;
    }
}

/**
 * Get current memory profiling statistics
 * 
 * @return Thread-safe copy of current memory statistics
 */
MemoryProfiler::MemoryStats MemoryProfiler::GetStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

/**
 * Print comprehensive memory profiling statistics
 * 
 * Outputs detailed information about memory allocation patterns,
 * current usage, peak usage, and allocation/deallocation counts
 * for both CPU and GPU memory.
 */
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

/**
 * Reset all profiling statistics to zero
 * 
 * Useful for benchmarking specific sections of code or starting
 * fresh memory profiling sessions.
 */
void MemoryProfiler::Reset() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats = MemoryStats{};
}
