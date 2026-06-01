#pragma once

#include <filesystem>
#include <optional>

#include <nlohmann/json.hpp>

namespace mvl {

class SessionStore {
 public:
  static std::filesystem::path sessionFilePath();
  static nlohmann::json load();
  static bool save(const nlohmann::json& blob);
};

}  // namespace mvl
