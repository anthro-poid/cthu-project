; Hand-written: i8 sdiv/srem, unreachable from C (integer promotion).
define i8 @main() {
run:
  %q = sdiv i8 13, 4
  %r = srem i8 13, 4
  %s = add i8 %q, %r
  ret i8 %s
}
