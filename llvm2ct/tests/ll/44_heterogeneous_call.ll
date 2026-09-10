; EXPECT: 1250

define i32 @add_char(i8 %a, i32 %b) {
entry:
  %wide = zext i8 %a to i32
  %result = add i32 %wide, %b
  ret i32 %result
}

define i32 @main() {
entry:
  %result = call i32 @add_char(i8 250, i32 1000)
  ret i32 %result
}
