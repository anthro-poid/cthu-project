; A dead 32-bit chain with synthesized constants must be consumed without
; disturbing the independently computed return value.
; EXPECT: 305419896
define i32 @main() {
run:
  %dead1 = add i32 1000000, 2000000
  %dead2 = udiv i32 %dead1, 37
  %dead3 = shl i32 %dead2, 5
  ret i32 305419896
}
