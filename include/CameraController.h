#pragma once
#include <array>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef std::array<float, 16> Mat4;

class CameraController {
public:
  CameraController();

  // Camera control methods
  void zoomIn(float step = 0.1f);
  void zoomOut(float step = 0.1f);
  void setZoom(float zoom);
  float getZoom() const { return zoomLevel; }

  // New panning methods
  void pan(float deltaX, float deltaY);
  void setPanOffset(float x, float y);
  void resetPan();
  std::pair<float, float> getPanOffset() const { return std::make_pair(panOffsetX, panOffsetY); }

  // New rotation methods
  void rotate(float deltaX, float deltaY);
  void setRotation(float yaw, float pitch);
  void resetRotation();
  std::pair<float, float> getRotation() const { return std::make_pair(rotationYaw, rotationPitch); }

  // Matrix generation methods
  Mat4 getViewMatrix() const;
  Mat4 getProjectionMatrix(float aspect, float fov = M_PI / 4.0f, float nearPlane = 0.1f, float farPlane = 100.0f) const;

  // Static utility methods for different camera types
  static Mat4 createIsometricViewMatrix(float distance, float panX = 0.0f, float panY = 0.0f, float yaw = 0.785f, float pitch = 0.615f);
  static Mat4 createPerspectiveViewMatrix(float eyeX, float eyeY, float eyeZ,
    float centerX = 0.0f, float centerY = 0.0f, float centerZ = 0.0f);
  static Mat4 createOrthographicProjection(float left, float right, float bottom, float top, float near, float far);

  // Camera settings
  void setMinMaxZoom(float minZoom, float maxZoom);
  void setCameraDistance(float distance) { baseCameraDistance = distance; }
  void setPanSensitivity(float sensitivity) { panSensitivity = sensitivity; }
  void setRotationSensitivity(float sensitivity) { rotationSensitivity = sensitivity; }

private:
  float zoomLevel;
  float minZoom;
  float maxZoom;
  float baseCameraDistance;

  // New panning variables
  float panOffsetX;
  float panOffsetY;
  float panSensitivity;

  // New rotation variables
  float rotationYaw;      // Y-axis rotation (left/right)
  float rotationPitch;    // X-axis rotation (up/down)
  float rotationSensitivity;

  // Helper methods for matrix operations
  static Mat4 mat4Identity();
  static Mat4 mat4Perspective(float fov, float aspect, float near, float far);
  static Mat4 mat4LookAt(float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ);
};