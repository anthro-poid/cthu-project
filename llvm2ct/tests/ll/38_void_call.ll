; EXPECT: 7

define void @consume(i32 %unused, i32 %value) {
entry:
  %dead = add i32 %value, 1
  ret void
}

define i32 @main() {
entry:
  call void @consume(i32 1234, i32 42)
  ret i32 7
}
