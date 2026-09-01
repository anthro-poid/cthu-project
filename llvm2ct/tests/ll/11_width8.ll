; Hand-written, not compiled from C: C's integer promotion means char
; arithmetic always widens to i32 before the actual operation (verified
; earlier this session — `char a; char b; char c = a + b;` compiles to
; sext/sext/add-i32/trunc, not add i8). This directly exercises the i8
; width path in codegen that no C source can reach without sext/trunc/zext
; support, which codegen doesn't have yet.
define i8 @main() {
entry:
  %a = add i8 3, 4
  ret i8 %a
}
