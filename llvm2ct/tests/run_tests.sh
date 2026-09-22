#!/usr/bin/env bash
cd "$(dirname "$0")"
ROOT="$(cd ../.. && pwd)"

RED="\e[31m"
GREEN="\e[32m"
NC="\e[0m"

# absolute, not ../../-relative: relative paths make the VM's own parser
# fail on builtins.ct itself (unrelated to anything llvm2ct produces)
LLVM2CT="$ROOT/_build/llvm2ct"
CTHUVM="$ROOT/vm/cthuvm.py"
PRELUDE="$ROOT/vm/prelude.ct"
BUILTINS="$ROOT/vm/builtins.ct"

for c in c/*.c; do
    clang-21 -g -S -emit-llvm -Xclang -disable-O0-optnone "$c" -o "${c%.c}.ll"
    opt-21 -passes=mem2reg -S "${c%.c}.ll" -o "${c%.c}.ll"
done

for f in c/*.ll ll/*.ll; do
    name="${f%.ll}"
    source="$f"

    if [ -f "$name.c" ]; then
        source="$name.c"
    fi

    expected=$(sed -n 's/.*EXPECT:[[:space:]]*//p' "$source" | head -n 1)

    if [ -z "$expected" ]; then
        echo -e "${RED}FAIL $f: missing EXPECT${NC}"
        continue
    fi

    if ! "$LLVM2CT" "$f" > /dev/null ||
            ! mv out.ct "$name.ct" ||
            ! mv out.signatures.ct "$name.signatures.ct" ||
            ! mv out.structures.ct "$name.structures.ct"; then
        echo -e "${RED}FAIL $f${NC}"
        continue
    fi

    if actual=$(python3 "$CTHUVM" --print-result \
            "$PRELUDE" "$BUILTINS" \
            "$name.signatures.ct" "$name.structures.ct" "$name.ct" \
            2> /dev/null); then
        if [ "$actual" = "$expected" ]; then
            echo -e "${GREEN}PASS $f${NC}"
        else
            echo -e "${RED}FAIL $f: expected $expected, got $actual${NC}"
        fi
    else
        echo -e "${RED}FAIL $f${NC}"
    fi
done

rm -f c/*.ll c/*.ct c/*.prelude.ct c/*.builtins.ct \
    ll/*.ct ll/*.prelude.ct ll/*.builtins.ct \
    out.ct out.prelude.ct out.builtins.ct
