#pragma once

#include <vector>

#include <QThread>

#include "audio/AudioEngine.hpp"

namespace mvl {

class EnumeratorWorker : public QThread {
  Q_OBJECT

 public:
  explicit EnumeratorWorker(QObject* parent = nullptr);

 signals:
  void finishedOk(const QStringList& endpointIds, const QStringList& friendlyNames);
  void finishedError(const QString& message);

 protected:
  void run() override;

 private:
  AudioEngine engine_;
};

}  // namespace mvl
