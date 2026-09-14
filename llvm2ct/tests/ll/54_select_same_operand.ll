; EXPECT: 73
define i32 @main() {
entry:
  %value = add i32 70, 3
  %condition = icmp eq i32 1, 2
  %selected = select i1 %condition, i32 %value, i32 %value
  ret i32 %selected
}
