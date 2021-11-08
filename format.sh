#!/bin/bash

function format {
    clang-format-11 -style=file -i "$1"    
}

for d in . test; do
    for f in "$d"/*.hpp "$d"/*.h "$d"/*.cpp; do
        if [[ -f "$f" && $(basename "$f") != 3rd* ]]; then
            (printf 'Formatting %s\n' "$f"; format "$f") &
        fi
    done
done
