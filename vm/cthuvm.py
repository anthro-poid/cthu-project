#!/usr/bin/env python3

import sys

from lexer        import Lexer
from parser       import Parser
from substitution import Substitution
from interpret    import Interpret
from builtin      import Bool, Word


def main() -> None:
    args = sys.argv[ 1 : ]
    print_result = False

    if args and args[ 0 ] == "--print-result":
        print_result = True
        args = args[ 1 : ]

    if len( args ) < 2:
        print( "Usage: python3 cthuvm.py [--print-result] "
               "<source.ct> [source.ct ...]" )
        sys.exit( 1 )

    parser = Parser()

    for arg in args:
        with open( arg ) as f:
            tokens = Lexer( f ).tokenize()
            parser.parse( tokens )

    parser.check_undefined_ops()
    program = parser.get_program()
    executable = Substitution( program ).run()

    i = Interpret( executable.run_idx, executable.lambdas, executable.mus )
    i.run()

    if print_result:
        assert len( i.executing.output ) == 1
        result = i.pop( i.executing.output[ 0 ] )

        if type( result ) is Bool:
            print( int( result.get_value() ) )
        elif type( result ) is Word:
            print( result )
        else:
            assert False, "Unprintable result."

    assert i.check_emptiness()


if __name__ == '__main__':
    main()
