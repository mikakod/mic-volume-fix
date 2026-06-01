#pragma once

#include <string>
#include <vector>

namespace mvl {

inline constexpr wchar_t kDefaultCaptureMarker[] = L"::windows_default_capture::";

struct CaptureEndpointBrief {
  std::wstring endpointId;
  std::wstring friendlyName;
};

struct RuntimePrefs {
  std::wstring endpointMarker = kDefaultCaptureMarker;
  bool lockVolume = true;
  double volumePercent = 100.0;
  double epsilon = 0.01;
  bool lockMute = false;
  bool targetMute = false;
  int pollIntervalMs = 50;
};

class AudioEngine {
 public:
  AudioEngine();
  ~AudioEngine();

  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  std::vector<CaptureEndpointBrief> enumerateCaptureEndpoints();

  bool readVolumeAndMute(const std::wstring& marker, float* outScalar, bool* outMuted);

  bool reconcile(const RuntimePrefs& prefs, std::wstring* logLine);

  static bool isPathNotFoundHresult(long hr);
  static std::wstring explainVolumePathError(const std::wstring& endpointTail);

 private:
  struct ComInit;
  ComInit* comInit_;

  bool openEndpointVolume(const std::wstring& marker, void** outVolume, void** outDevice);
  void closeEndpointVolume(void* volume, void* device);
};

}  // namespace mvl
