#include "win/StartupRegistry.hpp"

#include <windows.h>

#include <algorithm>
#include <sstream>

namespace mvl {

namespace {

constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kValueName[] = L"MicVolumeFixGui";

std::wstring normalizeCmd(std::wstring cmd) {
  cmd.erase(std::remove_if(cmd.begin(), cmd.end(),
                           [](wchar_t c) { return c == L'\r' || c == L'\n'; }),
            cmd.end());
  std::wstring out;
  bool space = false;
  for (wchar_t c : cmd) {
    if (c == L' ') {
      if (!space) {
        out.push_back(c);
        space = true;
      }
    } else {
      out.push_back(static_cast<wchar_t>(towlower(c)));
      space = false;
    }
  }
  return out;
}

std::wstring readRunValue() {
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &key) != ERROR_SUCCESS) {
    return {};
  }
  wchar_t buffer[2048];
  DWORD size = sizeof(buffer);
  DWORD type = 0;
  const LONG rc = RegQueryValueExW(key, kValueName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer),
                                   &size);
  RegCloseKey(key);
  if (rc != ERROR_SUCCESS || type != REG_SZ) {
    return {};
  }
  return std::wstring(buffer);
}

}  // namespace

std::wstring StartupRegistry::composeLaunchCommand() {
  wchar_t path[MAX_PATH]{};
  const DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) {
    return {};
  }
  std::wostringstream oss;
  oss << L'"' << path << L'"';
  return oss.str();
}

bool StartupRegistry::isRegistered() {
  const auto want = composeLaunchCommand();
  const auto have = readRunValue();
  if (want.empty() || have.empty()) {
    return false;
  }
  return normalizeCmd(want) == normalizeCmd(have);
}

bool StartupRegistry::setEnabled(bool enabled) {
  if (!enabled) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
      return false;
    }
    const LONG rc = RegDeleteValueW(key, kValueName);
    RegCloseKey(key);
    return rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND;
  }

  const auto cmd = composeLaunchCommand();
  if (cmd.empty()) {
    return false;
  }

  HKEY key = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key,
                      nullptr) != ERROR_SUCCESS) {
    return false;
  }
  const LONG rc =
      RegSetValueExW(key, kValueName, 0, REG_SZ,
                     reinterpret_cast<const BYTE*>(cmd.c_str()),
                     static_cast<DWORD>((cmd.size() + 1) * sizeof(wchar_t)));
  RegCloseKey(key);
  return rc == ERROR_SUCCESS;
}

}  // namespace mvl
