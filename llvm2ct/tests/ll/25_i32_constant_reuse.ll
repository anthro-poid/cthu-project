; LLVM uniques equal constants. Reusing a synthesized multi-nibble value
; throughout this block exercises the use-count driven dup chain and stack
; reclamation for a ConstantInt.
define i32 @main() {
run:
  %a = add i32 305419896, 305419896
  %b = mul i32 %a, 305419896
  %c = sub i32 %b, 305419896
  %d = xor i32 %c, 305419896
  %e = and i32 %d, 305419896
  ret i32 %e
}
