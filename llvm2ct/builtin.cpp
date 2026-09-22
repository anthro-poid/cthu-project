#include "builtin.hpp"

#include <cassert>

namespace cthu
{
builtin_operation nibble_operation( uint8_t value )
{
    switch ( value )
    {
        case 0:  return builtin_operation::cons_0;
        case 1:  return builtin_operation::cons_1;
        case 2:  return builtin_operation::cons_2;
        case 3:  return builtin_operation::cons_3;
        case 4:  return builtin_operation::cons_4;
        case 5:  return builtin_operation::cons_5;
        case 6:  return builtin_operation::cons_6;
        case 7:  return builtin_operation::cons_7;
        case 8:  return builtin_operation::cons_8;
        case 9:  return builtin_operation::cons_9;
        case 10: return builtin_operation::cons_10;
        case 11: return builtin_operation::cons_11;
        case 12: return builtin_operation::cons_12;
        case 13: return builtin_operation::cons_13;
        case 14: return builtin_operation::cons_14;
        case 15: return builtin_operation::cons_15;
        default: __builtin_unreachable();
    }
}
}
