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

  // Mouse rotation controls
  void startRotation(double mouseX, double mouseY);
  void updateRotation(double mouseX, double mouseY);
  void endRotation();
  bool isRotating() const { return rotating; }

  // Matrix generation methods
  Mat4 getViewMatrix() const;
  Mat4 getProjectionMatrix(float aspect, float fov = M_PI / 4.0f, float nearPlane = 0.1f, float farPlane = 100.0f) const;

  // Static utility methods for different camera types
  static Mat4 createIsometricViewMatrix(float distance);
  static Mat4 createPerspectiveViewMatrix(float eyeX, float eyeY, float eyeZ,
    float centerX = 0.0f, float centerY = 0.0f, float centerZ = 0.0f);
  static Mat4 createOrthographicProjection(float left, float right, float bottom, float top, float near, float far);

  // Camera settings
  void setMinMaxZoom(float minZoom, float maxZoom);
  void setCameraDistance(float distance) { baseCameraDistance = distance; }

private:
  float zoomLevel;
  float minZoom;
  float maxZoom;
  float baseCameraDistance;

  // Rotation state
  bool rotating;
  double lastMouseX, lastMouseY;
  float rotationX, rotationY; // Euler angles for rotation
  float rotationSensitivity;

  // Helper methods for matrix operations
  static Mat4 mat4Identity();
  static Mat4 mat4Perspective(float fov, float aspect, float near, float far);
  static Mat4 mat4LookAt(float eyeX, float eyeY, float eyeZ,
    float centerX, float centerY, float centerZ,
    float upX, float upY, float upZ);

  // Rotation helper methods
  Mat4 createRotatedViewMatrix(float distance) const;
};