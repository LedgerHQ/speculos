#!/bin/bash
# Update the version field of Speculos python package, in setup.py.
#
# This script is intended to be used before creating a Python package, in order
# to create a source tarball ("sdist package") with a version which has been
# defined using the number of git commits since the last tag, without requiring
# to have the git repository in the package.
#
# Git tags are expected to be "major.minor", such as "0.1".

set -eu
cd "$(dirname -- "${BASH_SOURCE[0]}")/.."

LAST_TAG="$(git describe --tags --abbrev=0)"
COUNT="$(git rev-list --count "${LAST_TAG}..HEAD")"
VERSION="${LAST_TAG}.${COUNT}"

sed 's/^\( *version=\)"[^"]*"/\1"'"${VERSION}"'"/' -i setup.py
