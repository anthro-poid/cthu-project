from symtab import BuiltinType, W8, W32, Bool, Func, Subr
from typing import Any, TYPE_CHECKING

if TYPE_CHECKING:
    from interpret import Interpret

### Bitvector operations

def bv_norm( n: int, exp: int ) -> int:
    mask: int = 2 ** exp - 1
    return n & mask 

def bv_signed( n: int, size: int ) -> int:
    m: int = 2 ** ( size - 1 )
    return n if -m <= n < m else ( n % m ) - ( n & m )

def bv_prepare_binary( vm: 'Interpret', params: list[ int ],
                       size: int ) -> tuple[ Any, Any ]:
    return ( bv_norm( vm.pop( params[ 0 ] ), size ),
             bv_norm( vm.pop( params[ 1 ] ), size ) )

def bv_join( vm: 'Interpret', params: list[ int ], t: type ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )

    if isinstance( a, t.Top ) or isinstance( b, t.Top ) or \
       ( isinstance( a, int ) and isinstance( b, int ) ):
        vm.push( params[ 2 ], t.Top() )

    elif isinstance( a, t.Bot ) and isinstance( b, t.Bot ):
        vm.push( params[ 2 ], t.Bot() )

    elif isinstance( a, int ):
        vm.push( params[ 2 ], a )

    else:
        vm.push( params[ 2 ], b )

def bv_bot( vm: 'Interpret', params: list[ int ], t: type ) -> None:
    vm.push( params[ 0 ], t.Bot() )

def bv_top( vm: 'Interpret', params: list[ int ], t: type ) -> None:
    vm.push( params[ 0 ], t.Top() )

def bv_opt( vm: 'Interpret', params: list[ int ], t: type ) -> None:
    cmp_val = vm.pop( params[ 0 ] )
    arg = vm.pop( params[ 1 ] )
    assert isinstance( cmp_val, bool )
    vm.push( params[ 2 ], arg if cmp_val else t.Bot() )

def bv_fork( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def bv_move( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    vm.push( params[ 1 ], vm.pop( params[ 0 ] ) )

# TODO: Will be implemented in the future, but for now we will just assert False.
def bv_push( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    assert False

# TODO: Will be implemented in the future, but for now we will just assert False.
def bv_pop( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    assert False

def bv_drop( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    vm.pop( params[ 0 ] )

def bv_dup( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def bv_add( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l + r, size ) )

def bv_sub( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l - r, size ) )

def bv_mul( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l * r, size ) )

def bv_sdiv( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    l, r = bv_signed( l, size ), bv_signed( r, size )

    vm.push( params[ 2 ],
        bv_norm( l // r + ( 0 if l % r == 0 else 1 ) if ( l // r ) < 0 else l // r, size ) )

def bv_srem( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    l, r = bv_signed( l, size ), bv_signed( r, size )

    vm.push( params[ 2 ],
        bv_norm( -r + ( l % r ) if max( l, r ) >= 0 and min( l, r ) < 0 else ( l % r ), size ) )

def bv_udiv( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l // r, size ) )

def bv_urem( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l % r, size ) )

def bv_eq( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], l == r )

def bv_ne( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], l != r )

def bv_slt( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    l, r = bv_signed( l, size ), bv_signed( r, size )
    vm.push( params[ 2 ], l < r )

def bv_sle( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    l, r = bv_signed( l, size ), bv_signed( r, size )
    vm.push( params[ 2 ], l <= r )

def bv_sge( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    l, r = bv_signed( l, size ), bv_signed( r, size )
    vm.push( params[ 2 ], l >= r )

def bv_sgt( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    l, r = bv_signed( l, size ), bv_signed( r, size )
    vm.push( params[ 2 ], l > r )

def bv_ult( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], l < r )

def bv_ule( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], l <= r )

def bv_uge( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], l >= r )

def bv_ugt( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], l > r )

def bv_shl( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l << r, size ) )

def bv_ashr( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( bv_signed( l, size ) >> r, size ) )

def bv_lshr( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l >> r, size ) )

def bv_and( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l & r, size ) )

def bv_or( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l | r, size ) )

def bv_xor( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    l, r = bv_prepare_binary( vm, params, size )
    vm.push( params[ 2 ], bv_norm( l ^ r, size ) )

def bv_neg( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    vm.push( params[ 1 ], bv_norm( ~vm.pop( params[ 0 ] ), size ) )

def bv_cons( vm: 'Interpret', params: list[ int ], size: int,
             number: int ) -> None:
    vm.push( params[ 0 ], bv_norm( number, size ) )

### Conversion operations

def cut( vm: 'Interpret', params: list[ int ], t: int ) -> None:
    vm.push( params[ 1 ], bv_norm( vm.pop( params[ 0 ] ), t ) )

def ext( vm: 'Interpret', params: list[ int ], f: int, t: int,
         sign: bool ) -> None:
    num = bv_norm( vm.pop( params[ 0 ] ), f )

    if sign and num & ( 1 << ( f - 1 ) ):
        num |= ( ( 1 << ( t - f ) ) - 1 ) << f

    vm.push( params[ 1 ], bv_norm( num, t ) )

def bool_cut( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    vm.push( params[ 1 ], bool( bv_norm( vm.pop( params[ 0 ] ), size ) & 1 ) )

def bool_ext( vm: 'Interpret', params: list[ int ], size: int ) -> None:
    value = vm.pop( params[ 0 ] )
    assert isinstance( value, bool )
    vm.push( params[ 1 ], int( value ) )

### Bool operations

def bool_true( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], True )

def bool_false( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], False )

def bool_join( vm: 'Interpret', params: list[ int ] ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )

    if isinstance( a, Bool.Top ) or isinstance( b, Bool.Top ) or \
       ( isinstance( a, bool ) and isinstance( b, bool ) ):
        vm.push( params[ 2 ], Bool.Top() )

    elif isinstance( a, Bool.Bot ) and isinstance( b, Bool.Bot ):
        vm.push( params[ 2 ], Bool.Bot() )

    elif isinstance( a, bool ):
        vm.push( params[ 2 ], a )

    else:
        vm.push( params[ 2 ], b )

def bool_bot( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], Bool.Bot() )

def bool_top( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 0 ], Bool.Top() )

def bool_opt( vm: 'Interpret', params: list[ int ] ) -> None:
    cmp_val = vm.pop( params[ 0 ] )
    arg = vm.pop( params[ 1 ] )
    assert isinstance( cmp_val, bool )
    vm.push( params[ 2 ], arg if cmp_val else Bool.Bot() )

def bool_fork( vm: 'Interpret', params: list[ int ] ) -> None:
    value = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def bool_move( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 1 ], vm.pop( params[ 0 ] ) )

# TODO: Will be implemented in the future, but for now we will just assert False.
def bool_push( vm: 'Interpret', params: list[ int ] ) -> None:
    assert False

# TODO: Will be implemented in the future, but for now we will just assert False.
def bool_pop( vm: 'Interpret', params: list[ int ] ) -> None:
    assert False

def bool_drop( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.pop( params[ 0 ] )

def bool_dup( vm: 'Interpret', params: list[ int ] ) -> None:
    value = vm.pop( params[ 0 ] )
    vm.push( params[ 1 ], value )
    vm.push( params[ 2 ], value )

def bool_not( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 1 ], not vm.pop( params[ 0 ] ) )

def bool_and( vm: 'Interpret', params: list[ int ] ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )
    vm.push( params[ 2 ], a and b )

def bool_or( vm: 'Interpret', params: list[ int ] ) -> None:
    a = vm.pop( params[ 0 ] )
    b = vm.pop( params[ 1 ] )
    vm.push( params[ 2 ], a or b )

def bool_xor( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 2 ], vm.pop( params[ 0 ] ) != vm.pop( params[ 1 ] ) )

def bool_assert( vm: 'Interpret', params: list[ int ] ) -> None:
    assert vm.pop( params[ 0 ] )

### Function operations

def _func_permute( mapping: dict[ int, int ], actual: list[ int ],
                   formal: list[ int ] ) -> dict[ int, int ]:
    result: dict[ int, int ] = {}
    used: set[ int ] = set()

    for caller, callee in zip( actual, formal ):
        result[ callee ] = mapping.get( caller, caller )
        used.add( mapping.get( caller, caller ) )

    for caller in actual:
        if mapping.get( caller, caller ) not in result:
            for callee in formal:
                if callee not in used:
                    result[ mapping.get( caller, caller ) ] = callee
                    used.add( callee )
                    break

    return result

# TODO: Will be implemented in the future, but for now we will just assert False.
def func_bot( vm: 'Interpret', params: list[ int ] ) -> None:
    assert False

# TODO: Will be implemented in the future, but for now we will just assert False.
def func_top( vm: 'Interpret', params: list[ int ] ) -> None:
    assert False

# TODO: Will be implemented in the future, but for now we will just assert False.
def func_fork( vm: 'Interpret', params: list[ int ] ) -> None:
    assert False

def func_move( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.push( params[ 1 ], vm.pop( params[ 0 ] ) )

# TODO: Will be implemented in the future, but for now we will just assert False.
def func_push( vm: 'Interpret', params: list[ int ] ) -> None:
    assert False

# TODO: Will be implemented in the future, but for now we will just assert False.
def func_pop( vm: 'Interpret', params: list[ int ] ) -> None:
    assert False

def func_drop( vm: 'Interpret', params: list[ int ] ) -> None:
    vm.pop( params[ 0 ] )

# TODO: Will be implemented in the future, but for now we will just assert False.
def func_dup( vm: 'Interpret', params: list[ int ] ) -> None:
    assert False

def func_call( vm: 'Interpret', params: list[ int ] ) -> None:
    func = vm.pop( params[ 0 ] )
    if isinstance( func, Func.Bot ) or isinstance( func, Func.Top ):
        return func( vm, params[ 1 : ] )

    partial_size = len( func.par_args )

    stored_pc      = vm.pc
    stored_exec    = vm.executing
    stored_mapping = vm.map

    vm.pc        = 0
    vm.executing = func
    vm.map       = _func_permute( vm.map, params[ 1 : ], func.input[ partial_size : ] + func.output )

    for arg, sid in zip( func.par_args, func.input ):
        vm.push( sid, arg )

    func.par_args = []

    vm.run()

    vm.pc        = stored_pc
    vm.executing = stored_exec
    vm.map       = stored_mapping

def func_join( vm: 'Interpret', params: list[ int ] ) -> None:
    frame = vm.pop( params[ 2 ] )
    assert isinstance( frame, Subr )

    frame.par_args = [ vm.pop( param ) for param in params[ : 2 ] ]

    vm.push( params[ -1 ], frame )

def func_opt( vm: 'Interpret', params: list[ int ] ) -> None:
    cmp_val = vm.pop( params[ 0 ] )
    arg = vm.pop( params[ 1 ] )
    assert isinstance( cmp_val, bool )

    vm.push( params[ 2 ], arg if cmp_val else Func.Bot( arg ) )

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
    "builtin_func_opt":     0xeff_007d,
    "builtin_func_top":     0xeff_007e,
    "builtin_func_bot":     0xeff_007f,
    "builtin_func_join":    0xeff_0080,

    "builtin_bv8cutbool":   0xeff_0081,
    "builtin_bool_ext8":    0xeff_0082,
    "builtin_bv32cutbool":  0xeff_0083,
    "builtin_bool_ext32":   0xeff_0084,
    "builtin_bool_true":    0xeff_0085,
    "builtin_bool_false":   0xeff_0086
}

code_to_subr: dict[ int, Any ] = {
    0xeff_0000: lambda vm, p: bv_join( vm, p, W8 ),
    0xeff_0001: lambda vm, p: bv_bot(  vm, p, W8 ),
    0xeff_0002: lambda vm, p: bv_top(  vm, p, W8 ),
    0xeff_0003: lambda vm, p: bv_opt(  vm, p, W8 ),
    0xeff_0004: lambda vm, p: bv_fork( vm, p, 8 ),
    0xeff_0005: lambda vm, p: bv_move( vm, p, 8 ),
    0xeff_0006: lambda vm, p: bv_push( vm, p, 8 ),
    0xeff_0007: lambda vm, p: bv_pop(  vm, p, 8 ),
    0xeff_0008: lambda vm, p: bv_drop( vm, p, 8 ),
    0xeff_0009: lambda vm, p: bv_dup(  vm, p, 8 ),

    0xeff_000a: lambda vm, p: bv_join( vm, p, W32 ),
    0xeff_000b: lambda vm, p: bv_bot(  vm, p, W32 ),
    0xeff_000c: lambda vm, p: bv_top(  vm, p, W32 ),
    0xeff_000d: lambda vm, p: bv_opt(  vm, p, W32 ),
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
    0xeff_0026: lambda vm, p: bv_sdiv( vm, p, 32 ),
    0xeff_0027: lambda vm, p: bv_srem( vm, p, 32 ),
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
    0xeff_004e: lambda vm, p: bv_sdiv( vm, p, 8 ),
    0xeff_004f: lambda vm, p: bv_srem( vm, p, 8 ),
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

    0xeff_0073: lambda vm, p: cut( vm, p, 32 ),
    0xeff_0074: lambda vm, p: ext( vm, p, 8, 32, True ),
    0xeff_0075: lambda vm, p: ext( vm, p, 8, 32, False ),

    0xeff_0076: func_call,
    0xeff_0077: func_dup,
    0xeff_0078: func_drop,
    0xeff_0079: func_pop,
    0xeff_007a: func_push,
    0xeff_007b: func_move,
    0xeff_007c: func_fork,
    0xeff_007d: func_opt,
    0xeff_007e: func_top,
    0xeff_007f: func_bot,
    0xeff_0080: func_join,

    0xeff_0081: lambda vm, p: bool_cut( vm, p, 8 ),
    0xeff_0082: lambda vm, p: bool_ext( vm, p, 8 ),
    0xeff_0083: lambda vm, p: bool_cut( vm, p, 32 ),
    0xeff_0084: lambda vm, p: bool_ext( vm, p, 32 ),

    0xeff_0085: bool_true,
    0xeff_0086: bool_false
}
