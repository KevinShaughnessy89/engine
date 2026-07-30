#!/usr/bin/env bash
set -e

echo "Installing build tools..."

sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    ccache

echo "Installing graphics dependencies..."

sudo apt install -y \
    libglfw3-dev \
    libglew-dev \
    libglm-dev \
    libeigen3-dev \
    libassimp-dev \
    zlib1g-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxrandr-dev \
    libxkbcommon-dev \
    rapidjson-dev

echo "Done."
