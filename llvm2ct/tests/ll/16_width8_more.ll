; Hand-written, extending 11_width8.ll's coverage beyond add: exercises
; i8 sub/mul/icmp, none of which any C source can reach directly (C
; promotes char arithmetic to i32 before the actual op, as verified
; earlier this session).
define i8 @main() {
run:
  %s = sub i8 10, 4
  %m = mul i8 3, 3
  %c = icmp slt i8 %s, %m
  ret i8 %s
}
