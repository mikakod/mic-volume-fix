#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace mvl {

struct LockConfig {
  std::wstring deviceIdContains;
  std::wstring deviceIdExact;
  bool useDefaultCapture = false;

  double targetVolumePercent = 100.0;
  bool targetMute = false;

  int pollIntervalMs = 50;
  double volumeMatchEpsilon = 0.01;

  std::wstring logLevel = L"INFO";
  std::wstring logFile;

  bool trayIcon = false;

  std::filesystem::path sourcedFrom;

  bool validate(std::wstring* error) const;
  double targetVolumeScalar() const;

  static std::optional<LockConfig> loadFromFile(const std::filesystem::path& path,
                                                std::wstring* error);
  static std::optional<LockConfig> discover(std::wstring* error);
};

bool saveLockConfig(const LockConfig& cfg, const std::filesystem::path& path,
                    std::wstring* error = nullptr);

}  // namespace mvl
