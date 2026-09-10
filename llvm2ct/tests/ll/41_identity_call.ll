; EXPECT: 231

define i8 @identity(i8 %value) {
entry:
  ret i8 %value
}

define i32 @main() {
entry:
  %result = call i8 @identity(i8 231)
  %wide = zext i8 %result to i32
  ret i32 %wide
}
