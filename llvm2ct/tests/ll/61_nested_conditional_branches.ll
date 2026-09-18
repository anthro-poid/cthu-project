; EXPECT: 10

define i32 @classify(i32 %value) {
entry:
  %positive = icmp sgt i32 %value, 0
  br i1 %positive, label %positive_case, label %nonpositive_case

positive_case:
  %small = icmp slt i32 %value, 10
  br i1 %small, label %positive_small, label %positive_large

nonpositive_case:
  %zero = icmp eq i32 %value, 0
  br i1 %zero, label %zero_case, label %negative_case

positive_small:
  ret i32 1

positive_large:
  ret i32 2

zero_case:
  ret i32 3

negative_case:
  ret i32 4
}

define i32 @main() {
entry:
  %small = call i32 @classify(i32 5)
  %large = call i32 @classify(i32 20)
  %zero = call i32 @classify(i32 0)
  %negative = call i32 @classify(i32 -4)
  %first = add i32 %small, %large
  %second = add i32 %zero, %negative
  %result = add i32 %first, %second
  ret i32 %result
}
