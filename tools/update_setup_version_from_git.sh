#!/bin/bash
# Update the version field of Speculos python package, in setup.py.
#
# This script is intended to be used before creating a Python package, in order
# to create a source tarball ("sdist package") with a version which has been
# defined using the number of git commits since the last time the package
# version was modified in setup.py, without requiring to have the git repository
# in the package.
#
# Git tags are expected to be "major.minor", such as "0.1".

set -eu
cd "$(dirname -- "${BASH_SOURCE[0]}")/.."

VERSION_LINE="$(git grep -h --line-number '^    version=".*",$' setup.py | cut -d : -f 1)"
LAST_CHANGE="$(git blame --abbrev=40 -L"${VERSION_LINE}",+1 HEAD setup.py | cut -d ' ' -f 1)"
COUNT="$(git rev-list --count "${LAST_CHANGE}..HEAD")"

sed 's/^\(    version=\)"\([0-9]\+\.[0-9]\+\)\.[0-9]\+"/\1"\2.'"${COUNT}"'"/' -i setup.py

# Display the new version
echo 'Changed setup.py:'
git grep -h '^    version=".*",$' setup.py
