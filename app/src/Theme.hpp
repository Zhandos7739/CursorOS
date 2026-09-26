#pragma once

#include <QColor>
#include <QFont>
#include <QString>

namespace Theme {

inline constexpr int kPanelH = 34;
inline constexpr int kDockH = 72;
inline constexpr int kDockIcon = 48;

inline QColor bgDeep() { return QColor(18, 28, 22); }
inline QColor bgMid() { return QColor(42, 72, 48); }
inline QColor bgWarm() { return QColor(92, 78, 42); }
inline QColor accent() { return QColor(196, 214, 120); }
inline QColor panelBg() { return QColor(12, 16, 14, 210); }
inline QColor panelText() { return QColor(230, 236, 220); }
inline QColor dockGlass() { return QColor(20, 28, 24, 180); }
inline QColor windowChrome() { return QColor(28, 36, 32); }
inline QColor windowBody() { return QColor(14, 18, 16); }
inline QColor terminalGreen() { return QColor(170, 230, 140); }

inline QFont brandFont(int px = 56) {
  QFont f("Inter", px, QFont::Bold);
  f.setStyleStrategy(QFont::PreferAntialias);
  return f;
}

inline QFont uiFont(int px = 12, QFont::Weight w = QFont::Medium) {
  QFont f("Inter", px, w);
  f.setStyleStrategy(QFont::PreferAntialias);
  return f;
}

inline QFont monoFont(int px = 12) {
  QFont f("JetBrains Mono", px);
  if (!f.exactMatch())
    f = QFont("DejaVu Sans Mono", px);
  f.setStyleStrategy(QFont::PreferAntialias);
  return f;
}

inline QString appStyle() {
  return QStringLiteral(
      "QWidget { color: #e6ecdc; }"
      "QScrollArea { border: none; background: transparent; }"
      "QTextEdit, QPlainTextEdit {"
      "  background: #0e1210; color: #aae68c;"
      "  border: none; selection-background-color: #3a5a3c;"
      "  font-family: 'JetBrains Mono', 'DejaVu Sans Mono', monospace;"
      "  font-size: 13px;"
      "}"
      "QListWidget {"
      "  background: #121816; border: none; outline: none;"
      "  font-family: Inter, sans-serif; font-size: 13px;"
      "}"
      "QListWidget::item { padding: 10px 12px; }"
      "QListWidget::item:selected { background: #2a4030; }"
      "QPushButton {"
      "  background: #2a382c; border: 1px solid #3e5240; border-radius: 8px;"
      "  padding: 8px 14px; color: #e6ecdc; font-family: Inter;"
      "}"
      "QPushButton:hover { background: #354836; }"
      "QPushButton:pressed { background: #1e2a20; }");
}

}  // namespace Theme
