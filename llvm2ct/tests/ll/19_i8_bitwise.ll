; Hand-written: i8 and/or/xor/shl/lshr/ashr, unreachable from C (integer
; promotion).
; EXPECT: 1
define i8 @main() {
run:
  %a = and i8 12, 10
  %o = or  i8 12, 10
  %x = xor i8 %a, %o
  %s = shl i8 %x, 1
  %l = lshr i8 %s, 2
  %r = ashr i8 %l, 1
  ret i8 %r
}
