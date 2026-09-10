; Exercise the i8 comparison predicates not covered together by the earlier
; hand-written tests. The boolean results are not combined: i1 arithmetic and
; duplication are not supported by codegen yet.
; EXPECT: 1
define i1 @main() {
run:
  %a = sub i8 3, 9
  %b = add i8 2, 4
  %sle = icmp sle i8 %a, %b
  %sge = icmp sge i8 %b, %a
  %ule = icmp ule i8 4, 9
  %uge = icmp uge i8 9, 4
  ret i1 %uge
}
