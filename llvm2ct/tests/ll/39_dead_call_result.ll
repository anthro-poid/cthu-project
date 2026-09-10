; EXPECT: 9

define i32 @increment(i32 %value) {
entry:
  %result = add i32 %value, 1
  ret i32 %result
}

define i32 @main() {
entry:
  %dead = call i32 @increment(i32 100)
  ret i32 9
}
