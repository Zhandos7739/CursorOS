# CursorOS App (C++)

Нативное Qt6-приложение: десктоп в стиле Cloud Agent OS — бренд, атмосфера, панель, док, окна Terminal / Files / About.

## Сборка

```bash
sudo apt-get install -y build-essential cmake qt6-base-dev
cd app
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/cursoros
```

Нужен дисплей (локальный X11 / VNC / Wayland).
