; Hand-written: i8 nested arithmetic with repeated operands (dup chains on
; both %a and the shared constant 4) feeding a final unsigned comparison,
; stressing the free-list type-partitioning fix in vm/symtab.py (a bool
; output must not reuse a freed w₈ slot as if it were still w₈).
define i8 @main() {
run:
  %a = add i8 3, 4
  %b = mul i8 %a, 2
  %c = sub i8 %b, %a
  %d = udiv i8 13, 4
  %e = urem i8 13, 4
  %f = mul i8 %d, %e
  %g = add i8 %c, %f
  %h = icmp ugt i8 %g, %d
  ret i8 %g
}
