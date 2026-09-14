; EXPECT: 42

define i32 @main() {
entry:
  %base = add i32 10, 5
  br label %middle

middle:
  %doubled = mul i32 %base, 2
  br label %exit

exit:
  %result = add i32 %doubled, 12
  ret i32 %result
}
