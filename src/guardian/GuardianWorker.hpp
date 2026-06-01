#pragma once

#include <atomic>
#include <functional>
#include <mutex>

#include <QThread>

#include "audio/AudioEngine.hpp"

namespace mvl {

class GuardianWorker : public QThread {
  Q_OBJECT

 public:
  using PrefsCallback = std::function<RuntimePrefs()>;
  using LogCallback = std::function<void(const QString&)>;
  using TelemetryCallback = std::function<void(double percent, bool muted)>;

  explicit GuardianWorker(PrefsCallback prefs, QObject* parent = nullptr);

  void requestStop();
  bool isRunningGuardian() const { return running_.load(); }

 signals:
  void logLine(const QString& line);
  void telemetry(double percent, bool muted);

 protected:
  void run() override;

 private:
  PrefsCallback prefs_;
  std::atomic<bool> stop_{false};
  std::atomic<bool> running_{false};

  AudioEngine engine_;
  std::wstring boundMarker_;
  bool volumeFallbackActive_ = false;
};

}  // namespace mvl
