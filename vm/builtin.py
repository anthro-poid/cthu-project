from typing import Any, TYPE_CHECKING

if TYPE_CHECKING:
    from interpret import Interpret


class Func:

    def __init__( self, inputs: list[ int ],
                  outputs: list[ int ],
                  instrs: list[ int ] ) -> None:
        self.input  = inputs
        self.output = outputs
        self.instrs = instrs

    def join( self, _ : 'Func' ) -> None:
        assert False, "Calling Func.join from interpreter isn't supported yet."


class Word:

    def __init__( self, value: int, size: int ) -> None:
        self.size  = size
        self.value = value & ( ( 1 << size ) - 1 )

    def __int__( self ) -> int:
        return self.value

    def __str__( self ) -> str:
        return str( self.value )

    def __eq__( self, other: object ) -> bool:
        assert type( other ) is Word
        assert self.size == other.size
        return self.value == other.value

    def signed( self ) -> int:
        mod = 2 ** self.size
        num = self.value % mod
        return num if num < mod // 2 else num - mod

    def get_value( self ) -> int:
        return self.value

    def create_top( self ) -> 'WordTop':
        return WordTop( self.size )

    def create_bot( self ) -> 'WordBot':
        return WordBot( self.size )

    def join( self, other: 'Word' ) -> 'Word':
        if isinstance( other, ( WordTop, WordBot ) ):
            return other.join( self )
        return self if self == other else WordTop( self.size )


class WordBot( Word ):

    def __init__( self, size: int ) -> None:
        self.size = size

    def join( self, other: Word ) -> Word:
        return other


class WordTop( Word ):

    def __init__( self, size: int ) -> None:
        self.size = size

    def join( self, _: Word ) -> Word:
        return self


class Bool:

    def __init__( self, value: bool ) -> None:
        self.value = value

    def __eq__( self, other: object ) -> bool:
        assert type( other ) is Bool
        return self.value == other.value

    def get_value( self ) -> bool:
        return self.value

    def join( self, other: 'Bool' ) -> 'Bool':
        if isinstance( other, ( BoolTop, BoolBot ) ):
            return other.join( self )
        return self if self == other else BoolTop()

    def create_top( self ) -> 'BoolTop':
        return BoolTop()

    def create_bot( self ) -> 'BoolBot':
        return BoolBot()


class BoolBot( Bool ):

    def __init__( self ) -> None:
        return

    def join( self, other: Bool ) -> Bool:
        return other


class BoolTop( Bool ):

    def __init__( self ) -> None:
        return

    def join( self, _: Bool ) -> Bool:
        return self


class Stck:

    def __init__( self ) -> None:
        self.stack: list[ Any ] = []

    def push( self, value: Any ) -> None:
        self.stack.append( value )

    def pop( self ) -> Any:
        return self.stack.pop()

    def join( self, other: 'Stck' ) -> 'Stck':
        assert False

    def create_top( self ) -> 'StckTop':
        return StckTop()

    def create_bot( self ) -> 'StckBot':
        return StckBot()


class StckBot( Stck ):

    def join( self, other: Stck ) -> Stck:
        assert False


class StckTop( Stck ):

    def join( self, _: Stck ) -> Stck:
        assert False


class Dict:

    def __init__( self ) -> None:
        self.dict: dict[ Any, Any ] = {}

    def __getitem__( self, key: Any ) -> Any:
        return self.dict[ key ]

    def __setitem__( self, key: Any, value: Any ) -> None:
        self.dict[ key ] = value

    def __contains__( self, item: Any ) -> bool:
        return item in self.dict

    def get_elements( self ) -> dict[ Any, Any ]:
        return self.dict

    def join( self, other: 'Dict' ) -> 'Dict':
        if self is other:
            return self

        if isinstance( other, DictBot ):
            return self

        if isinstance( other, DictTop ):
            return other

        result = Dict()

        for key, value in self.dict.items():
            if key in other:
                result[ key ] = value.join( other[ key ] )
            else:
                result[ key ] = value.join( value.create_bot() )

        for key, value in other.get_elements().items():
            if key not in self.dict:
                result[ key ] = value.join( value.create_bot() )

        return result

    def create_top( self ) -> 'DictTop':
        return DictTop()

    def create_bot( self ) -> 'DictBot':
        return DictBot()


class DictBot( Dict ):

    def join( self, other: Dict ) -> Dict:
        return self if isinstance( other, DictBot ) else other


class DictTop( Dict ):

    def join( self, other: Dict ) -> Dict:
        return self if isinstance( other, ( DictBot, DictTop ) ) else other.join( self )


class Memory( Dict ):

    def __init__( self, size: int ) -> None:
        super().__init__()
        self.size = size
        for i in range( self.size ):
            self.dict[ i ] = WordBot( 8 )

    def __setitem__( self, key: Any, value: Any ) -> None:
        if key not in self.dict:
            raise KeyError( f"Out of bounds access {key}." )

        self.dict[ key ] = value


class Token:

    def __init__( self, obj: Memory | None ) -> None:
        self.obj = obj

    def __eq__( self, other: object ) -> bool:
        assert type( other ) is Token
        return self.obj is other.obj

    def join( self, other: 'Token' ) -> 'Token':
        if isinstance( other, ( TokenTop, TokenBot ) ):
            return other.join( self )
        return self if self == other else TokenTop( None )

    def create_top( self ) -> 'TokenTop':
        return TokenTop( None )

    def create_bot( self ) -> 'TokenBot':
        return TokenBot( None )


class TokenBot( Token ):

    def join( self, other: Token ) -> Token:
        return other


class TokenTop( Token ):

    def join( self, _: Token ) -> Token:
        return self


class Ref:

    def __init__( self, obj: Memory | None, off: int | None ) -> None:
        self.obj = obj
        self.off = off

    def __eq__( self, other: object ) -> bool:
        assert type( other ) is Ref
        return self.obj is other.obj and self.off == other.off

    def get_off( self ) -> int:
        assert isinstance( self.off, int )
        return self.off

    def get_obj( self ) -> Memory:
        assert isinstance( self.obj, Memory )
        return self.obj

    def clone( self ) -> 'Ref':
        return Ref( self.obj, self.off )

    def join( self, other: 'Ref' ) -> 'Ref':
        if isinstance( other, ( RefTop, RefBot ) ):
            return other.join( self )
        return self if self == other else RefTop( None, None )

    def create_top( self ) -> 'RefTop':
        return RefTop( None, None )

    def create_bot( self ) -> 'RefBot':
        return RefBot( None, None )


class RefBot( Ref ):

    def join( self, other: Ref ) -> Ref:
        return other

    def clone( self ) -> 'RefBot':
        return RefBot( self.obj, self.off )


class RefTop( Ref ):

    def join( self, _: Ref ) -> Ref:
        return self

    def clone( self ) -> 'RefTop':
        return RefTop( self.obj, self.off )


### Bitvector operations

def bv_prepare_binary( vm: 'Interpret', params: list[ int ],
                       size: int ) -> tuple[ Word, Word ]:
    l = vm.pop( params[ 0 ] )
    r = vm.pop( params[ 1 ] )
    assert type( l ) is Word and l.size == size
    assert type( r ) is Word and r.size == size
    return l, r

def bv_join( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )
    assert isinstance( a, Word ) and a.size == size
    assert isinstance( b, Word ) and b.size == size
    vm.push( params[ 2 ], a.join( b ) )

def bv_bot( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    vm.push( params[ 0 ], WordBot( size ) )

def bv_top( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    vm.push( params[ 0 ], WordTop( size ) )

def bv_opt( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    cmp_val = vm.pop( params[ 0 ] )
    arg = vm.pop( params[ 1 ] )
    assert type( cmp_val ) is Bool
    assert isinstance( arg, Word ) and arg.size == size
    vm.push( params[ 2 ], arg if cmp_val.get_value() else WordBot( size ) )

def bv_fork( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    assert isinstance( value, Word ) and value.size == size
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def bv_move( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    assert isinstance( value, Word ) and value.size == size
    vm.push( params[ 1 ], value )

def bv_push( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    stck  = vm.pop( params[ 0 ] )
    value = vm.pop( params[ 1 ] )
    assert isinstance( value, Word ) and value.size == size
    stck.push( value )
    vm.push( params[ 2 ], stck )

def bv_pop( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    stck = vm.pop( params[ 0 ] )
    value = stck.pop()
    assert isinstance( value, Word ) and value.size == size
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], stck  )

def bv_drop( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    assert isinstance( value, Word ) and value.size == size

def bv_dup( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    assert isinstance( value, Word ) and value.size == size
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def bv_add( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value + r.value, size ) )

def bv_sub( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value - r.value, size ) )

def bv_mul( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value * r.value, size ) )

def bv_sdiv_rem( vm: 'Interpret', params: list[ int ], size: int, _div: bool ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    dividend, divisor = l.signed(), r.signed()
    quot, rem = divmod( dividend, divisor )

    if ( dividend > 0 ) != ( divisor > 0 ) and rem != 0:
        rem -= divisor
    if quot < 0 and rem != 0:
        quot += 1

    vm.push( params[ 2 ], Word( quot if _div else rem, size ) )

def bv_udiv( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value // r.value, size ) )

def bv_urem( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value % r.value, size ) )

def bv_eq( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.value == r.value ) )

def bv_ne( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.value != r.value ) )

def bv_slt( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.signed() < r.signed() ) )

def bv_sle( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.signed() <= r.signed() ) )

def bv_sge( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.signed() >= r.signed() ) )

def bv_sgt( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.signed() > r.signed() ) )

def bv_ult( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.value < r.value ) )

def bv_ule( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.value <= r.value ) )

def bv_uge( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.value >= r.value ) )

def bv_ugt( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Bool( l.value > r.value ) )

def bv_shl( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value << r.value, size ) )

def bv_ashr( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.signed() >> r.value, size ) )

def bv_lshr( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value >> r.value, size ) )

def bv_and( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value & r.value, size ) )

def bv_or( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value | r.value, size ) )

def bv_xor( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], Word( l.value ^ r.value, size ) )

def bv_neg( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    assert type( value ) is Word and value.size == size
    vm.push( params[ 1 ], Word( ~value.value, size ) )

def bv_cons( vm: 'Interpret', params: list[ int ], size: int,
             number: int ) -> None:
    vm.push( params[ 0 ], Word( number, size ) )

### Conversion operations

def cut( vm: 'Interpret', params: list[ int ], t: int ) -> None:
    value = vm.pop( params[ 0 ] )
    assert type( value ) is Word
    vm.push( params[ 1 ], Word( value.value, t ) )

def ext( vm: 'Interpret', params: list[ int ], f: int, t: int,
         sign: bool ) -> None:
    value = vm.pop( params[ 0 ] )
    assert type( value ) is Word and value.size == f
    vm.push( params[ 1 ], Word( value.signed() if sign else value.value, t ) )

def bool_cut( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    assert type( value ) is Word and value.size == size
    vm.push( params[ 1 ], Bool( bool( value.get_value() & 1 ) ) )

def bool_ext( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    assert type( value ) is Bool
    vm.push( params[ 1 ], Word( int( value.get_value() ), size ) )

### Bool operations

def bool_true( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], Bool( True ) )

def bool_false( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], Bool( False ) )

def bool_join( vm: 'Interpret', params: list[ int ] ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )
    vm.push( params[ 2 ], a.join( b ) )

def bool_bot( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], BoolBot() )

def bool_top( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], BoolTop() )

def bool_opt( vm: 'Interpret', params: list[ int ] ) -> None:
    cmp_val = vm.pop( params[ 0 ] )
    arg = vm.pop( params[ 1 ] )
    assert type( cmp_val ) is Bool
    vm.push( params[ 2 ], arg if cmp_val.get_value() else BoolBot() )

def bool_fork( vm: 'Interpret', params: list[ int ] ) -> None:
    value = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def bool_move( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 1 ], vm.pop( params[ 0 ] ) )

def bool_push( vm: 'Interpret', params: list[ int ] ) -> None:
    stck  = vm.pop( params[ 0 ] )
    value = vm.pop( params[ 1 ] )
    stck.push( value )
    vm.push( params[ 2 ], stck )

def bool_pop( vm: 'Interpret', params: list[ int ] ) -> None:
    stck = vm.pop( params[ 0 ] )
    value = stck.pop()
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], stck  )

def bool_drop( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.pop( params[ 0 ] )

def bool_dup( vm: 'Interpret', params: list[ int ] ) -> None:
    value = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def bool_not( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 1 ], Bool( not vm.pop( params[ 0 ] ).get_value() ) )

def bool_and( vm: 'Interpret', params: list[ int ] ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )
    vm.push( params[ 2 ], Bool( a.get_value() and b.get_value() ) )

def bool_or( vm: 'Interpret', params: list[ int ] ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )
    vm.push( params[ 2 ], Bool( a.get_value() or b.get_value() ) )

def bool_xor( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 2 ], Bool( vm.pop( params[ 0 ] ).get_value() != vm.pop( params[ 1 ] ).get_value() ) )

def bool_assert( vm: 'Interpret', params: list[ int ] ) -> None:
    assert vm.pop( params[ 0 ] ).get_value()

### Function operations

class Binder:

    def __init__( self, func: Func, *inputs: tuple[ Any, ... ] ) -> None:
        self.func   = func
        self.inputs = [ *inputs ]

    def add_input( self, i: Any ) -> None:
        self.inputs.append( i )

def func_fork( vm: 'Interpret', params: list[ int ] ) -> None:
    func = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], func )
    vm.push( params[ 2 ], func )

def func_move( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 1 ], vm.pop( params[ 0 ] ) )

def func_push( vm: 'Interpret', params: list[ int ] ) -> None:
    stck  = vm.pop( params[ 0 ] )
    value = vm.pop( params[ 1 ] )
    stck.push( value )
    vm.push( params[ 2 ], stck )

def func_pop( vm: 'Interpret', params: list[ int ] ) -> None:
    stck = vm.pop( params[ 0 ] )
    value = stck.pop()
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], stck  )

def func_drop( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.pop( params[ 0 ] )

def func_dup( vm: 'Interpret', params: list[ int ] ) -> None:
    func = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], func )
    vm.push( params[ 2 ], func )

def func_call( vm: 'Interpret', params: list[ int ] ) -> None:
    func  = vm.pop( params[ 0 ] )
    pargs = []

    if isinstance( func, Binder ):
        pargs = func.inputs
        func  = func.func

    assert isinstance( func, Func )

    vm.call( func, params[ 1 : ], pargs )

### Lambda operations

def lambda_bind( vm: 'Interpret', params: list[ int ] ) -> None:
    func = vm.pop( params[ 0 ] )
    if isinstance( func, Func ):
        vm.push( params[ 2 ], Binder( func, vm.pop( params[ 1 ] ) ) )
    else:
        assert isinstance( func, Binder )
        func.add_input( vm.pop( params[ 1 ] ) )
        vm.push( params[ 2 ], func )

def lambda_select( vm: 'Interpret', params: list[ int ] ) -> None:
    cmp_val = vm.pop( params[ 0 ] )
    func = vm.pop( params[ 1 ] )
    bot = vm.pop( params[ 2 ] )
    vm.push( params[ 3 ], func if cmp_val.get_value() else bot )

### Token operations

def token_join( vm: 'Interpret', params: list[ int ] ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )
    vm.push( params[ 2 ], a.join( b ) )

def token_bot( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], TokenBot( None ) )

def token_top( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], TokenTop( None ) )

def token_opt( vm: 'Interpret', params: list[ int ] ) -> None:
    cmp_val = vm.pop( params[ 0 ] )
    arg = vm.pop( params[ 1 ] )
    assert type( cmp_val ) is Bool
    vm.push( params[ 2 ], arg if cmp_val.get_value() else TokenBot( None ) )

def token_fork( vm: 'Interpret', params: list[ int ] ) -> None:
    value = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def token_move( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 1 ], vm.pop( params[ 0 ] ) )

def token_push( vm: 'Interpret', params: list[ int ] ) -> None:
    stck  = vm.pop( params[ 0 ] )
    value = vm.pop( params[ 1 ] )
    stck.push( value )
    vm.push( params[ 2 ], stck )

def token_pop( vm: 'Interpret', params: list[ int ] ) -> None:
    stck = vm.pop( params[ 0 ] )
    value = stck.pop()
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], stck  )

### Ref operations

def ref_join( vm: 'Interpret', params: list[ int ] ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )
    vm.push( params[ 2 ], a.join( b ) )

def ref_bot( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], RefBot( None, None ) )

def ref_top( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], RefTop( None, None ) )

def ref_opt( vm: 'Interpret', params: list[ int ] ) -> None:
    cmp_val = vm.pop( params[ 0 ] )
    arg = vm.pop( params[ 1 ] )
    assert type( cmp_val ) is Bool
    vm.push( params[ 2 ], arg if cmp_val.get_value() else RefBot( None, None ) )

def ref_fork( vm: 'Interpret', params: list[ int ] ) -> None:
    value = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value.clone() )

def ref_move( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 1 ], vm.pop( params[ 0 ] ) )

def ref_push( vm: 'Interpret', params: list[ int ] ) -> None:
    stck  = vm.pop( params[ 0 ] )
    value = vm.pop( params[ 1 ] )
    stck.push( value )
    vm.push( params[ 2 ], stck )

def ref_pop( vm: 'Interpret', params: list[ int ] ) -> None:
    stck = vm.pop( params[ 0 ] )
    value = stck.pop()
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], stck  )

def ref_drop( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.pop( params[ 0 ] )

def ref_dup( vm: 'Interpret', params: list[ int ] ) -> None:
    value = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value.clone() )

# TODO: Resolve `RefTop` and `RefBot`
def ref_add( vm: 'Interpret', params: list[ int ] ) -> None:
    ref = vm.pop( params[ 0 ] )
    value = vm.pop( params[ 1 ] )
    assert type( value ) is Word and value.size == 32
    assert isinstance( ref, Ref )
    vm.push( params[ 2 ], Ref( ref.get_obj(), ref.get_off() + value.signed() ) )

### Dict operations

def dict_join( vm: 'Interpret', params: list[ int ] ) -> None:
    dict1 = vm.pop( params[ 0 ] )
    dict2 = vm.pop( params[ 1 ] )
    vm.push( params[ 2 ], dict1.join( dict2 ) )

def dict_bot( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], DictBot() )

def dict_top( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], DictTop() )

def dict_opt( vm: 'Interpret', params: list[ int ] ) -> None:
    cmp_val = vm.pop( params[ 0 ] )
    arg = vm.pop( params[ 1 ] )
    assert type( cmp_val ) is Bool
    vm.push( params[ 2 ], arg if cmp_val.get_value() else DictBot() )

def dict_fork( vm: 'Interpret', params: list[ int ] ) -> None:
    value = vm.pop( params[ 0 ] ) # since only one function in frame is called, no need for copying
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def dict_move( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 1 ], vm.pop( params[ 0 ] ) )

def dict_push( vm: 'Interpret', params: list[ int ] ) -> None:
    stck  = vm.pop( params[ 0 ] )
    value = vm.pop( params[ 1 ] )
    stck.push( value )
    vm.push( params[ 2 ], stck )

def dict_pop( vm: 'Interpret', params: list[ int ] ) -> None:
    stck = vm.pop( params[ 0 ] )
    value = stck.pop()
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], stck  )

def dict_drop( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.pop( params[ 0 ] )

### Memory operations

def memory_alloca( vm: 'Interpret', params: list[ int ] ) -> None:
    size = vm.pop( params[ 0 ] )
    memory = Memory( size.get_value() )
    vm.push( params[ 1 ], Ref( memory, 0 ) )
    vm.push( params[ 2 ], Token( memory ) )
    vm.push( params[ 3 ], memory )

def memory_free( vm: 'Interpret', params: list[ int ] ) -> None:
    r = vm.pop( params[ 0 ] )
    t = vm.pop( params[ 1 ] )
    m = vm.pop( params[ 2 ] )

    assert r.get_off() == 0
    assert r.obj is t.obj is m

### Memory access operations

def w8_load( vm: 'Interpret', params: list[ int ] ) -> None:
    ref = vm.pop( params[ 0 ] )
    mem = vm.pop( params[ 1 ] )

    assert ref.obj is mem

    vm.push( params[ 2 ], mem[ ref.get_off() ] )
    vm.push( params[ 3 ], mem )

def w8_store( vm: 'Interpret', params: list[ int ] ) -> None:
    ref = vm.pop( params[ 0 ] )
    val = vm.pop( params[ 1 ] )
    mem = vm.pop( params[ 2 ] )

    assert ref.obj is mem

    mem[ ref.get_off() ] = val
    vm.push( params[ 3 ], mem )

def w32_load( vm: 'Interpret', params: list[ int ] ) -> None:
    ref = vm.pop( params[ 0 ] )
    mem = vm.pop( params[ 1 ] )
    value = 0

    assert ref.obj is mem

    for i in range( 4 ):
        byte = mem[ ref.get_off() + i ]
        # What to do if there is WordBot / WordTop?
        assert type( byte ) is Word and byte.size == 8
        value |= byte.value << ( 8 * i )

    vm.push( params[ 2 ], Word( value, 32 ) )
    vm.push( params[ 3 ], mem )

def w32_store( vm: 'Interpret', params: list[ int ] ) -> None:
    ref = vm.pop( params[ 0 ] )
    val = vm.pop( params[ 1 ] )
    mem = vm.pop( params[ 2 ] )

    assert type( val ) is Word and val.size == 32
    assert ref.obj is mem

    for i in range( 4 ):
        mem[ ref.get_off() + i ] = Word( val.value >> ( 8 * i ), 8 )

    vm.push( params[ 3 ], mem )

def bool_load( vm: 'Interpret', params: list[ int ] ) -> None:
    ref = vm.pop( params[ 0 ] )
    mem = vm.pop( params[ 1 ] )

    assert ref.obj is mem

    value = mem[ ref.get_off() ]
    # What to do if there is WordBot / WordTop?
    assert type( value ) is Word and value.size == 8
    vm.push( params[ 2 ], Bool( bool( value.value ) ) )
    vm.push( params[ 3 ], mem )

def bool_store( vm: 'Interpret', params: list[ int ] ) -> None:
    ref = vm.pop( params[ 0 ] )
    val = vm.pop( params[ 1 ] )
    mem = vm.pop( params[ 2 ] )

    assert ref.obj is mem

    mem[ ref.get_off() ] = Word( val.get_value(), 8 )
    vm.push( params[ 3 ], mem )

### Entry point

def exec_op( vm: 'Interpret', code: int, params: list[ int ] ) -> None:
    code_to_subr[ code ]( vm, params )


name_to_code: dict[ str, int ] = {
    "builtin_bv8join":      0xeff_0000,
    "builtin_bv8bot":       0xeff_0001,
    "builtin_bv8top":       0xeff_0002,
    "builtin_bv8opt":       0xeff_0003,
    "builtin_bv8fork":      0xeff_0004,
    "builtin_bv8move":      0xeff_0005,
    "builtin_bv8push":      0xeff_0006,
    "builtin_bv8pop":       0xeff_0007,
    "builtin_bv8drop":      0xeff_0008,
    "builtin_bv8dup":       0xeff_0009,

    "builtin_bv32join":     0xeff_000a,
    "builtin_bv32bot":      0xeff_000b,
    "builtin_bv32top":      0xeff_000c,
    "builtin_bv32opt":      0xeff_000d,
    "builtin_bv32fork":     0xeff_000e,
    "builtin_bv32move":     0xeff_000f,
    "builtin_bv32push":     0xeff_0010,
    "builtin_bv32pop":      0xeff_0011,
    "builtin_bv32drop":     0xeff_0012,
    "builtin_bv32dup":      0xeff_0013,

    "builtin_bool_join":    0xeff_0014,
    "builtin_bool_bot":     0xeff_0015,
    "builtin_bool_top":     0xeff_0016,
    "builtin_bool_opt":     0xeff_0017,
    "builtin_bool_fork":    0xeff_0018,
    "builtin_bool_move":    0xeff_0019,
    "builtin_bool_push":    0xeff_001a,
    "builtin_bool_pop":     0xeff_001b,
    "builtin_bool_drop":    0xeff_001c,
    "builtin_bool_dup":     0xeff_001d,
    "builtin_bool_not":     0xeff_001e,
    "builtin_bool_and":     0xeff_001f,
    "builtin_bool_or":      0xeff_0020,
    "builtin_bool_xor":     0xeff_0021,
    "builtin_bool_assert":  0xeff_0022,

    "builtin_bv32add":      0xeff_0023,
    "builtin_bv32sub":      0xeff_0024,
    "builtin_bv32mul":      0xeff_0025,
    "builtin_bv32sdiv":     0xeff_0026,
    "builtin_bv32srem":     0xeff_0027,
    "builtin_bv32eq":       0xeff_0028,
    "builtin_bv32ne":       0xeff_0029,
    "builtin_bv32slt":      0xeff_002a,
    "builtin_bv32sle":      0xeff_002b,
    "builtin_bv32sge":      0xeff_002c,
    "builtin_bv32sgt":      0xeff_002d,
    "builtin_bv32shl":      0xeff_002e,
    "builtin_bv32ashr":     0xeff_002f,
    "builtin_bv32cons_0":   0xeff_0030,
    "builtin_bv32cons_1":   0xeff_0031,
    "builtin_bv32cons_2":   0xeff_0032,
    "builtin_bv32cons_3":   0xeff_0033,
    "builtin_bv32cons_4":   0xeff_0034,
    "builtin_bv32cons_5":   0xeff_0035,
    "builtin_bv32cons_6":   0xeff_0036,
    "builtin_bv32cons_7":   0xeff_0037,
    "builtin_bv32cons_8":   0xeff_0038,
    "builtin_bv32cons_9":   0xeff_0039,
    "builtin_bv32cons_10":  0xeff_003a,
    "builtin_bv32cons_11":  0xeff_003b,
    "builtin_bv32cons_12":  0xeff_003c,
    "builtin_bv32cons_13":  0xeff_003d,
    "builtin_bv32cons_14":  0xeff_003e,
    "builtin_bv32cons_15":  0xeff_003f,
    "builtin_bv32and":      0xeff_0040,
    "builtin_bv32or":       0xeff_0041,
    "builtin_bv32xor":      0xeff_0042,
    "builtin_bv32neg":      0xeff_0043,

    "builtin_bv32udiv":     0xeff_0044,
    "builtin_bv32urem":     0xeff_0045,
    "builtin_bv32ult":      0xeff_0046,
    "builtin_bv32ule":      0xeff_0047,
    "builtin_bv32uge":      0xeff_0048,
    "builtin_bv32ugt":      0xeff_0049,
    "builtin_bv32lshr":     0xeff_004a,

    "builtin_bv8add":       0xeff_004b,
    "builtin_bv8sub":       0xeff_004c,
    "builtin_bv8mul":       0xeff_004d,
    "builtin_bv8sdiv":      0xeff_004e,
    "builtin_bv8srem":      0xeff_004f,
    "builtin_bv8eq":        0xeff_0050,
    "builtin_bv8ne":        0xeff_0051,
    "builtin_bv8slt":       0xeff_0052,
    "builtin_bv8sle":       0xeff_0053,
    "builtin_bv8sge":       0xeff_0054,
    "builtin_bv8sgt":       0xeff_0055,
    "builtin_bv8shl":       0xeff_0056,
    "builtin_bv8ashr":      0xeff_0057,
    "builtin_bv8cons_0":    0xeff_0058,
    "builtin_bv8cons_1":    0xeff_0059,
    "builtin_bv8cons_2":    0xeff_005a,
    "builtin_bv8cons_3":    0xeff_005b,
    "builtin_bv8cons_4":    0xeff_005c,
    "builtin_bv8cons_5":    0xeff_005d,
    "builtin_bv8cons_6":    0xeff_005e,
    "builtin_bv8cons_7":    0xeff_005f,
    "builtin_bv8cons_8":    0xeff_0060,
    "builtin_bv8cons_9":    0xeff_0061,
    "builtin_bv8cons_10":   0xeff_0062,
    "builtin_bv8cons_11":   0xeff_0063,
    "builtin_bv8cons_12":   0xeff_0064,
    "builtin_bv8cons_13":   0xeff_0065,
    "builtin_bv8cons_14":   0xeff_0066,
    "builtin_bv8cons_15":   0xeff_0067,
    "builtin_bv8and":       0xeff_0068,
    "builtin_bv8or":        0xeff_0069,
    "builtin_bv8xor":       0xeff_006a,
    "builtin_bv8neg":       0xeff_006b,

    "builtin_bv8udiv":      0xeff_006c,
    "builtin_bv8urem":      0xeff_006d,
    "builtin_bv8ult":       0xeff_006e,
    "builtin_bv8ule":       0xeff_006f,
    "builtin_bv8uge":       0xeff_0070,
    "builtin_bv8ugt":       0xeff_0071,
    "builtin_bv8lshr":      0xeff_0072,

    "builtin_bv32cut8":     0xeff_0073,
    "builtin_bv8sext32":    0xeff_0074,
    "builtin_bv8zext32":    0xeff_0075,

    "builtin_func_call":    0xeff_0076,
    "builtin_func_dup":     0xeff_0077,
    "builtin_func_drop":    0xeff_0078,
    "builtin_func_pop":     0xeff_0079,
    "builtin_func_push":    0xeff_007a,
    "builtin_func_move":    0xeff_007b,
    "builtin_func_fork":    0xeff_007c,

    "builtin_bv8cutbool":   0xeff_007d,
    "builtin_bool_ext8":    0xeff_007e,
    "builtin_bv32cutbool":  0xeff_007f,
    "builtin_bool_ext32":   0xeff_0080,
    "builtin_bool_true":    0xeff_0081,
    "builtin_bool_false":   0xeff_0082,

    "builtin_lambda_bind":   0xeff_0083,
    "builtin_lambda_select": 0xeff_0084,

    "builtin_token_join":    0xeff_0085,
    "builtin_token_bot":     0xeff_0086,
    "builtin_token_top":     0xeff_0087,
    "builtin_token_opt":     0xeff_0088,
    "builtin_token_fork":    0xeff_0089,
    "builtin_token_move":    0xeff_008a,
    "builtin_token_push":    0xeff_008b,
    "builtin_token_pop":     0xeff_008c,

    "builtin_dict_join":     0xeff_008d,
    "builtin_dict_bot":      0xeff_008e,
    "builtin_dict_top":      0xeff_008f,
    "builtin_dict_opt":      0xeff_0090,
    "builtin_dict_fork":     0xeff_0091,
    "builtin_dict_move":     0xeff_0092,
    "builtin_dict_push":     0xeff_0093,
    "builtin_dict_pop":      0xeff_0094,
    "builtin_dict_drop":     0xeff_0095,

    "builtin_memory_alloca": 0xeff_0096,
    "builtin_memory_free":   0xeff_0097,

    "builtin_w8_load":       0xeff_0098,
    "builtin_w8_store":      0xeff_0099,
    "builtin_w32_load":      0xeff_009a,
    "builtin_w32_store":     0xeff_009b,
    "builtin_bool_load":     0xeff_009c,
    "builtin_bool_store":    0xeff_009d,

    "builtin_ref_join":      0xeff_009e,
    "builtin_ref_bot":       0xeff_009f,
    "builtin_ref_top":       0xeff_00a0,
    "builtin_ref_opt":       0xeff_00a1,
    "builtin_ref_fork":      0xeff_00a2,
    "builtin_ref_move":      0xeff_00a3,
    "builtin_ref_push":      0xeff_00a4,
    "builtin_ref_pop":       0xeff_00a5,
    "builtin_ref_drop":      0xeff_00a6,
    "builtin_ref_dup":       0xeff_00a7,
    "builtin_ref_add":       0xeff_00a8
}

code_to_subr: dict[ int, Any ] = {
    0xeff_0000: lambda vm, p: bv_join( vm, p, 8 ),
    0xeff_0001: lambda vm, p: bv_bot(  vm, p, 8 ),
    0xeff_0002: lambda vm, p: bv_top(  vm, p, 8 ),
    0xeff_0003: lambda vm, p: bv_opt(  vm, p, 8 ),
    0xeff_0004: lambda vm, p: bv_fork( vm, p, 8 ),
    0xeff_0005: lambda vm, p: bv_move( vm, p, 8 ),
    0xeff_0006: lambda vm, p: bv_push( vm, p, 8 ),
    0xeff_0007: lambda vm, p: bv_pop(  vm, p, 8 ),
    0xeff_0008: lambda vm, p: bv_drop( vm, p, 8 ),
    0xeff_0009: lambda vm, p: bv_dup(  vm, p, 8 ),

    0xeff_000a: lambda vm, p: bv_join( vm, p, 32 ),
    0xeff_000b: lambda vm, p: bv_bot(  vm, p, 32 ),
    0xeff_000c: lambda vm, p: bv_top(  vm, p, 32 ),
    0xeff_000d: lambda vm, p: bv_opt(  vm, p, 32 ),
    0xeff_000e: lambda vm, p: bv_fork( vm, p, 32 ),
    0xeff_000f: lambda vm, p: bv_move( vm, p, 32 ),
    0xeff_0010: lambda vm, p: bv_push( vm, p, 32 ),
    0xeff_0011: lambda vm, p: bv_pop(  vm, p, 32 ),
    0xeff_0012: lambda vm, p: bv_drop( vm, p, 32 ),
    0xeff_0013: lambda vm, p: bv_dup(  vm, p, 32 ),

    0xeff_0014: bool_join,
    0xeff_0015: bool_bot,
    0xeff_0016: bool_top,
    0xeff_0017: bool_opt,
    0xeff_0018: bool_fork,
    0xeff_0019: bool_move,
    0xeff_001a: bool_push,
    0xeff_001b: bool_pop,
    0xeff_001c: bool_drop,
    0xeff_001d: bool_dup,
    0xeff_001e: bool_not,
    0xeff_001f: bool_and,
    0xeff_0020: bool_or,
    0xeff_0021: bool_xor,
    0xeff_0022: bool_assert,

    0xeff_0023: lambda vm, p: bv_add(  vm, p, 32 ),
    0xeff_0024: lambda vm, p: bv_sub(  vm, p, 32 ),
    0xeff_0025: lambda vm, p: bv_mul(  vm, p, 32 ),
    0xeff_0026: lambda vm, p: bv_sdiv_rem( vm, p, 32,  True ),
    0xeff_0027: lambda vm, p: bv_sdiv_rem( vm, p, 32, False ),
    0xeff_0028: lambda vm, p: bv_eq(   vm, p, 32 ),
    0xeff_0029: lambda vm, p: bv_ne(   vm, p, 32 ),
    0xeff_002a: lambda vm, p: bv_slt(  vm, p, 32 ),
    0xeff_002b: lambda vm, p: bv_sle(  vm, p, 32 ),
    0xeff_002c: lambda vm, p: bv_sge(  vm, p, 32 ),
    0xeff_002d: lambda vm, p: bv_sgt(  vm, p, 32 ),
    0xeff_002e: lambda vm, p: bv_shl(  vm, p, 32 ),
    0xeff_002f: lambda vm, p: bv_ashr( vm, p, 32 ),
    0xeff_0030: lambda vm, p: bv_cons( vm, p, 32, 0 ),
    0xeff_0031: lambda vm, p: bv_cons( vm, p, 32, 1 ),
    0xeff_0032: lambda vm, p: bv_cons( vm, p, 32, 2 ),
    0xeff_0033: lambda vm, p: bv_cons( vm, p, 32, 3 ),
    0xeff_0034: lambda vm, p: bv_cons( vm, p, 32, 4 ),
    0xeff_0035: lambda vm, p: bv_cons( vm, p, 32, 5 ),
    0xeff_0036: lambda vm, p: bv_cons( vm, p, 32, 6 ),
    0xeff_0037: lambda vm, p: bv_cons( vm, p, 32, 7 ),
    0xeff_0038: lambda vm, p: bv_cons( vm, p, 32, 8 ),
    0xeff_0039: lambda vm, p: bv_cons( vm, p, 32, 9 ),
    0xeff_003a: lambda vm, p: bv_cons( vm, p, 32, 10 ),
    0xeff_003b: lambda vm, p: bv_cons( vm, p, 32, 11 ),
    0xeff_003c: lambda vm, p: bv_cons( vm, p, 32, 12 ),
    0xeff_003d: lambda vm, p: bv_cons( vm, p, 32, 13 ),
    0xeff_003e: lambda vm, p: bv_cons( vm, p, 32, 14 ),
    0xeff_003f: lambda vm, p: bv_cons( vm, p, 32, 15 ),
    0xeff_0040: lambda vm, p: bv_and(  vm, p, 32 ),
    0xeff_0041: lambda vm, p: bv_or(   vm, p, 32 ),
    0xeff_0042: lambda vm, p: bv_xor(  vm, p, 32 ),
    0xeff_0043: lambda vm, p: bv_neg(  vm, p, 32 ),

    0xeff_0044: lambda vm, p: bv_udiv( vm, p, 32 ),
    0xeff_0045: lambda vm, p: bv_urem( vm, p, 32 ),
    0xeff_0046: lambda vm, p: bv_ult(  vm, p, 32 ),
    0xeff_0047: lambda vm, p: bv_ule(  vm, p, 32 ),
    0xeff_0048: lambda vm, p: bv_uge(  vm, p, 32 ),
    0xeff_0049: lambda vm, p: bv_ugt(  vm, p, 32 ),
    0xeff_004a: lambda vm, p: bv_lshr( vm, p, 32 ),

    0xeff_004b: lambda vm, p: bv_add(  vm, p, 8 ),
    0xeff_004c: lambda vm, p: bv_sub(  vm, p, 8 ),
    0xeff_004d: lambda vm, p: bv_mul(  vm, p, 8 ),
    0xeff_004e: lambda vm, p: bv_sdiv_rem( vm, p, 8,  True ),
    0xeff_004f: lambda vm, p: bv_sdiv_rem( vm, p, 8, False ),
    0xeff_0050: lambda vm, p: bv_eq(   vm, p, 8 ),
    0xeff_0051: lambda vm, p: bv_ne(   vm, p, 8 ),
    0xeff_0052: lambda vm, p: bv_slt(  vm, p, 8 ),
    0xeff_0053: lambda vm, p: bv_sle(  vm, p, 8 ),
    0xeff_0054: lambda vm, p: bv_sge(  vm, p, 8 ),
    0xeff_0055: lambda vm, p: bv_sgt(  vm, p, 8 ),
    0xeff_0056: lambda vm, p: bv_shl(  vm, p, 8 ),
    0xeff_0057: lambda vm, p: bv_ashr( vm, p, 8 ),
    0xeff_0058: lambda vm, p: bv_cons( vm, p, 8, 0 ),
    0xeff_0059: lambda vm, p: bv_cons( vm, p, 8, 1 ),
    0xeff_005a: lambda vm, p: bv_cons( vm, p, 8, 2 ),
    0xeff_005b: lambda vm, p: bv_cons( vm, p, 8, 3 ),
    0xeff_005c: lambda vm, p: bv_cons( vm, p, 8, 4 ),
    0xeff_005d: lambda vm, p: bv_cons( vm, p, 8, 5 ),
    0xeff_005e: lambda vm, p: bv_cons( vm, p, 8, 6 ),
    0xeff_005f: lambda vm, p: bv_cons( vm, p, 8, 7 ),
    0xeff_0060: lambda vm, p: bv_cons( vm, p, 8, 8 ),
    0xeff_0061: lambda vm, p: bv_cons( vm, p, 8, 9 ),
    0xeff_0062: lambda vm, p: bv_cons( vm, p, 8, 10 ),
    0xeff_0063: lambda vm, p: bv_cons( vm, p, 8, 11 ),
    0xeff_0064: lambda vm, p: bv_cons( vm, p, 8, 12 ),
    0xeff_0065: lambda vm, p: bv_cons( vm, p, 8, 13 ),
    0xeff_0066: lambda vm, p: bv_cons( vm, p, 8, 14 ),
    0xeff_0067: lambda vm, p: bv_cons( vm, p, 8, 15 ),
    0xeff_0068: lambda vm, p: bv_and(  vm, p, 8 ),
    0xeff_0069: lambda vm, p: bv_or(   vm, p, 8 ),
    0xeff_006a: lambda vm, p: bv_xor(  vm, p, 8 ),
    0xeff_006c: lambda vm, p: bv_udiv( vm, p, 8 ),

    0xeff_006d: lambda vm, p: bv_urem( vm, p, 8 ),
    0xeff_006e: lambda vm, p: bv_ult(  vm, p, 8 ),
    0xeff_006f: lambda vm, p: bv_ule(  vm, p, 8 ),
    0xeff_0070: lambda vm, p: bv_uge(  vm, p, 8 ),
    0xeff_0071: lambda vm, p: bv_ugt(  vm, p, 8 ),
    0xeff_0072: lambda vm, p: bv_lshr( vm, p, 8 ),

    0xeff_0073: lambda vm, p: cut( vm, p, 8 ),
    0xeff_0074: lambda vm, p: ext( vm, p, 8, 32, True ),
    0xeff_0075: lambda vm, p: ext( vm, p, 8, 32, False ),

    0xeff_0076: func_call,
    0xeff_0077: func_dup,
    0xeff_0078: func_drop,
    0xeff_0079: func_pop,
    0xeff_007a: func_push,
    0xeff_007b: func_move,
    0xeff_007c: func_fork,

    0xeff_007d: lambda vm, p: bool_cut( vm, p, 8 ),
    0xeff_007e: lambda vm, p: bool_ext( vm, p, 8 ),
    0xeff_007f: lambda vm, p: bool_cut( vm, p, 32 ),
    0xeff_0080: lambda vm, p: bool_ext( vm, p, 32 ),

    0xeff_0081: bool_true,
    0xeff_0082: bool_false,

    0xeff_0083: lambda_bind,
    0xeff_0084: lambda_select,

    0xeff_0085: token_join,
    0xeff_0086: token_bot,
    0xeff_0087: token_top,
    0xeff_0088: token_opt,
    0xeff_0089: token_fork,
    0xeff_008a: token_move,
    0xeff_008b: token_push,
    0xeff_008c: token_pop,

    0xeff_008d: dict_join,
    0xeff_008e: dict_bot,
    0xeff_008f: dict_top,
    0xeff_0090: dict_opt,
    0xeff_0091: dict_fork,
    0xeff_0092: dict_move,
    0xeff_0093: dict_push,
    0xeff_0094: dict_pop,
    0xeff_0095: dict_drop,

    0xeff_0096: memory_alloca,
    0xeff_0097: memory_free,

    0xeff_0098: w8_load,
    0xeff_0099: w8_store,
    0xeff_009a: w32_load,
    0xeff_009b: w32_store,
    0xeff_009c: bool_load,
    0xeff_009d: bool_store,

    0xeff_009e: ref_join,
    0xeff_009f: ref_bot,
    0xeff_00a0: ref_top,
    0xeff_00a1: ref_opt,
    0xeff_00a2: ref_fork,
    0xeff_00a3: ref_move,
    0xeff_00a4: ref_push,
    0xeff_00a5: ref_pop,
    0xeff_00a6: ref_drop,
    0xeff_00a7: ref_dup,
    0xeff_00a8: ref_add
}
