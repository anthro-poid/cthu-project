#pragma once

#include "builtin.hpp"

#include <llvm/IR/Function.h>

#include <initializer_list>
#include <list>
#include <string>
#include <string_view>
#include <utility>
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
            builtin_structure structure;
            builtin_operation subroutine;
            std::string struct_name;
        };

        using slist = std::initializer_list< uint16_t >;

        std::variant< l_call, b_call > call;
        std::vector< uint16_t > in;
        std::vector< uint16_t > out;

        insn( structure_ref s, subr_ref b, slist ins = {}, slist outs = {} ) :
            call( l_call{ s, b } ), in( ins ), out( outs ) {}

        insn( builtin_structure structure, builtin_operation operation, slist ins = {}, slist outs = {} ) :
            call( b_call{ structure, operation, {} } ),
            in( ins ), out( outs ) {}

        insn( const std::string &structure, builtin_operation operation, slist ins = {}, slist outs = {} ) :
            call( b_call{ builtin_structure::function, operation, structure } ),
            in( ins ), out( outs ) {}

        template< typename... Stacks >
        void add_in( Stacks... stacks ) { ( in.push_back( stacks ), ... ); }

        template< typename... Stacks >
        void add_out( Stacks... stacks ) { ( out.push_back( stacks ), ... ); }

        uint16_t &get_in(  size_t i ) { return in[  i ]; }
        uint16_t &get_out( size_t i ) { return out[ i ]; }

        std::string_view get_structure_name() const;
        std::string_view get_subr_name()      const;
    };

    struct subr_t
    {
        using slist = insn::slist;

        std::string name;
        std::vector< uint16_t > input;
        std::vector< uint16_t > output;
        std::vector< insn > body;

        subr_t( std::string n ) : name{ std::move( n ) } {}

        template< typename... Stacks >
        void add_in( Stacks... stacks ) { ( input.push_back( stacks ), ... ); }

        template< typename... Stacks >
        void add_out( Stacks... stacks ) { ( output.push_back( stacks ), ... ); }

        auto &get_input() { return input; }

        template< typename T, typename U >
        void add_insn( T &&strct, U &&op, slist ins, slist outs )
        {
            body.emplace_back( std::forward< T >( strct ), std::forward< U >( op ), ins, outs );
        }

        template< typename T >
        void add_insn( T &&i )
        {
            body.push_back( std::forward< T >( i ) );
        }
    };

    struct structure_t
    {
        std::string name;
        std::list< subr_t > subroutines;
        uint64_t next_subr_id;

        structure_t( std::string n ) : name{ std::move( n ) }, next_subr_id{ 0 } {}

        subr_ptr add_subroutine( llvm::BasicBlock * );
        subr_ptr add_subroutine();
    };

    struct module_t
    {
        std::list< structure_t > structures;
        uint64_t next_struct_id = 0;

        structure_ptr add_structure( llvm::Function * );
    };
}
