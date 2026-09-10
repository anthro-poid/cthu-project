; Negative literals arrive as their full two's-complement bit patterns and
; therefore exercise all eight hexadecimal digits during materialization.
; EXPECT: 134200939
define i32 @main() {
run:
  %q = sdiv i32 -123456789, 12345
  %r = srem i32 -123456789, 12345
  %a = ashr i32 -2147483648, 4
  %b = add i32 %q, %r
  %c = xor i32 %a, %b
  ret i32 %c
}
