#pragma once

#include <DirectXMath.h>
#include <Windows.h>
#include <vector>
#include "Structures.h"

using namespace DirectX;

/**
 * @file InteractiveCamera.h
 * @brief Interactive 3D camera system with automatic particle tracking
 * 
 * This file implements a sophisticated camera system that provides smooth interactive
 * controls while automatically adapting to the particle simulation's spatial extent.
 * The camera uses an orbit-style control scheme where the user can rotate around
 * and zoom into the particle cloud.
 * 
 * Key features:
 * - Spherical coordinate system for intuitive orbit controls
 * - Automatic particle cloud tracking and centering
 * - Percentile-based zoom for consistent framing
 * - Smooth mouse interaction with configurable sensitivity
 * - Automatic optimal distance calculation
 * - Real-time adaptation to changing particle distributions
 * 
 * The camera system ensures that users always have an optimal view of the
 * particle simulation regardless of how the particles move or spread.
 */

/**
 * @struct CameraState
 * @brief Complete state information for the interactive camera
 * 
 * Encapsulates all camera parameters including position, orientation, and
 * interaction state. The camera uses spherical coordinates for intuitive
 * orbit-style navigation around the particle system.
 */
struct CameraState {
    XMFLOAT3 position;          ///< Current camera position in world space
    XMFLOAT3 target;            ///< Point the camera is looking at (usually cloud center)
    XMFLOAT3 up;                ///< Camera up vector (usually world Y-axis)
    float distance;             ///< Distance from camera to target point
    float theta;                ///< Horizontal rotation angle in radians (azimuth)
    float phi;                  ///< Vertical rotation angle in radians (elevation)
    float zoomPercentile;       ///< Zoom level as percentile of particle distribution (1-100%)
};

/**
 * @class InteractiveCamera
 * @brief Advanced interactive camera system with automatic particle tracking
 * 
 * Provides an intuitive camera system that automatically tracks and frames the
 * particle simulation while allowing smooth user interaction. The camera uses
 * spherical coordinates for orbit-style controls and automatically adapts to
 * the spatial distribution of particles.
 * 
 * Camera behavior:
 * 1. Automatically centers on the particle cloud centroid
 * 2. Calculates optimal viewing distance based on particle distribution
 * 3. Responds smoothly to mouse input for rotation and zoom
 * 4. Updates in real-time as particles move and spread
 * 5. Maintains consistent framing through percentile-based zoom
 * 
 * The system provides both automated behavior for ease of use and manual
 * controls for precise camera positioning when needed.
 */
class InteractiveCamera {
private:
    CameraState state;                      ///< Current camera state and parameters
    RenderConfig config;                    ///< Rendering configuration for camera behavior
    
    // === Mouse Interaction State ===
    bool isDragging;                        ///< True if currently dragging with mouse
    POINT lastMousePos;                     ///< Previous mouse position for delta calculation
    POINT currentMousePos;                  ///< Current mouse position
    
    // === Particle Cloud Tracking ===
    XMFLOAT3 cloudCenter;                   ///< Computed center of particle cloud
    float cloudRadius;                      ///< Computed radius of particle cloud
    std::vector<float> particleDistances;   ///< Distances from center for percentile calculation

public:
    // === Construction and Configuration ===
    
    /**
     * @brief Construct interactive camera with render configuration
     * 
     * @param config Rendering configuration containing camera parameters
     */
    InteractiveCamera(const RenderConfig& config);
    
    /**
     * @brief Default destructor
     */
    ~InteractiveCamera() = default;
    
    // === Particle Tracking Methods ===
    
    /**
     * @brief Update camera bounds based on particle positions
     * 
     * Analyzes the current particle distribution to compute the centroid,
     * radius, and distance distribution. This information is used for
     * automatic camera centering and optimal zoom calculation.
     * 
     * @param particles Vector of particle structures with position data
     */
    void UpdateCloudBounds(const std::vector<Particle>& particles);
    
    /**
     * @brief Update camera bounds based on position array
     * 
     * Overloaded version that works directly with position arrays for
     * better performance when particle structures are not needed.
     * 
     * @param positions Array of particle positions
     * @param count Number of positions in the array
     */
    void UpdateCloudBounds(const XMFLOAT3* positions, size_t count);
    
    // === Mouse Interaction Methods ===
    
    /**
     * @brief Handle mouse movement for camera rotation
     * 
     * Processes mouse movement to rotate the camera around the target point.
     * Only affects camera when mouse button is pressed (isDragging = true).
     * 
     * @param x Current mouse X coordinate
     * @param y Current mouse Y coordinate
     */
    void OnMouseMove(int x, int y);
    
    /**
     * @brief Handle mouse button press to start camera interaction
     * 
     * Begins mouse interaction mode and captures the initial mouse position
     * for delta calculations.
     * 
     * @param x Mouse X coordinate at press
     * @param y Mouse Y coordinate at press
     * @param leftButton true if left button pressed, false for right button
     */
    void OnMouseDown(int x, int y, bool leftButton);
    
    /**
     * @brief Handle mouse button release to end camera interaction
     * 
     * Ends mouse interaction mode and stops camera rotation.
     * 
     * @param x Mouse X coordinate at release
     * @param y Mouse Y coordinate at release
     * @param leftButton true if left button released, false for right button
     */
    void OnMouseUp(int x, int y, bool leftButton);
    
    /**
     * @brief Handle mouse wheel for camera zoom
     * 
     * Adjusts camera zoom level based on mouse wheel input. Zoom is
     * implemented as percentile adjustment of the particle distribution.
     * 
     * @param delta Mouse wheel delta (positive = zoom in, negative = zoom out)
     */
    void OnMouseWheel(int delta);
    
    // === Matrix Generation ===
    
    /**
     * @brief Generate view transformation matrix
     * 
     * Creates the view matrix that transforms world coordinates to camera
     * coordinates. Based on current camera position, target, and up vector.
     * 
     * @return View transformation matrix
     */
    XMMATRIX GetViewMatrix() const;
    /**
     * @brief Generate projection transformation matrix
     * 
     * Creates the projection matrix that transforms camera coordinates to
     * normalized device coordinates. Uses perspective projection with
     * field of view and aspect ratio from configuration.
     * 
     * @param aspectRatio Screen width/height aspect ratio
     * @return Projection transformation matrix
     */
    XMMATRIX GetProjectionMatrix(float aspectRatio) const;
    
    /**
     * @brief Get current camera world position
     * @return Camera position in world coordinates
     */
    XMFLOAT3 GetPosition() const { return state.position; }
    
    /**
     * @brief Get current camera target point
     * @return Point the camera is looking at in world coordinates
     */
    XMFLOAT3 GetTarget() const { return state.target; }
    
    // === Configuration Methods ===
    
    /**
     * @brief Update camera with new render configuration
     * @param newConfig New rendering configuration to apply
     */
    void SetConfig(const RenderConfig& newConfig) { config = newConfig; }
    
    /**
     * @brief Set zoom level as percentile of particle distribution
     * 
     * Sets the camera distance based on a percentile of the particle
     * distance distribution from the cloud center. This provides
     * consistent framing regardless of particle distribution shape.
     * 
     * @param percentile Zoom percentile (0.01 = very close, 1.0 = include all particles)
     */
    void SetZoomPercentile(float percentile);
    
    /**
     * @brief Set camera rotation angles directly
     * 
     * Allows direct control of camera orientation using spherical coordinates.
     * 
     * @param theta Horizontal rotation angle in radians (azimuth)
     * @param phi Vertical rotation angle in radians (elevation)
     */
    void SetRotation(float theta, float phi);
    
    /**
     * @brief Reset camera to default configuration state
     * 
     * Restores camera to initial position and orientation as specified
     * in the render configuration.
     */
    void ResetCamera();
    
    // === Automatic Behavior ===
    
    /**
     * @brief Center camera on the current particle cloud
     * 
     * Automatically adjusts camera target to the computed cloud centroid
     * and sets optimal viewing distance. Called automatically when enabled.
     */
    void CenterOnCloud();
    
    /**
     * @brief Calculate optimal viewing distance for current particle distribution
     * 
     * Computes an appropriate camera distance to frame the entire particle
     * cloud comfortably, taking into account the current zoom percentile.
     * 
     * @return Optimal camera distance in world units
     */
    float CalculateOptimalDistance() const;
    
private:
    // === Internal Implementation Methods ===
    
    /**
     * @brief Update camera position based on spherical coordinates
     * 
     * Converts the spherical coordinate state (distance, theta, phi) into
     * a world position relative to the current target point. Called after
     * any change to the spherical parameters.
     */
    void UpdateCameraPosition();
    
    /**
     * @brief Clamp rotation angles to valid ranges
     * 
     * Ensures that rotation angles stay within reasonable bounds to prevent
     * camera gimbal lock and maintain intuitive behavior. Typically clamps
     * phi (elevation) to prevent flipping over the poles.
     */
    void ClampAngles();
    
    /**
     * @brief Calculate distance at specific percentile of particle distribution
     * 
     * Uses the precomputed particle distance array to find the distance
     * at a given percentile of the distribution. This enables consistent
     * zoom behavior regardless of particle distribution shape.
     * 
     * @param percentile Target percentile (0.0 to 1.0)
     * @return Distance value at the specified percentile
     */
    float CalculatePercentileDistance(float percentile) const;
};
