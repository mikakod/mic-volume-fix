#include "gui/EnumeratorWorker.hpp"

namespace mvl {

EnumeratorWorker::EnumeratorWorker(QObject* parent) : QThread(parent) {}

void EnumeratorWorker::run() {
  try {
    const auto rows = engine_.enumerateCaptureEndpoints();
    QStringList ids;
    QStringList names;
    for (const auto& row : rows) {
      ids << QString::fromStdWString(row.endpointId);
      names << QString::fromStdWString(row.friendlyName);
    }
    emit finishedOk(ids, names);
  } catch (...) {
    emit finishedError(QStringLiteral("Device enumeration failed."));
  }
}

}  // namespace mvl
