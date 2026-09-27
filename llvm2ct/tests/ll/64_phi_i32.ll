; EXPECT: 22

define i32 @choose(i32 %value, i1 %condition) {
entry:
  br i1 %condition, label %increment, label %decrement

increment:
  %increased = add i32 %value, 3
  br label %merge

decrement:
  %decreased = sub i32 %value, 2
  br label %merge

merge:
  %selected = phi i32 [ %increased, %increment ], [ %decreased, %decrement ]
  %result = mul i32 %selected, 2
  ret i32 %result
}

define i32 @main() {
entry:
  %when.true = call i32 @choose(i32 5, i1 true)
  %when.false = call i32 @choose(i32 5, i1 false)
  %result = add i32 %when.true, %when.false
  ret i32 %result
}
