; Only %live is returned. The arithmetic chain must end with an explicit
; w₈ drop even though its intermediate values are consumed normally.
; EXPECT: 42
define i8 @main() {
run:
  %dead1 = add i8 100, 27
  %dead2 = mul i8 %dead1, 3
  %dead3 = xor i8 %dead2, 240
  %live = add i8 20, 22
  ret i8 %live
}
