; EXPECT: 1

define i1 @less(i8 %a, i8 %b) {
entry:
  %result = icmp ult i8 %a, %b
  ret i1 %result
}

define i32 @main() {
entry:
  %result = call i1 @less(i8 17, i8 200)
  %wide = zext i1 %result to i32
  ret i32 %wide
}
