#!/bin/bash

set -eu

git submodule update --checkout yocton
cd yocton
git pull
make docs
cd ..
cp -R yocton/html/* .

