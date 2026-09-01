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
}