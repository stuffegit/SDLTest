#!/usr/bin/env bash
set -e

PRESET=${1:-debug}

if [ ! -f "build/$PRESET/build.ninja" ]; then
  cmake --preset "$PRESET"
fi
cmake --build --preset "$PRESET"

echo "-- Build successful ($PRESET)"
