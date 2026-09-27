; EXPECT: 18

define i32 @choose(i1 %condition, i32 %first, i32 %second) {
entry:
  br i1 %condition, label %merge, label %adjust

adjust:
  %adjusted = add i32 %second, 1
  br label %merge

merge:
  %selected = phi i32 [ %first, %entry ], [ %adjusted, %adjust ]
  ret i32 %selected
}

define i32 @main() {
entry:
  %when.true = call i32 @choose(i1 true, i32 7, i32 10)
  %when.false = call i32 @choose(i1 false, i32 7, i32 10)
  %result = add i32 %when.true, %when.false
  ret i32 %result
}
