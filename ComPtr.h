#pragma once

#include <d3d11.h>
#include <memory>

// RAII wrapper for DirectX COM objects
template<typename T>
class ComPtr {
private:
    T* ptr = nullptr;

public:
    ComPtr() = default;
    
    explicit ComPtr(T* p) : ptr(p) {}
    
    ~ComPtr() {
        if (ptr) {
            ptr->Release();
        }
    }

    // Move constructor
    ComPtr(ComPtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    // Move assignment
    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            if (ptr) ptr->Release();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    // Delete copy operations
    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;

    T* operator->() const { return ptr; }
    T& operator*() const { return *ptr; }
    T* get() const { return ptr; }
    T** getAddressOf() { return &ptr; }
    
    void reset() {
        if (ptr) {
            ptr->Release();
            ptr = nullptr;
        }
    }

    explicit operator bool() const { return ptr != nullptr; }
};
