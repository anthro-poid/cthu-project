#include "core.hpp"

namespace cthu
{
std::string_view insn::get_structure_name() const
{
    if ( auto *l = std::get_if< l_call >( &call ) )
        return l->structure.name;

    const auto &b = std::get< b_call >( call );
    return b.structure == builtin_structure::function
         ? std::string_view{ b.struct_name }
         : builtin_structure_name( b.structure );
}

std::string_view insn::get_subr_name() const
{
    if ( auto *l = std::get_if< l_call >( &call ) )
        return l->subroutine.name;

    return builtin_operation_name( std::get< b_call >( call ).subroutine );
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
