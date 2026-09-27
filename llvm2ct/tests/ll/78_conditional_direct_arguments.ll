; EXPECT: 10

define i32 @choose(i1 %condition, i32 %value) {
entry:
  br i1 %condition, label %increment, label %decrement

increment:
  %incremented = add i32 %value, 1
  ret i32 %incremented

decrement:
  %decremented = sub i32 %value, 1
  ret i32 %decremented
}

define i32 @main() {
entry:
  %incremented = call i32 @choose(i1 true, i32 5)
  %decremented = call i32 @choose(i1 false, i32 5)
  %result = add i32 %incremented, %decremented
  ret i32 %result
}
