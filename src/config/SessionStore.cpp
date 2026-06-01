#include "config/SessionStore.hpp"

#include <fstream>

#include <windows.h>
#include <shlobj.h>

namespace mvl {

namespace {

std::filesystem::path localAppDataDir() {
  PWSTR base = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &base))) {
    return {};
  }
  std::filesystem::path dir(base);
  CoTaskMemFree(base);
  return dir / L"mic-volume-fix";
}

}  // namespace

std::filesystem::path SessionStore::sessionFilePath() {
  const auto dir = localAppDataDir();
  if (dir.empty()) {
    return {};
  }
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  return dir / L"mic-volume-fix_gui_session.json";
}

nlohmann::json SessionStore::load() {
  const auto path = sessionFilePath();
  if (path.empty() || !std::filesystem::exists(path)) {
    return nlohmann::json::object();
  }
  try {
    std::ifstream in(path);
    nlohmann::json j;
    in >> j;
    return j.is_object() ? j : nlohmann::json::object();
  } catch (...) {
    return nlohmann::json::object();
  }
}

bool SessionStore::save(const nlohmann::json& blob) {
  const auto path = sessionFilePath();
  if (path.empty()) {
    return false;
  }
  const auto tmp = path.wstring() + L".tmp";
  try {
    std::ofstream out(tmp, std::ios::binary);
    out << blob.dump(2) << '\n';
    out.close();
    std::error_code ec;
    std::filesystem::remove(path, ec);
    std::filesystem::rename(tmp, path, ec);
    return !ec;
  } catch (...) {
    return false;
  }
}

}  // namespace mvl
