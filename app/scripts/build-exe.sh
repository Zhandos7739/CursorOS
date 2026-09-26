#!/usr/bin/env bash
# Cross-compile CursorOS.exe for Windows x64 from Linux using MinGW.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${ROOT}/dist"
SRC="${ROOT}/src/CursorOS.cpp"
mkdir -p "${OUT}"

CXX="${CXX:-x86_64-w64-mingw32-g++}"
if ! command -v "${CXX}" >/dev/null 2>&1; then
  echo "Need MinGW: sudo apt-get install -y g++-mingw-w64-x86-64"
  exit 1
fi

echo "Building CursorOS.exe with ${CXX}"
"${CXX}" -std=c++17 -O2 -municode -mwindows \
  "${SRC}" \
  -o "${OUT}/CursorOS.exe" \
  -luser32 -lgdi32 -lmsimg32 -lshell32 \
  -static-libgcc -static-libstdc++

echo "OK: ${OUT}/CursorOS.exe"
file "${OUT}/CursorOS.exe"
ls -lh "${OUT}/CursorOS.exe"
