# CursorOS — Windows .exe

Локальное **приложение Windows** (не ОС): окно в стиле Cloud Agent desktop — бренд, атмосфера, панель, док, Terminal / Files / About.

## Скачать / запустить

Готовый билд после `scripts/build-exe.sh`:

```
app/dist/CursorOS.exe
```

Скопируй `.exe` на Windows и запусти двойным кликом. Ничего ставить не нужно (статически линкуется runtime MinGW).

## Управление

| Действие | Как |
| --- | --- |
| Terminal | док **T** |
| Files | док **F** |
| About | док **A** |
| Закрыть окно | кнопка **X** или **Esc** |
| Переместить | тяни за заголовок |

## Сборка .exe

### На Linux (cross-compile MinGW) — как в этом репо

```bash
sudo apt-get install -y g++-mingw-w64-x86-64
cd app
bash scripts/build-exe.sh
# → dist/CursorOS.exe
```

### На Windows (MSVC / MinGW)

```bat
cmake -S . -B build -A x64
cmake --build build --config Release
```

Или MinGW:

```bash
g++ -std=c++17 -O2 -mwindows src/CursorOS.cpp -o CursorOS.exe -luser32 -lgdi32 -lmsimg32
```
