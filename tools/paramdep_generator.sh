#!/bin/sh

SOURCE="$1"
TARGET="$2"
PARAM_DIR="$3"

if [ -z "$SOURCE" ] || [ -z "$TARGET" ] || [ -z "$PARAM_DIR" ]
then
    echo "Usage: $0 <source-file> <dependency-target> <param-dir>" >&2
    exit 1
fi

# Find every PARAMETER(name) occurrence using only shell built-ins. Whitespace
# is allowed between the name and either parenthesis, just like before.
names=
while IFS= read -r line || [ -n "$line" ]
do
    rest=$line

    while :
    do
        case $rest in
            *PARAMETER*) ;;
            *) break ;;
        esac

        rest=${rest#*PARAMETER}
        rest=${rest#"${rest%%[![:space:]]*}"}

        case $rest in
            \(*) rest=${rest#?} ;;
            *) continue ;;
        esac

        rest=${rest#"${rest%%[![:space:]]*}"}
        name=${rest%%[![:alnum:]_]*}

        [ -n "$name" ] || continue

        rest=${rest#"$name"}
        rest=${rest#"${rest%%[![:space:]]*}"}

        case $rest in
            \)*) ;;
            *) continue ;;
        esac

        case " $names " in
            *" $name "*) ;;
            *) names="$names $name" ;;
        esac
    done
done < "$SOURCE"

printf '%s:' "$TARGET"
for name in $names
do
    printf ' %s/%s.param' "$PARAM_DIR" "$name"
done
printf '\n'
