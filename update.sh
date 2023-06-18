#!/bin/bash

set -eu

git submodule update --checkout yocton
cd yocton
make docs
cd ..
cp -R yocton/html/* .

