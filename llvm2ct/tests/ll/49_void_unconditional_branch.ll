; EXPECT: 9

define void @consume(i32 %value) {
entry:
  %dead = add i32 %value, 1
  br label %exit

exit:
  ret void
}

define i32 @main() {
entry:
  call void @consume(i32 100)
  ret i32 9
}
