#include "FloatingWindow.hpp"

#include "Theme.hpp"

#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QShowEvent>

FloatingWindow::FloatingWindow(const QString& title, QWidget* content, QWidget* parent)
    : QFrame(parent) {
  setAttribute(Qt::WA_TranslucentBackground);
  setMinimumSize(420, 280);
  resize(640, 420);

  root_ = new QVBoxLayout(this);
  root_->setContentsMargins(1, 1, 1, 1);
  root_->setSpacing(0);

  auto* chrome = new QWidget(this);
  chrome->setFixedHeight(36);
  chrome->setObjectName("chrome");
  auto* chromeLay = new QHBoxLayout(chrome);
  chromeLay->setContentsMargins(12, 0, 8, 0);

  title_ = new QLabel(title, chrome);
  title_->setFont(Theme::uiFont(12, QFont::DemiBold));
  title_->setStyleSheet("color: #dce6d0;");

  auto* closeBtn = new QPushButton("✕", chrome);
  closeBtn->setFixedSize(28, 24);
  closeBtn->setCursor(Qt::PointingHandCursor);
  closeBtn->setStyleSheet(
      "QPushButton { background: transparent; border: none; color: #aab8a0; border-radius: 6px; }"
      "QPushButton:hover { background: #5a3030; color: #ffd0d0; }");
  connect(closeBtn, &QPushButton::clicked, this, [this]() {
    hide();
    emit closed();
  });

  chromeLay->addWidget(title_);
  chromeLay->addStretch();
  chromeLay->addWidget(closeBtn);

  bodyHost_ = new QWidget(this);
  auto* bodyLay = new QVBoxLayout(bodyHost_);
  bodyLay->setContentsMargins(0, 0, 0, 0);
  bodyLay->setSpacing(0);
  if (content) {
    content->setParent(bodyHost_);
    bodyLay->addWidget(content);
  }

  root_->addWidget(chrome);
  root_->addWidget(bodyHost_, 1);
}

void FloatingWindow::setBody(QWidget* content) {
  auto* lay = qobject_cast<QVBoxLayout*>(bodyHost_->layout());
  while (QLayoutItem* it = lay->takeAt(0)) {
    if (it->widget())
      it->widget()->deleteLater();
    delete it;
  }
  if (content) {
    content->setParent(bodyHost_);
    lay->addWidget(content);
  }
}

void FloatingWindow::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
  QPainterPath path;
  path.addRoundedRect(r, 14, 14);

  p.fillPath(path, Theme::windowChrome());

  QPainterPath body;
  body.addRoundedRect(r.adjusted(0, 36, 0, 0), 14, 14);
  p.setClipPath(path);
  p.fillRect(QRect(0, 36, width(), height() - 36), Theme::windowBody());
  p.setClipping(false);

  p.setPen(QPen(QColor(90, 110, 80, 120), 1));
  p.drawPath(path);
}

void FloatingWindow::showEvent(QShowEvent* event) {
  QFrame::showEvent(event);
  auto* fx = new QGraphicsOpacityEffect(this);
  setGraphicsEffect(fx);
  auto* anim = new QPropertyAnimation(fx, "opacity", this);
  anim->setDuration(220);
  anim->setStartValue(0.0);
  anim->setEndValue(1.0);
  anim->setEasingCurve(QEasingCurve::OutCubic);
  anim->start(QAbstractAnimation::DeleteWhenStopped);

  const QPoint end = pos();
  move(end + QPoint(0, 18));
  auto* slide = new QPropertyAnimation(this, "pos", this);
  slide->setDuration(240);
  slide->setStartValue(pos());
  slide->setEndValue(end);
  slide->setEasingCurve(QEasingCurve::OutCubic);
  slide->start(QAbstractAnimation::DeleteWhenStopped);
}

void FloatingWindow::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && event->position().y() <= 36) {
    dragging_ = true;
    dragOffset_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
    raise();
    event->accept();
    return;
  }
  QFrame::mousePressEvent(event);
}

void FloatingWindow::mouseMoveEvent(QMouseEvent* event) {
  if (dragging_ && (event->buttons() & Qt::LeftButton)) {
    move(event->globalPosition().toPoint() - dragOffset_);
    event->accept();
    return;
  }
  QFrame::mouseMoveEvent(event);
}

void FloatingWindow::mouseReleaseEvent(QMouseEvent* event) {
  dragging_ = false;
  QFrame::mouseReleaseEvent(event);
}
