#!/bin/bash

set -eu

git submodule update --checkout yocton
cd yocton
git checkout trunk
git pull
head_commit=$(git show-ref -s HEAD)
rm -rf html
make docs
cd ..
git rm -f *.{css,html,js,png}
cp -R yocton/html/* .
git add *.{css,html,js,png}
git add yocton
git commit -m "Update doxygen documentation to commit $head_commit."

