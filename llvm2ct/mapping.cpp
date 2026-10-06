#include "mapping.hpp"

#include "builtin.hpp"
#include "debuginfo.hpp"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/DebugInfoMetadata.h>

#include <cassert>
#include <string>
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

cthu::b_strct type_to_structure( llvm::Type *type )
{
    if ( type->isIntegerTy() )
        switch ( integer_width( type ) )
        {
            case 1:  return cthu::b_strct::boolean;
            case 8:  return cthu::b_strct::w8;
            case 32: return cthu::b_strct::w32;
            default: __builtin_unreachable();
        }
    
    __builtin_unreachable();
}

cthu::b_strct type_to_structure( cthu::b_type type )
{
    switch ( type )
    {
        case cthu::b_type::w8:         return cthu::b_strct::w8;
        case cthu::b_type::w32:        return cthu::b_strct::w32;
        case cthu::b_type::boolean: return cthu::b_strct::boolean;
        default: __builtin_unreachable();
    }
}

cthu::b_type type_to_builtin( llvm::Type *type )
{
    if ( type->isPointerTy() )
        return cthu::b_type::ref;

    if ( type->isIntegerTy() )
        switch ( integer_width( type ) )
        {
            case 1:  return cthu::b_type::boolean;
            case 8:  return cthu::b_type::w8;
            case 32: return cthu::b_type::w32;
            default: __builtin_unreachable();
        }

    __builtin_unreachable();
}

cthu::b_strct arithmetic_structure( bool is_unsigned, unsigned width )
{
    if ( width == 1 )
        return cthu::b_strct::boolean;

    return width == 8 ? ( is_unsigned ? cthu::b_strct::u8  : cthu::b_strct::i8  )
                      : ( is_unsigned ? cthu::b_strct::u32 : cthu::b_strct::i32 );
}

cthu::b_strct arithmetic_structure( llvm::Value *value )
{
    if ( value->getType()->isIntegerTy( 1 ) )
        return cthu::b_strct::boolean;

    bool is_unsigned = false;

    if ( auto *type = debug_type( value ) )
        if ( auto *basic = llvm::dyn_cast< llvm::DIBasicType >( type ) )
            is_unsigned = basic->getEncoding() == llvm::dwarf::DW_ATE_unsigned;

    return arithmetic_structure( is_unsigned, integer_width( value ) );
}

function_type::function_type( llvm::ArrayRef< llvm::Type * > input_types,
                              llvm::Type *output )
{
    for ( llvm::Type *input : input_types )
        inputs.push_back( type_to_builtin( input ) );

    if ( !output->isVoidTy() )
        outputs.push_back( type_to_builtin( output ) );

}

function_type::function_type( llvm::ArrayRef< llvm::Value * > input_values,
                              llvm::Type *output )
{
    for ( llvm::Value *input : input_values )
        inputs.push_back( type_to_builtin( input->getType() ) );

    if ( !output->isVoidTy() )
        outputs.push_back( type_to_builtin( output ) );

}

std::string function_structure_name( const function_type &type )
{
    std::string name = "f";

    for ( cthu::b_type input : type.inputs )
        name += "_" + std::string{ cthu::builtin_type_name( input ) };

    name += "__";

    for ( size_t i = 0; i < type.outputs.size(); ++ i )
        name += ( i ? "_" : "" ) + std::string{ cthu::builtin_type_name( type.outputs[ i ] ) };

    return name;
}

std::string function_signature_name( size_t inputs, size_t outputs )
{
    return "f" + scripted_number( inputs, false )
               + scripted_number( outputs, true );
}

std::string function_signature_name( const function_type &type )
{
    return function_signature_name( type.inputs.size(), type.outputs.size() );
}
}
