#pragma once

#include "core.hpp"

#include <llvm/IR/Function.h>

#include <map>
#include <string>

/* Each llvm::Function becomes one structure_t, and each of its BasicBlocks
 * becomes one subroutine (subr_t) registered in that structure — a
 * branch between blocks compiles to a call between subroutines.
 *
 * structure_t/subr_t carry a name purely for pretty-printing, taken from
 * the LLVM side when it has one; unnamed values (basic blocks routinely have
 * no name) get one assigned from a global counter (f0, f1, ... / b0, b1, ...)
 * the first time they're looked up. */

namespace cthu
{
    struct symtab
    {
        std::map< llvm::Function *,   structure_t > structures;
        std::map< llvm::BasicBlock *, subr_t      > subroutines;

        uint64_t next_function_id = 0;
        uint64_t next_block_id    = 0;

        auto get( auto *elem, auto &to, auto pref, auto &counter )
        {
            auto [ it, inserted ] = to.try_emplace( elem );

            if ( inserted )
                it->second.name = elem->hasName() ? elem->getName().str()
                                : pref + std::to_string( counter ++ );

            return std::tuple{ it, inserted };
        }

        structure_ref get_structure( llvm::Function *function )
        {
            auto [ it, inserted ] = get( function, structures, "f", next_function_id );
            return it->second;
        }

        subr_ref get_subroutine( llvm::BasicBlock *block )
        {
            auto [ it, ins ] = get( block, subroutines, "b", next_block_id );

            if ( ins )
                get_structure( block->getParent() ).subroutines.push_back( &it->second );

            return it->second;
        }
    };
}
