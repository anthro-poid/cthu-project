#pragma once

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Value.h>

namespace llvm2ct
{
// Resolves the source-level type of an operand from LLVM debug info (dbg.value/
// dbg.declare or their non-instruction DbgVariableRecord equivalent), which
// carries information plain IR types don't (signedness, struct layout, ...).
// Returns nullptr if no debug-variable metadata refers to this value, e.g. it
// is a plain SSA temporary or the module wasn't compiled with -g.
llvm::DIType *debug_type( llvm::Value *value );
}