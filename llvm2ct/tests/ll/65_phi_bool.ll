; EXPECT: 1

define i32 @choose_bool(i1 %condition, i32 %lhs, i32 %rhs) {
entry:
  br i1 %condition, label %compare_less, label %compare_equal

compare_less:
  %less = icmp slt i32 %lhs, %rhs
  br label %merge

compare_equal:
  %equal = icmp eq i32 %lhs, %rhs
  br label %merge

merge:
  %selected = phi i1 [ %less, %compare_less ], [ %equal, %compare_equal ]
  %result = zext i1 %selected to i32
  ret i32 %result
}

define i32 @main() {
entry:
  %first = call i32 @choose_bool(i1 true, i32 2, i32 5)
  %second = call i32 @choose_bool(i1 false, i32 2, i32 5)
  %result = add i32 %first, %second
  ret i32 %result
}
