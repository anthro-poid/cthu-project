#pragma once

#include "core.hpp"

namespace cthu
{
    inline auto &operator<<( auto &os, const std::vector< uint16_t > &stacks )
    {
        for ( size_t i = 0; i < stacks.size(); ++ i )
            os << ( i ? " " : "" ) << stacks[ i ];

        return os;
    }

    inline auto &operator<<( auto &os, const insn &i )
    {
        os << "        " << i.get_structure_name() << " " << i.get_subr_name();

        if ( !i.in.empty() )
            os << " " << i.in;

        if ( !i.out.empty() )
            os << " -> " << i.out;

        os << "\n";

        return os;
    }

    inline auto &operator<<( auto &os, const subr_t &s )
    {
        os << "    " << s.name << " = λ";

        if ( !s.output.empty() )
            os << " -> " << s.output;

        os << "\n    (\n";

        for ( const insn &i : s.body )
            os << i;

        os << "    )\n";

        return os;
    }

    inline auto &operator<<( auto &os, const structure_t &s )
    {
        os << "structure " << s.name << "\n(";

        for ( subr_ptr sub : s.subroutines )
            os << "\n" << *sub;

        os << ")\n";

        return os;
    }
}
