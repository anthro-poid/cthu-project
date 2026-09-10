; EXPECT: 1

define i32 @bool_to_word(i1 %value) {
entry:
  %result = zext i1 %value to i32
  ret i32 %result
}

define i32 @main() {
entry:
  %condition = icmp eq i32 1234, 1234
  %result = call i32 @bool_to_word(i1 %condition)
  ret i32 %result
}
