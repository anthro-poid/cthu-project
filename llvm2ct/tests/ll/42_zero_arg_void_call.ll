; EXPECT: 15

define void @noop() {
entry:
  ret void
}

define i32 @main() {
entry:
  call void @noop()
  ret i32 15
}
