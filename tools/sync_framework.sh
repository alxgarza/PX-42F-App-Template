#!/bin/bash
# Keeps the framework's files the same in every app project.
#
#   tools/sync_framework.sh           copy them into each sibling project
#   tools/sync_framework.sh --check   only say which differ (exit 1 if any)
#
# px_app.h comes from the firmware (../PX-42F-Plus/px_app.h: the original);
# px42f/ and sim/ (not sim/drive, which is each app's own) from here.
# Sibling projects: every ../PX-42F-* folder with a px42f/ folder.
set -e
cd "$(dirname "$0")/.."
HERE=$PWD
CHECK=0
[ "$1" = "--check" ] && CHECK=1
FIRMWARE=../PX-42F-Plus/px_app.h
status=0

# The template's own copy of px_app.h first: it's the one the others get.
if [ -f "$FIRMWARE" ]; then
    if ! cmp -s "$FIRMWARE" px42f/px_app.h; then
        echo "differs: px42f/px_app.h (the firmware's is newer or changed)"
        status=1
        [ $CHECK = 0 ] && cp "$FIRMWARE" px42f/px_app.h
    fi
else
    echo "no firmware project at $FIRMWARE: px_app.h not checked against it"
fi

for project in ../PX-42F-*/; do
    project=${project%/}
    [ "$project" = ".." ] && continue
    [ -d "$project/px42f" ] || continue
    [ "$(cd "$project" && pwd)" = "$HERE" ] && continue
    [ "$project" = "../PX-42F-Plus" ] && continue
    for part in px42f sim; do
        if ! diff -rq -x drive -x '.DS_Store' "$part" "$project/$part" >/dev/null 2>&1; then
            echo "differs: $project/$part"
            status=1
            if [ $CHECK = 0 ]; then
                rsync -a --exclude drive --exclude .DS_Store "$part/" "$project/$part/"
            fi
        fi
    done
done

if [ $status = 0 ]; then
    echo "all the same"
elif [ $CHECK = 1 ]; then
    exit 1
else
    echo "copied"
fi
