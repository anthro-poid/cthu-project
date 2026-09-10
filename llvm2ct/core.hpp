#pragma once

#include "builtin.hpp"

#include <string>
#include <variant>
#include <vector>

namespace cthu
{
    struct subr_t;
    struct structure_t;

    using subr_ptr  = subr_t *;
    using subr_ref  = subr_t &;
    using subr_cref = const subr_t &;

    using structure_ptr  = structure_t *;
    using structure_ref  = structure_t &;
    using structure_cref = const structure_t &;

    struct insn
    {
        struct l_call
        {
            structure_ref structure;
            subr_ref subroutine;
        };

        struct b_call
        {
            std::string struct_name;
            std::string subr_name;
            builtin code;
        };

        std::variant< l_call, b_call > call;
        std::vector< uint16_t > in;
        std::vector< uint16_t > out;

        insn( structure_ref s, subr_ref b ) : call( l_call{ s, b } ) {}
        insn( const std::string &s_name, const std::string &b_name, builtin code ) :
            call( b_call{ s_name, b_name, code } ) {} 

        void add_in(  uint16_t v ) { in.push_back(  v ); }
        void add_out( uint16_t v ) { out.push_back( v ); }

        uint16_t &get_in(  size_t i ) { return in[  i ]; }
        uint16_t &get_out( size_t i ) { return out[ i ]; }

        const std::string &get_structure_name() const;
        const std::string &get_subr_name()      const;
    };

    struct subr_t
    {
        std::string name;
        std::vector< uint16_t > output;
        std::vector< insn > body;
    };

    struct structure_t
    {
        std::string name;
        std::vector< subr_ptr > subroutines;
    };
}