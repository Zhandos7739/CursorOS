#pragma once

#include <QHash>
#include <QMainWindow>
#include <QString>

class FloatingWindow;
class QLabel;
class QTimer;
class QWidget;

class DesktopShell : public QMainWindow {
  Q_OBJECT
 public:
  explicit DesktopShell(QWidget* parent = nullptr);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  void buildChrome();
  void layoutChrome();
  void openApp(const QString& id);
  QWidget* makeTerminal();
  QWidget* makeFiles();
  QWidget* makeAbout();
  void tickClock();

  QWidget* panel_ = nullptr;
  QWidget* dock_ = nullptr;
  QLabel* brandMark_ = nullptr;
  QLabel* clockLabel_ = nullptr;
  QLabel* heroBrand_ = nullptr;
  QLabel* heroLine_ = nullptr;
  QTimer* clockTimer_ = nullptr;
  QHash<QString, FloatingWindow*> windows_;
};
