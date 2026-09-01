#include "debuginfo.hpp"

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IntrinsicInst.h>

namespace llvm2ct
{

llvm::DIType *debug_type( llvm::Value *value )
{
    llvm::SmallVector< llvm::DbgVariableIntrinsic *, 1 > intrinsics;
    llvm::SmallVector< llvm::DbgVariableRecord *, 1 > records;
    llvm::findDbgUsers( intrinsics, value, &records );

    if ( !records.empty() )
        return records.front()->getVariable()->getType();

    if ( !intrinsics.empty() )
        return intrinsics.front()->getVariable()->getType();

    return nullptr;
}
}