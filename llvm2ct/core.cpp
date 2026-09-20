#include "core.hpp"

namespace cthu
{
const std::string &insn::get_structure_name() const
{
    if ( auto *l = std::get_if< l_call >( &call ) )
        return l->structure.name;
    else
        return std::get< b_call >( call ).struct_name;
}

const std::string &insn::get_subr_name() const
{
    if ( auto *l = std::get_if< l_call >( &call ) )
        return l->subroutine.name;
    else
        return std::get< b_call >( call ).subr_name;
}

subr_ptr structure_t::add_subroutine( llvm::BasicBlock *block )
{
    subroutines.emplace_back( block->hasName() ? block->getName().str()
                            : "b" + std::to_string( next_subr_id ++ ) );
    return &subroutines.back();
}

structure_ptr module_t::add_structure( llvm::Function *function )
{
    structures.emplace_back( function->hasName() ? function->getName().str()
                            : "s" + std::to_string( next_struct_id ++ ) );
    return &structures.back();
}
}
