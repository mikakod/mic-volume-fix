#pragma once

#include <optional>

#include <QMainWindow>
#include <QTimer>

#include <nlohmann/json.hpp>

#include "audio/AudioEngine.hpp"
#include "config/LockConfig.hpp"
#include "guardian/GuardianWorker.hpp"
#include "gui/EnumeratorWorker.hpp"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QTextEdit;

namespace mvl {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

 protected:
  void closeEvent(QCloseEvent* event) override;

 private slots:
  void scanDevices();
  void onDevicesReady(const QStringList& ids, const QStringList& names);
  void onDevicesError(const QString& message);
  void armGuardian();
  void disarmGuardian();
  void onGuardianLog(const QString& line);
  void onTelemetry(double percent, bool muted);
  void syncSessionControls();
  void onRunAtStartupToggled(bool checked);
  void maybeOfferStartupOnce();
  void importConfig();
  void exportConfig();

 private:
  void pushPrefs();
  RuntimePrefs snapshotPrefs() const;
  void restoreCheckboxFlags();
  void persistSession();
  void finishStartupAfterEnum(const QStringList& ids, const QStringList& names, bool configApplied);
  void applyHardDefaults();
  void applyGuiSnapshot(const nlohmann::json& snap, const QStringList& ids);
  int comboIndexForMarker(const QString& marker, const QStringList& ids) const;
  void refreshStartupCheckbox();

  QComboBox* micCombo_ = nullptr;
  QPushButton* refreshBtn_ = nullptr;
  QSlider* levelSlider_ = nullptr;
  QLabel* levelLabel_ = nullptr;
  QCheckBox* lockVolume_ = nullptr;
  QCheckBox* lockMute_ = nullptr;
  QComboBox* muteTarget_ = nullptr;
  QSpinBox* pollSpin_ = nullptr;
  QDoubleSpinBox* epsilonSpin_ = nullptr;

  QCheckBox* saveOnExit_ = nullptr;
  QCheckBox* autoStartGuardian_ = nullptr;
  QCheckBox* rememberMicOnly_ = nullptr;
  QCheckBox* runAtStartup_ = nullptr;

  QPushButton* startBtn_ = nullptr;
  QPushButton* stopBtn_ = nullptr;
  QPushButton* importBtn_ = nullptr;
  QPushButton* exportBtn_ = nullptr;
  QLabel* liveLabel_ = nullptr;
  QTextEdit* logView_ = nullptr;

  nlohmann::json session_;
  std::optional<LockConfig> pendingConfig_;
  bool startupRoundDone_ = false;
  GuardianWorker* guardian_ = nullptr;
  EnumeratorWorker* enumWorker_ = nullptr;
  QTimer startupOfferTimer_;
};

}  // namespace mvl
