; Equality is signedness-independent, but has separate width-specific Cthu
; builtins. Keep both results live as independent values; i1 arithmetic is not
; part of the currently supported LLVM subset.
define i1 @main() {
run:
  %a = mul i8 3, 4
  %eq = icmp eq i8 %a, 12
  %ne = icmp ne i8 %a, 11
  ret i1 %ne
}
