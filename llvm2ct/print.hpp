#pragma once

#include "core.hpp"

#include <llvm/ADT/ArrayRef.h>
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

        for ( const subr_t &sub : s.subroutines )
            os << "\n" << sub;

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

    inline std::string function_type_name( llvm::ArrayRef< llvm::Type * > inputs,
                                           llvm::Type *output )
    {
        std::string name = "f";

        for ( llvm::Type *parameter : inputs )
            name += "_" + function_type_code( parameter );

        name += "__";

        if ( !output->isVoidTy() )
            name += function_type_code( output );

        return name;
    }

    inline std::string function_type_name( llvm::FunctionType *type )
    {
        return function_type_name( type->params(), type->getReturnType() );
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

    inline std::string function_signature_name( size_t inputs, bool has_output )
    {
        return "f" + scripted_number( inputs, false )
                   + scripted_number( has_output ? 1 : 0, true );
    }

    inline std::string function_signature_name( llvm::FunctionType *type )
    {
        return function_signature_name( type->getNumParams(),
                                        !type->getReturnType()->isVoidTy() );
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

    inline auto &print_function_signature( auto &os,
                                            llvm::ArrayRef< llvm::Type * > inputs,
                                            llvm::Type *output )
    {
        std::string name = function_signature_name( inputs.size(), !output->isVoidTy() );
        os << "signature " << name << "[ F, S, B";

        for ( size_t i = 0; i < inputs.size(); ++ i )
            os << ", I" << i;

        if ( !output->isVoidTy() )
            os << ", O";

        os << " ] : simple[ F, S, B ]\n(\n"
           << "    call ∷ F";

        for ( size_t i = 0; i < inputs.size(); ++ i )
            os << " × I" << i;

        os << " → " << ( output->isVoidTy() ? "∅" : "O" );
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
                                               llvm::ArrayRef< llvm::Type * > inputs,
                                               llvm::Type *output )
    {
        auto print = [ & ]( const std::string &subroutine,
                                      const std::string &operation )
        {
            os << "    " << subroutine << " = λ";

            for ( size_t i = 0; i < inputs.size(); ++ i )
                os << " " << i;

            if ( !output->isVoidTy() )
                os << " -> out";

            os << "\n    (\n";

            for ( size_t i = 0; i < inputs.size(); ++ i )
                os << "        " << cthu_type_name( inputs[ i ] )
                   << " drop " << i << "\n";

            if ( !output->isVoidTy() )
                os << "        " << cthu_type_name( output ) << " "
                   << operation << " -> out\n";

            os << "    )\n\n";
        };

        print( "f_bot", "bot" );
        print( "f_top", "top" );

        os << "    bot = λ -> out\n"
           << "    (\n"
           << "        " << name << " f_bot -> out\n"
           << "    )\n\n"
           << "    top = λ -> out\n"
           << "    (\n"
           << "        " << name << " f_top -> out\n"
           << "    )\n\n";
    }

    static inline void print_function_frame( auto &os, const std::string &name,
                                             llvm::ArrayRef< llvm::Type * > inputs,
                                             llvm::Type *output )
    {
        os << "    frame = λ A B";

        for ( size_t i = 0; i < inputs.size(); ++ i )
            os << " " << i;

        if ( !output->isVoidTy() )
            os << " -> out";

        os << "\n    (\n";

        for ( size_t i = 0; i < inputs.size(); ++ i )
            os << "        " << cthu_type_name( inputs[ i ] )
               << " fork " << i << " -> " << i << "_1 " << i << "_2\n";

        auto print_call = []( auto &os, const std::string &name,
                              llvm::ArrayRef< llvm::Type * > inputs,
                              llvm::Type *output, const std::string &function,
                              const std::string &copy )
        {
            os << "        " << name << " call " << function;

            for ( size_t i = 0; i < inputs.size(); ++ i )
                os << " " << i << "_" << copy;

            if ( !output->isVoidTy() )
                os << " -> out" << copy;

            os << "\n";
        };

        print_call( os, name, inputs, output, "A", "1" );
        print_call( os, name, inputs, output, "B", "2" );

        if ( !output->isVoidTy() )
            os << "        " << cthu_type_name( output )
               << " join out1 out2 -> out\n";
    }

    static inline void print_function_join_opt( auto &os, const std::string &name )
    {
        os << "    )\n\n"
           << "    join = λ a b -> out\n"
           << "    (\n"
           << "        " << name << " frame -> frame\n"
           << "        lambda bind frame a -> partial\n"
           << "        lambda bind partial b -> out\n"
           << "    )\n\n"
           << "    opt = λ condition function -> out\n"
           << "    (\n"
           << "        " << name << " f_bot -> bot\n"
           << "        lambda select condition function bot -> out\n"
           << "    )\n"
           << ")\n";
    }

    inline auto &print_function_structure( auto &os,
                                           llvm::ArrayRef< llvm::Type * > inputs,
                                           llvm::Type *output )
    {
        std::string name = function_type_name( inputs, output );
        os << "structure " << name << " : "
           << function_signature_name( inputs.size(), !output->isVoidTy() )
           << "[ func, stck, bool";

        for ( llvm::Type *parameter : inputs )
            os << ", " << cthu_type_name( parameter );

        if ( !output->isVoidTy() )
            os << ", " << cthu_type_name( output );

        print_function_builtins( os );
        print_function_top_bot( os, name, inputs, output );
        print_function_frame( os, name, inputs, output );
        print_function_join_opt( os, name );

        return os;
    }
}
