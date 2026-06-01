#include "config/LockConfig.hpp"

#include <algorithm>
#include <fstream>

#include <windows.h>

#include <nlohmann/json.hpp>

namespace mvl {

bool saveLockConfig(const LockConfig& cfg, const std::filesystem::path& path,
                    std::wstring* error);

namespace {

std::wstring utf8ToWide(const std::string& s) {
  if (s.empty()) {
    return L"";
  }
  const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
  if (len <= 0) {
    return L"";
  }
  std::wstring out(static_cast<size_t>(len - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
  return out;
}

std::string wideToUtf8(const std::wstring& w) {
  if (w.empty()) {
    return {};
  }
  const int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if (len <= 0) {
    return {};
  }
  std::string out(static_cast<size_t>(len - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), len, nullptr, nullptr);
  return out;
}

LockConfig fromJson(const nlohmann::json& j, const std::filesystem::path& source) {
  LockConfig cfg;
  if (j.contains("device_id_contains")) {
    cfg.deviceIdContains = utf8ToWide(j.at("device_id_contains").get<std::string>());
  }
  if (j.contains("device_id_exact")) {
    cfg.deviceIdExact = utf8ToWide(j.at("device_id_exact").get<std::string>());
  }
  if (j.contains("use_default_capture")) {
    cfg.useDefaultCapture = j.at("use_default_capture").get<bool>();
  }
  if (j.contains("target_volume_percent")) {
    cfg.targetVolumePercent = j.at("target_volume_percent").get<double>();
  }
  if (j.contains("target_mute")) {
    cfg.targetMute = j.at("target_mute").get<bool>();
  }
  if (j.contains("poll_interval_ms")) {
    cfg.pollIntervalMs = j.at("poll_interval_ms").get<int>();
  }
  if (j.contains("volume_match_epsilon")) {
    cfg.volumeMatchEpsilon = j.at("volume_match_epsilon").get<double>();
  }
  if (j.contains("log_level")) {
    cfg.logLevel = utf8ToWide(j.at("log_level").get<std::string>());
  }
  if (j.contains("log_file")) {
    cfg.logFile = utf8ToWide(j.at("log_file").get<std::string>());
  }
  if (j.contains("tray_icon")) {
    cfg.trayIcon = j.at("tray_icon").get<bool>();
  }
  cfg.sourcedFrom = source;
  return cfg;
}

}  // namespace

double LockConfig::targetVolumeScalar() const {
  double v = targetVolumePercent;
  if (v > 1.0) {
    v /= 100.0;
  }
  return std::max(0.0, std::min(1.0, v));
}

bool LockConfig::validate(std::wstring* error) const {
  if (pollIntervalMs < 10) {
    if (error) {
      *error = L"poll_interval_ms must be >= 10";
    }
    return false;
  }
  const double scalar = targetVolumeScalar();
  if (scalar < 0.0 || scalar > 1.0) {
    if (error) {
      *error = L"target_volume_percent out of range";
    }
    return false;
  }
  int selectors = 0;
  if (!deviceIdContains.empty()) {
    ++selectors;
  }
  if (!deviceIdExact.empty()) {
    ++selectors;
  }
  if (useDefaultCapture) {
    ++selectors;
  }
  if (selectors != 1) {
    if (error) {
      *error = L"Pick exactly one of: device_id_contains, device_id_exact, use_default_capture";
    }
    return false;
  }
  return true;
}

std::optional<LockConfig> LockConfig::loadFromFile(const std::filesystem::path& path,
                                                   std::wstring* error) {
  std::ifstream in(path);
  if (!in) {
    if (error) {
      *error = L"Cannot open config: " + path.wstring();
    }
    return std::nullopt;
  }
  try {
    nlohmann::json j;
    in >> j;
    auto cfg = fromJson(j, path);
    if (!cfg.validate(error)) {
      return std::nullopt;
    }
    return cfg;
  } catch (const std::exception& ex) {
    if (error) {
      *error = utf8ToWide(ex.what());
    }
    return std::nullopt;
  }
}

std::optional<LockConfig> LockConfig::discover(std::wstring* error) {
  const std::filesystem::path p = std::filesystem::current_path() / L"config.json";
  if (std::filesystem::exists(p)) {
    return loadFromFile(p, error);
  }
  if (error) {
    *error = L"No config.json found in current directory";
  }
  return std::nullopt;
}

nlohmann::json toJson(const LockConfig& cfg) {
  nlohmann::json j;
  j["device_id_contains"] = wideToUtf8(cfg.deviceIdContains);
  j["device_id_exact"] = wideToUtf8(cfg.deviceIdExact);
  j["use_default_capture"] = cfg.useDefaultCapture;
  j["target_volume_percent"] = cfg.targetVolumePercent;
  j["target_mute"] = cfg.targetMute;
  j["poll_interval_ms"] = cfg.pollIntervalMs;
  j["volume_match_epsilon"] = cfg.volumeMatchEpsilon;
  j["log_level"] = wideToUtf8(cfg.logLevel);
  j["log_file"] = wideToUtf8(cfg.logFile);
  j["tray_icon"] = cfg.trayIcon;
  return j;
}

bool saveLockConfig(const LockConfig& cfg, const std::filesystem::path& path,
                    std::wstring* error) {
  try {
    std::ofstream out(path);
    out << toJson(cfg).dump(2) << '\n';
    return true;
  } catch (const std::exception& ex) {
    if (error) {
      *error = utf8ToWide(ex.what());
    }
    return false;
  }
}

}  // namespace mvl
