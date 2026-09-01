; Hand-written: i8 signed nested arithmetic (sdiv/srem/ashr), with %c read
; three times (a two-deep dup chain) feeding a final signed comparison —
; same free-list stress as 21_i8_nested_cmp.ll but through the signed
; builtin table.
define i8 @main() {
run:
  %a = sdiv i8 13, 4
  %b = srem i8 13, 4
  %c = add i8 %a, %b
  %d = mul i8 %c, 2
  %e = sub i8 %d, %c
  %f = ashr i8 %e, 1
  %g = icmp slt i8 %f, %c
  ret i8 %e
}
