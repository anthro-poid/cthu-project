#pragma once

#include "core.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace cthu
{
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

        for ( subr_ptr sub : s.subroutines )
            os << "\n" << *sub;

        os << ")\n";

        return os;
    }

    inline std::string function_type_code( llvm::Type *type )
    {
        if ( type->isIntegerTy( 1 ) )
            return "b";
        if ( type->isIntegerTy( 8 ) )
            return "w8";
        if ( type->isIntegerTy( 32 ) )
            return "w32";

        assert( false && "only bool/8/32-bit function arguments are supported so far" );
        return {};
    }

    inline std::string function_type_name( llvm::FunctionType *type )
    {
        std::string name = "f";

        for ( llvm::Type *parameter : type->params() )
            name += "_" + function_type_code( parameter );

        name += "__";

        if ( !type->getReturnType()->isVoidTy() )
            name += function_type_code( type->getReturnType() );

        return name;
    }

    inline std::string scripted_number( size_t value, bool superscript )
    {
        static const std::string subscript[] =
            { "₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉" };
        static const std::string superscript_digits[] =
            { "⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹" };
        const std::string *digits = superscript ? superscript_digits : subscript;
        std::string number;

        do
        {
            number.insert( 0, digits[ value % 10 ] );
            value /= 10;
        }
        while ( value != 0 );

        return number;
    }

    inline std::string function_signature_name( llvm::FunctionType *type )
    {
        size_t outputs = type->getReturnType()->isVoidTy() ? 0 : 1;
        return "f" + scripted_number( type->getNumParams(), false )
                   + scripted_number( outputs, true );
    }

    inline std::string cthu_type_name( llvm::Type *type )
    {
        if ( type->isIntegerTy( 1 ) )
            return "bool";
        if ( type->isIntegerTy( 8 ) )
            return "w₈";
        if ( type->isIntegerTy( 32 ) )
            return "w₃₂";

        assert( false && "only bool/8/32-bit function arguments are supported so far" );
        return {};
    }

    inline auto &print_function_signature( auto &os, llvm::FunctionType *type )
    {
        std::string name = function_signature_name( type );
        os << "signature " << name << "[ F, S, B";

        for ( size_t i = 0; i < type->getNumParams(); ++ i )
            os << ", I" << i;

        if ( !type->getReturnType()->isVoidTy() )
            os << ", O";

        os << " ] : simple[ F, S, B ]\n(\n"
           << "    call ∷ F";

        for ( size_t i = 0; i < type->getNumParams(); ++ i )
            os << " × I" << i;

        os << " → " << ( type->getReturnType()->isVoidTy() ? "∅" : "O" );
        os << "\n)\n";
        return os;
    }

    inline auto &print_function_structure( auto &os, llvm::FunctionType *type )
    {
        std::string name = function_type_name( type );
        os << "structure " << name << " : " << function_signature_name( type )
           << "[ func, stck, bool";

        for ( llvm::Type *parameter : type->params() )
            os << ", " << cthu_type_name( parameter );

        if ( !type->getReturnType()->isVoidTy() )
            os << ", " << cthu_type_name( type->getReturnType() );

        os << " ]\n(\n"
           << "    join = builtin_func_join\n"
           << "    bot  = builtin_func_bot\n"
           << "    top  = builtin_func_top\n"
           << "    opt  = builtin_func_opt\n\n"
           << "    fork = builtin_func_fork\n"
           << "    move = builtin_func_move\n"
           << "    push = builtin_func_push\n"
           << "    pop  = builtin_func_pop\n\n"
           << "    drop = builtin_func_drop\n\n"
           << "    dup  = builtin_func_dup\n\n"
           << "    call = builtin_func_call\n"
           << ")\n";
        return os;
    }
}
