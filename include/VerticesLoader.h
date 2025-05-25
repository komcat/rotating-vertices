#pragma once
#include <vector>
#include <string>

struct ScanPoint {
  float x, y, z;     // Normalized position
  float value;       // Measurement value for color mapping
  bool isPeak;       // Whether this point is a peak
  std::string axis;  // Axis that was scanned
  std::string direction; // Direction of scan
};

class VerticesLoader {
public:
  // Load scan data from JSON file
  static bool loadScanFromFile(const std::string& filePath, float scaleFactor = 1000.0f);

  // Load the most recent scan file from logs\scanning folder
  static bool loadMostRecentScan(float scaleFactor = 1000.0f);

  // Initialize file list and load first file
  static bool initializeScanFiles(const std::string& directory = "logs/scanning", float scaleFactor = 1000.0f);

  // Cycle to next scan file
  static bool loadNextScanFile(float scaleFactor = 1000.0f);

  // Cycle to previous scan file
  static bool loadPreviousScanFile(float scaleFactor = 1000.0f);

  // Get current file index and total count
  static std::pair<int, int> getCurrentFileInfo();

  // Generate vertices from loaded scan data
  static std::vector<float> generateScanVertices();

  // Generate vertices with values (x, y, z, value) for color mapping
  static std::vector<float> generateScanVerticesWithValues();

  // Generate line indices to connect all vertices in sequence
  static std::vector<unsigned int> generateScanLineIndices();

  // Generate point indices for rendering individual points
  static std::vector<unsigned int> generateScanPointIndices();

  // Get measurement values for color mapping
  static std::vector<float> getMeasurementValues();

  // Get min/max values for color scaling
  static std::pair<float, float> getValueRange();

  // Get scan information
  static std::string getScanInfo();

  // Clear loaded data
  static void clear();

private:
  static std::vector<ScanPoint> scanPoints;
  static std::string currentScanFile;
  static float minValue, maxValue;
  static std::vector<std::string> availableFiles;
  static int currentFileIndex;

  // Helper functions
  static std::vector<std::string> findScanFiles(const std::string& directory);
  static std::string getMostRecentFile(const std::vector<std::string>& files);
  static bool parseScanFile(const std::string& filePath, float scaleFactor);
  static void sortFilesByDate(std::vector<std::string>& files);
};