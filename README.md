# CursorOS

**Windows-приложение** `.exe` в стиле Cloud Agent OS + (опционально) установщик XFCE/VNC на Ubuntu VDS.

## CursorOS.exe (главное)

Локальный Win32 GUI: панель, док, Terminal / Files / About — выглядит как ОС агента, работает как обычная программа.

Готовый файл: [`app/dist/CursorOS.exe`](app/dist/CursorOS.exe)

Сборка на Linux:

```bash
sudo apt-get install -y g++-mingw-w64-x86-64
cd app && bash scripts/build-exe.sh
```

На Windows скопируй `CursorOS.exe` и запусти. Подробнее: [`app/README.md`](app/README.md)

## VDS (Ubuntu desktop + VNC)

```bash
git clone -b cursor/os-overview-9808 https://github.com/Zhandos7739/CursorOS.git
cd CursorOS && sudo bash vds/install.sh
```

См. [`vds/INSTALL.md`](vds/INSTALL.md).

## Снимок среды агента

| | |
| --- | --- |
| OS | Ubuntu 24.04.4 LTS |
| Desktop | XFCE, 1920×1200 |
| VNC / noVNC | TigerVNC + noVNC |
| Toolchain | Node 22 · Python 3.12 · Git 2.43 |
