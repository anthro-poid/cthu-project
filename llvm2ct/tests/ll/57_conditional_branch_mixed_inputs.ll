; EXPECT: 223

define i32 @adjust(i8 %byte, i32 %word) {
entry:
  %condition = icmp ugt i8 %byte, 100
  br i1 %condition, label %large, label %small

large:
  %extended.large = zext i8 %byte to i32
  %flag.large = zext i1 %condition to i32
  %sum.large = add i32 %word, %extended.large
  %result.large = add i32 %sum.large, %flag.large
  ret i32 %result.large

small:
  %extended.small = zext i8 %byte to i32
  %flag.small = zext i1 %condition to i32
  %sum.small = add i32 %word, %extended.small
  %result.small = sub i32 %sum.small, %flag.small
  ret i32 %result.small
}

define i32 @main() {
entry:
  %result = call i32 @adjust(i8 200, i32 22)
  ret i32 %result
}
