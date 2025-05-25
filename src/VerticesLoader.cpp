#include "VerticesLoader.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <limits>

// Static member definitions
std::vector<ScanPoint> VerticesLoader::scanPoints;
std::string VerticesLoader::currentScanFile;
float VerticesLoader::minValue = std::numeric_limits<float>::max();
float VerticesLoader::maxValue = std::numeric_limits<float>::lowest();
std::vector<std::string> VerticesLoader::availableFiles;
int VerticesLoader::currentFileIndex = -1;

bool VerticesLoader::loadScanFromFile(const std::string& filePath, float scaleFactor) {
  return parseScanFile(filePath, scaleFactor);
}

bool VerticesLoader::initializeScanFiles(const std::string& directory, float scaleFactor) {
  try {
    // Check if directory exists
    if (!std::filesystem::exists(directory)) {
      std::cerr << "Scan directory not found: " << directory << std::endl;
      return false;
    }

    availableFiles = findScanFiles(directory);
    if (availableFiles.empty()) {
      std::cerr << "No scan files found in " << directory << std::endl;
      return false;
    }

    // Sort files by date (newest first)
    sortFilesByDate(availableFiles);

    std::cout << "Found " << availableFiles.size() << " scan files:" << std::endl;
    for (size_t i = 0; i < availableFiles.size(); ++i) {
      std::cout << "  [" << i << "] " << std::filesystem::path(availableFiles[i]).filename().string() << std::endl;
    }

    // Load the most recent file (index 0)
    currentFileIndex = 0;
    return parseScanFile(availableFiles[currentFileIndex], scaleFactor);
  }
  catch (const std::exception& e) {
    std::cerr << "Error initializing scan files: " << e.what() << std::endl;
    return false;
  }
}

bool VerticesLoader::loadNextScanFile(float scaleFactor) {
  if (availableFiles.empty()) {
    std::cerr << "No files available. Call initializeScanFiles first." << std::endl;
    return false;
  }

  currentFileIndex = (currentFileIndex + 1) % availableFiles.size();
  std::cout << "Loading file [" << currentFileIndex << "/" << (availableFiles.size() - 1) << "]: "
    << std::filesystem::path(availableFiles[currentFileIndex]).filename().string() << std::endl;

  return parseScanFile(availableFiles[currentFileIndex], scaleFactor);
}

bool VerticesLoader::loadPreviousScanFile(float scaleFactor) {
  if (availableFiles.empty()) {
    std::cerr << "No files available. Call initializeScanFiles first." << std::endl;
    return false;
  }

  currentFileIndex = (currentFileIndex - 1 + availableFiles.size()) % availableFiles.size();
  std::cout << "Loading file [" << currentFileIndex << "/" << (availableFiles.size() - 1) << "]: "
    << std::filesystem::path(availableFiles[currentFileIndex]).filename().string() << std::endl;

  return parseScanFile(availableFiles[currentFileIndex], scaleFactor);
}

std::pair<int, int> VerticesLoader::getCurrentFileInfo() {
  return std::make_pair(currentFileIndex, static_cast<int>(availableFiles.size()));
}

std::vector<float> VerticesLoader::generateScanVertices() {
  std::vector<float> vertices;
  vertices.reserve(scanPoints.size() * 3);

  for (const auto& point : scanPoints) {
    vertices.push_back(point.x);
    vertices.push_back(point.y);
    vertices.push_back(point.z);
  }

  return vertices;
}

std::vector<float> VerticesLoader::generateScanVerticesWithValues() {
  std::vector<float> vertices;
  vertices.reserve(scanPoints.size() * 4);

  for (const auto& point : scanPoints) {
    vertices.push_back(point.x);
    vertices.push_back(point.y);
    vertices.push_back(point.z);
    vertices.push_back(point.value);
  }

  return vertices;
}

std::vector<unsigned int> VerticesLoader::generateScanLineIndices() {
  std::vector<unsigned int> indices;

  if (scanPoints.size() < 2) return indices;

  // Connect all points in sequence
  for (unsigned int i = 0; i < static_cast<unsigned int>(scanPoints.size()) - 1; ++i) {
    indices.push_back(i);
    indices.push_back(i + 1);
  }

  return indices;
}

std::vector<unsigned int> VerticesLoader::generateScanPointIndices() {
  std::vector<unsigned int> indices;

  for (unsigned int i = 0; i < static_cast<unsigned int>(scanPoints.size()); ++i) {
    indices.push_back(i);
  }

  return indices;
}

std::vector<float> VerticesLoader::getMeasurementValues() {
  std::vector<float> values;
  values.reserve(scanPoints.size());

  for (const auto& point : scanPoints) {
    values.push_back(point.value);
  }

  return values;
}

std::pair<float, float> VerticesLoader::getValueRange() {
  return std::make_pair(minValue, maxValue);
}

std::string VerticesLoader::getScanInfo() {
  if (scanPoints.empty()) {
    return "No scan data loaded";
  }

  std::stringstream info;
  info << "Scan File: " << std::filesystem::path(currentScanFile).filename().string() << "\\n";

  if (!availableFiles.empty()) {
    info << "File: [" << currentFileIndex << "/" << (availableFiles.size() - 1) << "]\\n";
  }

  info << "Points: " << scanPoints.size() << "\\n";
  info << "Value Range: " << minValue << " to " << maxValue << "\\n";

  // Count peaks
  int peakCount = 0;
  for (const auto& point : scanPoints) {
    if (point.isPeak) peakCount++;
  }
  info << "Peaks: " << peakCount;

  return info.str();
}

void VerticesLoader::clear() {
  scanPoints.clear();
  currentScanFile.clear();
  minValue = std::numeric_limits<float>::max();
  maxValue = std::numeric_limits<float>::lowest();
  // Don't clear availableFiles and currentFileIndex - keep them for cycling
}

std::vector<std::string> VerticesLoader::findScanFiles(const std::string& directory) {
  std::vector<std::string> files;

  try {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
      if (entry.is_regular_file()) {
        std::string filename = entry.path().filename().string();
        // Look for JSON files that contain "scan" in the name
        if (filename.find(".json") != std::string::npos &&
          filename.find("scan") != std::string::npos) {
          files.push_back(entry.path().string());
        }
      }
    }
  }
  catch (const std::exception& e) {
    std::cerr << "Error reading directory: " << e.what() << std::endl;
  }

  return files;
}

std::string VerticesLoader::getMostRecentFile(const std::vector<std::string>& files) {
  if (files.empty()) return "";

  std::string mostRecent = files[0];
  std::filesystem::file_time_type mostRecentTime = std::filesystem::last_write_time(files[0]);

  for (const auto& file : files) {
    auto fileTime = std::filesystem::last_write_time(file);
    if (fileTime > mostRecentTime) {
      mostRecentTime = fileTime;
      mostRecent = file;
    }
  }

  return mostRecent;
}

bool VerticesLoader::parseScanFile(const std::string& filePath, float scaleFactor) {
  clear();

  // Initialize min/max properly
  minValue = std::numeric_limits<float>::max();
  maxValue = std::numeric_limits<float>::lowest();

  std::cout << "Initial min/max values: " << minValue << " to " << maxValue << std::endl;

  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Cannot open file: " << filePath << std::endl;
    return false;
  }

  // Read entire file content
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();
  file.close();

  try {
    // Simple JSON parsing - find baseline position first
    float baselineX = 0, baselineY = 0, baselineZ = 0, baselineValue = 0;

    // Find baseline section
    size_t baselinePos = content.find("\"baseline\"");
    if (baselinePos != std::string::npos) {
      // Extract baseline position
      size_t posStart = content.find("\"position\"", baselinePos);
      if (posStart != std::string::npos) {
        // Find x, y, z values in baseline
        size_t xPos = content.find("\"x\":", posStart);
        size_t yPos = content.find("\"y\":", posStart);
        size_t zPos = content.find("\"z\":", posStart);

        if (xPos != std::string::npos && yPos != std::string::npos && zPos != std::string::npos) {
          // Parse baseline coordinates
          sscanf(content.substr(xPos + 4, 20).c_str(), "%f", &baselineX);
          sscanf(content.substr(yPos + 4, 20).c_str(), "%f", &baselineY);
          sscanf(content.substr(zPos + 4, 20).c_str(), "%f", &baselineZ);
        }
      }

      // Extract baseline value
      size_t valuePos = content.find("\"value\":", baselinePos);
      if (valuePos != std::string::npos) {
        sscanf(content.substr(valuePos + 8, 30).c_str(), "%f", &baselineValue);
      }
    }

    std::cout << "Baseline: (" << baselineX << ", " << baselineY << ", " << baselineZ << "), value: " << baselineValue << std::endl;

    // Don't add baseline as a scan point - it's just a reference
    // The measurements are already relative to baseline position

    // Find measurements array
    size_t measurementsPos = content.find("\"measurements\"");
    if (measurementsPos == std::string::npos) {
      std::cerr << "No measurements found in file" << std::endl;
      return false;
    }

    // Parse each measurement with better brace matching
    size_t currentPos = measurementsPos;
    int measurementCount = 0;

    std::cout << "Starting to parse measurements with improved JSON parsing..." << std::endl;

    while (true) {
      // Find next measurement object
      size_t nextMeasurement = content.find("{", currentPos + 1);
      if (nextMeasurement == std::string::npos) break;

      // Find the matching closing brace by counting nested braces
      size_t bracePos = nextMeasurement;
      int braceCount = 0;
      size_t measurementEnd = std::string::npos;

      do {
        if (content[bracePos] == '{') {
          braceCount++;
        }
        else if (content[bracePos] == '}') {
          braceCount--;
          if (braceCount == 0) {
            measurementEnd = bracePos;
            break;
          }
        }
        bracePos++;
      } while (bracePos < content.size());

      if (measurementEnd == std::string::npos) {
        std::cout << "Could not find matching brace for measurement " << measurementCount << std::endl;
        break;
      }

      std::string measurementJson = content.substr(nextMeasurement, measurementEnd - nextMeasurement + 1);

      // Debug: Show raw JSON for first few measurements
      if (measurementCount < 2) {
        std::cout << "=== Complete JSON for measurement " << measurementCount << " ===" << std::endl;
        std::cout << measurementJson.substr(0, 300) << std::endl;
        if (measurementJson.size() > 300) {
          std::cout << "... (total length: " << measurementJson.size() << " chars)" << std::endl;
        }
        std::cout << "=============================================" << std::endl;
      }

      // Parse this measurement
      ScanPoint point;
      float rawX = 0, rawY = 0, rawZ = 0;

      // Extract position
      size_t posStart = measurementJson.find("\"position\"");
      if (posStart != std::string::npos) {
        size_t xPos = measurementJson.find("\"x\":", posStart);
        size_t yPos = measurementJson.find("\"y\":", posStart);
        size_t zPos = measurementJson.find("\"z\":", posStart);

        if (xPos != std::string::npos && yPos != std::string::npos && zPos != std::string::npos) {
          sscanf(measurementJson.substr(xPos + 4, 20).c_str(), "%f", &rawX);
          sscanf(measurementJson.substr(yPos + 4, 20).c_str(), "%f", &rawY);
          sscanf(measurementJson.substr(zPos + 4, 20).c_str(), "%f", &rawZ);
        }
      }

      // Normalize relative to baseline and scale
      point.x = (rawX - baselineX) * scaleFactor;
      point.y = (rawY - baselineY) * scaleFactor;
      point.z = (rawZ - baselineZ) * scaleFactor;

      // Extract value
      size_t valuePos = measurementJson.find("\"value\":");
      if (valuePos != std::string::npos) {
        // Debug: show the string we're trying to parse
        std::string valueStr = measurementJson.substr(valuePos + 8, 30);
        if (measurementCount < 5) {
          std::cout << "  Raw value string [" << measurementCount << "]: '" << valueStr.substr(0, 20) << "'" << std::endl;
        }

        sscanf(measurementJson.substr(valuePos + 8, 30).c_str(), "%f", &point.value);

        // Debug: Print first few values to check parsing
        if (measurementCount < 5) {
          std::cout << "  Parsed value [" << measurementCount << "]: " << point.value << std::endl;
        }

        // Check for suspicious values
        if (point.value < -1000000 || point.value > 1000000) {
          std::cout << "Warning: Suspicious value detected: " << point.value << std::endl;
        }
      }
      else {
        std::cout << "Warning: Could not find 'value' field in measurement " << measurementCount << std::endl;
        point.value = 0.0f; // Default value
      }

      // Extract isPeak
      size_t peakPos = measurementJson.find("\"isPeak\":");
      if (peakPos != std::string::npos) {
        std::string peakStr = measurementJson.substr(peakPos + 9, 5);
        point.isPeak = (peakStr.find("true") != std::string::npos);
      }

      // Extract axis
      size_t axisPos = measurementJson.find("\"axis\":");
      if (axisPos != std::string::npos) {
        size_t axisStart = measurementJson.find("\"", axisPos + 7) + 1;
        size_t axisEnd = measurementJson.find("\"", axisStart);
        if (axisStart != std::string::npos && axisEnd != std::string::npos) {
          point.axis = measurementJson.substr(axisStart, axisEnd - axisStart);
        }
      }

      // Extract direction
      size_t dirPos = measurementJson.find("\"direction\":");
      if (dirPos != std::string::npos) {
        size_t dirStart = measurementJson.find("\"", dirPos + 11) + 1;
        size_t dirEnd = measurementJson.find("\"", dirStart);
        if (dirStart != std::string::npos && dirEnd != std::string::npos) {
          point.direction = measurementJson.substr(dirStart, dirEnd - dirStart);
        }
      }

      scanPoints.push_back(point);

      // Update min/max values with outlier filtering
      // Only include reasonable measurement values (not extreme outliers)
      if (point.value > -1000 && point.value < 1000) {
        minValue = std::min(minValue, point.value);
        maxValue = std::max(maxValue, point.value);
      }
      else {
        std::cout << "Excluding outlier from min/max: " << point.value << std::endl;
      }

      currentPos = measurementEnd;
      measurementCount++;

      // Safety check to prevent infinite loop
      if (measurementCount > 1000) {
        std::cout << "Warning: Stopped parsing at 1000 measurements" << std::endl;
        break;
      }
    }

    // Don't include baseline value in min/max since we're not adding it as a scan point

    std::cout << "Loaded " << scanPoints.size() << " points from " << filePath << std::endl;

    // Try to get better min/max from statistics section if available
    size_t statsPos = content.find("\"statistics\"");
    if (statsPos != std::string::npos) {
      size_t minValuePos = content.find("\"minValue\":", statsPos);
      size_t maxValuePos = content.find("\"maxValue\":", statsPos);

      if (minValuePos != std::string::npos && maxValuePos != std::string::npos) {
        float statsMinValue, statsMaxValue;
        if (sscanf(content.substr(minValuePos + 11, 30).c_str(), "%f", &statsMinValue) == 1 &&
          sscanf(content.substr(maxValuePos + 11, 30).c_str(), "%f", &statsMaxValue) == 1) {

          std::cout << "Using statistics min/max: " << statsMinValue << " to " << statsMaxValue << std::endl;
          std::cout << "Original parsed min/max: " << minValue << " to " << maxValue << std::endl;

          // Use statistics values if they seem reasonable
          if (statsMinValue > -1000 && statsMaxValue < 1000 && statsMaxValue > statsMinValue) {
            minValue = statsMinValue;
            maxValue = statsMaxValue;
            std::cout << "Updated to use statistics values!" << std::endl;
          }
        }
      }
    }

    std::cout << "Final value range: " << minValue << " to " << maxValue << std::endl;

    currentScanFile = filePath;
    return true;
  }
  catch (const std::exception& e) {
    std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    return false;
  }
}

void VerticesLoader::sortFilesByDate(std::vector<std::string>& files) {
  std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
    try {
      auto timeA = std::filesystem::last_write_time(a);
      auto timeB = std::filesystem::last_write_time(b);
      return timeA > timeB; // Newest first
    }
    catch (const std::exception&) {
      return a > b; // Fallback to string comparison
    }
  });
}

bool VerticesLoader::loadMostRecentScan(float scaleFactor) {
  try {
    std::string scanDirectory = "logs/scanning";

    // Check if directory exists
    if (!std::filesystem::exists(scanDirectory)) {
      std::cerr << "Scan directory not found: " << scanDirectory << std::endl;
      return false;
    }

    auto files = findScanFiles(scanDirectory);
    if (files.empty()) {
      std::cerr << "No scan files found in " << scanDirectory << std::endl;
      return false;
    }

    std::string mostRecentFile = getMostRecentFile(files);
    std::cout << "Loading scan file: " << mostRecentFile << std::endl;

    return parseScanFile(mostRecentFile, scaleFactor);
  }
  catch (const std::exception& e) {
    std::cerr << "Error loading scan files: " << e.what() << std::endl;
    return false;
  }
}