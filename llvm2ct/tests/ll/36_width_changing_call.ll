; EXPECT: 250

define i32 @widen(i8 %value) {
entry:
  %result = zext i8 %value to i32
  ret i32 %result
}

define i32 @main() {
entry:
  %result = call i32 @widen(i8 250)
  ret i32 %result
}
