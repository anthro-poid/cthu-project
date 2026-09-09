; Exercise all conversion builtins, including two consumers of the truncated
; value so the width-specific duplication path is used before extension.
define i32 @main() {
run:
  %byte = trunc i32 305419896 to i8
  %signed = sext i8 %byte to i32
  %unsigned = zext i8 %byte to i32
  %result = add i32 %signed, %unsigned
  ret i32 %result
}
