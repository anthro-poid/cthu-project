; EXPECT: 135

define i32 @choose(i8 %byte, i32 %word, i1 %condition) {
entry:
  br i1 %condition, label %use_word, label %use_byte

use_word:
  %word.result = add i32 %word, 5
  ret i32 %word.result

use_byte:
  %byte.result = zext i8 %byte to i32
  ret i32 %byte.result
}

define i32 @main() {
entry:
  %true.condition = icmp eq i32 1, 1
  %false.condition = icmp eq i32 1, 2
  %word.result = call i32 @choose(i8 70, i32 200, i1 %true.condition)
  %byte.result = call i32 @choose(i8 70, i32 200, i1 %false.condition)
  %difference = sub i32 %word.result, %byte.result
  ret i32 %difference
}
