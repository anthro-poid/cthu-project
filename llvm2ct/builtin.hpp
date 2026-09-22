#pragma once

#include <boost/preprocessor.hpp>

#include <cstdint>
#include <string_view>

namespace cthu
{
#define CTHU_BUILTIN_STRUCTURES \
    ( ( w8,       "w₈"       ) ) \
    ( ( w32,      "w₃₂"      ) ) \
    ( ( i8,       "i₈"       ) ) \
    ( ( u8,       "u₈"       ) ) \
    ( ( i32,      "i₃₂"      ) ) \
    ( ( u32,      "u₃₂"      ) ) \
    ( ( boolean,  "bool"      ) ) \
    ( ( i8_32,    "i₈³²"     ) ) \
    ( ( u8_32,    "u₈³²"     ) ) \
    ( ( bool_8,   "b⁸"        ) ) \
    ( ( bool_32,  "b³²"       ) ) \
    ( ( function, "function"  ) ) \
    ( ( lambda,   "lambda"    ) )

#define CTHU_BUILTIN_OPERATIONS \
    ( ( join,          "join"    ) ) \
    ( ( bot,           "bot"     ) ) \
    ( ( top,           "top"     ) ) \
    ( ( opt,           "opt"     ) ) \
    ( ( fork,          "fork"    ) ) \
    ( ( move,          "move"    ) ) \
    ( ( push,          "push"    ) ) \
    ( ( pop,           "pop"     ) ) \
    ( ( drop,          "drop"    ) ) \
    ( ( dup,           "dup"     ) ) \
    ( ( logical_not,   "not"     ) ) \
    ( ( true_value,    "true"    ) ) \
    ( ( false_value,   "false"   ) ) \
    ( ( add,           "add"     ) ) \
    ( ( sub,           "sub"     ) ) \
    ( ( mul,           "mul"     ) ) \
    ( ( div,           "div"     ) ) \
    ( ( rem,           "rem"     ) ) \
    ( ( shl,           "shl"     ) ) \
    ( ( shr,           "shr"     ) ) \
    ( ( bit_and,       "and"     ) ) \
    ( ( bit_or,        "or"      ) ) \
    ( ( bit_xor,       "xor"     ) ) \
    ( ( bssert,        "assert"  ) ) \
    ( ( equal,         "eq?"     ) ) \
    ( ( not_equal,     "ne?"     ) ) \
    ( ( less,          "lt?"     ) ) \
    ( ( less_equal,    "le?"     ) ) \
    ( ( greater_equal, "ge?"     ) ) \
    ( ( greater,       "gt?"     ) ) \
    ( ( cut,           "cut"     ) ) \
    ( ( ext,           "ext"     ) ) \
    ( ( call,          "call"    ) ) \
    ( ( bind,          "bind"    ) ) \
    ( ( select,        "select"  ) ) \
    ( ( cons_0,        "cons_0"  ) ) \
    ( ( cons_1,        "cons_1"  ) ) \
    ( ( cons_2,        "cons_2"  ) ) \
    ( ( cons_3,        "cons_3"  ) ) \
    ( ( cons_4,        "cons_4"  ) ) \
    ( ( cons_5,        "cons_5"  ) ) \
    ( ( cons_6,        "cons_6"  ) ) \
    ( ( cons_7,        "cons_7"  ) ) \
    ( ( cons_8,        "cons_8"  ) ) \
    ( ( cons_9,        "cons_9"  ) ) \
    ( ( cons_10,       "cons_10" ) ) \
    ( ( cons_11,       "cons_11" ) ) \
    ( ( cons_12,       "cons_12" ) ) \
    ( ( cons_13,       "cons_13" ) ) \
    ( ( cons_14,       "cons_14" ) ) \
    ( ( cons_15,       "cons_15" ) )

#define CTHU_ENUM_VALUE( r, data, value ) BOOST_PP_TUPLE_ELEM( 2, 0, value ),

    enum struct builtin_structure
    {
        BOOST_PP_SEQ_FOR_EACH( CTHU_ENUM_VALUE, _, CTHU_BUILTIN_STRUCTURES )
    };

    enum struct builtin_operation
    {
        BOOST_PP_SEQ_FOR_EACH( CTHU_ENUM_VALUE, _, CTHU_BUILTIN_OPERATIONS )
    };

#undef CTHU_ENUM_VALUE

#define CTHU_ENUM_STRING_CASE( r, type, value ) \
    case type::BOOST_PP_TUPLE_ELEM( 2, 0, value ): \
        return BOOST_PP_TUPLE_ELEM( 2, 1, value );

    constexpr std::string_view builtin_structure_name( builtin_structure structure )
    {
        switch ( structure )
        {
            BOOST_PP_SEQ_FOR_EACH( CTHU_ENUM_STRING_CASE,
                                   builtin_structure, CTHU_BUILTIN_STRUCTURES )
        }

        __builtin_unreachable();
    }

    constexpr std::string_view builtin_operation_name( builtin_operation operation )
    {
        switch ( operation )
        {
            BOOST_PP_SEQ_FOR_EACH( CTHU_ENUM_STRING_CASE,
                                   builtin_operation, CTHU_BUILTIN_OPERATIONS )
        }

        __builtin_unreachable();
    }

#undef CTHU_ENUM_STRING_CASE
#undef CTHU_BUILTIN_OPERATIONS
#undef CTHU_BUILTIN_STRUCTURES

    builtin_operation nibble_operation( uint8_t value );
}
