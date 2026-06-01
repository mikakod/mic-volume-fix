#pragma once

#include <string>

namespace mvl {

class StartupRegistry {
 public:
  static std::wstring composeLaunchCommand();
  static bool isRegistered();
  static bool setEnabled(bool enabled);
};

}  // namespace mvl
