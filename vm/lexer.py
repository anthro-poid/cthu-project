from typing import Union, TextIO
from enum import Enum


class Category( Enum ):
    INVALID  = 1
    COMMENT  = 2
    LAMBDA   = 3
    KWSTRUCT = 4
    KWSIG    = 5
    KWTYPE   = 6
    EOL      = 7
    PUNCT    = 8
    IDENT    = 9
    ARROW    = 10
    BRACKET  = 11
    PAREN    = 12


class Token:

    def __init__( self, lexeme: str, category: Category ) -> None:
        self.lexeme   = lexeme
        self.category = category

    def __repr__( self ) -> str:
        return str( self.category )

    def __str__( self ) -> str:
        return str( self.category )

    def has_category( self, c: Category ) -> bool:
        return c == self.category

    def get_category( self ) -> Category:
        return self.category

    def get_lexeme( self ) -> str:
        return self.lexeme

    @staticmethod
    def build_token( lexeme: str, category: Category ) -> 'Token':
        return Token( lexeme, category )


class Lexer:

    def __init__( self, _file: TextIO ) -> None:
        self.file: str = _file.read()
        self.idx  = 0        

        self.matched: Union[ str, None ] = None

    def eof( self ) -> bool:
        return self.idx >= len( self.file )

    def next( self ) -> None:
        self.idx += 1

    def spaces( self ) -> None:
        while not self.eof() and self.peek().isspace() and self.peek() != '\n':
            self.next() 

    def peek( self ) -> str:
        return self.file[ self.idx ]

    def match( self, string: str ) -> bool:
        stored_idx = self.idx

        for char in string:
            if char != self.peek():
                self.idx = stored_idx
                return False

            self.next()

        return True

    def match_any( self, opts: list[ str ] ) -> bool:
        for opt in opts:
            if self.match( opt ):
                self.matched = opt
                return True

        return False

    def get_match( self ) -> str:
        assert self.matched is not None
        res = self.matched
        self.matched = None
        return res

    def comment( self ) -> Token:
        symbols = []

        while not self.eof() and self.peek() != '\n':
            symbols.append( self.peek() )
            self.next()

        return Token.build_token( "".join( symbols ), Category.COMMENT )

    def is_ident_char( self ) -> bool:
        if self.eof():
            return False

        ch = self.peek()

        return ( ord( ch ) <= 255 and ch.isalnum() ) or \
               ( ord( ch ) >= 0x1d62 and ord( ch ) < 0x1d66 ) or \
               ( ord( ch ) >= 0x2070 and ord( ch ) < 0x20A0 ) or \
               ( ord( ch ) >= 0x00b0 and ord( ch ) < 0x00c0 ) or \
                 ch in [ U'?', U'_', U'∅', U"'" ]

    def identifier( self ) -> Token:
        symbols = []

        while self.is_ident_char():
            symbols.append( self.peek() )
            self.next()

        lexeme = "".join( symbols )

        if lexeme == "type":
            return Token.build_token( lexeme, Category.KWTYPE )
        if lexeme == "signature":
            return Token.build_token( lexeme, Category.KWSIG )
        if lexeme == "structure":
            return Token.build_token( lexeme, Category.KWSTRUCT )

        return Token.build_token( lexeme, Category.IDENT )

    def get_lexeme( self ) -> Token:
        if self.match( '\n' ):
            return Token.build_token( '\n', Category.EOL )

        if self.match( ';' ):
            return self.comment()

        if self.match_any( [ ':', '∷', '×', '=', ',' ] ):
            return Token.build_token( self.get_match(), Category.PUNCT )

        if self.match_any( [ "->", '→' ] ):
            return Token.build_token( self.get_match(), Category.ARROW )

        if self.match_any( [ '(', ')' ] ):
            return Token.build_token( self.get_match(), Category.PAREN )

        if self.match_any( [ '[', ']' ] ):
            return Token.build_token( self.get_match(), Category.BRACKET )

        if self.match( 'λ' ):
            return Token.build_token( 'λ', Category.LAMBDA )

        return self.identifier()

    def tokenize( self ) -> list[ Token ]:
        tokens: list[ Token ] = []

        while not self.eof():
            self.spaces()
            tokens.append( self.get_lexeme() )

        return tokens
