#!/bin/bash
pwd=$(pwd)

if [ -d "/var/tmp/emsdk" ]; then
    echo "Emscripten SDK already installed. Skipping installation."
    cd /var/tmp/emsdk
    source ./emsdk_env.sh
else
    echo "Emscripten not found. Cloning and installing Emscripten SDK in /var/tmp."
    cd /var/tmp
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
    ./emsdk install latest
    ./emsdk activate latest
    source ./emsdk_env.sh
fi

cd $pwd
scons platform=web "$@"
