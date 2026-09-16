#!/bin/sh
# Copyright (c) 2026 Kilian Chung
# SPDX-License-Identifier: NLPL

set -e

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 VERSION" >&2
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
VERSION="$1"
VERSION_FILE="$ROOT_DIR/include/nyx/version.h"

echo "#define NYX_VERSION \"$VERSION\"" > "$VERSION_FILE"

git add "$VERSION_FILE"
git commit -am "Bump version to $VERSION" 2>/dev/null || true

git tag -a "v$VERSION"

git push
git push origin "v$VERSION"
