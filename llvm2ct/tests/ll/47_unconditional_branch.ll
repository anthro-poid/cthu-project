; EXPECT: 42

define i32 @main() {
entry:
  %value = add i32 20, 22
  br label %exit

exit:
  ret i32 %value
}
