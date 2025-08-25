#pragma once

#include <d3d11.h>
#include <memory>

/**
 * @file ComPtr.h
 * @brief Smart pointer wrapper for DirectX COM (Component Object Model) objects
 * 
 * This file provides a lightweight RAII (Resource Acquisition Is Initialization) wrapper
 * for DirectX COM objects. It automatically manages reference counting and ensures
 * proper cleanup when objects go out of scope.
 * 
 * COM objects in DirectX use manual reference counting through AddRef() and Release()
 * methods. This smart pointer eliminates the need for manual memory management
 * and prevents resource leaks.
 */

/**
 * @class ComPtr
 * @brief RAII smart pointer wrapper for DirectX COM objects
 * 
 * This template class provides automatic memory management for DirectX COM objects
 * by wrapping the raw COM interface pointer and automatically calling Release()
 * when the object is destroyed. It supports move semantics but explicitly disables
 * copy operations to prevent accidental double-deletion.
 * 
 * Key features:
 * - Automatic Release() call in destructor
 * - Move-only semantics (no copying allowed)
 * - Convenient operator overloads for transparent usage
 * - Null pointer safety
 * 
 * @tparam T The type of COM interface to wrap (e.g., ID3D11Device, ID3D11Buffer)
 */
template<typename T>
class ComPtr {
private:
    /**
     * @brief Raw COM interface pointer
     * 
     * The underlying COM object pointer that this smart pointer manages.
     * Initialized to nullptr and automatically released in destructor.
     */
    T* ptr = nullptr;

public:
    /**
     * @brief Default constructor
     * 
     * Creates an empty ComPtr with null pointer.
     */
    ComPtr() = default;
    
    /**
     * @brief Explicit constructor from raw pointer
     * 
     * Takes ownership of an existing COM object pointer. The caller
     * must ensure the object has been properly AddRef'd if needed.
     * 
     * @param p Raw COM interface pointer to take ownership of
     */
    explicit ComPtr(T* p) : ptr(p) {}
    
    /**
     * @brief Destructor
     * 
     * Automatically releases the COM object if it exists, decrementing
     * its reference count and potentially destroying it.
     */
    ~ComPtr() {
        if (ptr) {
            ptr->Release();
        }
    }

    /**
     * @brief Move constructor
     * 
     * Transfers ownership from another ComPtr, leaving the source empty.
     * This is efficient as it doesn't change reference counts.
     * 
     * @param other ComPtr to move from
     */
    ComPtr(ComPtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    /**
     * @brief Move assignment operator
     * 
     * Releases current object (if any) and takes ownership of the moved object.
     * Provides strong exception safety and handles self-assignment.
     * 
     * @param other ComPtr to move from
     * @return Reference to this ComPtr
     */
    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            if (ptr) ptr->Release();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    /**
     * @brief Deleted copy constructor
     * 
     * Copy operations are explicitly disabled to prevent accidental
     * double-release of COM objects, which would cause crashes.
     */
    ComPtr(const ComPtr&) = delete;
    
    /**
     * @brief Deleted copy assignment operator
     * 
     * Copy operations are explicitly disabled to prevent accidental
     * double-release of COM objects, which would cause crashes.
     */
    ComPtr& operator=(const ComPtr&) = delete;

    /**
     * @brief Arrow operator for method access
     * 
     * Provides transparent access to the wrapped COM object's methods.
     * 
     * @return Raw pointer to the COM object
     */
    T* operator->() const { return ptr; }
    
    /**
     * @brief Dereference operator
     * 
     * Provides reference access to the wrapped COM object.
     * Use with caution - ensure pointer is not null.
     * 
     * @return Reference to the COM object
     */
    T& operator*() const { return *ptr; }
    
    /**
     * @brief Get raw pointer
     * 
     * Returns the raw COM interface pointer without transferring ownership.
     * Useful for passing to functions that expect raw pointers.
     * 
     * @return Raw COM interface pointer (may be nullptr)
     */
    T* get() const { return ptr; }
    
    /**
     * @brief Get address of internal pointer
     * 
     * Returns the address of the internal pointer for use with DirectX
     * creation functions that output through a T** parameter.
     * 
     * WARNING: This should only be used when the current pointer is null,
     * otherwise it can cause memory leaks.
     * 
     * @return Pointer to the internal pointer storage
     */
    T** getAddressOf() { return &ptr; }
    
    /**
     * @brief Reset the pointer to null
     * 
     * Releases the current COM object (if any) and sets the pointer to null.
     * This is equivalent to assigning a default-constructed ComPtr.
     */
    void reset() {
        if (ptr) {
            ptr->Release();
            ptr = nullptr;
        }
    }

    /**
     * @brief Boolean conversion operator
     * 
     * Allows the ComPtr to be used in boolean contexts to check if it
     * contains a valid COM object pointer.
     * 
     * @return true if the pointer is not null, false otherwise
     */
    explicit operator bool() const { return ptr != nullptr; }
};
