from typing import Any, Union


class BuiltinType:
    pass


class W8( BuiltinType ):

    class Bot:
        def __init__( self ) -> None:
            self.size = 8

    class Top:
        def __init__( self ) -> None:
            self.size = 8

    def get_bot( self ) -> Any:
        return W8.Bot()

    def get_top( self ) -> Any:
        return W8.Top()


class W32( BuiltinType ):

    class Bot:
        def __init__( self ) -> None:
            self.size = 32

    class Top:
        def __init__( self ) -> None:
            self.size = 32

    def get_bot( self ) -> Any:
        return W32.Bot()

    def get_top( self ) -> Any:
        return W32.Top()


class Bool( BuiltinType ):

    class Bot:
        def __init__( self ) -> None:
            pass

    class Top:
        def __init__( self ) -> None:
            pass

    def get_bot( self ) -> Any:
        return Bool.Bot()

    def get_top( self ) -> Any:
        return Bool.Top()


class Func( BuiltinType ):

    class Bot:
        def __init__( self, subr: 'Subr' ) -> None:
            self.in_size = len( subr.input )
            self.bots    = [ subr.stack_types[ sid ].get_bot() for sid in subr.output ]

        def __call__( self, vm: Any, params: list[ int ] ) -> None:
            for i in range( self.in_size ):
                vm.pop( params[ i ] )

            for sid, bot in zip( params[ self.in_size : ], self.bots ):
                vm.push( sid, bot )

    class Top:
        def __init__( self, subr: 'Subr' ) -> None:
            self.in_size = len( subr.input )
            self.tops    = [ subr.stack_types[ sid ].get_top() for sid in subr.output ]

        def __call__( self, vm: Any, params: list[ int ] ) -> None:
            for i in range( self.in_size ):
                vm.pop( params[ i ] )

            for sid, top in zip( params[ self.in_size : ], self.tops ):
                vm.push( sid, top )


class Stck( BuiltinType ):
    pass


class Dict( BuiltinType ):
    pass


NAME_TO_BUILTIN: dict[ str, BuiltinType ] = {
    "w₈":   W8(),
    "w₃₂":  W32(),
    "bool": Bool(),
    "func": Func(),
    "stck": Stck(),
    "dict": Dict(),
}


class Subr:

    def __init__( self, subr_code: int ) -> None:
        self.subr_code = subr_code

        self.stack_to_code: dict[ str, int ] = {}

        self.input:  list[ int ] = []
        self.output: list[ int ] = []
        self.instrs: list[ int ] = []

        self.par_args: list[ Any ] = []

        self.stack_types: dict[ int, BuiltinType ] = {}
        self.is_defined = False

    def get_code( self ) -> int:
        return self.subr_code

    def register_stack( self, name: str ) -> int:
        if name in self.stack_to_code:
            return self.stack_to_code[ name ]

        assert len( self.stack_to_code ) < 0x1ff

        self.stack_to_code[ name ] = len( self.stack_to_code )
        return len( self.stack_to_code ) - 1

    def add_input( self, symbols: list[ str ] ) -> None:
        for symbol in symbols:
            self.input.append( self.register_stack( symbol ) )

    def add_output( self, symbols: list[ str ] ) -> None:
        for symbol in symbols:
            self.output.append( self.register_stack( symbol ) )

    def stack_id( self, name: str ) -> int:
        return self.stack_to_code[ name ]

    def add_instr( self, struct: int, opcode: int, params: list[ str ] ) -> None:
        for param in params:
            self.register_stack( param )

        enc = ( struct << 8 ) | opcode
        enc_param = 2

        for i in range( ( ( len( params ) + 1 ) // 6 + 1 ) * 6 - 2 ):
            enc <<= 9

            enc |= 0x1ff if i >= len( params ) else self.stack_id( params[ i ] )
            enc_param += 1

            if enc_param >= 6:
                self.instrs.append( enc )
                enc = 0x3c
                enc_param = 0

    def note_types( self, struct: 'Structure', name: str, outputs: list[ str ] ) -> None:
        resolved = struct.resolve_op_types( name )
        if resolved is None:
            return

        _, out_types = resolved

        for symbol, t in zip( outputs, out_types ):
            sid = self.stack_id( symbol )
            if sid in self.output:
                self.stack_types[ sid ] = t

    def input_types( self ) -> list[ Union[ BuiltinType, None ] ]:
        return [ self.stack_types.get( sid ) for sid in self.input ]

    def output_types( self ) -> list[ Union[ BuiltinType, None ] ]:
        return [ self.stack_types.get( sid ) for sid in self.output ]


class Declaration:

    def __init__( self, name: str, inputs: list[ str ],
                  outputs: list[ str ] ) -> None:
        self.name    = name
        self.inputs  = inputs
        self.outputs = outputs


class Signature:

    def __init__( self, sig_code: int ) -> None:
        self.sig_code = sig_code

        self.types: list[ str ]         = []
        self.decls: list[ Declaration ] = []

    def get_code( self ) -> int:
        return self.sig_code

    def set_types( self, types: list[ str ] ) -> None:
        self.types = types

    def add_decl( self, name: str, ins: list[ str ],
                  outs: list[ str ] ) -> None:
        self.decls.append( Declaration( name, ins, outs ) )

    def add_parent( self, parent: 'Signature', args: list[ str ] ) -> None:
        subst = dict( zip( parent.types, args ) )

        for decl in parent.decls:
            self.add_decl( decl.name,
                            [ subst[ t ] for t in decl.inputs  ],
                            [ subst[ t ] for t in decl.outputs ] )


class Structure:

    def __init__( self, struct_code: int ) -> None:
        self.struct_code = struct_code

        self.subr_name_to_code:      dict[ str, int ] = {}
        self.subr_code_to_impl_code: list[ int ]      = []

        self.signatures: list[ tuple[ Signature, list[ BuiltinType ] ] ] = []

    def get_code( self ) -> int:
        return self.struct_code

    def add_signature( self, signature: Signature, types: list[ BuiltinType ] ) -> None:
        self.signatures.append( ( signature, types ) )

    def resolve_op_types( self, name: str
                         ) -> Union[ tuple[ list[ BuiltinType ], list[ BuiltinType ] ], None ]:
        for signature, args in self.signatures:
            subst = dict( zip( signature.types, args ) )

            for decl in signature.decls:
                if decl.name == name:
                    return ( [ subst[ t ] for t in decl.inputs  ],
                             [ subst[ t ] for t in decl.outputs ] )

        return None

    def has_subr( self, name: str ) -> bool:
        return name in self.subr_name_to_code

    def subr_opcode( self, name: str ) -> int:
        return self.subr_name_to_code[ name ]

    def subr_impl_code( self, _id: Union[ str, int ] ) -> int:
        if isinstance( _id, str ):
            return self.subr_code_to_impl_code[ self.subr_opcode( _id ) ]
        else:
            return self.subr_code_to_impl_code[ _id ]

    def register_subr( self, name: str, impl_code: int ) -> int:
        if name in self.subr_name_to_code:
            return self.subr_opcode( name )

        struct_code = len( self.subr_code_to_impl_code )
        assert struct_code < 0xff

        self.subr_name_to_code[ name ] = struct_code
        self.subr_code_to_impl_code.append( impl_code )

        return struct_code


class Symtab:

    def __init__( self ) -> None:
        self.struct_name_to_code: dict[ str, int ]    = {}
        self.signature_name_to_code: dict[ str, int ] = {}

        self.structures: list[ Structure ] = []
        self.signatures: list[ Signature ] = []

        self.lambdas: list[ Subr ] = []

    def has_structure( self, _id: Union[ str, int ] ) -> bool:
        if isinstance( _id, str ):
            return _id in self.struct_name_to_code
        else:
            return _id < len( self.structures )

    def has_signature( self, _id: Union[ str, int ] ) -> bool:
        if isinstance( _id, str ):
            return _id in self.signature_name_to_code
        else:
            return _id < len( self.signatures )

    def get_structure( self, code: int ) -> Structure:
        return self.structures[ code ]

    def get_signature( self, code: int ) -> Signature:
        return self.signatures[ code ]

    def get_signature_by_name( self, name: str ) -> Signature:
        return self.signatures[ self.signature_name_to_code[ name ] ]

    def get_type_by_name( self, name: str ) -> BuiltinType:
        if name not in NAME_TO_BUILTIN:
            raise RuntimeError( "Unknown type '" + name + "'." )

        return NAME_TO_BUILTIN[ name ]

    def get_lambda( self, code: int ) -> Subr:
        return self.lambdas[ code ]

    def get_subr_code( self, struct: Structure, name: str ) -> int:
        if struct.has_subr( name ):
            return struct.subr_opcode( name )

        return self.register_lambda( struct, name ).get_code()

    def register_structure( self, name: str ) -> Structure:
        if self.has_structure( name ):
            return self.structures[ self.struct_name_to_code[ name ] ]

        assert len( self.structures ) < 0x10000

        self.struct_name_to_code[ name ] = len( self.structures )
        self.structures.append( Structure( len( self.structures ) ) )

        return self.structures[ -1 ]

    def register_lambda( self, struct: Structure, name: str ) -> Subr:
        if struct.has_subr( name ):
            return self.lambdas[ struct.subr_impl_code( name ) ]

        assert len( self.lambdas ) < 0xdff_0000

        lambda_opcode = struct.register_subr( name, len( self.lambdas ) )
        self.lambdas.append( Subr( lambda_opcode ) )

        return self.lambdas[ -1 ]

    def register_builtin( self, struct: Structure, name: str, code: int ) -> None:
        struct.register_subr( name, code )

    def register_type( self, name: str ) -> BuiltinType:
        if name not in NAME_TO_BUILTIN:
            raise RuntimeError( "Unknown type '" + name + "'." )

        return NAME_TO_BUILTIN[ name ]

    def register_signature( self, name: str ) -> Signature:
        if self.has_signature( name ):
            return self.signatures[ self.signature_name_to_code[ name ] ]

        self.signature_name_to_code[ name ] = len( self.signatures )
        self.signatures.append( Signature( len( self.signatures ) ) )

        return self.signatures[ -1 ]