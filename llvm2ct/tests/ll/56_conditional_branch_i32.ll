; EXPECT: 20

define i32 @adjust(i32 %value) {
entry:
  %condition = icmp sgt i32 %value, 0
  br i1 %condition, label %positive, label %nonpositive

positive:
  %increased = add i32 %value, 10
  ret i32 %increased

nonpositive:
  %negated = sub i32 0, %value
  ret i32 %negated
}

define i32 @main() {
entry:
  %positive = call i32 @adjust(i32 7)
  %negative = call i32 @adjust(i32 -3)
  %result = add i32 %positive, %negative
  ret i32 %result
}
