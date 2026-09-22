#include "mapping.hpp"

#include "builtin.hpp"
#include "debuginfo.hpp"

#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/DebugInfoMetadata.h>

#include <cassert>
#include <string_view>

namespace llvm2ct
{
namespace
{
    std::string scripted_number( size_t value, bool up )
    {
        static constexpr std::string_view subscript[] =
            { "₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉" };
        static constexpr std::string_view superscript[] =
            { "⁰", "¹", "²", "³", "⁴", "⁵", "⁶", "⁷", "⁸", "⁹" };

        const auto &digits = up ? superscript : subscript;
        std::string number;

        do
        {
            number.insert( 0, std::string{ digits[ value % 10 ] } );
            value /= 10;
        }
        while ( value != 0 );

        return number;
    }
}

unsigned integer_width( llvm::Type *type )
{
    auto *integer = llvm::dyn_cast< llvm::IntegerType >( type );
    assert( integer && "not an integer type" );
    return integer->getIntegerBitWidth();
}

unsigned integer_width( llvm::Value *value )
{
    return integer_width( value->getType() );
}

cthu::builtin_structure type_to_structure( llvm::Type *type )
{
    if ( type->isIntegerTy() )
        switch ( integer_width( type ) )
        {
            case 1:  return cthu::builtin_structure::boolean;
            case 8:  return cthu::builtin_structure::w8;
            case 32: return cthu::builtin_structure::w32;
            default: __builtin_unreachable();
        }
    
    __builtin_unreachable();
}

std::string type_name( llvm::Type *type )
{
    return std::string{ cthu::builtin_structure_name( type_to_structure( type ) ) };
}

cthu::builtin_structure arithmetic_structure( bool is_unsigned, unsigned width )
{
    if ( width == 1 )
        return cthu::builtin_structure::boolean;

    return width == 8
        ? ( is_unsigned ? cthu::builtin_structure::u8 : cthu::builtin_structure::i8 )
        : ( is_unsigned ? cthu::builtin_structure::u32 : cthu::builtin_structure::i32 );
}

cthu::builtin_structure arithmetic_structure( llvm::Value *value )
{
    if ( value->getType()->isIntegerTy( 1 ) )
        return cthu::builtin_structure::boolean;

    bool is_unsigned = false;

    if ( auto *type = debug_type( value ) )
        if ( auto *basic = llvm::dyn_cast< llvm::DIBasicType >( type ) )
            is_unsigned = basic->getEncoding() == llvm::dwarf::DW_ATE_unsigned;

    return arithmetic_structure( is_unsigned, integer_width( value ) );
}

std::string arithmetic_structure_name( bool is_unsigned, unsigned width )
{
    return std::string{ cthu::builtin_structure_name(
        arithmetic_structure( is_unsigned, width ) ) };
}

std::string function_structure_name( llvm::ArrayRef< llvm::Type * > inputs,
                                     llvm::Type *output )
{
    auto function_type_name = []( llvm::Type *type )
    {
        return type->isIntegerTy( 1 ) ? "b" : type_name( type );
    };
    std::string name = "f";

    for ( llvm::Type *input : inputs )
        name += "_" + function_type_name( input );

    name += "__";

    if ( !output->isVoidTy() )
        name += function_type_name( output );

    return name;
}

std::string function_structure_name( llvm::FunctionType *type )
{
    return function_structure_name( type->params(), type->getReturnType() );
}

std::string function_structure_name( llvm::ArrayRef< llvm::Value * > inputs,
                                     llvm::Type *output )
{
    llvm::SmallVector< llvm::Type * > types;

    for ( llvm::Value *input : inputs )
        types.push_back( input->getType() );

    return function_structure_name( types, output );
}

std::string function_signature_name( size_t inputs, bool has_output )
{
    return "f" + scripted_number( inputs, false )
               + scripted_number( has_output ? 1 : 0, true );
}

std::string function_signature_name( llvm::FunctionType *type )
{
    return function_signature_name( type->getNumParams(),
                                    !type->getReturnType()->isVoidTy() );
}
}
