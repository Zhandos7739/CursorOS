#pragma once

#include <QFrame>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

class FloatingWindow : public QFrame {
  Q_OBJECT
 public:
  FloatingWindow(const QString& title, QWidget* content, QWidget* parent = nullptr);

  void setBody(QWidget* content);

 signals:
  void closed();

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void showEvent(QShowEvent* event) override;

 private:
  QLabel* title_ = nullptr;
  QVBoxLayout* root_ = nullptr;
  QWidget* bodyHost_ = nullptr;
  QPoint dragOffset_;
  bool dragging_ = false;
};
