#include <QApplication>

#include "gui/MainWindow.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("Mic Volume Fix"));
  app.setOrganizationName(QStringLiteral("MicVolumeFix"));

  mvl::MainWindow window;
  window.show();
  return app.exec();
}
