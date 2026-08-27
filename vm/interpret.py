from symtab  import Subr
from typing  import Any
from builtin import exec_op


class Interpret:

    def __init__( self, main: int, subrs: list[ Subr ] ) -> None:
        self.stacks: list[ list[ Any ] ] = [ [] for _ in range( 511 ) ]
        self.subrs = subrs
        self.executing = subrs[ main ]
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
        return 0x100_0000 <= ( ( instr >> 36 ) & 0xffff ) < 0xeff_0000

    def is_physical( self, instr: int ) -> bool:
        return 0xeff_0000 <= ( instr >> 36 ) < 0xf00_0000

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
            else:
                self.push( params[ 0 ], self.subrs[ code - 0x100_0000 ] )

            self.pc += 1

    def check_emptiness( self ) -> bool:
        for stck in self.stacks:
            if len( stck ) != 0:
                return False

        return True
