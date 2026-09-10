; Every computed value except the return value must be consumed explicitly.
; EXPECT: 7
define i32 @main() {
run:
  %dead8 = mul i8 20, 30
  %dead32 = add i32 1000, 2000
  %deadbool = icmp ult i32 3, 5
  ret i32 7
}
