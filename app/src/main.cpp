#include "DesktopShell.hpp"

#include <QApplication>
#include <QFontDatabase>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName("CursorOS");
  QApplication::setOrganizationName("CursorOS");

  // Prefer Inter / JetBrains Mono when present on the system.
  QFontDatabase::addApplicationFont("/usr/share/fonts/truetype/inter/Inter-Regular.ttf");
  QFontDatabase::addApplicationFont("/usr/share/fonts/truetype/inter/Inter-Bold.ttf");

  DesktopShell shell;
  shell.show();
  return app.exec();
}
