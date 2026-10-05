from parser import Program
from symtab import LAMBDA_BASE, MU_BASE, Subr
from builtin import Func


class Executable:

    def __init__( self, run_idx: int, lambdas: list[ Func ],
                  mus: list[ Func ] ) -> None:
        self.run_idx = run_idx
        self.lambdas = lambdas
        self.mus = mus


class Substitution:

    def __init__( self, program: Program ) -> None:
        self.program = program

    def get_struct_code( self, opcode: int ) -> int:
        return ( opcode >> 44 ) & 0xffff

    def get_subr_code( self, opcode: int ) -> int:
        return ( opcode >> 36 ) & 0xff

    def is_extension( self, opcode: int ) -> bool:
        return ( opcode >> 54 ) & 0x3ff == 0x3c

    def substitute_subr( self, subr: Subr ) -> Func:
        substituted: list[ int ] = []

        for instr in subr.instrs:
            if self.is_extension( instr ):
                substituted.append( instr )
                continue

            struct = self.get_struct_code( instr )
            opcode = self.get_subr_code(   instr )

            impl_code = self.program.get_subr_impl_code( struct, opcode )
            if impl_code < MU_BASE:
                impl_code += LAMBDA_BASE

            substituted.append( ( impl_code << 36 ) | ( instr & ( ( 1 << 36 ) - 1 ) ) ) 

        return Func( subr.input, subr.output, substituted )

    def run( self ) -> Executable:
        lambdas = [ self.substitute_subr( subr ) for subr in self.program.lambdas ]
        mus = [ self.substitute_subr( subr ) for subr in self.program.mus ]
        return Executable( self.program.run_idx, lambdas, mus )
