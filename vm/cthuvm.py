#!/usr/bin/env python3

import sys

from lexer        import Lexer, Token
from parser       import Parser
from substitution import Substitution
from interpret    import Interpret


def main() -> None:
    args = sys.argv[ 1 : ]
    print_result = False

    if args and args[ 0 ] == "--print-result":
        print_result = True
        args = args[ 1 : ]

    if len( args ) != 3:
        print( "Usage: python3 cthuvm.py [--print-result] "
               "<prelude.ct> <builtins.ct> <source.ct>" )
        sys.exit( 1 )

    prelude_tokens: list[ Token ] = []
    builtin_tokens: list[ Token ] = []
    source_tokens:  list[ Token ] = []

    with open( args[ 0 ] ) as f:
        prelude_tokens = Lexer( f ).tokenize()

    with open( args[ 1 ] ) as f:
        builtin_tokens = Lexer( f ).tokenize()

    with open( args[ 2 ] ) as f:
        source_tokens = Lexer( f ).tokenize()

    parser = Parser()
    parser.parse( prelude_tokens )
    parser.parse( builtin_tokens )
    parser.parse( source_tokens )
    parser.check_undefined_ops()

    program = parser.get_program()
    executable = Substitution( program ).run()

    i = Interpret( executable.run_idx, executable.lambdas )
    i.run()

    if print_result:
        assert len( i.executing.output ) == 1
        result = i.pop( i.executing.output[ 0 ] )
        print( int( result ) if isinstance( result, bool ) else result )

    assert i.check_emptiness()


if __name__ == '__main__':
    main()
