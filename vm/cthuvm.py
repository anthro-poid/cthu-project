#!/usr/bin/env python3

import sys

from lexer        import Lexer, Token
from parser       import Parser
from substitution import Substitution
from interpret    import Interpret


def main() -> None:
    if len( sys.argv ) < 3:
        print( "Usage: python3 cthuvm.py <prelude.ct> <builtins.ct> <source.ct>" ) 
        sys.exit( 1 )

    prelude_tokens: list[ Token ] = []
    builtin_tokens: list[ Token ] = []
    source_tokens:  list[ Token ] = []

    with open( sys.argv[ 1 ] ) as f:
        prelude_tokens = Lexer( f ).tokenize()

    with open( sys.argv[ 2 ] ) as f:
        builtin_tokens = Lexer( f ).tokenize()

    with open( sys.argv[ 3 ] ) as f:
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

    assert i.check_emptiness()


if __name__ == '__main__':
    main()
