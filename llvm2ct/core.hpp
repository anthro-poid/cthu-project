#pragma once

#include "builtin.hpp"

#include <llvm/IR/Function.h>

#include <list>
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

        template< typename... Stacks >
        void add_in( Stacks... stacks ) { ( in.push_back( stacks ), ... ); }

        template< typename... Stacks >
        void add_out( Stacks... stacks ) { ( out.push_back( stacks ), ... ); }

        uint16_t &get_in(  size_t i ) { return in[  i ]; }
        uint16_t &get_out( size_t i ) { return out[ i ]; }

        const std::string &get_structure_name() const;
        const std::string &get_subr_name()      const;
    };

    struct subr_t
    {
        std::string name;
        std::vector< uint16_t > input;
        std::vector< uint16_t > output;
        std::vector< insn > body;

        subr_t( std::string n ) : name{ std::move( n ) } {}
    };

    struct structure_t
    {
        std::string name;
        std::list< subr_t > subroutines;
        uint64_t next_subr_id;

        structure_t( std::string n ) : name{ std::move( n ) }, next_subr_id{ 0 } {}

        subr_ptr add_subroutine( llvm::BasicBlock * );
        subr_ptr create_subroutine();
    };

    struct module_t
    {
        std::list< structure_t > structures;
        uint64_t next_struct_id = 0;

        structure_ptr add_structure( llvm::Function * );
    };
}
