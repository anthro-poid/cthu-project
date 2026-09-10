; EXPECT: 610

define i32 @combine(i8 %a, i32 %b, i8 %c, i32 %d,
                    i8 %e, i32 %f, i8 %g) {
entry:
  %aw = zext i8 %a to i32
  %cw = zext i8 %c to i32
  %ew = zext i8 %e to i32
  %gw = zext i8 %g to i32
  %s0 = add i32 %aw, %b
  %s1 = add i32 %s0, %cw
  %s2 = add i32 %s1, %d
  %s3 = add i32 %s2, %ew
  %s4 = add i32 %s3, %f
  %result = add i32 %s4, %gw
  ret i32 %result
}

define i32 @main() {
entry:
  %result = call i32 @combine(i8 1, i32 100, i8 2, i32 200,
                              i8 3, i32 300, i8 4)
  ret i32 %result
}
