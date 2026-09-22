#include "codegen.hpp"
#include "mapping.hpp"

#include <llvm/ADT/DenseSet.h>
#include <llvm/IR/CFG.h>

#include <algorithm>
#include <utility>

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

uint16_t codegen::use( llvm::Value *value, cthu::builtin_structure structure )
{
    auto it = _stack_of.find( value );
    uint16_t stack;

    if ( it != _stack_of.end() )
        stack = it->second;
    else
    {
        stack = define( value );

        if ( auto *c = llvm::dyn_cast< llvm::ConstantInt >( value ) )
            materialize_constant( stack, *c, structure );
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
        auto value_type = type_to_structure( value->getType() );
        cthu::insn dup{ value_type, cthu::builtin_operation::dup };
        dup.add_in( stack );
        dup.add_out( copy_a, copy_b );

        _current_subr->body.push_back( std::move( dup ) );

        _stack_of[ value ] = copy_b;
        return copy_a;
    }

    _pending_frees.push_back( stack );
    _stack_of.erase( value );
    return stack;
}

void codegen::drop_unused( llvm::Value *value )
{
    auto it = _stack_of.find( value );

    if ( it == _stack_of.end() )
        return;

    auto structure = type_to_structure( value->getType() );
    cthu::insn drop{ structure, cthu::builtin_operation::drop };
    drop.add_in( it->second );
    _current_subr->body.push_back( std::move( drop ) );

    _pending_frees.push_back( it->second );
    _stack_of.erase( it );
}

void codegen::drop_remaining_values()
{
    std::vector< llvm::Value * > values;

    for ( auto [ value, stack ] : _stack_of )
        values.emplace_back( value );

    for ( auto value : values )
        drop_unused( value );
}

void codegen::emit_nibble( uint16_t stack, uint8_t value,
                           cthu::builtin_structure structure )
{
    assert( value < 16 );

    auto operation = cthu::nibble_operation( value );
    cthu::insn i{ structure, operation };
    i.add_out( stack );
    _current_subr->body.push_back( std::move( i ) );
}

void codegen::append_nibble( uint16_t accumulator, uint16_t out, uint8_t value,
                             cthu::builtin_structure structure )
{
    uint16_t shift_stack = allocate_stack();
    emit_nibble( shift_stack, 4, structure );

    uint16_t shifted = allocate_stack();
    cthu::insn shift_insn{ structure, cthu::builtin_operation::shl };
    shift_insn.add_in( accumulator, shift_stack );
    shift_insn.add_out( shifted );
    _current_subr->body.push_back( std::move( shift_insn ) );

    /* These stacks are safe to reuse after the completed shl. Do not
     * commit _pending_frees here: they may belong to operands of the
     * LLVM instruction for which this constant is being materialized. */
    add_free_stacks( accumulator, shift_stack );

    uint16_t digit_stack = allocate_stack();
    emit_nibble( digit_stack, value, structure );

    cthu::insn or_insn{ structure, cthu::builtin_operation::bit_or };
    or_insn.add_in( shifted, digit_stack );
    or_insn.add_out( out );
    _current_subr->body.push_back( std::move( or_insn ) );

    add_free_stacks( shifted, digit_stack );
}

void codegen::materialize_constant( uint16_t stack, llvm::ConstantInt &c,
                                    cthu::builtin_structure structure )
{
    if ( c.getType()->isIntegerTy( 1 ) )
    {
        auto operation = c.isOne() ? cthu::builtin_operation::true_value
                                                      : cthu::builtin_operation::false_value;
        cthu::insn cons{ cthu::builtin_structure::boolean, operation };
        cons.add_out( stack );
        _current_subr->body.push_back( std::move( cons ) );
        return;
    }

    uint64_t value = c.getZExtValue();
    unsigned shift = 0;

    for ( uint64_t rest = value; rest >= 16; rest >>= 4 )
        shift += 4;

    if ( shift == 0 )
    {
        emit_nibble( stack, value, structure );
        return;
    }

    uint16_t accumulator = allocate_stack();
    emit_nibble( accumulator, ( value >> shift ) & 0xf, structure );

    while ( shift > 0 )
    {
        shift -= 4;
        uint16_t out = shift == 0 ? stack : allocate_stack();
        append_nibble( accumulator, out, ( value >> shift ) & 0xf, structure );
        accumulator = out;
    }
}

static bool unify_conditional_inputs( llvm::Function &function, auto &live_ins )
{
    bool changed = false;

    for ( llvm::BasicBlock &block : function )
    {
        auto *branch = llvm::dyn_cast< llvm::BranchInst >( block.getTerminator() );
        if ( branch == nullptr || branch->isUnconditional() )
            continue;

        auto &true_inputs = live_ins[ branch->getSuccessor( 0 ) ];
        auto &false_inputs = live_ins[ branch->getSuccessor( 1 ) ];
        std::vector< llvm::Value * > inputs( true_inputs.begin(), true_inputs.end() );

        for ( llvm::Value *value : false_inputs )
            if ( true_inputs.insert( value ).second )
            {
                inputs.push_back( value );
                changed = true;
            }

        for ( llvm::Value *value : inputs )
            changed |= false_inputs.insert( value ).second;
    }

    return changed;
}

void codegen::compute_block_inputs( llvm::Function &function )
{
    llvm::DenseMap< llvm::BasicBlock *, llvm::DenseSet< llvm::Value * > > live_ins;
    std::vector< llvm::Value * > values;

    for ( llvm::Argument &argument : function.args() )
        values.push_back( &argument );

    for ( llvm::BasicBlock &block : function )
        for ( llvm::Instruction &instruction : block )
        {
            values.push_back( &instruction );

            for ( llvm::Value *operand : instruction.operands() )
                if ( llvm::isa< llvm::Argument >( operand ) ||
                     ( llvm::isa< llvm::Instruction >( operand ) &&
                       llvm::cast< llvm::Instruction >( operand )->getParent() != &block ) )
                    live_ins[ &block ].insert( operand );
        }

    bool changed;
    do
    {
        changed = unify_conditional_inputs( function, live_ins );

        for ( llvm::BasicBlock &block : llvm::reverse( function ) )
            for ( llvm::BasicBlock *successor : llvm::successors( &block ) )
                for ( llvm::Value *value : live_ins[ successor ] )
                {
                    /* Definition is either llvm::Instruction or llvm::Argument. */
                    auto *definition = llvm::dyn_cast< llvm::Instruction >( value );

                    if ( definition == nullptr || definition->getParent() != &block )
                        changed |= live_ins[ &block ].insert( value ).second;
                }
    }
    while ( changed );

    for ( llvm::BasicBlock &block : function )
    {
        auto &inputs = _block_inputs[ &block ];

        if ( &block == &function.getEntryBlock() )
            for ( llvm::Argument &argument : function.args() )
                inputs.push_back( &argument );
        else
            for ( llvm::Value *value : values )
                if ( live_ins[ &block ].contains( value ) )
                    inputs.push_back( value );
    }
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

    if ( !instruction.getType()->isVoidTy() && _remaining_uses.lookup( &instruction ) == 0 )
        drop_unused( &instruction );

    commit_frees();
}

void codegen::visitModule( llvm::Module & )
{}

void codegen::visitFunction( llvm::Function &function )
{
    compute_block_inputs( function );
    _symtab.get_structure( &function );
    _symtab.get_subroutine( &function.getEntryBlock() ).name =
        function.getName() == "main" ? "run" : function.getName().str();
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

    for ( llvm::Value *value : _block_inputs[ &block ] )
        _current_subr->input.push_back( define( value ) );

    if ( auto *branch = llvm::dyn_cast< llvm::BranchInst >( block.getTerminator() ) )
        for ( llvm::Value *value : _block_inputs[ branch->getSuccessor( 0 ) ] )
            ++ _remaining_uses[ value ];
}

void codegen::visitReturnInst( llvm::ReturnInst &instruction )
{
    if ( auto *value = instruction.getReturnValue() )
    {
        uint16_t output = use( value );

        if ( std::find( _current_subr->input.begin(), _current_subr->input.end(), output )
                != _current_subr->input.end() )
        {
            uint16_t moved = _next_stack ++;
            auto structure = type_to_structure( value->getType() );
            cthu::insn move{ structure, cthu::builtin_operation::move };
            move.add_in( output );
            move.add_out( moved );
            _current_subr->body.push_back( std::move( move ) );
            output = moved;
        }

        _current_subr->output.push_back( output );
    }

    drop_remaining_values();
}

void codegen::visitConditionalBranch( llvm::BranchInst &instruction )
{
    llvm::BasicBlock *target = instruction.getSuccessor( 0 );
    auto &target_inputs = _block_inputs[ target ];

    llvm::BasicBlock *false_target = instruction.getSuccessor( 1 );
    auto &branch_inputs = target_inputs;
    llvm::Type *return_type = instruction.getFunction()->getReturnType();
    std::string function_structure = function_structure_name( branch_inputs, return_type );

    uint16_t condition = use( instruction.getCondition(), cthu::builtin_structure::boolean );
    uint16_t true_condition = allocate_stack();
    uint16_t condition_to_negate = allocate_stack();
    cthu::insn duplicate{ cthu::builtin_structure::boolean,
                          cthu::builtin_operation::dup };
    duplicate.add_in( condition );
    duplicate.add_out( true_condition, condition_to_negate );
    _current_subr->body.push_back( std::move( duplicate ) );

    uint16_t false_condition = allocate_stack();
    cthu::insn negate{ cthu::builtin_structure::boolean,
                       cthu::builtin_operation::logical_not };
    negate.add_in( condition_to_negate );
    negate.add_out( false_condition );
    _current_subr->body.push_back( std::move( negate ) );

    cthu::structure_ref structure = _symtab.get_structure( instruction.getFunction() );
    uint16_t true_function = allocate_stack();
    cthu::insn true_value{ structure, _symtab.get_subroutine( target ) };
    true_value.add_out( true_function );
    _current_subr->body.push_back( std::move( true_value ) );

    uint16_t false_function = allocate_stack();
    cthu::insn false_value{ structure, _symtab.get_subroutine( false_target ) };
    false_value.add_out( false_function );
    _current_subr->body.push_back( std::move( false_value ) );

    uint16_t true_opt = allocate_stack();
    cthu::insn true_opt_value{ function_structure, cthu::builtin_operation::opt };
    true_opt_value.add_out( true_opt );
    _current_subr->body.push_back( std::move( true_opt_value ) );

    uint16_t true_alternative = allocate_stack();
    cthu::insn choose_true{ "f_b_f__f", cthu::builtin_operation::call };
    choose_true.add_in( true_opt, true_condition, true_function );
    choose_true.add_out( true_alternative );
    _current_subr->body.push_back( std::move( choose_true ) );

    uint16_t false_opt = allocate_stack();
    cthu::insn false_opt_value{ function_structure, cthu::builtin_operation::opt };
    false_opt_value.add_out( false_opt );
    _current_subr->body.push_back( std::move( false_opt_value ) );

    uint16_t false_alternative = allocate_stack();
    cthu::insn choose_false{ "f_b_f__f", cthu::builtin_operation::call };
    choose_false.add_in( false_opt, false_condition, false_function );
    choose_false.add_out( false_alternative );
    _current_subr->body.push_back( std::move( choose_false ) );

    uint16_t join = allocate_stack();
    cthu::insn join_value{ function_structure, cthu::builtin_operation::join };
    join_value.add_out( join );
    _current_subr->body.push_back( std::move( join_value ) );

    uint16_t continuation = allocate_stack();
    cthu::insn join_alternatives{ "f_f_f__f", cthu::builtin_operation::call };
    join_alternatives.add_in( join, true_alternative, false_alternative );
    join_alternatives.add_out( continuation );
    _current_subr->body.push_back( std::move( join_alternatives ) );

    cthu::insn call{ function_structure, cthu::builtin_operation::call };
    call.add_in( continuation );
    for ( llvm::Value *value : branch_inputs )
            call.add_in( use( value, arithmetic_structure( value ) ) );

    drop_remaining_values();

    if ( !return_type->isVoidTy() )
    {
        uint16_t output = _next_stack ++;
        call.add_out( output );
        _current_subr->output.push_back( output );
    }

    _current_subr->body.push_back( std::move( call ) );
}

void codegen::visitBranchInst( llvm::BranchInst &instruction )
{
    if ( instruction.isConditional() )
        return visitConditionalBranch( instruction );

    llvm::BasicBlock *target = instruction.getSuccessor( 0 );
    auto &target_inputs = _block_inputs[ target ];

    auto function_stack = allocate_stack();
    cthu::insn func{ _symtab.get_structure( instruction.getFunction() ),
                        _symtab.get_subroutine( target ) };
    func.add_out( function_stack );
    _current_subr->body.push_back( std::move( func ) );

    llvm::Type *return_type = instruction.getFunction()->getReturnType();
    cthu::insn call{ function_structure_name( target_inputs, return_type ), cthu::builtin_operation::call };
    call.add_in( function_stack );

    for ( llvm::Value *value : target_inputs )
        call.add_in( use( value, arithmetic_structure( value ) ) );

    drop_remaining_values();

    if ( !return_type->isVoidTy() )
    {
        uint16_t output = _next_stack ++;
        call.add_out( output );
        _current_subr->output.push_back( output );
    }

    _current_subr->body.push_back( std::move( call ) );
    _pending_frees.push_back( function_stack );
}

void codegen::visitCallInst( llvm::CallInst &instruction )
{
    llvm::Function *callee = instruction.getCalledFunction();

    if ( callee && callee->isIntrinsic() )
        return;

    assert( callee && !callee->isDeclaration() && "only direct calls to defined functions are supported" );
    uint16_t function_stack = allocate_stack();
    cthu::insn function_value{ _symtab.get_structure( callee ), _symtab.get_subroutine( &callee->getEntryBlock() ) };
    function_value.add_out( function_stack );
    _current_subr->body.push_back( std::move( function_value ) );

    cthu::insn call{ function_structure_name( callee->getFunctionType() ), cthu::builtin_operation::call };
    call.add_in( function_stack );

    for ( llvm::Value *argument : instruction.args() )
        call.add_in( use( argument, arithmetic_structure( argument ) ) );

    if ( !instruction.getType()->isVoidTy() )
        call.add_out( define( &instruction ) );

    _current_subr->body.push_back( std::move( call ) );
    _pending_frees.push_back( function_stack );
}

void codegen::visitPHINode( llvm::PHINode & )
{
    assert( false && "PHI nodes are not supported yet" );
}

void codegen::visitSelectInst( llvm::SelectInst &instruction )
{
    llvm::Type *type = instruction.getType();
    assert( type->isIntegerTy() && "only integer select values are supported so far" );

    auto structure = type_to_structure( type );

    uint16_t condition  = use( instruction.getCondition(), cthu::builtin_structure::boolean );
    uint16_t when_true  = use( instruction.getTrueValue(), arithmetic_structure( &instruction ) );
    uint16_t when_false = use( instruction.getFalseValue(), arithmetic_structure( &instruction ) );

    uint16_t true_condition = allocate_stack();
    uint16_t condition_to_negate = allocate_stack();
    cthu::insn duplicate{ cthu::builtin_structure::boolean, cthu::builtin_operation::dup };
    duplicate.add_in( condition );
    duplicate.add_out( true_condition, condition_to_negate );
    _current_subr->body.push_back( std::move( duplicate ) );

    uint16_t false_condition = allocate_stack();
    cthu::insn negate{ cthu::builtin_structure::boolean, cthu::builtin_operation::logical_not };
    negate.add_in( condition_to_negate );
    negate.add_out( false_condition );
    _current_subr->body.push_back( std::move( negate ) );

    uint16_t true_result = allocate_stack();
    cthu::insn choose_true{ structure, cthu::builtin_operation::opt };
    choose_true.add_in( true_condition, when_true );
    choose_true.add_out( true_result );
    _current_subr->body.push_back( std::move( choose_true ) );

    uint16_t false_result = allocate_stack();
    cthu::insn choose_false{ structure, cthu::builtin_operation::opt };
    choose_false.add_in( false_condition, when_false );
    choose_false.add_out( false_result );
    _current_subr->body.push_back( std::move( choose_false ) );

    uint16_t out = define( &instruction );
    cthu::insn join_insn{ structure, cthu::builtin_operation::join };
    join_insn.add_in( true_result, false_result );
    join_insn.add_out( out );
    _current_subr->body.push_back( std::move( join_insn ) );

    add_free_stacks( true_condition, condition_to_negate, false_condition, true_result, false_result );
}

void codegen::binop_insn( llvm::Instruction &instruction,
                          cthu::builtin_structure structure,
                          cthu::builtin_operation operation )
{
    /* Repeated-operand duplication (e.g. `a * a`) is handled generally by
     * use() now — it dup()s ahead of every read except the last one,
     * whether the repeats are within this one instruction or spread
     * across the subroutine. */
    uint16_t lhs = use( instruction.getOperand( 0 ), structure );
    uint16_t rhs = use( instruction.getOperand( 1 ), structure );
    uint16_t out = define( &instruction );

    cthu::insn i{ structure, operation };
    i.add_in( lhs, rhs );
    i.add_out( out );

    _current_subr->body.push_back( std::move( i ) );
}

void codegen::cast_insn( llvm::CastInst &instruction,
                         cthu::builtin_structure source_structure,
                         cthu::builtin_structure cast_structure,
                         cthu::builtin_operation operation )
{
    uint16_t in = use( instruction.getOperand( 0 ), source_structure );
    uint16_t out = define( &instruction );

    cthu::insn i{ cast_structure, operation };
    i.add_in( in );
    i.add_out( out );

    _current_subr->body.push_back( std::move( i ) );
}

void codegen::bool_sext_insn( llvm::CastInst &instruction, unsigned width )
{
    uint16_t in = use( instruction.getOperand( 0 ), cthu::builtin_structure::boolean );
    uint16_t extended = allocate_stack();

    auto conversion = width == 8 ? cthu::builtin_structure::bool_8
                                 : cthu::builtin_structure::bool_32;
    cthu::insn ext{ conversion, cthu::builtin_operation::ext };
    ext.add_in( in );
    ext.add_out( extended );
    _current_subr->body.push_back( std::move( ext ) );

    uint16_t zero = allocate_stack();
    auto signed_structure = arithmetic_structure( false, width );
    emit_nibble( zero, 0, signed_structure );

    uint16_t out = define( &instruction );
    cthu::insn negate{ signed_structure, cthu::builtin_operation::sub };
    negate.add_in( zero, extended );
    negate.add_out( out );
    _current_subr->body.push_back( std::move( negate ) );

    add_free_stacks( zero, extended );
}

void codegen::visitTruncInst( llvm::TruncInst &instruction )
{
    unsigned target_width = integer_width( &instruction );

    if ( target_width == 1 )
    {
        unsigned source_width = integer_width( instruction.getOperand( 0 ) );
        auto conversion = source_width == 8 ? cthu::builtin_structure::bool_8
                                            : cthu::builtin_structure::bool_32;
        cast_insn( instruction, arithmetic_structure( instruction.getOperand( 0 ) ),
                   conversion, cthu::builtin_operation::cut );
        return;
    }

    assert( integer_width( instruction.getOperand( 0 ) ) == 32 );
    assert( integer_width( &instruction ) == 8 );

    bool is_unsigned = arithmetic_structure( &instruction ) == cthu::builtin_structure::u8;
    auto conversion = is_unsigned ? cthu::builtin_structure::u8_32
                                  : cthu::builtin_structure::i8_32;
    cast_insn( instruction, arithmetic_structure( is_unsigned, 32 ),
               conversion, cthu::builtin_operation::cut );
}

void codegen::visitSExtInst( llvm::SExtInst &instruction )
{
    unsigned source_width = integer_width( instruction.getOperand( 0 ) );

    if ( source_width == 1 )
    {
        unsigned target_width = integer_width( &instruction );
        bool_sext_insn( instruction, target_width );
        return;
    }

    assert( integer_width( instruction.getOperand( 0 ) ) == 8 );
    assert( integer_width( &instruction ) == 32 );

    cast_insn( instruction, cthu::builtin_structure::i8,
               cthu::builtin_structure::i8_32, cthu::builtin_operation::ext );
}

void codegen::visitZExtInst( llvm::ZExtInst &instruction )
{
    unsigned source_width = integer_width( instruction.getOperand( 0 ) );

    if ( source_width == 1 )
    {
        unsigned target_width = integer_width( &instruction );
        auto conversion = target_width == 8 ? cthu::builtin_structure::bool_8
                                            : cthu::builtin_structure::bool_32;
        cast_insn( instruction, cthu::builtin_structure::boolean,
                   conversion, cthu::builtin_operation::ext );
        return;
    }

    assert( integer_width( instruction.getOperand( 0 ) ) == 8 );
    assert( integer_width( &instruction ) == 32 );

    cast_insn( instruction, cthu::builtin_structure::u8,
               cthu::builtin_structure::u8_32, cthu::builtin_operation::ext );
}

void codegen::visitAdd( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( &instruction ), cthu::builtin_operation::add );
}

void codegen::visitSub( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( &instruction ), cthu::builtin_operation::sub );
}

void codegen::visitMul( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( &instruction ), cthu::builtin_operation::mul );
}

void codegen::visitUDiv( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( true, integer_width( &instruction ) ), cthu::builtin_operation::div );
}

void codegen::visitSDiv( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( false, integer_width( &instruction ) ), cthu::builtin_operation::div );
}

void codegen::visitURem( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( true, integer_width( &instruction ) ), cthu::builtin_operation::rem );
}

void codegen::visitSRem( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( false, integer_width( &instruction ) ), cthu::builtin_operation::rem );
}

void codegen::visitShl( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( &instruction ), cthu::builtin_operation::shl );
}

void codegen::visitLShr( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( true, integer_width( &instruction ) ), cthu::builtin_operation::shr );
}

void codegen::visitAShr( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( false, integer_width( &instruction ) ), cthu::builtin_operation::shr );
}

void codegen::visitAnd( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( &instruction ), cthu::builtin_operation::bit_and );
}

void codegen::visitOr( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( &instruction ), cthu::builtin_operation::bit_or );
}

void codegen::visitXor( llvm::BinaryOperator &instruction )
{
    binop_insn( instruction, arithmetic_structure( &instruction ), cthu::builtin_operation::bit_xor );
}

void codegen::visitICmpInst( llvm::ICmpInst &instruction )
{
    cthu::builtin_operation operation;

    switch ( instruction.getPredicate() )
    {
        case llvm::CmpInst::ICMP_EQ:  operation = cthu::builtin_operation::equal;         break;
        case llvm::CmpInst::ICMP_NE:  operation = cthu::builtin_operation::not_equal;     break;
        case llvm::CmpInst::ICMP_SGT: operation = cthu::builtin_operation::greater;       break;
        case llvm::CmpInst::ICMP_SGE: operation = cthu::builtin_operation::greater_equal; break;
        case llvm::CmpInst::ICMP_SLT: operation = cthu::builtin_operation::less;          break;
        case llvm::CmpInst::ICMP_SLE: operation = cthu::builtin_operation::less_equal;    break;
        case llvm::CmpInst::ICMP_UGT: operation = cthu::builtin_operation::greater;       break;
        case llvm::CmpInst::ICMP_UGE: operation = cthu::builtin_operation::greater_equal; break;
        case llvm::CmpInst::ICMP_ULT: operation = cthu::builtin_operation::less;          break;
        case llvm::CmpInst::ICMP_ULE: operation = cthu::builtin_operation::less_equal;    break;
        default: __builtin_unreachable();
    }

    unsigned width = integer_width( instruction.getOperand( 0 ) );
    binop_insn( instruction, arithmetic_structure( instruction.isUnsigned(), width ), operation );
}
}
