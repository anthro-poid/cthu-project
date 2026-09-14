; EXPECT: 9
define i32 @main() {
entry:
  %condition = icmp sgt i8 10, 3
  %dead = select i1 %condition, i8 120, i8 40
  ret i32 9
}
