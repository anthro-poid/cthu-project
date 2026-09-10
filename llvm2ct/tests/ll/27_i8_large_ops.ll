; Exercise multi-nibble i8 constants through arithmetic, reuse, and an
; unsigned comparison without introducing unsupported i1 arithmetic.
; EXPECT: 1
define i8 @main() {
run:
  %a = add i8 200, 55
  %b = sub i8 240, %a
  %c = mul i8 %b, 17
  %cmp = icmp ugt i8 %c, 128
  ret i8 %c
}
