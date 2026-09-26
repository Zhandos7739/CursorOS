# CursorOS

Обзор среды **Cursor Cloud Agent** + готовый пакет, чтобы поставить похожий desktop на свой **Ubuntu VDS**.

> **Поставить на VDS:** см. **[vds/INSTALL.md](vds/INSTALL.md)**  
> Коротко: `sudo bash vds/install.sh` → задать VNC-пароль → открыть `http://IP:6080/vnc.html`

## Что это

Не отдельная ОС Cursor. В агенте крутится Linux-контейнер на **Ubuntu 24.04** с XFCE, TigerVNC и noVNC (бренд AnyOS).

В этом репо:

1. Снимок того, что видит агент (ниже)
2. Установщик **того же стиля desktop** для VDS, но с **принудительным VNC-логином** (`VncAuth`)
3. **C++ Qt6-приложение** [`app/`](app/) — десктоп CursorOS в одном окне (панель, док, Terminal/Files/About)

```bash
cd app && cmake -S . -B build && cmake --build build -j && ./build/cursoros
```

## Система агента (снимок 2026-09-20)

| Параметр | Значение |
| --- | --- |
| Дистрибутив | **Ubuntu 24.04.4 LTS** (Noble Numbat) |
| Ядро | Linux **6.12.94+** |
| Архитектура | **x86_64** / KVM |
| Desktop | XFCE + Plank, 1920×1200 @ 96 DPI |
| VNC | TigerVNC `:1` → порт `5901` (localhost) |
| Web | noVNC (у агента порт `26058`) |
| User | `ubuntu` · bash · `/workspace` |
| RAM / диск | ~16 GiB · overlay ~252 GiB |

### Toolchain

| Инструмент | Версия |
| --- | --- |
| Node.js | v22.14.0 |
| Python | 3.12.3 |
| Git | 2.43.0 |

## VDS-пакет (`vds/`)

| Файл | Назначение |
| --- | --- |
| `vds/install.sh` | установка XFCE + TigerVNC + noVNC + systemd |
| `vds/INSTALL.md` | пошаговая инструкция на русском |
| `vds/scripts/xstartup` | старт XFCE (как у агента) |
| `vds/systemd/*.service` | автозапуск VNC / noVNC |
| `vds/config.env.example` | порты и геометрия |

**Важно:** на VDS пароль VNC обязателен. Без `~/.vnc/passwd` сервис не стартует. Сырой порт `5901` наружу не открывается.

```bash
git clone https://github.com/Zhandos7739/CursorOS.git
cd CursorOS
sudo bash vds/install.sh
```

Подробности и SSH-туннель: [vds/INSTALL.md](vds/INSTALL.md).

---

*Снимок агента может отличаться между запусками; VDS-скрипт целится в Ubuntu 22.04/24.04.*
