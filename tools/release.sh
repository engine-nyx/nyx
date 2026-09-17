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

echo "\
/* CODE GENERATED AUTOMATICALLY. DO NOT EDIT. */

#ifndef NYX_VERSION_H
#define NYX_VERSION_H

#define NYX_VERSION \"$VERSION\"

#endif // NYX_VERSION_H" > "$VERSION_FILE"

git commit -m "Bump version to $VERSION" -- "$VERSION_FILE" 2>/dev/null || true

git tag -a "v$VERSION"

git push
git push origin "v$VERSION"
