#!/bin/sh

set -ex

if [ `lsb_release -is` != "Ubuntu" ] || [ `lsb_release -cs` != "focal" ]; then
    echo "This script was only tested on Ubuntu Linux 20.04."
    sleep 5
fi

sudo apt install \
    bison \
    curl \
    flex \
    git \
    g++-8 \
    python3-pip

pip3 install pytest-xdist
