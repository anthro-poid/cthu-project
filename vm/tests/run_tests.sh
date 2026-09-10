#!/usr/bin/env bash
cd "$(dirname "$0")"

RED="\e[31m"
GREEN="\e[32m"

run_valid() {
    for f in valid/*.ct; do
        python3 ../cthuvm.py ../prelude.ct signatures.ct ../builtins.ct structures.ct "$f" > /dev/null 2>&1 \
            && echo -e "${GREEN}PASS $f${ENDCOLOR}" || echo -e "${RED}SHOULD PASS: $f${ENDCOLOR}"
    done
}

run_invalid() {
    for f in invalid/*.ct; do
        python3 ../cthuvm.py ../prelude.ct signatures.ct ../builtins.ct structures.ct "$f" > /dev/null 2>&1 \
            && echo -e "${RED}SHOULD FAIL: $f${ENDCOLOR}" || echo -e "${GREEN}PASS $f${ENDCOLOR}"
    done
}

case "$1" in
    valid)   run_valid ;;
    invalid) run_invalid ;;
    *)       run_valid; run_invalid ;;
esac
