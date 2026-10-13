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
TAG_FILE="$(mktemp)"
trap 'rm -f "$TAG_FILE"' EXIT INT TERM
LAST_TAG="$(git describe --tags --abbrev=0)"

{
    echo "# Type your message for v${VERSION} above the comments."
    echo "# Lines starting with # will be ignored."
    echo
    git log --oneline "$LAST_TAG"..HEAD | sed 's/^/# /'
} > "$TAG_FILE"
${EDITOR:-vi} "$TAG_FILE"

TAG_MESSAGE="$(sed '/^#/d' "$TAG_FILE" | sed '/./,$!d' | sed -e :a -e '/^\n*$/{$d;N;ba' -e '}')"

if [ -z "$TAG_MESSAGE" ]; then
    echo "Aborting: empty tag message." >&2
    exit 1
fi

echo "\
/* CODE GENERATED AUTOMATICALLY. DO NOT EDIT. */

#ifndef NYX_VERSION_H
#define NYX_VERSION_H

#define NYX_VERSION \"$VERSION\"

#endif // NYX_VERSION_H" > "$VERSION_FILE"

git commit -m "Bump version to $VERSION" -- "$VERSION_FILE" 2>/dev/null || true

git tag -a "v$VERSION" -F "$TAG_FILE"

git push
git push origin "v$VERSION"
