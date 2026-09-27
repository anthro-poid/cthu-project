#include "builtin.hpp"

#include <cassert>

namespace cthu
{
b_op nibble_operation( uint8_t value )
{
    switch ( value )
    {
        case 0:  return b_op::cons_0;
        case 1:  return b_op::cons_1;
        case 2:  return b_op::cons_2;
        case 3:  return b_op::cons_3;
        case 4:  return b_op::cons_4;
        case 5:  return b_op::cons_5;
        case 6:  return b_op::cons_6;
        case 7:  return b_op::cons_7;
        case 8:  return b_op::cons_8;
        case 9:  return b_op::cons_9;
        case 10: return b_op::cons_10;
        case 11: return b_op::cons_11;
        case 12: return b_op::cons_12;
        case 13: return b_op::cons_13;
        case 14: return b_op::cons_14;
        case 15: return b_op::cons_15;
        default: __builtin_unreachable();
    }
}
}
