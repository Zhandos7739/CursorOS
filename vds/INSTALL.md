# CursorOS на Ubuntu VDS

Повторяет стек Cloud Agent: **Ubuntu + XFCE + TigerVNC + noVNC**, но с **принудительным входом** (пароль VNC обязателен).  
В агенте стоит `SecurityTypes None` — на VDS так делать нельзя.

## Что получишь

| Компонент | Как в агенте | На VDS |
| --- | --- | --- |
| Desktop | XFCE 4.18 | XFCE |
| VNC | TigerVNC `:1`, 1920×1200 | то же |
| Auth | **нет** (`SecurityTypes None`) | **VncAuth** (пароль) |
| Bind | localhost | localhost |
| Браузер | noVNC | noVNC `:6080` |
| Dock | Plank | Plank (если есть) |
| Автозапуск | desktop-init | systemd |

## Требования

- VDS с **Ubuntu 22.04 / 24.04** (x86_64)
- root / sudo
- открытый SSH (22)
- желательно ≥2 vCPU, ≥2 GiB RAM

## Установка (3 команды)

На VDS:

```bash
# 1) склонируй репо
sudo apt-get update && sudo apt-get install -y git
git clone https://github.com/Zhandos7739/CursorOS.git
cd CursorOS

# 2) запусти установщик
sudo bash vds/install.sh

# 3) когда спросит — задай VNC-пароль (два раза)
```

Установщик:

1. поставит XFCE, TigerVNC, noVNC, websockify, plank, шрифты  
2. положит `xstartup` в `~/.vnc/`  
3. потребует `vncpasswd` (без файла пароля сервис **не** стартует)  
4. включит `cursoros-vnc` и `cursoros-novnc`  
5. откроет в UFW только SSH + порт noVNC (сырой 5901 наружу не открывает)

## Как зайти

### Вариант A — noVNC в браузере (проще)

1. Открой: `http://IP-ТВОЕГО-VDS:6080/vnc.html`
2. Введи **VNC-пароль** (тот, что задал в `vncpasswd`)
3. Жми Connect

Без пароля сессия не откроется — это и есть forced login.

### Вариант B — через SSH-туннель (безопаснее)

На своём ПК:

```bash
ssh -L 6080:127.0.0.1:6080 ubuntu@IP-VDS
```

Потом в браузере: `http://127.0.0.1:6080/vnc.html`  
И снова VNC-пароль.

Можно закрыть порт 6080 в UFW и ходить только через туннель:

```bash
sudo ufw delete allow 6080/tcp
```

### Вариант C — обычный VNC-клиент

```bash
ssh -L 5901:127.0.0.1:5901 ubuntu@IP-VDS
# клиент: localhost:5901 + VNC-пароль
```

## Управление

```bash
sudo systemctl status cursoros-vnc cursoros-novnc
sudo systemctl restart cursoros-vnc cursoros-novnc
sudo journalctl -u cursoros-vnc -f

# сменить пароль VNC
sudo -u ubuntu vncpasswd
sudo systemctl restart cursoros-vnc
```

Конфиг портов/геометрии: `/etc/cursoros/config.env`

## Жёстче по безопасности (рекомендую)

1. **SSH только по ключу**, пароль root выключить  
2. noVNC только через SSH-туннель (закрыть 6080 в UFW)  
3. `ufw enable`, fail2ban на ssh  
4. Не открывать `5901` в интернет  
5. Сильный VNC-пароль (≥8 символов; TigerVNC хранит хеш в `~/.vnc/passwd`)

Пример минимального UFW:

```bash
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow OpenSSH
# sudo ufw allow 6080/tcp   # только если noVNC нужен снаружи
sudo ufw enable
```

## Удаление

```bash
sudo systemctl disable --now cursoros-vnc cursoros-novnc
sudo rm -f /etc/systemd/system/cursoros-*.service
sudo systemctl daemon-reload
# пакеты desktop по желанию:
# sudo apt-get remove --purge xfce4 tigervnc-standalone-server novnc
```

## Отличия от «живой» Cloud Agent OS

- Нет Cursor exec-daemon / agent runtime — только desktop-окружение  
- Пароль VNC обязателен (в агенте часто без auth)  
- Порты по умолчанию: VNC `5901`, noVNC `6080` (у агента noVNC был `26058`)
