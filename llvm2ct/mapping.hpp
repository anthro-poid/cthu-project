#pragma once

#include "builtin.hpp"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Value.h>

#include <cstddef>
#include <string>

namespace llvm2ct
{
    unsigned integer_width( llvm::Type *type );
    unsigned integer_width( llvm::Value *value );
    cthu::builtin_structure type_to_structure( llvm::Type *type );
    std::string type_name( llvm::Type *type );
    cthu::builtin_structure arithmetic_structure( bool is_unsigned, unsigned width );
    std::string arithmetic_structure_name( bool is_unsigned, unsigned width );
    cthu::builtin_structure arithmetic_structure( llvm::Value *value );
    std::string value_structure_name( llvm::Value *value );
    std::string function_structure_name( llvm::ArrayRef< llvm::Type * > inputs,
                                         llvm::Type *output );
    std::string function_structure_name( llvm::FunctionType *type );
    std::string function_structure_name( llvm::ArrayRef< llvm::Value * > inputs,
                                         llvm::Type *output );
    std::string function_signature_name( size_t inputs, bool has_output );
    std::string function_signature_name( llvm::FunctionType *type );
}
