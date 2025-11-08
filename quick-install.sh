#!/bin/sh
set -e

TMP_DIR=$(mktemp -d)

git clone https://github.com/Maqi-x/tpv.git "$TMP_DIR/tpv"

cd "$TMP_DIR/tpv"

make
sudo make install

cd /
rm -rf "$TMP_DIR"
