#pragma once

#include "builtin.hpp"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Value.h>

#include <cstddef>
#include <string>
#include <vector>

namespace llvm2ct
{
    struct function_type
    {
        std::vector< cthu::b_type > inputs;
        std::vector< cthu::b_type > outputs;

        function_type( llvm::ArrayRef< llvm::Type * > inputs,
                       llvm::Type *output );
        function_type( llvm::ArrayRef< llvm::Value * > inputs,
                       llvm::Type *output );
    };

    unsigned integer_width( llvm::Type *type );
    unsigned integer_width( llvm::Value *value );
    cthu::b_type type_to_builtin( llvm::Type *type );
    cthu::b_strct type_to_structure( cthu::b_type type );
    cthu::b_strct type_to_structure( llvm::Type *type );
    std::string type_name( llvm::Type *type );
    cthu::b_strct arithmetic_structure( bool is_unsigned, unsigned width );
    cthu::b_strct arithmetic_structure( llvm::Value *value );
    std::string function_structure_name( const function_type &type );
    std::string function_signature_name( size_t inputs, size_t outputs );
    std::string function_signature_name( const function_type &type );
}
