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
        module_t module;
        std::map< llvm::Function *,   structure_ptr > structures;
        std::map< llvm::BasicBlock *, subr_ptr      > subroutines;

        structure_ref get_structure( llvm::Function *function )
        {
            if ( structures.contains( function ) )
                return *structures[ function ];

            structure_ptr structure = module.add_structure( function );
            structures.try_emplace( function, structure );
            return *structure;
        }

        subr_ref get_subroutine( llvm::BasicBlock *block )
        {
            if ( subroutines.contains( block ) )
                return *subroutines[ block ];

            structure_ref structure = get_structure( block->getParent() );
            subr_ptr subroutine = structure.add_subroutine( block );
            subroutines.try_emplace( block, subroutine );
            return *subroutine;
        }

        subr_ref create_subroutine( llvm::Function *function )
        {
            auto &structure = get_structure( function );
            return *structure.create_subroutine();
        }
    };
}
