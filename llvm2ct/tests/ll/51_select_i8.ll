; EXPECT: 200
define i8 @main() {
entry:
  %condition = icmp ugt i8 200, 17
  %selected = select i1 %condition, i8 200, i8 17
  ret i8 %selected
}
