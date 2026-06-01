#include "guardian/GuardianWorker.hpp"

#include <chrono>
#include <thread>

namespace mvl {

GuardianWorker::GuardianWorker(PrefsCallback prefs, QObject* parent)
    : QThread(parent), prefs_(std::move(prefs)) {}

void GuardianWorker::requestStop() { stop_.store(true); }

void GuardianWorker::run() {
  running_.store(true);
  stop_.store(false);
  boundMarker_.clear();
  volumeFallbackActive_ = false;

  while (!stop_.load()) {
    const RuntimePrefs prefs = prefs_ ? prefs_() : RuntimePrefs{};
    const int pollMs = std::max(10, prefs.pollIntervalMs);
    const auto sleepDur = std::chrono::milliseconds(pollMs);

    std::wstring openMarker = prefs.endpointMarker;
    if (volumeFallbackActive_ && openMarker == boundMarker_) {
      openMarker = kDefaultCaptureMarker;
    }

    float scalar = 0.f;
    bool muted = false;
    if (engine_.readVolumeAndMute(openMarker, &scalar, &muted)) {
      emit telemetry(static_cast<double>(scalar) * 100.0, muted);
      boundMarker_ = openMarker;

      RuntimePrefs active = prefs;
      active.endpointMarker = openMarker;
      std::wstring reconcileMessage;
      if (!engine_.reconcile(active, &reconcileMessage)) {
        // reconcile failed silently; loop continues
      } else if (!reconcileMessage.empty()) {
        emit logLine(QString::fromStdWString(reconcileMessage));
      }
    } else {
      if (!volumeFallbackActive_ && openMarker != kDefaultCaptureMarker) {
        volumeFallbackActive_ = true;
        emit logLine(
            QStringLiteral(
                "Endpoint rejects volume API (0x80070003); retrying via Windows default mic."));
      }
      boundMarker_.clear();
      std::this_thread::sleep_for(std::chrono::milliseconds(350));
    }

    for (int elapsed = 0; elapsed < pollMs && !stop_.load(); elapsed += 10) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  running_.store(false);
}

}  // namespace mvl
