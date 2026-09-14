; EXPECT: 1
define i32 @main() {
entry:
  %condition = icmp eq i32 4, 4
  %selected = select i1 %condition, i1 true, i1 false
  %result = zext i1 %selected to i32
  ret i32 %result
}
