from symtab  import BUILTIN_BASE, LAMBDA_BASE, MU_BASE, Subr
from typing  import Any, Union
from builtin import exec_op


class Interpret:

    def __init__( self, main: int, lambdas: list[ Subr ], mus: list[ Subr ] ) -> None:
        self.stacks: list[ list[ Any ] ] = [ [] for _ in range( 511 ) ]
        self.lambdas = lambdas
        self.mus = mus
        self.executing = lambdas[ main ]
        self.pc = 0
        self.map: dict[ int, int ] = {}

    def end( self, diff: int = 0 ) -> bool:
        return self.pc + diff >= len( self.executing.instrs )

    def get_instr( self, diff: int = 0 ) -> int:
        return self.executing.instrs[ self.pc + diff ]

    def pop( self, sid: int ) -> Any:
        assert sid < 511
        return self.stacks[ self.map.get( sid, sid ) ].pop()

    def push( self, sid: int, value: Any ) -> None:
        assert sid < 511
        self.stacks[ self.map.get( sid, sid ) ].append( value )

    def is_lambda( self, instr: int ) -> bool:
        code = instr >> 36
        return LAMBDA_BASE <= code < MU_BASE

    def is_mu( self, instr: int ) -> bool:
        code = instr >> 36
        return MU_BASE <= code < BUILTIN_BASE

    def is_physical( self, instr: int ) -> bool:
        return BUILTIN_BASE <= ( instr >> 36 ) < 0xf00_0000

    def _permute( self, actual: list[ int ], formal: list[ int ] ) -> dict[ int, int ]:
        result: dict[ int, int ] = {}
        used: set[ int ] = set()

        for caller, callee in zip( actual, formal ):
            result[ callee ] = self.map.get( caller, caller )
            used.add( self.map.get( caller, caller ) )

        for caller in actual:
            if self.map.get( caller, caller ) not in result:
                for callee in formal:
                    if callee not in used:
                        result[ self.map.get( caller, caller ) ] = callee
                        used.add( callee )
                        break

        return result

    def call( self, subr: Subr, actual: list[ int ], pargs: list[ Any ] = [] ) -> None:
        stored_pc        = self.pc
        stored_executing = self.executing
        stored_mapping   = self.map

        self.pc        = 0
        self.executing = subr
        self.map       = self._permute( actual, subr.input[ len( pargs ) : ] + subr.output )

        for arg, sid in zip( pargs, subr.input ):
            self.push( sid, arg )

        self.run()

        self.pc        = stored_pc
        self.executing = stored_executing
        self.map       = stored_mapping

    def is_extend( self, instr: int ) -> bool:
        return ( ( instr >> 54 ) & 0x3ff ) == 0x3c 

    def next_is_extend( self ) -> bool:
        return not self.end( 1 ) and self.is_extend( self.get_instr( 1 ) )

    def get_operand( self, instr: int, i: int ) -> int:
        return ( instr >> ( 9 * i ) ) & 511

    def get_operands( self, instr: int ) -> list[ int ]:
        operands: list[ int ] = []
        idx = 3

        while True:
            if idx == -1:
                if not self.next_is_extend():
                    break

                self.pc += 1
                instr = self.get_instr()
                idx = 5

            op = self.get_operand( instr, idx )
            if op == 511:
                break

            operands.append( op )
            idx -= 1

        return operands

    def run( self ) -> None:
        while not self.end():
            instr = self.get_instr()

            assert not self.is_extend( instr )

            code   = instr >> 36
            params = self.get_operands( instr )

            if self.is_physical( instr ):
                exec_op( self, code, params )
            elif self.is_mu( instr ):
                self.call( self.mus[ code - MU_BASE ], params )
            else:
                assert self.is_lambda( instr )
                self.push( params[ 0 ], self.lambdas[ code - LAMBDA_BASE ] )

            self.pc += 1

    def check_emptiness( self ) -> bool:
        for stck in self.stacks:
            if len( stck ) != 0:
                return False

        return True
