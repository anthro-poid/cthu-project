from typing  import Union
from lexer   import Category, Token
from symtab  import Subr, Structure, Symtab, Signature
from builtin import name_to_code


class Program:

    def __init__( self, symtab: Symtab, run_idx: int ) -> None:
        self.structures = symtab.structures
        self.lambdas = symtab.lambdas
        self.run_idx = run_idx

    def get_structure( self, code: int ) -> Structure:
        return self.structures[ code ]

    def get_subr_impl_code( self, scode: int, opcode: int ) -> int:
        return self.get_structure( scode ).subr_code_to_impl_code[ opcode ]


class Parser:

    def __init__( self ) -> None:
        self.idx     = 0
        self.tokens: list[ Token ]  = []
        self.symtab  = Symtab()
        self.run_idx = 0

    def eof( self ) -> bool:
        return self.idx >= len( self.tokens )

    def next( self ) -> None:
        self.idx += 1

    def peek( self ) -> Token:
        return self.tokens[ self.idx ]

    def advance( self ) -> Token:
        token = self.tokens[ self.idx ]
        self.next()
        return token

    def match( self, cat: Category, lex: Union[ str, None ] = None ) -> bool:
        t = self.peek()

        if t.get_category() == cat and ( lex is None or t.get_lexeme() == lex ):
            self.next()
            return True

        return False

    def skip( self ) -> None:
        while not self.eof() and \
                self.peek().get_category() in [ Category.EOL, Category.COMMENT ]:
            self.next()

    def consume( self, c: Category, lex: Union[ str, None ] = None ) -> None:
        if self.eof():
            raise RuntimeError( "Unexpected end-of-file." )

        t = self.peek()

        if not t.has_category( c ) or ( lex is not None and t.get_lexeme() != lex ):
            raise RuntimeError( "Expected '" + str( c ) + ", got '" + str( t.get_category() ) + "'." )

        self.next()
        self.skip()

    def identifier( self ) -> str:
        t = self.advance()

        if t.get_category() != Category.IDENT:
            raise RuntimeError( "Expected identifier, got " + str( t.get_category() ) )

        return t.get_lexeme()

    def identifiers( self ) -> list[ str ]:
        res: list[ str ] = []

        while self.peek().has_category( Category.IDENT ):
            res.append( self.identifier() )

        self.skip()
        return res

    def parse_instr( self, par_subr: Subr ) -> None:
        struct = self.symtab.register_structure( self.identifier() )
        name   = self.identifier()
        subr   = self.symtab.get_subr_code( struct, name )

        inputs  = self.identifiers()
        outputs = []

        if self.match( Category.ARROW ):
            outputs = self.identifiers()

        par_subr.add_instr( struct.get_code(), subr, inputs + outputs )
        par_subr.note_types( struct, name, outputs )

    def parse_subr( self, struct: Structure, in_main: bool ) -> None:
        name = self.identifier()

        self.consume( Category.PUNCT, '=' )

        if not self.match( Category.LAMBDA ):
            self.symtab.register_builtin( struct, name, name_to_code[ self.identifier() ] )
            return
            
        subr = self.symtab.register_lambda( struct, name )
        subr.is_defined = True
        subr.add_input( self.identifiers() )

        if self.match( Category.ARROW ):
            subr.add_output( self.identifiers() )

        self.consume( Category.PAREN, '(' )

        while not self.match( Category.PAREN, ')' ):
            self.parse_instr( subr )

        if in_main and name == "run":
            self.run_idx = struct.subr_impl_code( name )

    def parse_subrs( self, struct: Structure, in_main: bool ) -> None:
        while not self.match( Category.PAREN, ')' ):
            self.parse_subr( struct, in_main )
            self.skip()

    def parse_structure( self ) -> None:
        name = self.identifier()
        self.skip()

        implements: list[ tuple[ Signature, list[ str ] ] ] = []

        if self.match( Category.PUNCT, ':' ):
            self.skip()
            implements = self.parse_signature_refs()

        self.consume( Category.PAREN, '(' )

        struct = self.symtab.register_structure( name )

        for signature, args in implements:
            struct.add_signature( signature,
                [ self.symtab.get_type_by_name( t ) for t in args ] )

        self.parse_subrs( struct, name == "main" )

    def parse_types( self ) -> list[ str ]:
        names: list[ str ] = []

        self.consume( Category.BRACKET, '[' )

        while not self.match( Category.BRACKET, ']' ):
            names.append( self.identifier() )
            self.skip()
            self.match( Category.PUNCT, ',' )
            self.skip()

        return names

    def parse_product( self ) -> list[ str ]:
        if self.peek().get_lexeme() == '∅':
            self.next()
            self.skip()
            return []

        names = [ self.identifier() ]

        while self.match( Category.PUNCT, '×' ):
            names.append( self.identifier() )

        self.skip()
        return names

    def parse_decls( self, signature: Signature ) -> None:
        while not self.match( Category.PAREN, ')' ):
            name = self.identifier()
            self.skip()

            self.consume( Category.PUNCT, '∷' )
            inputs = self.parse_product()

            self.consume( Category.ARROW )
            outputs = self.parse_product()

            signature.add_decl( name, inputs, outputs )

    def parse_signature_refs( self ) -> list[ tuple[ Signature, list[ str ] ] ]:
        refs: list[ tuple[ Signature, list[ str ] ] ] = []

        while True:
            signature = self.symtab.get_signature_by_name( self.identifier() )
            args      = self.parse_types()

            refs.append( ( signature, args ) )
            self.skip()

            if not self.match( Category.PUNCT, ',' ):
                break

            self.skip()

        return refs

    def parse_signature( self ) -> None:
        name = self.identifier()
        self.skip()

        stypes = self.parse_types()
        self.skip()

        signature = self.symtab.register_signature( name )
        signature.set_types( stypes )

        if self.match( Category.PUNCT, ':' ):
            self.skip()

            for parent, args in self.parse_signature_refs():
                signature.add_parent( parent, args )

        self.consume( Category.PAREN, '(' )
        self.parse_decls( signature )

    def parse( self, tokens: list[ Token ] ) -> None:
        self.idx     = 0
        self.tokens  = tokens

        while not self.eof():
            if self.match( Category.COMMENT ) or self.match( Category.EOL ):
                continue

            if self.match( Category.KWTYPE ):
                self.symtab.register_type( self.identifier() )

            elif self.match( Category.KWSIG ):
                self.parse_signature()

            elif self.match( Category.KWSTRUCT ):
                self.parse_structure()

            else:
                raise RuntimeError( "Invalid symbol '" + str( self.peek() ) + "' used." )

            self.skip()

    def check_undefined_ops( self ) -> None:
        for struct_name, struct_code in self.symtab.struct_name_to_code.items():
            struct = self.symtab.get_structure( struct_code )

            for op_name, local_opcode in struct.subr_name_to_code.items():
                impl_code = struct.subr_code_to_impl_code[ local_opcode ]

                if impl_code < 0xeff_0000 and not self.symtab.get_lambda( impl_code ).is_defined:
                    raise RuntimeError( "Operation '" + op_name + "' on structure '" +
                                         struct_name + "' is referenced but never defined." )

    def get_program( self ) -> Program:
        return Program( self.symtab, self.run_idx )
