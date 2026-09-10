; Values above 15 must be assembled from hexadecimal digits at width 8.
; EXPECT: 255
define i8 @main() {
run:
  %sum = add i8 200, 55
  ret i8 %sum
}
