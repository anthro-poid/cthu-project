; EXPECT: 55

define i32 @sum10(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e,
                  i32 %f, i32 %g, i32 %h, i32 %i, i32 %j) {
entry:
  %s0 = add i32 %a, %b
  %s1 = add i32 %s0, %c
  %s2 = add i32 %s1, %d
  %s3 = add i32 %s2, %e
  %s4 = add i32 %s3, %f
  %s5 = add i32 %s4, %g
  %s6 = add i32 %s5, %h
  %s7 = add i32 %s6, %i
  %result = add i32 %s7, %j
  ret i32 %result
}

define i32 @main() {
entry:
  %result = call i32 @sum10(i32 1, i32 2, i32 3, i32 4, i32 5,
                            i32 6, i32 7, i32 8, i32 9, i32 10)
  ret i32 %result
}
