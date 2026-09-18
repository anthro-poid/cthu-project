; EXPECT: 9

define void @consume(i32 %value) {
entry:
  %condition = icmp sgt i32 %value, 10
  br i1 %condition, label %large, label %small

large:
  %dead.large = add i32 %value, 20
  ret void

small:
  %dead.small = sub i32 %value, 3
  ret void
}

define i32 @main() {
entry:
  call void @consume(i32 20)
  call void @consume(i32 5)
  ret i32 9
}
