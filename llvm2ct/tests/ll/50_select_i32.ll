; EXPECT: 17
define i32 @main() {
entry:
  %condition = icmp slt i32 17, 42
  %selected = select i1 %condition, i32 17, i32 42
  ret i32 %selected
}
