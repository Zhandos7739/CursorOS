#!/usr/bin/env bash
# CursorOS VDS installer — Ubuntu 22.04 / 24.04
# Ставит XFCE + TigerVNC (обязательный пароль) + noVNC + systemd.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Запусти от root: sudo bash $0"
  exit 1
fi

if [[ ! -f /etc/os-release ]]; then
  echo "Нужен Ubuntu/Debian."
  exit 1
fi
# shellcheck source=/dev/null
source /etc/os-release
if [[ "${ID:-}" != "ubuntu" && "${ID_LIKE:-}" != *"debian"* ]]; then
  echo "Ожидался Ubuntu. Обнаружено: ${PRETTY_NAME:-unknown}"
  exit 1
fi

DEFAULT_USER="${SUDO_USER:-ubuntu}"
read -r -p "Пользователь desktop [${DEFAULT_USER}]: " CURSOROS_USER
CURSOROS_USER="${CURSOROS_USER:-$DEFAULT_USER}"

if ! id "$CURSOROS_USER" >/dev/null 2>&1; then
  echo "Пользователь '$CURSOROS_USER' не существует. Создаю..."
  adduser --disabled-password --gecos "CursorOS" "$CURSOROS_USER"
  usermod -aG sudo "$CURSOROS_USER"
fi
USER_HOME="$(getent passwd "$CURSOROS_USER" | cut -d: -f6)"

echo "==> Пакеты"
export DEBIAN_FRONTEND=noninteractive
apt-get update -y
apt-get install -y \
  xfce4 xfce4-terminal xfce4-session thunar mousepad \
  dbus-x11 x11-utils x11-xserver-utils \
  tigervnc-standalone-server tigervnc-common tigervnc-tools \
  novnc websockify \
  fonts-inter fonts-jetbrains-mono \
  curl ca-certificates ufw \
  plank \
  mesa-utils libgl1-mesa-dri

# websockify CLI: в Ubuntu пакет даёт python-модуль; сделаем удобный wrapper
if [[ ! -x /usr/local/bin/websockify ]]; then
  cat >/usr/local/bin/websockify <<'EOF'
#!/usr/bin/env bash
exec python3 -m websockify "$@"
EOF
  chmod +x /usr/local/bin/websockify
fi

# noVNC web root (пакет novnc кладёт файлы сюда)
NOVNC_WEB="/usr/share/novnc"
if [[ ! -d "$NOVNC_WEB" ]]; then
  echo "Пакет novnc не дал /usr/share/novnc — ставлю вручную"
  mkdir -p /usr/share/novnc
  curl -fsSL https://github.com/novnc/noVNC/archive/refs/tags/v1.5.0.tar.gz \
    | tar -xz -C /tmp
  cp -a /tmp/noVNC-1.5.0/. /usr/share/novnc/
fi

echo "==> Конфиг /etc/cursoros"
mkdir -p /etc/cursoros
install -m 0644 "${SCRIPT_DIR}/config.env.example" /etc/cursoros/config.env
sed -i "s/^CURSOROS_USER=.*/CURSOROS_USER=${CURSOROS_USER}/" /etc/cursoros/config.env

# shellcheck source=/dev/null
source /etc/cursoros/config.env
DISPLAY="${DISPLAY:-:1}"
VNC_PORT="${VNC_PORT:-5901}"
NOVNC_PORT="${NOVNC_PORT:-6080}"
GEOMETRY="${GEOMETRY:-1920x1200}"
DEPTH="${DEPTH:-24}"
DPI="${DPI:-96}"
DESKTOP_NAME="${DESKTOP_NAME:-CursorOS}"

echo "==> VNC xstartup + обязательный пароль (Forced login)"
install -d -o "$CURSOROS_USER" -g "$CURSOROS_USER" "${USER_HOME}/.vnc"
install -m 0755 -o "$CURSOROS_USER" -g "$CURSOROS_USER" \
  "${SCRIPT_DIR}/scripts/xstartup" "${USER_HOME}/.vnc/xstartup"

# config для tigervnc (VncAuth only)
cat >"${USER_HOME}/.vnc/config" <<EOF
geometry=${GEOMETRY}
depth=${DEPTH}
dpi=${DPI}
localhost
SecurityTypes=VncAuth
desktop=${DESKTOP_NAME}
EOF
chown "$CURSOROS_USER:$CURSOROS_USER" "${USER_HOME}/.vnc/config"
chmod 0644 "${USER_HOME}/.vnc/config"

if [[ ! -f "${USER_HOME}/.vnc/passwd" ]]; then
  echo
  echo "Задай VNC-пароль (обязательно, иначе сервис не стартанет):"
  sudo -u "$CURSOROS_USER" vncpasswd
fi
if [[ ! -f "${USER_HOME}/.vnc/passwd" ]]; then
  echo "ERROR: пароль не создан (${USER_HOME}/.vnc/passwd отсутствует)"
  exit 1
fi
chmod 0600 "${USER_HOME}/.vnc/passwd"
chown "$CURSOROS_USER:$CURSOROS_USER" "${USER_HOME}/.vnc/passwd"

echo "==> systemd units"
for unit in cursoros-vnc.service cursoros-novnc.service; do
  sed "s/CURSOROS_USER_PLACEHOLDER/${CURSOROS_USER}/g" \
    "${SCRIPT_DIR}/systemd/${unit}" >"/etc/systemd/system/${unit}"
done
systemctl daemon-reload
systemctl enable --now cursoros-vnc.service
systemctl enable --now cursoros-novnc.service

echo "==> Firewall (UFW): SSH + noVNC; сырой VNC наружу НЕ открываем"
ufw allow OpenSSH >/dev/null 2>&1 || ufw allow 22/tcp >/dev/null 2>&1 || true
ufw allow "${NOVNC_PORT}/tcp" comment 'CursorOS noVNC' >/dev/null 2>&1 || true
# Не открываем 5901 наружу — только localhost + SSH tunnel
if ufw status | grep -qi inactive; then
  echo "UFW выключен. Включить? (y/N)"
  read -r ans || true
  if [[ "${ans:-}" =~ ^[Yy]$ ]]; then
    ufw --force enable
  fi
fi

echo
echo "============================================"
echo "  CursorOS desktop установлен"
echo "============================================"
echo "  Пользователь : ${CURSOROS_USER}"
echo "  VNC (local)  : localhost:${VNC_PORT}  [пароль обязателен]"
echo "  noVNC (web)  : http://<IP-VDS>:${NOVNC_PORT}/vnc.html"
echo
echo "  Безопасный вход через SSH-туннель:"
echo "    ssh -L ${NOVNC_PORT}:127.0.0.1:${NOVNC_PORT} ${CURSOROS_USER}@<IP-VDS}"
echo "    потом открой http://127.0.0.1:${NOVNC_PORT}/vnc.html"
echo
echo "  Статус:"
echo "    systemctl status cursoros-vnc cursoros-novnc"
echo "  Логи VNC:"
echo "    journalctl -u cursoros-vnc -f"
echo "============================================"
