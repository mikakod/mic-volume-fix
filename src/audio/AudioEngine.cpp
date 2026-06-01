#include "audio/AudioEngine.hpp"

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

#include <cmath>
#include <sstream>

namespace mvl {

namespace {

constexpr long kPathNotFoundHresult = static_cast<long>(0x80070003);

std::wstring comWideToString(LPCWSTR w) {
  if (!w) {
    return L"";
  }
  return std::wstring(w);
}

}  // namespace

struct AudioEngine::ComInit {
  ComInit() { CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); }
  ~ComInit() { CoUninitialize(); }
};

AudioEngine::AudioEngine() : comInit_(new ComInit()) {}

AudioEngine::~AudioEngine() { delete comInit_; }

bool AudioEngine::isPathNotFoundHresult(long hr) {
  return (static_cast<unsigned long>(hr) & 0xFFFFFFFFu) ==
         static_cast<unsigned long>(kPathNotFoundHresult);
}

std::wstring AudioEngine::explainVolumePathError(const std::wstring& endpointTail) {
  std::wstring suffix;
  if (!endpointTail.empty()) {
    const auto tail = endpointTail.size() > 72 ? endpointTail.substr(endpointTail.size() - 72)
                                               : endpointTail;
    suffix = L" Endpoint id tail: ..." + tail;
  }
  return L"This endpoint is listed under Recording devices, but Core Audio refuses the "
         L"classic volume API on this binding (HRESULT 0x80070003). "
         L"Set the headset as default Recording device in Windows Sound, then choose "
         L"(Windows default microphone), or try another listing."
         + suffix;
}

std::vector<CaptureEndpointBrief> AudioEngine::enumerateCaptureEndpoints() {
  std::vector<CaptureEndpointBrief> rows;
  IMMDeviceEnumerator* enumerator = nullptr;
  if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                              __uuidof(IMMDeviceEnumerator),
                              reinterpret_cast<void**>(&enumerator)))) {
    return rows;
  }

  IMMDeviceCollection* collection = nullptr;
  if (SUCCEEDED(enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection))) {
    UINT count = 0;
    collection->GetCount(&count);
    for (UINT i = 0; i < count; ++i) {
      IMMDevice* device = nullptr;
      if (FAILED(collection->Item(i, &device))) {
        continue;
      }
      LPWSTR id = nullptr;
      std::wstring endpointId;
      if (SUCCEEDED(device->GetId(&id))) {
        endpointId = comWideToString(id);
        CoTaskMemFree(id);
      }
      std::wstring friendly = L"(unknown)";
      IPropertyStore* store = nullptr;
      if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &store))) {
        PROPVARIANT var;
        PropVariantInit(&var);
        if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &var)) && var.vt == VT_LPWSTR) {
          friendly = comWideToString(var.pwszVal);
        }
        PropVariantClear(&var);
        store->Release();
      }
      rows.push_back({endpointId, friendly});
      device->Release();
    }
    collection->Release();
  }
  enumerator->Release();
  return rows;
}

bool AudioEngine::openEndpointVolume(const std::wstring& marker, void** outVolume,
                                       void** outDevice) {
  *outVolume = nullptr;
  *outDevice = nullptr;

  IMMDeviceEnumerator* enumerator = nullptr;
  if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                              __uuidof(IMMDeviceEnumerator),
                              reinterpret_cast<void**>(&enumerator)))) {
    return false;
  }

  IMMDevice* device = nullptr;
  HRESULT hr = E_FAIL;
  if (marker.empty() || marker == kDefaultCaptureMarker) {
    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
  } else {
    hr = enumerator->GetDevice(marker.c_str(), &device);
  }
  enumerator->Release();
  if (FAILED(hr) || !device) {
    return false;
  }

  IAudioEndpointVolume* volume = nullptr;
  hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                        reinterpret_cast<void**>(&volume));
  if (FAILED(hr) || !volume) {
    device->Release();
    return false;
  }

  *outVolume = volume;
  *outDevice = device;
  return true;
}

void AudioEngine::closeEndpointVolume(void* volume, void* device) {
  if (volume) {
    static_cast<IAudioEndpointVolume*>(volume)->Release();
  }
  if (device) {
    static_cast<IMMDevice*>(device)->Release();
  }
}

bool AudioEngine::readVolumeAndMute(const std::wstring& marker, float* outScalar,
                                    bool* outMuted) {
  void* volume = nullptr;
  void* device = nullptr;
  if (!openEndpointVolume(marker, &volume, &device)) {
    return false;
  }
  auto* vol = static_cast<IAudioEndpointVolume*>(volume);
  float scalar = 0.f;
  BOOL muted = FALSE;
  const bool ok =
      SUCCEEDED(vol->GetMasterVolumeLevelScalar(&scalar)) && SUCCEEDED(vol->GetMute(&muted));
  closeEndpointVolume(volume, device);
  if (ok) {
    *outScalar = scalar;
    *outMuted = muted == TRUE;
  }
  return ok;
}

bool AudioEngine::reconcile(const RuntimePrefs& prefs, std::wstring* logLine) {
  const double targetScalar = std::max(0.0, std::min(100.0, prefs.volumePercent)) / 100.0;

  void* volume = nullptr;
  void* device = nullptr;
  if (!openEndpointVolume(prefs.endpointMarker, &volume, &device)) {
    return false;
  }

  auto* vol = static_cast<IAudioEndpointVolume*>(volume);
  bool changed = false;

  if (prefs.lockVolume) {
    float current = 0.f;
    if (SUCCEEDED(vol->GetMasterVolumeLevelScalar(&current))) {
      if (std::fabs(current - static_cast<float>(targetScalar)) > prefs.epsilon) {
        if (SUCCEEDED(vol->SetMasterVolumeLevelScalar(static_cast<float>(targetScalar), nullptr))) {
          changed = true;
        }
      }
    }
  }

  if (prefs.lockMute) {
    BOOL muted = FALSE;
    if (SUCCEEDED(vol->GetMute(&muted))) {
      const BOOL want = prefs.targetMute ? TRUE : FALSE;
      if (muted != want) {
        if (SUCCEEDED(vol->SetMute(want, nullptr))) {
          changed = true;
        }
      }
    }
  }

  closeEndpointVolume(volume, device);
  if (changed && logLine) {
    std::wostringstream oss;
    oss << L"Reconciled volume=" << targetScalar << L" mute=" << (prefs.targetMute ? L"true" : L"false");
    *logLine = oss.str();
  }
  return true;
}

}  // namespace mvl
