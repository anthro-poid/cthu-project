; Hand-written: i8 udiv/urem, unreachable from C (integer promotion).
; EXPECT: 4
define i8 @main() {
run:
  %q = udiv i8 13, 4
  %r = urem i8 13, 4
  %s = add i8 %q, %r
  ret i8 %s
}
