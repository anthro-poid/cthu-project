#pragma once

#include "core.hpp"
#include "mapping.hpp"

#include <llvm/ADT/ArrayRef.h>

namespace cthu
{
    using llvm2ct::function_type;
    using llvm2ct::type_to_structure;

    using cthu::builtin_structure_name;

    inline auto &operator<<( auto &os, const std::vector< uint16_t > &stacks )
    {
        for ( size_t i = 0; i < stacks.size(); ++ i )
            os << ( i ? " " : "" ) << stacks[ i ];
        return os;
    }

    inline auto &operator<<( auto &os, const insn &i )
    {
        os << "        " << i.get_structure_name() << " " << i.get_subr_name();

        if ( !i.in.empty() )
            os << " " << i.in;
        if ( !i.out.empty() )
            os << " -> " << i.out;

        os << "\n";
        return os;
    }

    inline auto &operator<<( auto &os, const subr_t &s )
    {
        os << "    " << s.name << " = λ";

        if ( !s.input.empty() )
            os << " " << s.input;
        if ( !s.output.empty() )
            os << " -> " << s.output;

        os << "\n    (\n";

        for ( const insn &i : s.body )
            os << i;

        os << "    )\n";
        return os;
    }

    inline auto &operator<<( auto &os, const structure_t &s )
    {
        os << "structure " << s.name << "\n(";

        for ( const subr_t &sub : s.subroutines )
            os << "\n" << sub;

        os << ")\n";
        return os;
    }

    inline auto &print_function_signature( auto &os,
                                           const function_type &type )
    {
        os << "signature " << llvm2ct::function_signature_name( type )
           << "[ F, S, B";

        for ( size_t i = 0; i < type.inputs.size(); ++ i )
            os << ", I" << i;

        for ( size_t i = 0; i < type.outputs.size(); ++ i )
            os << ", O" << i;

        os << " ] : simple[ F, S, B ]\n(\n"
           << "    call ∷ F";

        for ( size_t i = 0; i < type.inputs.size(); ++ i )
            os << " × I" << i;

        os << " → ";

        if ( type.outputs.empty() )
            os << "∅";
        else
            for ( size_t i = 0; i < type.outputs.size(); ++ i )
                os << ( i ? " × " : "" ) << "O" << i;

        os << "\n)\n";
        return os;
    }

    static inline void print_function_builtins( auto &os )
    {
        os << " ]\n(\n"
           << "    fork = builtin_func_fork\n"
           << "    move = builtin_func_move\n"
           << "    push = builtin_func_push\n"
           << "    pop  = builtin_func_pop\n"
           << "    drop = builtin_func_drop\n"
           << "    dup  = builtin_func_dup\n"
           << "    call = builtin_func_call\n\n";
    }

    static inline void print_function_top_bot( auto &os, const std::string &name,
                                               const function_type &type )
    {
        auto print = [ & ]( const std::string &subroutine,
                            const std::string &operation )
        {
            os << "    " << subroutine << " = λ";

            for ( size_t i = 0; i < type.inputs.size(); ++ i )
                os << " " << i;

            if ( !type.outputs.empty() )
            {
                os << " ->";
                for ( size_t i = 0; i < type.outputs.size(); ++ i )
                    os << " out" << i;
            }

            os << "\n    (\n";

            for ( size_t i = 0; i < type.inputs.size(); ++ i )
                os << "        " << builtin_structure_name( type_to_structure( type.inputs[ i ] ) )
                   << " drop " << i << "\n";

            for ( size_t i = 0; i < type.outputs.size(); ++ i )
                os << "        " << builtin_structure_name( type_to_structure( type.outputs[ i ] ) )
                   << " " << operation << " -> out" << i << "\n";

            os << "    )\n\n";
        };

        print( "f_bot", "bot" );
        print( "f_top", "top" );

        os << "    bot = μ -> out\n"
           << "    (\n"
           << "        " << name << " f_bot -> out\n"
           << "    )\n\n"
           << "    top = μ -> out\n"
           << "    (\n"
           << "        " << name << " f_top -> out\n"
           << "    )\n\n";
    }

    static inline void print_function_frame( auto &os, const std::string &name,
                                             const function_type &type )
    {
        os << "    frame = λ A B";

        for ( size_t i = 0; i < type.inputs.size(); ++ i )
            os << " " << i;

        if ( !type.outputs.empty() )
        {
            os << " ->";
            for ( size_t i = 0; i < type.outputs.size(); ++ i )
                os << " out" << i;
        }

        os << "\n    (\n";

        for ( size_t i = 0; i < type.inputs.size(); ++ i )
            os << "        " << builtin_structure_name( type_to_structure( type.inputs[ i ] ) )
               << " fork " << i << " -> " << i << "_1 " << i << "_2\n";

        auto print_call = [ & ]( const char *function, const char *copy )
        {
            os << "        " << name << " call " << function;

            for ( size_t i = 0; i < type.inputs.size(); ++ i )
                os << " " << i << "_" << copy;

            if ( !type.outputs.empty() )
            {
                os << " ->";
                for ( size_t i = 0; i < type.outputs.size(); ++ i )
                    os << " out" << i << "_" << copy;
            }

            os << "\n";
        };

        print_call( "A", "1" );
        print_call( "B", "2" );

        for ( size_t i = 0; i < type.outputs.size(); ++ i )
            os << "        " << builtin_structure_name( type_to_structure( type.outputs[ i ] ) )
               << " join out" << i << "_1 out" << i << "_2 -> out" << i << "\n";
    }

    static inline void print_function_join_opt( auto &os, const std::string &name )
    {
        os << "    )\n\n"
           << "    join = μ a b -> out\n"
           << "    (\n"
           << "        " << name << " frame -> frame\n"
           << "        lambda bind frame a -> partial\n"
           << "        lambda bind partial b -> out\n"
           << "    )\n\n"
           << "    opt = μ condition function -> out\n"
           << "    (\n"
           << "        " << name << " f_bot -> bot\n"
           << "        lambda select condition function bot -> out\n"
           << "    )\n"
           << ")\n";
    }

    inline auto &print_function_structure( auto &os, const function_type &type )
    {
        std::string name = llvm2ct::function_structure_name( type );
        os << "structure " << name << " : " << llvm2ct::function_signature_name( type )
           << "[ func, stck, bool";

        for ( auto input : type.inputs )
            os << ", " << cthu::builtin_type_name( input );

        for ( auto output : type.outputs )
            os << ", " << cthu::builtin_type_name( output );

        print_function_builtins( os );
        print_function_top_bot( os, name, type );
        print_function_frame( os, name, type );
        print_function_join_opt( os, name );

        return os;
    }
}
