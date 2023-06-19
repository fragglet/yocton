#!/bin/bash

set -eu

git submodule update --checkout yocton
cd yocton
git pull
make docs
cd ..
git rm *.{css,html,js,png}
cp -R yocton/html/* .
git add *.{css,html,js,png}

