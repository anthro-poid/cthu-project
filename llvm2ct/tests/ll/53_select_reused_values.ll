; EXPECT: 40
define i32 @main() {
entry:
  %a = add i32 4, 6
  %b = mul i32 4, 5
  %condition = icmp ult i32 %a, %b
  %selected = select i1 %condition, i32 %a, i32 %b
  %with_a = add i32 %selected, %a
  %result = add i32 %with_a, %b
  ret i32 %result
}
