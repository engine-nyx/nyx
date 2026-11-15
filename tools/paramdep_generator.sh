#!/bin/sh

SOURCE="$1"
TARGET="$2"
PARAM_DIR="$3"

if [ -z "$SOURCE" ] || [ -z "$TARGET" ] || [ -z "$PARAM_DIR" ]
then
    echo "Usage: $0 <source-file> <dependency-target> <param-dir>"
    exit 1
fi

printf '%s: ' "$TARGET"
grep -oE 'PARAMETER[[:space:]]*\([[:space:]]*[[:alnum:]_]+[[:space:]]*\)' "$SOURCE" | \
    sed -E "s/PARAMETER[[:space:]]*\([[:space:]]*/${PARAM_DIR}\//; s/[[:space:]]*\)/.param/" | \
    sort -u | tr '\n' ' '
printf '\n'
