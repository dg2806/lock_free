#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$ROOT/main.cpp"
BUILD_DIR="$ROOT/build"
OUT="$BUILD_DIR/out"

mkdir -p "$BUILD_DIR"

if [ -n "${CXX:-}" ]; then
  : "Using CXX from environment: $CXX"
elif [ -x /usr/local/bin/g++-15 ]; then
  CXX=/usr/local/bin/g++-15
  : "Using CXX from /usr/local/-15"
elif command -v clang++ >/dev/null 2>&1; then
  CXX=clang++
elif command -v g++ >/dev/null 2>&1; then
  CXX=g++
else
  echo "No C++ compiler (clang++ or g++) found in PATH" >&2
  exit 1
fi

CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -g"

echo "Compiling $SRC -> $OUT using $CXX"
"$CXX" $CXXFLAGS -o "$OUT" "$SRC"

echo "Build complete: $OUT" 
