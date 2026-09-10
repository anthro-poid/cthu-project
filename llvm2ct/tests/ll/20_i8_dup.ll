; Hand-written: exercises use()'s dup path at width 8 specifically (every
; existing dup test so far is 32-bit) — the literal `5` is the same
; uniqued ConstantInt used for both operands.
; EXPECT: 25
define i8 @main() {
run:
  %m = mul i8 5, 5
  ret i8 %m
}
