; The convert structures define cut in the other direction as LLVM truncation
; to i1, which keeps the low bit of the source bitvector.
; EXPECT: 1
define i32 @main() {
run:
  %even = trunc i8 42 to i1
  %even32 = zext i1 %even to i32
  %odd = trunc i32 43 to i1
  %odd32 = zext i1 %odd to i32
  %result = add i32 %even32, %odd32
  ret i32 %result
}
