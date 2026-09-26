#include <windows.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <cmath>
#include <ctime>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "shell32.lib")

namespace {

constexpr wchar_t kClassName[] = L"CursorOSAppWindow";
constexpr int kPanelH = 36;
constexpr int kDockH = 76;
constexpr int kDockBtn = 52;

enum class AppId { None, Terminal, Files, About };

struct FloatingWin {
  AppId id = AppId::None;
  RECT rc{};
  bool open = false;
  bool dragging = false;
  POINT dragOffset{};
};

struct AppState {
  HWND hwnd = nullptr;
  FloatingWin wins[3]{};
  AppId focus = AppId::None;
  int hoverDock = -1;
  HFONT fontUi = nullptr;
  HFONT fontBrand = nullptr;
  HFONT fontMono = nullptr;
  HFONT fontBrandHuge = nullptr;
  HBITMAP backBuffer = nullptr;
  HBITMAP backOld = nullptr;
  HDC backDc = nullptr;
  int bufW = 0;
  int bufH = 0;
  wchar_t clockText[64]{};
};

AppState g;

COLORREF C(int r, int g, int b) { return RGB(r, g, b); }

void EnsureFonts() {
  if (g.fontUi) return;
  g.fontUi = CreateFontW(18, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
  g.fontBrand = CreateFontW(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
  g.fontBrandHuge = CreateFontW(72, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
  g.fontMono = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
}

void EnsureBuffer(HDC hdc, int w, int h) {
  if (g.backBuffer && g.bufW == w && g.bufH == h) return;
  if (g.backBuffer) {
    SelectObject(g.backDc, g.backOld);
    DeleteObject(g.backBuffer);
    DeleteDC(g.backDc);
    g.backBuffer = nullptr;
    g.backOld = nullptr;
    g.backDc = nullptr;
  }
  g.backDc = CreateCompatibleDC(hdc);
  g.backBuffer = CreateCompatibleBitmap(hdc, w, h);
  g.backOld = static_cast<HBITMAP>(SelectObject(g.backDc, g.backBuffer));
  g.bufW = w;
  g.bufH = h;
}

void FillRectColor(HDC hdc, RECT r, COLORREF c) {
  HBRUSH br = CreateSolidBrush(c);
  FillRect(hdc, &r, br);
  DeleteObject(br);
}

void RoundRectFill(HDC hdc, RECT r, int rad, COLORREF fill, COLORREF border) {
  HBRUSH br = CreateSolidBrush(fill);
  HPEN pen = CreatePen(PS_SOLID, 1, border);
  HGDIOBJ oldBr = SelectObject(hdc, br);
  HGDIOBJ oldPen = SelectObject(hdc, pen);
  RoundRect(hdc, r.left, r.top, r.right, r.bottom, rad, rad);
  SelectObject(hdc, oldBr);
  SelectObject(hdc, oldPen);
  DeleteObject(br);
  DeleteObject(pen);
}

void DrawGradient(HDC hdc, RECT r) {
  // Vertical-ish atmospheric blend via several strips
  const int steps = 48;
  for (int i = 0; i < steps; ++i) {
    float t = static_cast<float>(i) / (steps - 1);
    int R = static_cast<int>(22 + t * 56 + (1 - t) * 6);
    int G = static_cast<int>(36 + std::sin(t * 3.14f) * 40);
    int B = static_cast<int>(28 + (1 - t) * 10);
    // warm earth toward bottom-right tint
    R = static_cast<int>(R * (1 - t * 0.15f) + (78 * t * 0.35f));
    G = static_cast<int>(G * (1 - t * 0.2f) + (70 * t * 0.25f));
    B = static_cast<int>(B * (1 - t * 0.4f) + (18 * t));
    RECT band{r.left, r.top + (r.bottom - r.top) * i / steps,
              r.right, r.top + (r.bottom - r.top) * (i + 1) / steps + 1};
    FillRectColor(hdc, band, C(R, G, B));
  }

  // Soft bloom (concentric translucent circles approximated with ellipses)
  for (int i = 6; i >= 1; --i) {
    int alphaTone = 12 * i;
    HBRUSH br = CreateSolidBrush(RGB(180 - i * 8, 200 - i * 10, 120));
    // Can't do real alpha easily in GDI — use lighter overlapping ellipses sparsely
    // Skip heavy bloom; vignette instead below.
    DeleteObject(br);
    (void)alphaTone;
  }

  // Vignette edges
  for (int i = 0; i < 40; ++i) {
    int a = 3;
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(a, a, a));
    HGDIOBJ old = SelectObject(hdc, pen);
    HGDIOBJ oldBr = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Ellipse(hdc, r.left - 80 + i * 4, r.top - 40 + i * 3, r.right + 80 - i * 4,
            r.bottom + 40 - i * 3);
    SelectObject(hdc, old);
    SelectObject(hdc, oldBr);
    DeleteObject(pen);
  }
}

RECT DockRect(int clientW, int clientH) {
  int dockW = 280;
  RECT rc{ (clientW - dockW) / 2, clientH - kDockH - 20,
           (clientW - dockW) / 2 + dockW, clientH - 20 };
  return rc;
}

RECT DockBtnRect(RECT dock, int index) {
  int gap = 18;
  int total = 3 * kDockBtn + 2 * gap;
  int x0 = dock.left + (dock.right - dock.left - total) / 2;
  int y0 = dock.top + (dock.bottom - dock.top - kDockBtn) / 2;
  RECT r{ x0 + index * (kDockBtn + gap), y0,
          x0 + index * (kDockBtn + gap) + kDockBtn, y0 + kDockBtn };
  return r;
}

const wchar_t* WinTitle(AppId id) {
  switch (id) {
    case AppId::Terminal: return L"Terminal";
    case AppId::Files: return L"Files — workspace";
    case AppId::About: return L"About CursorOS";
    default: return L"";
  }
}

FloatingWin* FindWin(AppId id) {
  for (auto& w : g.wins) if (w.id == id) return &w;
  return nullptr;
}

void OpenApp(AppId id, int clientW, int clientH) {
  FloatingWin* w = FindWin(id);
  if (!w) return;
  if (w->open) {
    g.focus = id;
    return;
  }
  int ww = 620, wh = 400;
  int idx = static_cast<int>(id) - 1;
  w->rc = { 100 + idx * 36, 90 + idx * 28, 100 + idx * 36 + ww, 90 + idx * 28 + wh };
  if (w->rc.right > clientW - 20) OffsetRect(&w->rc, clientW - 20 - w->rc.right, 0);
  if (w->rc.bottom > clientH - 100) OffsetRect(&w->rc, 0, clientH - 100 - w->rc.bottom);
  w->open = true;
  g.focus = id;
}

void DrawPanel(HDC hdc, int w) {
  RECT panel{0, 0, w, kPanelH};
  FillRectColor(hdc, panel, RGB(12, 16, 14));
  HPEN pen = CreatePen(PS_SOLID, 1, RGB(70, 90, 70));
  HGDIOBJ old = SelectObject(hdc, pen);
  MoveToEx(hdc, 0, kPanelH, nullptr);
  LineTo(hdc, w, kPanelH);
  SelectObject(hdc, old);
  DeleteObject(pen);

  SetBkMode(hdc, TRANSPARENT);
  SelectObject(hdc, g.fontBrand);
  SetTextColor(hdc, RGB(196, 214, 120));
  TextOutW(hdc, 16, 8, L"CursorOS", 8);

  SelectObject(hdc, g.fontUi);
  SetTextColor(hdc, RGB(154, 171, 144));
  TextOutW(hdc, 110, 9, L"secure session  ·  local app", 28);

  SIZE sz{};
  GetTextExtentPoint32W(hdc, g.clockText, lstrlenW(g.clockText), &sz);
  SetTextColor(hdc, RGB(230, 236, 220));
  TextOutW(hdc, w - sz.cx - 18, 9, g.clockText, lstrlenW(g.clockText));
}

void DrawHero(HDC hdc, int w, int h) {
  SetBkMode(hdc, TRANSPARENT);
  SelectObject(hdc, g.fontBrandHuge);
  SetTextColor(hdc, RGB(236, 244, 220));
  const wchar_t* brand = L"CursorOS";
  SIZE sz{};
  GetTextExtentPoint32W(hdc, brand, 8, &sz);
  TextOutW(hdc, (w - sz.cx) / 2, h * 34 / 100, brand, 8);

  SelectObject(hdc, g.fontUi);
  SetTextColor(hdc, RGB(200, 214, 180));
  const wchar_t* line = L"Local Windows app — looks like the agent OS, runs as a normal .exe";
  SIZE sz2{};
  GetTextExtentPoint32W(hdc, line, lstrlenW(line), &sz2);
  TextOutW(hdc, (w - sz2.cx) / 2, h * 34 / 100 + sz.cy + 6, line, lstrlenW(line));
}

void DrawDock(HDC hdc, int clientW, int clientH) {
  RECT dock = DockRect(clientW, clientH);
  RoundRectFill(hdc, {dock.left - 8, dock.top - 6, dock.right + 8, dock.bottom + 6},
                24, RGB(20, 28, 24), RGB(120, 140, 100));

  const wchar_t* glyphs[3] = {L"⌘", L"▤", L"◉"};
  // Fallback ASCII-ish if glyph missing — use letters
  const wchar_t* labels[3] = {L"T", L"F", L"A"};

  for (int i = 0; i < 3; ++i) {
    RECT b = DockBtnRect(dock, i);
    COLORREF fill = (g.hoverDock == i) ? RGB(58, 80, 56) : RGB(42, 58, 48);
    COLORREF border = (g.hoverDock == i) ? RGB(196, 214, 120) : RGB(74, 96, 80);
    RoundRectFill(hdc, b, 14, fill, border);

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, g.fontBrand);
    SetTextColor(hdc, RGB(232, 240, 216));
    SIZE sz{};
    GetTextExtentPoint32W(hdc, labels[i], 1, &sz);
    TextOutW(hdc, b.left + (kDockBtn - sz.cx) / 2, b.top + (kDockBtn - sz.cy) / 2, labels[i], 1);
    (void)glyphs;
  }
}

void DrawWindowBody(HDC hdc, FloatingWin& win) {
  RECT body = win.rc;
  body.top += 36;
  FillRectColor(hdc, body, RGB(14, 18, 16));

  SetBkMode(hdc, TRANSPARENT);
  int x = body.left + 18;
  int y = body.top + 16;

  if (win.id == AppId::Terminal) {
    SelectObject(hdc, g.fontMono);
    SetTextColor(hdc, RGB(170, 230, 140));
    const wchar_t* lines[] = {
        L"C:\\Users\\you> uname",
        L"CursorOS local shell (Windows app)",
        L"",
        L"C:\\Users\\you> ver",
        L"Microsoft Windows  [local .exe host]",
        L"",
        L"C:\\Users\\you> about",
        L"Desktop look: olive atmosphere, panel, dock",
        L"Stack: C++17 · Win32 GDI · no installer needed",
        L"",
        L"C:\\Users\\you> _",
    };
    for (auto* line : lines) {
      TextOutW(hdc, x, y, line, lstrlenW(line));
      y += 20;
    }
  } else if (win.id == AppId::Files) {
    SelectObject(hdc, g.fontUi);
    SetTextColor(hdc, RGB(220, 230, 210));
    const wchar_t* rows[] = {
        L"📁  app\\",
        L"📄  README.md",
        L"📁  vds\\",
        L"📄  CursorOS.exe",
        L"📄  build-exe scripts",
    };
    for (auto* row : rows) {
      TextOutW(hdc, x, y, row, lstrlenW(row));
      y += 28;
    }
  } else if (win.id == AppId::About) {
    SelectObject(hdc, g.fontBrandHuge);
    SetTextColor(hdc, RGB(196, 214, 120));
    // smaller for about
    HFONT mid = CreateFontW(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, FF_SWISS, L"Segoe UI");
    SelectObject(hdc, mid);
    TextOutW(hdc, x, y, L"CursorOS", 8);
    DeleteObject(mid);
    y += 56;
    SelectObject(hdc, g.fontUi);
    SetTextColor(hdc, RGB(200, 214, 180));
    const wchar_t* about[] = {
        L"Windows application that looks like the Cloud Agent OS.",
        L"Not a real OS — a local .exe with panel, dock, and apps.",
        L"",
        L"Dock: T = Terminal · F = Files · A = About",
        L"Drag windows by the title bar. Esc closes focused window.",
    };
    for (auto* line : about) {
      TextOutW(hdc, x, y, line, lstrlenW(line));
      y += 24;
    }
  }
}

void DrawFloating(HDC hdc, FloatingWin& win) {
  if (!win.open) return;
  RoundRectFill(hdc, win.rc, 16, RGB(28, 36, 32), RGB(90, 110, 80));
  RECT chrome = win.rc;
  chrome.bottom = chrome.top + 36;
  FillRectColor(hdc, chrome, RGB(28, 36, 32));

  SetBkMode(hdc, TRANSPARENT);
  SelectObject(hdc, g.fontUi);
  SetTextColor(hdc, RGB(220, 230, 208));
  const wchar_t* title = WinTitle(win.id);
  TextOutW(hdc, chrome.left + 14, chrome.top + 9, title, lstrlenW(title));

  // close button
  RECT closeBtn{ chrome.right - 36, chrome.top + 6, chrome.right - 10, chrome.top + 30 };
  RoundRectFill(hdc, closeBtn, 8, RGB(60, 40, 40), RGB(100, 70, 70));
  SetTextColor(hdc, RGB(255, 210, 210));
  TextOutW(hdc, closeBtn.left + 7, closeBtn.top + 2, L"X", 1);

  DrawWindowBody(hdc, win);
}

void Paint(HWND hwnd) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);
  RECT cr{};
  GetClientRect(hwnd, &cr);
  EnsureFonts();
  EnsureBuffer(hdc, cr.right, cr.bottom);
  HDC dc = g.backDc;

  DrawGradient(dc, cr);
  DrawHero(dc, cr.right, cr.bottom);
  DrawPanel(dc, cr.right);
  DrawDock(dc, cr.right, cr.bottom);

  // draw unfocused then focused
  for (auto& w : g.wins) {
    if (w.open && w.id != g.focus) DrawFloating(dc, w);
  }
  for (auto& w : g.wins) {
    if (w.open && w.id == g.focus) DrawFloating(dc, w);
  }

  BitBlt(hdc, 0, 0, cr.right, cr.bottom, dc, 0, 0, SRCCOPY);
  EndPaint(hwnd, &ps);
}

int HitDock(int x, int y, int cw, int ch) {
  RECT dock = DockRect(cw, ch);
  for (int i = 0; i < 3; ++i) {
    RECT b = DockBtnRect(dock, i);
    if (PtInRect(&b, POINT{x, y})) return i;
  }
  return -1;
}

FloatingWin* HitWindow(int x, int y, RECT* closeHit) {
  // topmost first = focus, then others
  auto tryHit = [&](FloatingWin& w) -> FloatingWin* {
    if (!w.open) return nullptr;
    if (!PtInRect(&w.rc, POINT{x, y})) return nullptr;
    RECT closeBtn{ w.rc.right - 36, w.rc.top + 6, w.rc.right - 10, w.rc.top + 30 };
    if (closeHit) *closeHit = closeBtn;
    return &w;
  };
  for (auto& w : g.wins) if (w.id == g.focus) if (auto* h = tryHit(w)) return h;
  for (int i = 2; i >= 0; --i) if (auto* h = tryHit(g.wins[i])) return h;
  return nullptr;
}

void UpdateClock() {
  SYSTEMTIME st;
  GetLocalTime(&st);
  const wchar_t* days[] = {L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"};
  const wchar_t* months[] = {L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
                             L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"};
  int hour = st.wHour % 12;
  if (hour == 0) hour = 12;
  wsprintfW(g.clockText, L"%s %s %d  %d:%02d %s", days[st.wDayOfWeek], months[st.wMonth - 1],
            st.wDay, hour, st.wMinute, st.wHour >= 12 ? L"PM" : L"AM");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE: {
      g.hwnd = hwnd;
      g.wins[0] = {AppId::Terminal, {}, false, false, {}};
      g.wins[1] = {AppId::Files, {}, false, false, {}};
      g.wins[2] = {AppId::About, {}, false, false, {}};
      UpdateClock();
      SetTimer(hwnd, 1, 1000, nullptr);
      return 0;
    }
    case WM_TIMER:
      UpdateClock();
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT:
      Paint(hwnd);
      return 0;
    case WM_SIZE:
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;
    case WM_MOUSEMOVE: {
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      RECT cr{};
      GetClientRect(hwnd, &cr);

      for (auto& w : g.wins) {
        if (w.dragging) {
          int ww = w.rc.right - w.rc.left;
          int wh = w.rc.bottom - w.rc.top;
          w.rc.left = x - w.dragOffset.x;
          w.rc.top = y - w.dragOffset.y;
          w.rc.right = w.rc.left + ww;
          w.rc.bottom = w.rc.top + wh;
          InvalidateRect(hwnd, nullptr, FALSE);
          return 0;
        }
      }

      int hover = HitDock(x, y, cr.right, cr.bottom);
      if (hover != g.hoverDock) {
        g.hoverDock = hover;
        InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }
    case WM_LBUTTONUP:
      for (auto& w : g.wins) w.dragging = false;
      ReleaseCapture();
      return 0;
    case WM_LBUTTONDOWN: {
      int x = GET_X_LPARAM(lParam);
      int y = GET_Y_LPARAM(lParam);
      RECT cr{};
      GetClientRect(hwnd, &cr);

      RECT closeBtn{};
      if (FloatingWin* w = HitWindow(x, y, &closeBtn)) {
        g.focus = w->id;
        if (PtInRect(&closeBtn, POINT{x, y})) {
          w->open = false;
          InvalidateRect(hwnd, nullptr, FALSE);
          return 0;
        }
        if (y >= w->rc.top && y <= w->rc.top + 36) {
          w->dragging = true;
          w->dragOffset = {x - w->rc.left, y - w->rc.top};
          SetCapture(hwnd);
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
      }

      int dock = HitDock(x, y, cr.right, cr.bottom);
      if (dock >= 0) {
        OpenApp(static_cast<AppId>(dock + 1), cr.right, cr.bottom);
        InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }
    case WM_KEYDOWN:
      if (wParam == VK_ESCAPE) {
        if (FloatingWin* w = FindWin(g.focus); w && w->open) {
          w->open = false;
          InvalidateRect(hwnd, nullptr, FALSE);
        }
      }
      return 0;
    case WM_DESTROY:
      KillTimer(hwnd, 1);
      if (g.backBuffer) {
        SelectObject(g.backDc, g.backOld);
        DeleteObject(g.backBuffer);
        DeleteDC(g.backDc);
      }
      if (g.fontUi) DeleteObject(g.fontUi);
      if (g.fontBrand) DeleteObject(g.fontBrand);
      if (g.fontBrandHuge) DeleteObject(g.fontBrandHuge);
      if (g.fontMono) DeleteObject(g.fontMono);
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int show) {
  SetProcessDPIAware();

  WNDCLASSEXW wc{sizeof(wc)};
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hi;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  wc.hbrBackground = nullptr;
  wc.lpszClassName = kClassName;
  RegisterClassExW(&wc);

  HWND hwnd = CreateWindowExW(
      0, kClassName, L"CursorOS",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800,
      nullptr, nullptr, hi, nullptr);

  ShowWindow(hwnd, show);
  UpdateWindow(hwnd);

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return static_cast<int>(msg.wParam);
}

// MinGW entry if unicode main not linked the same way
#ifdef __MINGW32__
int main() {
  return wWinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), SW_SHOWNORMAL);
}
#endif
