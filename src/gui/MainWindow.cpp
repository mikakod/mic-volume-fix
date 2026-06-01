#include "gui/MainWindow.hpp"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

#include "config/SessionStore.hpp"
#include "win/StartupRegistry.hpp"

namespace mvl {

namespace {

QString defaultMarkerQString() {
  return QString::fromStdWString(kDefaultCaptureMarker);
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("Mic Volume Fix"));
  resize(520, 500);

  session_ = SessionStore::load();

  micCombo_ = new QComboBox(this);
  micCombo_->setMinimumWidth(300);
  refreshBtn_ = new QPushButton(QStringLiteral("Refresh mics"), this);

  levelSlider_ = new QSlider(Qt::Horizontal, this);
  levelSlider_->setRange(0, 100);
  levelSlider_->setValue(100);
  levelLabel_ = new QLabel(QStringLiteral("100%"), this);

  lockVolume_ = new QCheckBox(QStringLiteral("Lock Windows capture volume level"), this);
  lockVolume_->setChecked(true);
  lockMute_ = new QCheckBox(QStringLiteral("Lock Windows capture mute state"), this);
  muteTarget_ = new QComboBox(this);
  muteTarget_->addItem(QStringLiteral("Force unmuted in Windows"), false);
  muteTarget_->addItem(QStringLiteral("Force muted in Windows"), true);
  muteTarget_->setEnabled(false);

  pollSpin_ = new QSpinBox(this);
  pollSpin_->setRange(10, 2500);
  pollSpin_->setValue(50);
  pollSpin_->setSuffix(QStringLiteral(" ms"));

  epsilonSpin_ = new QDoubleSpinBox(this);
  epsilonSpin_->setDecimals(4);
  epsilonSpin_->setRange(0.0005, 0.35);
  epsilonSpin_->setSingleStep(0.005);
  epsilonSpin_->setValue(0.01);

  saveOnExit_ = new QCheckBox(QStringLiteral("Remember everything for next launch"), this);
  autoStartGuardian_ =
      new QCheckBox(QStringLiteral("Auto start guardian upon program launch"), this);
  rememberMicOnly_ =
      new QCheckBox(QStringLiteral("Only remember my microphone (everything else resets)"), this);
  runAtStartup_ = new QCheckBox(QStringLiteral("Run at Windows startup (current user)"), this);

  startBtn_ = new QPushButton(QStringLiteral("Start guardian"), this);
  stopBtn_ = new QPushButton(QStringLiteral("Stop guardian"), this);
  stopBtn_->setEnabled(false);
  importBtn_ = new QPushButton(QStringLiteral("Import config JSON..."), this);
  exportBtn_ = new QPushButton(QStringLiteral("Export config JSON..."), this);

  liveLabel_ = new QLabel(QStringLiteral("Live telemetry appears after Start"), this);
  logView_ = new QTextEdit(this);
  logView_->setReadOnly(true);

  auto* micRow = new QHBoxLayout;
  micRow->addWidget(micCombo_);
  micRow->addWidget(refreshBtn_);

  auto* levelRow = new QHBoxLayout;
  levelRow->addWidget(levelSlider_);
  levelRow->addWidget(levelLabel_);

  auto* muteRow = new QHBoxLayout;
  muteRow->addWidget(new QLabel(QStringLiteral("Mute target"), this));
  muteRow->addWidget(muteTarget_);

  auto* form = new QFormLayout;
  form->addRow(QStringLiteral("Mic:"), micRow);
  form->addRow(QStringLiteral("Level:"), levelRow);
  form->addRow(lockVolume_);
  form->addRow(lockMute_);
  form->addRow(muteRow);
  form->addRow(QStringLiteral("Poll ms:"), pollSpin_);
  form->addRow(QStringLiteral("Vol epsilon:"), epsilonSpin_);

  auto* guardianGroup = new QGroupBox(QStringLiteral("Guardian settings"), this);
  guardianGroup->setLayout(form);

  auto* persistLayout = new QVBoxLayout;
  persistLayout->addWidget(saveOnExit_);
  persistLayout->addWidget(autoStartGuardian_);
  persistLayout->addWidget(rememberMicOnly_);
  persistLayout->addWidget(runAtStartup_);
  auto* persistGroup = new QGroupBox(QStringLiteral("Persistency"), this);
  persistGroup->setLayout(persistLayout);

  auto* goRow = new QHBoxLayout;
  goRow->addWidget(startBtn_);
  goRow->addWidget(stopBtn_);
  auto* ioRow = new QHBoxLayout;
  ioRow->addWidget(importBtn_);
  ioRow->addWidget(exportBtn_);

  auto* root = new QWidget(this);
  auto* col = new QVBoxLayout(root);
  col->addWidget(guardianGroup);
  col->addWidget(persistGroup);
  col->addLayout(goRow);
  col->addLayout(ioRow);
  col->addWidget(liveLabel_);
  col->addWidget(new QLabel(QStringLiteral("Log"), this));
  col->addWidget(logView_);
  setCentralWidget(root);

  connect(refreshBtn_, &QPushButton::clicked, this, &MainWindow::scanDevices);
  connect(startBtn_, &QPushButton::clicked, this, &MainWindow::armGuardian);
  connect(stopBtn_, &QPushButton::clicked, this, &MainWindow::disarmGuardian);
  connect(importBtn_, &QPushButton::clicked, this, &MainWindow::importConfig);
  connect(exportBtn_, &QPushButton::clicked, this, &MainWindow::exportConfig);
  connect(levelSlider_, &QSlider::valueChanged, this, [this](int) { pushPrefs(); });
  connect(lockVolume_, &QCheckBox::toggled, this, [this](bool) { pushPrefs(); });
  connect(lockMute_, &QCheckBox::toggled, this, [this](bool on) {
    muteTarget_->setEnabled(on);
    pushPrefs();
  });
  connect(muteTarget_, &QComboBox::currentIndexChanged, this, [this](int) { pushPrefs(); });
  connect(pollSpin_, &QSpinBox::valueChanged, this, [this](int) { pushPrefs(); });
  connect(epsilonSpin_, &QDoubleSpinBox::valueChanged, this, [this](double) { pushPrefs(); });
  connect(micCombo_, &QComboBox::currentIndexChanged, this, [this](int) { pushPrefs(); });
  connect(saveOnExit_, &QCheckBox::toggled, this, &MainWindow::syncSessionControls);
  connect(runAtStartup_, &QCheckBox::toggled, this, &MainWindow::onRunAtStartupToggled);

  micCombo_->addItem(QStringLiteral("(Windows default microphone)"), defaultMarkerQString());

  restoreCheckboxFlags();
  refreshStartupCheckbox();

  std::wstring cfgErr;
  if (auto cfg = LockConfig::discover(&cfgErr)) {
    pendingConfig_ = *cfg;
    logView_->append(QStringLiteral("Queued config from disk; applying after mic scan..."));
  }

  pushPrefs();
  scanDevices();

  connect(&startupOfferTimer_, &QTimer::timeout, this, &MainWindow::maybeOfferStartupOnce);
  startupOfferTimer_.setSingleShot(true);
  startupOfferTimer_.start(850);
}

MainWindow::~MainWindow() { disarmGuardian(); }

void MainWindow::closeEvent(QCloseEvent* event) {
  disarmGuardian();
  persistSession();
  QMainWindow::closeEvent(event);
}

void MainWindow::restoreCheckboxFlags() {
  saveOnExit_->setChecked(session_.value("save_on_exit", false));
  autoStartGuardian_->setChecked(session_.value("start_guardian_on_launch", false));
  rememberMicOnly_->setChecked(session_.value("remember_mic_without_full_save", false));
  syncSessionControls();
}

void MainWindow::syncSessionControls() {
  const bool saving = saveOnExit_->isChecked();
  autoStartGuardian_->setEnabled(saving);
  rememberMicOnly_->setEnabled(!saving);
  if (saving) {
    rememberMicOnly_->setChecked(false);
  } else {
    autoStartGuardian_->setChecked(false);
  }
}

void MainWindow::refreshStartupCheckbox() {
  runAtStartup_->blockSignals(true);
  runAtStartup_->setChecked(StartupRegistry::isRegistered());
  runAtStartup_->blockSignals(false);
}

void MainWindow::onRunAtStartupToggled(bool checked) {
  if (!StartupRegistry::setEnabled(checked)) {
    QMessageBox::warning(this, QStringLiteral("Mic Volume Fix"),
                         QStringLiteral("Could not update Windows startup entry."));
    refreshStartupCheckbox();
    return;
  }
  logView_->append(checked ? QStringLiteral("Registered for Windows startup (current user).")
                           : QStringLiteral("Removed from Windows startup (current user)."));
  refreshStartupCheckbox();
}

void MainWindow::maybeOfferStartupOnce() {
  if (session_.value("startup_offer_shown", false)) {
    return;
  }
  if (StartupRegistry::isRegistered()) {
    session_["startup_offer_shown"] = true;
    persistSession();
    return;
  }
  const auto answer = QMessageBox::question(
      this, QStringLiteral("Mic Volume Fix — startup"),
      QStringLiteral(
          "Also launch Mic Volume Fix automatically when Windows starts?\n\n"
          "You can change this anytime under Persistency (Run at Windows startup)."),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (answer == QMessageBox::Yes) {
    StartupRegistry::setEnabled(true);
    logView_->append(QStringLiteral("Registered for Windows startup (current user)."));
  }
  session_["startup_offer_shown"] = true;
  persistSession();
  refreshStartupCheckbox();
}

void MainWindow::scanDevices() {
  if (enumWorker_ && enumWorker_->isRunning()) {
    return;
  }
  enumWorker_ = new EnumeratorWorker(this);
  connect(enumWorker_, &EnumeratorWorker::finishedOk, this, &MainWindow::onDevicesReady);
  connect(enumWorker_, &EnumeratorWorker::finishedError, this, &MainWindow::onDevicesError);
  connect(enumWorker_, &QThread::finished, enumWorker_, &QObject::deleteLater);
  enumWorker_->start();
}

void MainWindow::onDevicesError(const QString& message) {
  logView_->append(message);
}

void MainWindow::onDevicesReady(const QStringList& ids, const QStringList& names) {
  const QString current = micCombo_->currentData().toString();
  micCombo_->blockSignals(true);
  micCombo_->clear();
  micCombo_->addItem(QStringLiteral("(Windows default microphone)"), defaultMarkerQString());
  for (int i = 0; i < ids.size(); ++i) {
    const QString label =
        names.value(i, QStringLiteral("(unknown)")) + QStringLiteral(" — ") + ids.value(i).left(48);
    micCombo_->addItem(label, ids.at(i));
  }
  const int idx = comboIndexForMarker(current, ids);
  micCombo_->setCurrentIndex(idx >= 0 ? idx + 1 : 0);
  micCombo_->blockSignals(false);

  const bool configApplied = pendingConfig_.has_value();
  if (pendingConfig_) {
    const auto& cfg = *pendingConfig_;
    if (cfg.useDefaultCapture) {
      micCombo_->setCurrentIndex(0);
    } else if (!cfg.deviceIdExact.empty()) {
      const QString exact = QString::fromStdWString(cfg.deviceIdExact);
      const int j = micCombo_->findData(exact);
      if (j >= 0) {
        micCombo_->setCurrentIndex(j);
      }
    }
    levelSlider_->setValue(static_cast<int>(cfg.targetVolumePercent));
    muteTarget_->setCurrentIndex(cfg.targetMute ? 1 : 0);
    pollSpin_->setValue(cfg.pollIntervalMs);
    epsilonSpin_->setValue(cfg.volumeMatchEpsilon);
    pendingConfig_.reset();
    logView_->append(QStringLiteral("Applied config.json overrides."));
  }

  finishStartupAfterEnum(ids, names, configApplied);
  pushPrefs();
}

int MainWindow::comboIndexForMarker(const QString& marker, const QStringList& ids) const {
  if (marker == defaultMarkerQString()) {
    return -1;
  }
  return ids.indexOf(marker);
}

void MainWindow::applyHardDefaults() {
  micCombo_->setCurrentIndex(0);
  levelSlider_->setValue(100);
  lockVolume_->setChecked(true);
  lockMute_->setChecked(false);
  muteTarget_->setCurrentIndex(0);
  muteTarget_->setEnabled(false);
  pollSpin_->setValue(50);
  epsilonSpin_->setValue(0.01);
}

void MainWindow::applyGuiSnapshot(const nlohmann::json& snap, const QStringList& ids) {
  const QString marker = QString::fromStdString(snap.value("endpoint_marker", std::string()));
  int pct = snap.value("target_volume_percent", 100);
  if (pct <= 1) {
    pct *= 100;
  }
  levelSlider_->setValue(std::max(0, std::min(100, pct)));
  lockVolume_->setChecked(snap.value("lock_volume", true));
  lockMute_->setChecked(snap.value("lock_mute", false));
  muteTarget_->setEnabled(lockMute_->isChecked());
  muteTarget_->setCurrentIndex(snap.value("target_mute", false) ? 1 : 0);
  pollSpin_->setValue(std::max(10, std::min(2500, snap.value("poll_interval_ms", 50))));
  epsilonSpin_->setValue(snap.value("volume_match_epsilon", 0.01));

  const int idx = comboIndexForMarker(marker, ids);
  micCombo_->setCurrentIndex(idx >= 0 ? idx + 1 : 0);
}

void MainWindow::finishStartupAfterEnum(const QStringList& ids, const QStringList& /*names*/,
                                        bool configApplied) {
  if (startupRoundDone_) {
    return;
  }
  startupRoundDone_ = true;

  if (!configApplied) {
    if (saveOnExit_->isChecked() && session_.contains("gui_snapshot")) {
      applyGuiSnapshot(session_.at("gui_snapshot"), ids);
    } else if (rememberMicOnly_->isChecked()) {
      const QString marker =
          QString::fromStdString(session_.value("remembered_endpoint_marker", std::string()));
      applyHardDefaults();
      const int idx = comboIndexForMarker(marker, ids);
      if (idx >= 0) {
        micCombo_->setCurrentIndex(idx + 1);
      }
    } else {
      applyHardDefaults();
    }
  }

  if (saveOnExit_->isChecked() && autoStartGuardian_->isChecked()) {
    QTimer::singleShot(250, this, &MainWindow::armGuardian);
  }
}

RuntimePrefs MainWindow::snapshotPrefs() const {
  RuntimePrefs p;
  p.endpointMarker = micCombo_->currentData().toString().toStdWString();
  p.lockVolume = lockVolume_->isChecked();
  p.volumePercent = levelSlider_->value();
  p.epsilon = epsilonSpin_->value();
  p.lockMute = lockMute_->isChecked();
  p.targetMute = muteTarget_->currentData().toBool();
  p.pollIntervalMs = pollSpin_->value();
  return p;
}

void MainWindow::pushPrefs() {
  levelLabel_->setText(QString::number(levelSlider_->value()) + QStringLiteral("%"));
}

void MainWindow::armGuardian() {
  if (guardian_ && guardian_->isRunning()) {
    logView_->append(QStringLiteral("Guardian already running."));
    return;
  }
  disarmGuardian();
  guardian_ = new GuardianWorker([this]() { return snapshotPrefs(); }, this);
  connect(guardian_, &GuardianWorker::logLine, this, &MainWindow::onGuardianLog);
  connect(guardian_, &GuardianWorker::telemetry, this, &MainWindow::onTelemetry);
  connect(guardian_, &QThread::finished, this, [this]() {
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
  });
  guardian_->start();
  startBtn_->setEnabled(false);
  stopBtn_->setEnabled(true);
  logView_->append(QStringLiteral("Guardian started."));
}

void MainWindow::disarmGuardian() {
  if (!guardian_) {
    return;
  }
  guardian_->requestStop();
  guardian_->wait(5000);
  guardian_->deleteLater();
  guardian_ = nullptr;
  startBtn_->setEnabled(true);
  stopBtn_->setEnabled(false);
  logView_->append(QStringLiteral("Guardian stopped."));
}

void MainWindow::onGuardianLog(const QString& line) { logView_->append(line); }

void MainWindow::onTelemetry(double percent, bool muted) {
  liveLabel_->setText(QStringLiteral("Live level: %1% mute=%2")
                          .arg(percent, 0, 'f', 1)
                          .arg(muted ? QStringLiteral("true") : QStringLiteral("false")));
}

void MainWindow::persistSession() {
  nlohmann::json blob;
  blob["schema"] = 1;
  blob["save_on_exit"] = saveOnExit_->isChecked();
  blob["start_guardian_on_launch"] = saveOnExit_->isChecked() && autoStartGuardian_->isChecked();
  blob["remember_mic_without_full_save"] = rememberMicOnly_->isChecked();
  blob["startup_offer_shown"] = session_.value("startup_offer_shown", false);

  if (saveOnExit_->isChecked()) {
    nlohmann::json snap;
    snap["schema"] = 1;
    snap["endpoint_marker"] = micCombo_->currentData().toString().toStdString();
    snap["target_volume_percent"] = levelSlider_->value();
    snap["lock_volume"] = lockVolume_->isChecked();
    snap["lock_mute"] = lockMute_->isChecked();
    snap["target_mute"] = muteTarget_->currentData().toBool();
    snap["poll_interval_ms"] = pollSpin_->value();
    snap["volume_match_epsilon"] = epsilonSpin_->value();
    blob["gui_snapshot"] = snap;
  } else if (rememberMicOnly_->isChecked()) {
    blob["remembered_endpoint_marker"] = micCombo_->currentData().toString().toStdString();
  } else {
    blob["remembered_endpoint_marker"] = "";
  }

  SessionStore::save(blob);
  session_ = blob;
}

void MainWindow::importConfig() {
  const QString path =
      QFileDialog::getOpenFileName(this, QStringLiteral("Import config JSON"), QString(),
                                   QStringLiteral("JSON (*.json)"));
  if (path.isEmpty()) {
    return;
  }
  std::wstring err;
  auto cfg = LockConfig::loadFromFile(path.toStdWString(), &err);
  if (!cfg) {
    logView_->append(QStringLiteral("Import failed: ") + QString::fromStdWString(err));
    return;
  }
  pendingConfig_ = *cfg;
  scanDevices();
}

void MainWindow::exportConfig() {
  const QString path =
      QFileDialog::getSaveFileName(this, QStringLiteral("Export config JSON"), QString(),
                                   QStringLiteral("JSON (*.json)"));
  if (path.isEmpty()) {
    return;
  }
  LockConfig cfg;
  const auto marker = micCombo_->currentData().toString();
  if (marker == defaultMarkerQString()) {
    cfg.useDefaultCapture = true;
  } else {
    cfg.deviceIdExact = marker.toStdWString();
  }
  cfg.targetVolumePercent = levelSlider_->value();
  cfg.targetMute = muteTarget_->currentData().toBool();
  cfg.pollIntervalMs = pollSpin_->value();
  cfg.volumeMatchEpsilon = epsilonSpin_->value();

  std::wstring err;
  if (!saveLockConfig(cfg, path.toStdWString(), &err)) {
    logView_->append(QStringLiteral("Export failed: ") + QString::fromStdWString(err));
    return;
  }
  logView_->append(QStringLiteral("Wrote config to ") + path);
}

}  // namespace mvl
