#include "codegen.hpp"

namespace llvm2ct
{

codegen::codegen( llvm::LLVMContext &context, std::unique_ptr< llvm::Module > module )
    : _context( context ), _module( std::move( module ) )
{}

uint16_t codegen::allocate_stack()
{
    if ( !_free_stacks.empty() )
    {
        uint16_t stack = _free_stacks.back();
        _free_stacks.pop_back();
        return stack;
    }

    return _next_stack ++;
}

uint16_t codegen::define( llvm::Value *value )
{
    uint16_t stack = allocate_stack();
    _stack_of[ value ] = stack;
    return stack;
}

uint16_t codegen::use( llvm::Value *value, const std::string &struct_name )
{
    auto it = _stack_of.find( value );
    uint16_t stack;

    if ( it != _stack_of.end() )
        stack = it->second;
    else
    {
        stack = define( value );

        if ( auto *c = llvm::dyn_cast< llvm::ConstantInt >( value ) )
            materialize_constant( stack, *c, struct_name );
    }

    if ( -- _remaining_uses[ value ] > 0 )
    {
        /* More reads of this value remain after this one. A Cthu stack is
         * linear (vm/interpret.py's pop() is a real, destructive
         * list.pop()) — reading it now would consume the only copy, so
         * dup it: this read gets one fresh copy, _stack_of is updated to
         * the other fresh copy for whichever read comes next (which may
         * itself dup again, if further reads remain after that one). */
        uint16_t copy_a = allocate_stack();
        uint16_t copy_b = allocate_stack();
        bool w8 = width_of( value ) == 8;

        cthu::insn dup{ w8 ? "w₈" : "w₃₂", "dup",
                        w8 ? cthu::builtin::builtin_bv8dup : cthu::builtin::builtin_bv32dup };
        dup.add_in( stack );
        dup.add_out( copy_a );
        dup.add_out( copy_b );

        _current_subr->body.push_back( std::move( dup ) );

        _stack_of[ value ] = copy_b;
        return copy_a;
    }

    _pending_frees.push_back( stack );
    _stack_of.erase( value );
    return stack;
}

void codegen::emit_nibble( uint16_t stack, uint8_t value,
                           const std::string &struct_name, unsigned width )
{
    assert( value < 16 );

    static constexpr cthu::builtin cons32[] =
    {
        cthu::builtin::builtin_bv32cons_0,  cthu::builtin::builtin_bv32cons_1,
        cthu::builtin::builtin_bv32cons_2,  cthu::builtin::builtin_bv32cons_3,
        cthu::builtin::builtin_bv32cons_4,  cthu::builtin::builtin_bv32cons_5,
        cthu::builtin::builtin_bv32cons_6,  cthu::builtin::builtin_bv32cons_7,
        cthu::builtin::builtin_bv32cons_8,  cthu::builtin::builtin_bv32cons_9,
        cthu::builtin::builtin_bv32cons_10, cthu::builtin::builtin_bv32cons_11,
        cthu::builtin::builtin_bv32cons_12, cthu::builtin::builtin_bv32cons_13,
        cthu::builtin::builtin_bv32cons_14, cthu::builtin::builtin_bv32cons_15,
    };
    static constexpr cthu::builtin cons8[] =
    {
        cthu::builtin::builtin_bv8cons_0,  cthu::builtin::builtin_bv8cons_1,
        cthu::builtin::builtin_bv8cons_2,  cthu::builtin::builtin_bv8cons_3,
        cthu::builtin::builtin_bv8cons_4,  cthu::builtin::builtin_bv8cons_5,
        cthu::builtin::builtin_bv8cons_6,  cthu::builtin::builtin_bv8cons_7,
        cthu::builtin::builtin_bv8cons_8,  cthu::builtin::builtin_bv8cons_9,
        cthu::builtin::builtin_bv8cons_10, cthu::builtin::builtin_bv8cons_11,
        cthu::builtin::builtin_bv8cons_12, cthu::builtin::builtin_bv8cons_13,
        cthu::builtin::builtin_bv8cons_14, cthu::builtin::builtin_bv8cons_15,
    };

    const cthu::builtin *cons = width == 8 ? cons8 : cons32;
    cthu::insn i{ struct_name, "cons_" + std::to_string( value ), cons[ value ] };
    i.add_out( stack );
    _current_subr->body.push_back( std::move( i ) );
}

void codegen::append_nibble( uint16_t accumulator, uint16_t out, uint8_t value,
                             const std::string &struct_name, unsigned width )
{
    cthu::builtin shl = width == 8 ? cthu::builtin::builtin_bv8shl
                                   : cthu::builtin::builtin_bv32shl;
    cthu::builtin bit_or = width == 8 ? cthu::builtin::builtin_bv8or
                                      : cthu::builtin::builtin_bv32or;

    uint16_t shift_stack = allocate_stack();
    emit_nibble( shift_stack, 4, struct_name, width );

    uint16_t shifted = allocate_stack();
    cthu::insn shift_insn{ struct_name, "shl", shl };
    shift_insn.add_in( accumulator );
    shift_insn.add_in( shift_stack );
    shift_insn.add_out( shifted );
    _current_subr->body.push_back( std::move( shift_insn ) );

    /* These stacks are safe to reuse after the completed shl. Do not
     * commit _pending_frees here: they may belong to operands of the
     * LLVM instruction for which this constant is being materialized. */
    _free_stacks.push_back( accumulator );
    _free_stacks.push_back( shift_stack );

    uint16_t digit_stack = allocate_stack();
    emit_nibble( digit_stack, value, struct_name, width );

    cthu::insn or_insn{ struct_name, "or", bit_or };
    or_insn.add_in( shifted );
    or_insn.add_in( digit_stack );
    or_insn.add_out( out );
    _current_subr->body.push_back( std::move( or_insn ) );

    _free_stacks.push_back( shifted );
    _free_stacks.push_back( digit_stack );
}

void codegen::materialize_constant( uint16_t stack, llvm::ConstantInt &c, const std::string &struct_name )
{
    unsigned width = width_of( &c );

    uint64_t value = c.getZExtValue();
    unsigned shift = 0;

    for ( uint64_t rest = value; rest >= 16; rest >>= 4 )
        shift += 4;

    if ( shift == 0 )
    {
        emit_nibble( stack, value, struct_name, width );
        return;
    }

    uint16_t accumulator = allocate_stack();
    emit_nibble( accumulator, ( value >> shift ) & 0xf, struct_name, width );

    while ( shift > 0 )
    {
        shift -= 4;
        uint16_t out = shift == 0 ? stack : allocate_stack();
        append_nibble( accumulator, out, ( value >> shift ) & 0xf, struct_name, width );
        accumulator = out;
    }
}

unsigned codegen::width_of( llvm::Value *value )
{
    unsigned bits = llvm::cast< llvm::IntegerType >( value->getType() )->getBitWidth();
    assert( ( bits == 8 || bits == 32 ) && "only 8/32-bit integers are supported so far" );
    return bits;
}

std::string codegen::struct_name( bool is_unsigned, unsigned width )
{
    return width == 8 ? ( is_unsigned ? "u₈" : "i₈" ) : ( is_unsigned ? "u₃₂" : "i₃₂" );
}

std::string codegen::struct_name_for( llvm::Value *value )
{
    bool is_unsigned = false;

    if ( auto *type = debug_type( value ) )
        if ( auto *basic = llvm::dyn_cast< llvm::DIBasicType >( type ) )
            is_unsigned = basic->getEncoding() == llvm::dwarf::DW_ATE_unsigned;

    return struct_name( is_unsigned, width_of( value ) );
}

void codegen::commit_frees()
{
    for ( uint16_t stack : _pending_frees )
        _free_stacks.push_back( stack );

    _pending_frees.clear();
}

void codegen::visit( llvm::Instruction &instruction )
{
    llvm::InstVisitor< codegen >::visit( instruction );
    commit_frees();
}

void codegen::visitModule( llvm::Module & )
{}

void codegen::visitFunction( llvm::Function &function )
{
    _symtab.get_structure( &function );
    if ( function.getName() == "main" )
        _symtab.get_subroutine( &function.getEntryBlock() ).name = "run";
}

void codegen::visitBasicBlock( llvm::BasicBlock &block )
{
    _current_subr = &_symtab.get_subroutine( &block );

    _stack_of.clear();
    _remaining_uses.clear();
    _free_stacks.clear();
    _pending_frees.clear();
    _next_stack = 0;

    for ( auto &instruction : block )
        for ( auto &operand : instruction.operands() )
            ++ _remaining_uses[ operand.get() ];
}

void codegen::visitReturnInst( llvm::ReturnInst &instruction )
{
    if ( auto *value = instruction.getReturnValue() )
        use( value );
}

void codegen::binop_insn( llvm::Instruction &instruction,
                           const std::string &struct_name, const std::string &op_name, cthu::builtin code )
{
    /* Repeated-operand duplication (e.g. `a * a`) is handled generally by
     * use() now — it dup()s ahead of every read except the last one,
     * whether the repeats are within this one instruction or spread
     * across the subroutine. */
    uint16_t lhs = use( instruction.getOperand( 0 ), struct_name );
    uint16_t rhs = use( instruction.getOperand( 1 ), struct_name );
    uint16_t out = define( &instruction );

    cthu::insn i{ struct_name, op_name, code };
    i.add_in( lhs );
    i.add_in( rhs );
    i.add_out( out );

    _current_subr->body.push_back( std::move( i ) );
}

void codegen::cast_insn( llvm::CastInst &instruction, const std::string &source_struct_name,
                         const std::string &cast_struct_name, const std::string &op_name,
                         cthu::builtin code )
{
    uint16_t in = use( instruction.getOperand( 0 ), source_struct_name );
    uint16_t out = define( &instruction );

    cthu::insn i{ cast_struct_name, op_name, code };
    i.add_in( in );
    i.add_out( out );

    _current_subr->body.push_back( std::move( i ) );
}

void codegen::visitTruncInst( llvm::TruncInst &instruction )
{
    assert( width_of( instruction.getOperand( 0 ) ) == 32 );
    assert( width_of( &instruction ) == 8 );

    bool is_unsigned = struct_name_for( &instruction ) == "u₈";
    cast_insn( instruction, struct_name( is_unsigned, 32 ),
               is_unsigned ? "u₈³²" : "i₈³²", "cut", cthu::builtin::builtin_bv32cut8 );
}

void codegen::visitSExtInst( llvm::SExtInst &instruction )
{
    unsigned source_width = llvm::cast< llvm::IntegerType >(
        instruction.getOperand( 0 )->getType() )->getBitWidth();

    /* There is no Cthu boolean-to-bitvector conversion builtin yet. */
    if ( source_width == 1 )
        return;

    assert( width_of( instruction.getOperand( 0 ) ) == 8 );
    assert( width_of( &instruction ) == 32 );

    cast_insn( instruction, "i₈", "i₈³²", "ext", cthu::builtin::builtin_bv8sext32 );
}

void codegen::visitZExtInst( llvm::ZExtInst &instruction )
{
    unsigned source_width = llvm::cast< llvm::IntegerType >(
        instruction.getOperand( 0 )->getType() )->getBitWidth();

    /* Clang emits this for functions that return a comparison result. */
    if ( source_width == 1 )
        return;

    assert( width_of( instruction.getOperand( 0 ) ) == 8 );
    assert( width_of( &instruction ) == 32 );

    cast_insn( instruction, "u₈", "u₈³²", "ext", cthu::builtin::builtin_bv8zext32 );
}

void codegen::visitAdd( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8add : cthu::builtin::builtin_bv32add;
    binop_insn( instruction, struct_name_for( &instruction ), "add", code );
}

void codegen::visitSub( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8sub : cthu::builtin::builtin_bv32sub;
    binop_insn( instruction, struct_name_for( &instruction ), "sub", code );
}

void codegen::visitMul( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8mul : cthu::builtin::builtin_bv32mul;
    binop_insn( instruction, struct_name_for( &instruction ), "mul", code );
}

void codegen::visitUDiv( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8udiv : cthu::builtin::builtin_bv32udiv;
    binop_insn( instruction, struct_name( true, width ), "div", code );
}

void codegen::visitSDiv( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8sdiv : cthu::builtin::builtin_bv32sdiv;
    binop_insn( instruction, struct_name( false, width ), "div", code );
}

void codegen::visitURem( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8urem : cthu::builtin::builtin_bv32urem;
    binop_insn( instruction, struct_name( true, width ), "rem", code );
}

void codegen::visitSRem( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8srem : cthu::builtin::builtin_bv32srem;
    binop_insn( instruction, struct_name( false, width ), "rem", code );
}

void codegen::visitShl( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8shl : cthu::builtin::builtin_bv32shl;
    binop_insn( instruction, struct_name_for( &instruction ), "shl", code );
}

void codegen::visitLShr( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8lshr : cthu::builtin::builtin_bv32lshr;
    binop_insn( instruction, struct_name( true, width ), "shr", code );
}

void codegen::visitAShr( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8ashr : cthu::builtin::builtin_bv32ashr;
    binop_insn( instruction, struct_name( false, width ), "shr", code );
}

void codegen::visitAnd( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8and : cthu::builtin::builtin_bv32and;
    binop_insn( instruction, struct_name_for( &instruction ), "and", code );
}

void codegen::visitOr( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8or : cthu::builtin::builtin_bv32or;
    binop_insn( instruction, struct_name_for( &instruction ), "or", code );
}

void codegen::visitXor( llvm::BinaryOperator &instruction )
{
    unsigned width = width_of( &instruction );
    auto code = width == 8 ? cthu::builtin::builtin_bv8xor : cthu::builtin::builtin_bv32xor;
    binop_insn( instruction, struct_name_for( &instruction ), "xor", code );
}

void codegen::visitICmpInst( llvm::ICmpInst &instruction )
{
    /* icmp's own type is always i1 (the boolean result) — the width being
     * compared is the operands', not the instruction's own. */
    unsigned width = width_of( instruction.getOperand( 0 ) );
    bool w8 = width == 8;

    switch ( instruction.getPredicate() )
    {
        case llvm::CmpInst::ICMP_EQ:
            binop_insn( instruction, struct_name( false, width ), "eq?",
                        w8 ? cthu::builtin::builtin_bv8eq : cthu::builtin::builtin_bv32eq );
            break;
        case llvm::CmpInst::ICMP_NE:
            binop_insn( instruction, struct_name( false, width ), "ne?",
                        w8 ? cthu::builtin::builtin_bv8ne : cthu::builtin::builtin_bv32ne );
            break;
        case llvm::CmpInst::ICMP_SGT:
            binop_insn( instruction, struct_name( false, width ), "gt?",
                        w8 ? cthu::builtin::builtin_bv8sgt : cthu::builtin::builtin_bv32sgt );
            break;
        case llvm::CmpInst::ICMP_SGE:
            binop_insn( instruction, struct_name( false, width ), "ge?",
                        w8 ? cthu::builtin::builtin_bv8sge : cthu::builtin::builtin_bv32sge );
            break;
        case llvm::CmpInst::ICMP_SLT:
            binop_insn( instruction, struct_name( false, width ), "lt?",
                        w8 ? cthu::builtin::builtin_bv8slt : cthu::builtin::builtin_bv32slt );
            break;
        case llvm::CmpInst::ICMP_SLE:
            binop_insn( instruction, struct_name( false, width ), "le?",
                        w8 ? cthu::builtin::builtin_bv8sle : cthu::builtin::builtin_bv32sle );
            break;
        case llvm::CmpInst::ICMP_UGT:
            binop_insn( instruction, struct_name( true, width ), "gt?",
                        w8 ? cthu::builtin::builtin_bv8ugt : cthu::builtin::builtin_bv32ugt );
            break;
        case llvm::CmpInst::ICMP_UGE:
            binop_insn( instruction, struct_name( true, width ), "ge?",
                        w8 ? cthu::builtin::builtin_bv8uge : cthu::builtin::builtin_bv32uge );
            break;
        case llvm::CmpInst::ICMP_ULT:
            binop_insn( instruction, struct_name( true, width ), "lt?",
                        w8 ? cthu::builtin::builtin_bv8ult : cthu::builtin::builtin_bv32ult );
            break;
        case llvm::CmpInst::ICMP_ULE:
            binop_insn( instruction, struct_name( true, width ), "le?",
                        w8 ? cthu::builtin::builtin_bv8ule : cthu::builtin::builtin_bv32ule );
            break;
        default:
            break;
    }
}

}
