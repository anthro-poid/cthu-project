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

    "$LLVM2CT" "$f" > /dev/null && mv out.ct "$name.ct" \
        && python3 "$CTHUVM" "$PRELUDE" "$BUILTINS" "$name.ct" > /dev/null 2>&1 \
        && echo -e "${GREEN}PASS $f${NC}" || echo -e "${RED}FAIL $f${NC}"
done

rm -f c/*.ll c/*.ct ll/*.ct
