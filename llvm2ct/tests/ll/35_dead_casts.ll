; Results of each supported cast family are deliberately unused and therefore
; require drops matching their destination types.
; EXPECT: 9
define i32 @main() {
run:
  %dead_trunc = trunc i32 4660 to i8
  %dead_sext = sext i8 240 to i32
  %dead_zext = zext i8 240 to i32
  %condition = icmp eq i32 7, 7
  %dead_bool_ext = zext i1 %condition to i32
  ret i32 9
}
