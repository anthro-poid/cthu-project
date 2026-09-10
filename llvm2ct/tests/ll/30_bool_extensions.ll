; Exercise both boolean extension rules at both supported destination widths.
; True zero-extends to one and sign-extends to an all-one bitvector.
; EXPECT: 0
define i32 @main() {
run:
  %z8cond = icmp eq i8 7, 7
  %z8 = zext i1 %z8cond to i8
  %s8cond = icmp eq i8 9, 9
  %s8 = sext i1 %s8cond to i8
  %sum8 = add i8 %z8, %s8

  %z32cond = icmp eq i32 11, 11
  %z32 = zext i1 %z32cond to i32
  %s32cond = icmp eq i32 13, 13
  %s32 = sext i1 %s32cond to i32
  %sum32 = add i32 %z32, %s32

  %sum8wide = zext i8 %sum8 to i32
  %result = add i32 %sum32, %sum8wide
  ret i32 %result
}
