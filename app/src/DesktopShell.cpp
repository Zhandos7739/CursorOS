#include "DesktopShell.hpp"

#include "FloatingWindow.hpp"
#include "Theme.hpp"

#include <QDateTime>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QSysInfo>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QPushButton* makeDockButton(const QString& glyph, const QString& tip, QWidget* parent) {
  auto* btn = new QPushButton(glyph, parent);
  btn->setFixedSize(Theme::kDockIcon + 8, Theme::kDockIcon + 8);
  btn->setCursor(Qt::PointingHandCursor);
  btn->setToolTip(tip);
  btn->setStyleSheet(QStringLiteral(
      "QPushButton {"
      "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
      "    stop:0 #3a4a38, stop:1 #243028);"
      "  border: 1px solid #4a6050; border-radius: 14px;"
      "  color: #e8f0d8; font-size: 22px; font-family: Inter;"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
      "    stop:0 #4a6048, stop:1 #2e3c34);"
      "  border-color: #c4d678;"
      "}"
      "QPushButton:pressed { background: #1a241c; }"));

  auto* shadow = new QGraphicsDropShadowEffect(btn);
  shadow->setBlurRadius(18);
  shadow->setOffset(0, 4);
  shadow->setColor(QColor(0, 0, 0, 120));
  btn->setGraphicsEffect(shadow);
  return btn;
}

}  // namespace

DesktopShell::DesktopShell(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle("CursorOS");
  setMinimumSize(1100, 700);
  resize(1280, 800);
  setStyleSheet(Theme::appStyle());

  auto* central = new QWidget(this);
  setCentralWidget(central);
  central->setAttribute(Qt::WA_OpaquePaintEvent, false);

  buildChrome();

  clockTimer_ = new QTimer(this);
  connect(clockTimer_, &QTimer::timeout, this, &DesktopShell::tickClock);
  clockTimer_->start(1000);
  tickClock();

  // Soft brand fade-in
  auto* brandFx = new QGraphicsOpacityEffect(heroBrand_);
  heroBrand_->setGraphicsEffect(brandFx);
  auto* fade = new QPropertyAnimation(brandFx, "opacity", this);
  fade->setDuration(900);
  fade->setStartValue(0.0);
  fade->setEndValue(1.0);
  fade->setEasingCurve(QEasingCurve::OutCubic);
  fade->start(QAbstractAnimation::DeleteWhenStopped);

  auto* lineFx = new QGraphicsOpacityEffect(heroLine_);
  heroLine_->setGraphicsEffect(lineFx);
  auto* fade2 = new QPropertyAnimation(lineFx, "opacity", this);
  fade2->setDuration(1100);
  fade2->setStartValue(0.0);
  fade2->setEndValue(1.0);
  fade2->setEasingCurve(QEasingCurve::OutCubic);
  QTimer::singleShot(180, fade2, [fade2]() { fade2->start(QAbstractAnimation::DeleteWhenStopped); });
}

void DesktopShell::buildChrome() {
  auto* root = centralWidget();

  panel_ = new QWidget(root);
  panel_->setFixedHeight(Theme::kPanelH);
  panel_->setStyleSheet("background: transparent;");

  auto* panelLay = new QHBoxLayout(panel_);
  panelLay->setContentsMargins(16, 0, 16, 0);

  brandMark_ = new QLabel("CursorOS", panel_);
  brandMark_->setFont(Theme::uiFont(13, QFont::DemiBold));
  brandMark_->setStyleSheet("color: #c4d678;");

  auto* status = new QLabel("secure session · VncAuth", panel_);
  status->setFont(Theme::uiFont(11));
  status->setStyleSheet("color: #9aab90;");

  clockLabel_ = new QLabel(panel_);
  clockLabel_->setFont(Theme::uiFont(12, QFont::Medium));
  clockLabel_->setStyleSheet("color: #e6ecdc;");

  panelLay->addWidget(brandMark_);
  panelLay->addSpacing(14);
  panelLay->addWidget(status);
  panelLay->addStretch();
  panelLay->addWidget(clockLabel_);

  heroBrand_ = new QLabel("CursorOS", root);
  heroBrand_->setFont(Theme::brandFont(64));
  heroBrand_->setStyleSheet("color: rgba(236, 244, 220, 230); background: transparent;");
  heroBrand_->setAlignment(Qt::AlignCenter);

  heroLine_ = new QLabel("Your desktop shell — XFCE energy, one window deep.", root);
  heroLine_->setFont(Theme::uiFont(15));
  heroLine_->setStyleSheet("color: rgba(200, 214, 180, 200); background: transparent;");
  heroLine_->setAlignment(Qt::AlignCenter);

  dock_ = new QWidget(root);
  dock_->setFixedHeight(Theme::kDockH);
  auto* dockLay = new QHBoxLayout(dock_);
  dockLay->setContentsMargins(18, 10, 18, 10);
  dockLay->setSpacing(14);
  dockLay->addStretch();

  auto* term = makeDockButton("⌘", "Terminal", dock_);
  auto* files = makeDockButton("▤", "Files", dock_);
  auto* about = makeDockButton("◉", "About", dock_);
  connect(term, &QPushButton::clicked, this, [this]() { openApp("terminal"); });
  connect(files, &QPushButton::clicked, this, [this]() { openApp("files"); });
  connect(about, &QPushButton::clicked, this, [this]() { openApp("about"); });

  dockLay->addWidget(term);
  dockLay->addWidget(files);
  dockLay->addWidget(about);
  dockLay->addStretch();

  layoutChrome();
}

void DesktopShell::layoutChrome() {
  const QRect r = centralWidget()->rect();
  panel_->setGeometry(0, 0, r.width(), Theme::kPanelH);

  heroBrand_->adjustSize();
  heroLine_->adjustSize();
  const int brandY = r.height() * 0.34;
  heroBrand_->move((r.width() - heroBrand_->width()) / 2, brandY);
  heroLine_->move((r.width() - heroLine_->width()) / 2, brandY + heroBrand_->height() + 8);

  const int dockW = qMin(420, r.width() - 80);
  dock_->setGeometry((r.width() - dockW) / 2, r.height() - Theme::kDockH - 18, dockW,
                     Theme::kDockH);
  dock_->raise();
  panel_->raise();
}

void DesktopShell::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const QRect r = rect();

  // Atmospheric backdrop — forest olive into warm earth (AnyOS-like)
  QLinearGradient g(r.topLeft(), r.bottomRight());
  g.setColorAt(0.0, QColor(22, 36, 28));
  g.setColorAt(0.35, QColor(48, 78, 52));
  g.setColorAt(0.7, QColor(78, 70, 38));
  g.setColorAt(1.0, QColor(28, 24, 18));
  p.fillRect(r, g);

  // Soft light bloom
  QRadialGradient bloom(r.width() * 0.55, r.height() * 0.28, r.width() * 0.55);
  bloom.setColorAt(0.0, QColor(180, 200, 120, 55));
  bloom.setColorAt(0.45, QColor(90, 110, 60, 20));
  bloom.setColorAt(1.0, QColor(0, 0, 0, 0));
  p.fillRect(r, bloom);

  // Vignette
  QRadialGradient vig(r.center(), r.width() * 0.75);
  vig.setColorAt(0.55, QColor(0, 0, 0, 0));
  vig.setColorAt(1.0, QColor(0, 0, 0, 110));
  p.fillRect(r, vig);

  // Top panel glass
  p.fillRect(QRect(0, 0, r.width(), Theme::kPanelH), Theme::panelBg());
  p.setPen(QPen(QColor(120, 140, 100, 50), 1));
  p.drawLine(0, Theme::kPanelH, r.width(), Theme::kPanelH);

  // Dock glass pill
  if (dock_) {
    QRectF dr = dock_->geometry().adjusted(-6, -4, 6, 4);
    QPainterPath path;
    path.addRoundedRect(dr, 22, 22);
    p.fillPath(path, Theme::dockGlass());
    p.setPen(QPen(QColor(160, 180, 130, 70), 1));
    p.drawPath(path);
  }
}

void DesktopShell::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  layoutChrome();
}

void DesktopShell::tickClock() {
  clockLabel_->setText(QDateTime::currentDateTime().toString("ddd MMM d  h:mm AP"));
}

void DesktopShell::openApp(const QString& id) {
  if (windows_.contains(id) && windows_[id]) {
    windows_[id]->show();
    windows_[id]->raise();
    return;
  }

  QString title;
  QWidget* body = nullptr;
  if (id == "terminal") {
    title = "Terminal";
    body = makeTerminal();
  } else if (id == "files") {
    title = "Files — /workspace";
    body = makeFiles();
  } else {
    title = "About CursorOS";
    body = makeAbout();
  }

  auto* win = new FloatingWindow(title, body, centralWidget());
  windows_[id] = win;
  win->move(120 + windows_.size() * 28, 90 + windows_.size() * 24);
  win->show();
  win->raise();
  connect(win, &FloatingWindow::closed, this, [this, id]() {
    // keep instance for reopen
  });
}

QWidget* DesktopShell::makeTerminal() {
  auto* edit = new QPlainTextEdit;
  edit->setReadOnly(true);
  edit->setFont(Theme::monoFont(12));

  QString out;
  out += "ubuntu@cursoros:~$ uname -a\n";
  out += QSysInfo::prettyProductName() + " · " + QSysInfo::currentCpuArchitecture() + "\n";
  out += "Kernel: " + QSysInfo::kernelVersion() + "\n\n";
  out += "ubuntu@cursoros:~$ cat /etc/os-release | head -4\n";

  QProcess proc;
  proc.start("bash", {"-lc", "cat /etc/os-release 2>/dev/null | head -6; echo; free -h 2>/dev/null | head -2; echo; node -v 2>/dev/null; python3 --version 2>/dev/null; git --version 2>/dev/null"});
  proc.waitForFinished(2000);
  out += QString::fromUtf8(proc.readAllStandardOutput());
  out += "\nubuntu@cursoros:~$ # CursorOS desktop app (Qt6)\n";
  out += "ubuntu@cursoros:~$ ▌";
  edit->setPlainText(out);
  return edit;
}

QWidget* DesktopShell::makeFiles() {
  auto* list = new QListWidget;
  list->setFont(Theme::uiFont(13));
  const QStringList entries = {
      "📁  .git",
      "📄  README.md",
      "📁  app/",
      "📁  vds/",
      "📄  vds/INSTALL.md",
      "📄  vds/install.sh",
  };
  list->addItems(entries);
  return list;
}

QWidget* DesktopShell::makeAbout() {
  auto* box = new QWidget;
  auto* lay = new QVBoxLayout(box);
  lay->setContentsMargins(28, 28, 28, 28);
  lay->setSpacing(10);

  auto* brand = new QLabel("CursorOS");
  brand->setFont(Theme::brandFont(36));
  brand->setStyleSheet("color: #c4d678;");

  auto* line = new QLabel(
      "Desktop shell application — same atmosphere as the Cloud Agent:\n"
      "XFCE-inspired panel, dock, and secure-session chrome.\n\n"
      "Stack: C++17 · Qt6 Widgets · forced-auth VDS installer in /vds.");
  line->setFont(Theme::uiFont(13));
  line->setStyleSheet("color: #c8d4b8;");
  line->setWordWrap(true);

  lay->addWidget(brand);
  lay->addWidget(line);
  lay->addStretch();
  return box;
}
