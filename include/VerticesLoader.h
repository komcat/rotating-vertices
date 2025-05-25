#pragma once
#include <vector>

class VerticesLoader {
public:
  // Generate scan vertices from JSON data - all 45 points
  static std::vector<float> generateScanVertices();

  // Generate scaled scan vertices for better visibility
  static std::vector<float> generateScaledScanVertices(float scaleFactor = 1000.0f);

  // Generate line indices to connect all vertices in sequence
  static std::vector<unsigned int> generateScanLineIndices();

  // Generate point indices for rendering individual points
  static std::vector<unsigned int> generateScanPointIndices();

private:
  // Raw vertex data based on scan measurements
  static const std::vector<std::vector<float>> rawScanData;
};