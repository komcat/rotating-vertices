#include "CameraController.h"
#include <algorithm>
#include <iostream>

CameraController::CameraController()
  : zoomLevel(1.0f), minZoom(0.1f), maxZoom(10.0f), baseCameraDistance(3.0f),
  panOffsetX(0.0f), panOffsetY(0.0f), panSensitivity(1.0f),
  rotationYaw(0.785f), rotationPitch(0.615f), rotationSensitivity(1.0f) {
}

void CameraController::zoomIn(float step) {
  zoomLevel = std::min(zoomLevel + step, maxZoom);
  std::cout << "Zoom: " << zoomLevel << "x" << std::endl;
}

void CameraController::zoomOut(float step) {
  zoomLevel = std::max(zoomLevel - step, minZoom);
  std::cout << "Zoom: " << zoomLevel << "x" << std::endl;
}

void CameraController::setZoom(float zoom) {
  zoomLevel = std::clamp(zoom, minZoom, maxZoom);
}

void CameraController::setMinMaxZoom(float minZoom, float maxZoom) {
  this->minZoom = minZoom;
  this->maxZoom = maxZoom;
  zoomLevel = std::clamp(zoomLevel, minZoom, maxZoom);
}

void CameraController::pan(float deltaX, float deltaY) {
  // Scale pan movement by zoom level and sensitivity
  float scaledDeltaX = deltaX * panSensitivity / zoomLevel;
  float scaledDeltaY = deltaY * panSensitivity / zoomLevel;

  panOffsetX += scaledDeltaX;
  panOffsetY += scaledDeltaY;

  // Optional: Add pan limits to prevent going too far off-screen
  const float maxPan = 50.0f / zoomLevel;
  panOffsetX = std::clamp(panOffsetX, -maxPan, maxPan);
  panOffsetY = std::clamp(panOffsetY, -maxPan, maxPan);
}

void CameraController::setPanOffset(float x, float y) {
  panOffsetX = x;
  panOffsetY = y;
}

void CameraController::resetPan() {
  panOffsetX = 0.0f;
  panOffsetY = 0.0f;
  std::cout << "Pan reset to center" << std::endl;
}

void CameraController::rotate(float deltaX, float deltaY) {
  // Apply rotation with sensitivity
  rotationYaw += deltaX * rotationSensitivity;
  rotationPitch += deltaY * rotationSensitivity;

  // Clamp pitch to prevent flipping
  const float maxPitch = M_PI * 0.49f; // Just under 90 degrees
  rotationPitch = std::clamp(rotationPitch, -maxPitch, maxPitch);

  // Wrap yaw around 2*PI
  while (rotationYaw > 2.0f * M_PI) rotationYaw -= 2.0f * M_PI;
  while (rotationYaw < 0.0f) rotationYaw += 2.0f * M_PI;
}

void CameraController::setRotation(float yaw, float pitch) {
  rotationYaw = yaw;
  rotationPitch = pitch;
}

void CameraController::resetRotation() {
  rotationYaw = 0.785f;   // ~45 degrees default
  rotationPitch = 0.615f; // ~35 degrees default  
  std::cout << "Rotation reset to isometric view" << std::endl;
}

Mat4 CameraController::getViewMatrix() const {
  float distance = baseCameraDistance / zoomLevel; // Inverse relationship for intuitive zoom
  return createIsometricViewMatrix(distance, panOffsetX, panOffsetY, rotationYaw, rotationPitch);
}

Mat4 CameraController::getProjectionMatrix(float aspect, float fov, float nearPlane, float farPlane) const {
  return mat4Perspective(fov, aspect, nearPlane, farPlane);
}

Mat4 CameraController::createIsometricViewMatrix(float distance, float panX, float panY, float yaw, float pitch) {
  // Calculate camera position based on spherical coordinates
  float cosYaw = cos(yaw);
  float sinYaw = sin(yaw);
  float cosPitch = cos(pitch);
  float sinPitch = sin(pitch);

  // Position camera at distance with rotation
  float eyeX = distance * cosPitch * cosYaw;
  float eyeY = distance * sinPitch;
  float eyeZ = distance * cosPitch * sinYaw;

  // Look at center with pan offset
  return mat4LookAt(eyeX, eyeY, eyeZ,
    panX, panY, 0.0f,                              // look at center with pan offset
    0.0f, 1.0f, 0.0f);                            // up vector
}

Mat4 CameraController::createPerspectiveViewMatrix(float eyeX, float eyeY, float eyeZ,
  float centerX, float centerY, float centerZ) {
  return mat4LookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, 0.0f, 1.0f, 0.0f);
}

Mat4 CameraController::createOrthographicProjection(float left, float right, float bottom, float top, float near, float far) {
  Mat4 result = { 0 };

  result[0] = 2.0f / (right - left);
  result[5] = 2.0f / (top - bottom);
  result[10] = -2.0f / (far - near);
  result[12] = -(right + left) / (right - left);
  result[13] = -(top + bottom) / (top - bottom);
  result[14] = -(far + near) / (far - near);
  result[15] = 1.0f;

  return result;
}

// Private helper methods
Mat4 CameraController::mat4Identity() {
  Mat4 result = { 0 };
  result[0] = result[5] = result[10] = result[15] = 1.0f;
  return result;
}

Mat4 CameraController::mat4Perspective(float fov, float aspect, float near, float far) {
  Mat4 result = { 0 };
  float tanHalfFov = tan(fov / 2.0f);

  result[0] = 1.0f / (aspect * tanHalfFov);
  result[5] = 1.0f / tanHalfFov;
  result[10] = -(far + near) / (far - near);
  result[11] = -1.0f;
  result[14] = -(2.0f * far * near) / (far - near);

  return result;
}

Mat4 CameraController::mat4LookAt(float eyeX, float eyeY, float eyeZ,
  float centerX, float centerY, float centerZ,
  float upX, float upY, float upZ) {
  // Calculate forward vector
  float forwardX = centerX - eyeX;
  float forwardY = centerY - eyeY;
  float forwardZ = centerZ - eyeZ;

  // Normalize forward
  float forwardLength = sqrt(forwardX * forwardX + forwardY * forwardY + forwardZ * forwardZ);
  forwardX /= forwardLength;
  forwardY /= forwardLength;
  forwardZ /= forwardLength;

  // Calculate right vector (cross product of forward and up)
  float rightX = forwardY * upZ - forwardZ * upY;
  float rightY = forwardZ * upX - forwardX * upZ;
  float rightZ = forwardX * upY - forwardY * upX;

  // Normalize right
  float rightLength = sqrt(rightX * rightX + rightY * rightY + rightZ * rightZ);
  rightX /= rightLength;
  rightY /= rightLength;
  rightZ /= rightLength;

  // Calculate up vector (cross product of right and forward)
  float newUpX = rightY * (-forwardZ) - rightZ * (-forwardY);
  float newUpY = rightZ * (-forwardX) - rightX * (-forwardZ);
  float newUpZ = rightX * (-forwardY) - rightY * (-forwardX);

  Mat4 result = { 0 };
  result[0] = rightX;   result[1] = newUpX;   result[2] = -forwardX;  result[3] = 0;
  result[4] = rightY;   result[5] = newUpY;   result[6] = -forwardY;  result[7] = 0;
  result[8] = rightZ;   result[9] = newUpZ;   result[10] = -forwardZ; result[11] = 0;
  result[12] = -(rightX * eyeX + rightY * eyeY + rightZ * eyeZ);
  result[13] = -(newUpX * eyeX + newUpY * eyeY + newUpZ * eyeZ);
  result[14] = -(-forwardX * eyeX + -forwardY * eyeY + -forwardZ * eyeZ);
  result[15] = 1;

  return result;
}