# CursorOS

Обзор среды, в которой крутится **Cursor Cloud Agent** (снимок от 2026-09-20).

Это не отдельная ОС Cursor — это Linux-контейнер/ВМ на базе Ubuntu, поднятая под агента.

## Система

| Параметр | Значение |
| --- | --- |
| Дистрибутив | **Ubuntu 24.04.4 LTS** (Noble Numbat) |
| Ядро | Linux **6.12.94+** (`PREEMPT_DYNAMIC`, SMP) |
| Архитектура | **x86_64** |
| Hostname | `cursor` |
| Виртуализация | **KVM** (full), CPU: Intel Xeon (hypervisor) |
| Пользователь | `ubuntu` (uid 1000, группа `sudo`) |
| Shell | `/bin/bash` |
| Locale | `en_US.UTF-8` |
| Workspace | `/workspace` |

Корневая ФС — **overlay** (~252 GiB, занято ~5%). Swap отсутствует.

## Железо (эмулированное / выделенное)

- **CPU:** 4 vCPU, Intel Xeon, VT-x, AVX-512 / AMX и др.
- **RAM:** ~16 GiB (`MemTotal` ≈ 15.6 GiB), без swap
- **Load:** типичный свежий бут агента — низкая нагрузка в первые минуты

## Toolchain (из коробки)

| Инструмент | Версия |
| --- | --- |
| Node.js | v22.14.0 (`/exec-daemon/node`) |
| npm | 10.9.7 (через nvm, Node v22.22.2 path) |
| Python | 3.12.3 |
| Git | 2.43.0 |

Также присутствуют служебные пути Cursor: `/exec-daemon`, `/cursor`, `/pod-daemon`, `/packages`.

## Layout ФС (верхний уровень)

```
/anyrun-init  /bin  /boot  /cursor  /dev  /etc  /exec-daemon
/home  /lib  /opt  /packages  /pod-daemon  /proc  /root  /run
/sys  /tmp  /usr  /var  /workspace
```

Рабочий репозиторий монтируется в **`/workspace`**.

## Среда Cloud Agent

- Репозиторий по умолчанию: `github.com/Zhandos7739/CursorOS`
- Egress: без жёсткого allowlist (неrestricted в этом запуске)
- Environment: Personal (runtime forward-fill), без готового environment build snapshot

## Как воспроизвести снимок

На машине агента:

```bash
uname -a
cat /etc/os-release
lscpu
free -h
df -h /
node -v && python3 --version && git --version
```

---

*Документ описывает конкретный ран Cloud Agent; версии и квоты могут отличаться между запусками.*
