; EXPECT: 2

define i1 @predicate(i32 %value, i1 %fallback) {
entry:
  %positive = icmp sgt i32 %value, 0
  br i1 %positive, label %calculate, label %use_fallback

calculate:
  %result = icmp eq i32 %value, 7
  ret i1 %result

use_fallback:
  ret i1 %fallback
}

define i32 @main() {
entry:
  %false = icmp eq i32 1, 2
  %true = icmp eq i32 1, 1
  %calculated = call i1 @predicate(i32 7, i1 %false)
  %fallback = call i1 @predicate(i32 -1, i1 %true)
  %calculated.int = zext i1 %calculated to i32
  %fallback.int = zext i1 %fallback to i32
  %result = add i32 %calculated.int, %fallback.int
  ret i32 %result
}
