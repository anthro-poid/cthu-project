#include "codegen.hpp"
#include "builtin.hpp"
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
        auto type = type_to_structure( value->getType() );
        _current_subr->add_insn( type, cthu::builtin_operation::dup, { stack }, { copy_a, copy_b } );
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
    _current_subr->add_insn( structure, cthu::builtin_operation::drop,
                             { it->second }, {} );

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
    _current_subr->add_insn( structure, operation, {}, { stack } );
}

void codegen::append_nibble( uint16_t accumulator, uint16_t out, uint8_t value,
                             cthu::builtin_structure structure )
{
    uint16_t shift_stack = allocate_stack();
    emit_nibble( shift_stack, 4, structure );

    uint16_t shifted = allocate_stack();
    _current_subr->add_insn( structure, cthu::builtin_operation::shl,
                             { accumulator, shift_stack }, { shifted } );

    /* These stacks are safe to reuse after the completed shl. Do not
     * commit _pending_frees here: they may belong to operands of the
     * LLVM instruction for which this constant is being materialized. */
    add_free_stacks( accumulator, shift_stack );

    uint16_t digit_stack = allocate_stack();
    emit_nibble( digit_stack, value, structure );

    _current_subr->add_insn( structure, cthu::builtin_operation::bit_or,
                             { shifted, digit_stack }, { out } );

    add_free_stacks( shifted, digit_stack );
}

void codegen::materialize_constant( uint16_t stack, llvm::ConstantInt &c,
                                    cthu::builtin_structure structure )
{
    if ( c.getType()->isIntegerTy( 1 ) )
    {
        auto operation = c.isOne() ? cthu::builtin_operation::true_value
                                   : cthu::builtin_operation::false_value;
        _current_subr->add_insn( cthu::builtin_structure::boolean, operation,
                                 {}, { stack } );
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

static llvm::Value *incoming_value( llvm::BasicBlock &predecessor,
                                    llvm::BasicBlock &successor,
                                    llvm::Value *input )
{
    auto *phi = llvm::dyn_cast< llvm::PHINode >( input );

    if ( phi == nullptr || phi->getParent() != &successor )
        return input;

    llvm::Value *incoming = phi->getIncomingValueForBlock( &predecessor );
    assert( incoming && "PHI node has no value for predecessor" );
    return incoming;
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

            if ( llvm::isa< llvm::PHINode >( instruction ) )
            {
                live_ins[ &block ].insert( &instruction );
                continue;
            }

            for ( llvm::Value *operand : instruction.operands() )
                if ( llvm::isa< llvm::Argument >( operand ) ||
                     ( llvm::isa< llvm::Instruction >( operand ) &&
                       llvm::cast< llvm::Instruction >( operand )->getParent() != &block ) )
                    live_ins[ &block ].insert( operand );
        }

    bool changed;
    do
    {
        changed = false;

        for ( llvm::BasicBlock &block : llvm::reverse( function ) )
            for ( llvm::BasicBlock *successor : llvm::successors( &block ) )
                for ( llvm::Value *value : live_ins[ successor ] )
                {
                    value = incoming_value( block, *successor, value );

                    /* A null definition covers Arguments, which must propagate,
                     * but also constants, which the final ordered pass ignores.
                     * This can be narrowed to explicit Argument handling later. */
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
    {
        if ( llvm::isa< llvm::PHINode >( instruction ) )
            continue;

        for ( auto &operand : instruction.operands() )
            ++ _remaining_uses[ operand.get() ];
    }

    if ( auto *branch = llvm::dyn_cast< llvm::BranchInst >( block.getTerminator() ) )
    {
        if ( branch->isUnconditional() )
        {
            llvm::BasicBlock *successor = branch->getSuccessor( 0 );
            for ( llvm::Value *value : _block_inputs[ successor ] )
                ++ _remaining_uses[ incoming_value( block, *successor, value ) ];
        }
        else
        {
            auto targs = edge_arguments( *branch, branch->getSuccessor( 0 ) );
            auto fargs = edge_arguments( *branch, branch->getSuccessor( 1 ) );
            for ( llvm::Value *value : branch_arguments( targs, fargs ) )
                ++ _remaining_uses[ value ];
        }
    }

    for ( llvm::Value *value : _block_inputs[ &block ] )
        _current_subr->add_in( define( value ) );
}

void codegen::visitReturnInst( llvm::ReturnInst &instruction )
{
    if ( auto *value = instruction.getReturnValue() )
    {
        uint16_t output = use( value );
        auto &in_vec = _current_subr->get_input();

        if ( std::find( in_vec.begin(), in_vec.end(), output ) != in_vec.end() )
        {
            uint16_t moved = _next_stack ++;
            auto structure = type_to_structure( value->getType() );
            _current_subr->add_insn( structure, cthu::builtin_operation::move,
                                     { output }, { moved } );
            output = moved;
        }

        _current_subr->add_out( output );
    }

    drop_remaining_values();
}

std::vector< llvm::Value * > codegen::edge_arguments( llvm::BranchInst &instruction, llvm::BasicBlock *target )
{
    std::vector< llvm::Value * > arguments;

    for ( llvm::Value *input : _block_inputs[ target ] )
        arguments.push_back( incoming_value( *instruction.getParent(), *target, input ) );

    return arguments;
}

std::vector< llvm::Value * > codegen::branch_arguments( llvm::ArrayRef< llvm::Value * > first, 
                                                        llvm::ArrayRef< llvm::Value * > second )
{
    std::vector< llvm::Value * > args;

    auto append = [ &args ]( auto values )
    {
        for ( llvm::Value *value : values )
            if ( std::find( args.begin(), args.end(), value ) == args.end() )
                args.push_back( value );
    };

    append( first );
    append( second );

    return args;
}

cthu::subr_ref codegen::create_branch_frame( llvm::BranchInst &instruction,
                                             llvm::BasicBlock *target, 
                                             llvm::ArrayRef< llvm::Value * > arguments,
                                             llvm::ArrayRef< llvm::Value * > target_arguments )
{
    cthu::structure_ref structure = _symtab.get_structure( instruction.getFunction() );
    cthu::subr_ref frame = *structure.add_subroutine();
    uint16_t next_stack = 0;
    llvm::DenseMap< llvm::Value *, uint16_t > input_stacks;

    for ( llvm::Value *value : arguments )
    {
        input_stacks[ value ] = next_stack;
        frame.add_in( next_stack ++ );
    }

    llvm::DenseMap< llvm::Value *, unsigned > uses;

    for ( llvm::Value *value : target_arguments )
        ++ uses[ value ];

    llvm::DenseMap< llvm::Value *, std::vector< uint16_t > > available;

    for ( llvm::Value *value : arguments )
    {
        uint16_t stack  = input_stacks[ value ];
        unsigned copies = uses.lookup( value );

        if ( copies == 0 )
        {
            frame.add_insn( type_to_structure( value->getType() ),
                            cthu::builtin_operation::drop, { stack }, {} );
            continue;
        }

        for ( ; copies > 1; -- copies )
        {
            uint16_t first = next_stack ++;
            uint16_t rest  = next_stack ++;
            frame.add_insn( type_to_structure( value->getType() ),
                            cthu::builtin_operation::dup, { stack }, { first, rest } );
            available[ value ].push_back( first );
            stack = rest;
        }

        available[ value ].push_back( stack );
    }

    uint16_t function_stack = next_stack ++;
    frame.add_insn( structure, _symtab.get_subroutine( target ), {}, { function_stack } );

    llvm::Type *return_type = instruction.getFunction()->getReturnType();
    cthu::insn call{ function_structure_name( _block_inputs[ target ], return_type ),
                     cthu::builtin_operation::call, { function_stack } };

    for ( llvm::Value *value : target_arguments )
    {
        auto &stacks = available[ value ];
        assert( !stacks.empty() );
        call.add_in( stacks.back() );
        stacks.pop_back();
    }

    if ( !return_type->isVoidTy() )
    {
        uint16_t output = next_stack ++;
        call.add_out( output );
        frame.add_out( output );
    }

    frame.add_insn( std::move( call ) );
    return frame;
}

void codegen::visitConditionalBranch( llvm::BranchInst &instruction )
{
    llvm::BasicBlock *true_target = instruction.getSuccessor( 0 );
    llvm::BasicBlock *false_target = instruction.getSuccessor( 1 );
    std::vector< llvm::Value * > true_arguments = edge_arguments( instruction,
                                                                  true_target );
    std::vector< llvm::Value * > false_arguments = edge_arguments( instruction,
                                                                   false_target );
    std::vector< llvm::Value * > arguments = branch_arguments( true_arguments,
                                                               false_arguments );
    llvm::Type *return_type = instruction.getFunction()->getReturnType();
    std::vector< llvm::Type * > types;

    for ( llvm::Value *value : arguments )
        types.push_back( value->getType() );

    _generated_function_types.emplace_back( types, return_type );
    std::string function_structure = function_structure_name( types, return_type );
    cthu::subr_ptr true_subroutine = true_arguments == arguments
        ? &_symtab.get_subroutine( true_target )
        : &create_branch_frame( instruction, true_target, arguments, true_arguments );
    cthu::subr_ptr false_subroutine = false_arguments == arguments
        ? &_symtab.get_subroutine( false_target )
        : &create_branch_frame( instruction, false_target, arguments, false_arguments );

    uint16_t condition = use( instruction.getCondition(), cthu::builtin_structure::boolean );
    uint16_t true_condition = allocate_stack();
    uint16_t condition_to_negate = allocate_stack();
    _current_subr->add_insn( cthu::builtin_structure::boolean,
                             cthu::builtin_operation::dup, { condition },
                             { true_condition, condition_to_negate } );

    uint16_t false_condition = allocate_stack();
    _current_subr->add_insn( cthu::builtin_structure::boolean,
                             cthu::builtin_operation::logical_not,
                             { condition_to_negate }, { false_condition } );

    cthu::structure_ref structure = _symtab.get_structure( instruction.getFunction() );
    uint16_t true_function = allocate_stack();
    _current_subr->add_insn( structure, *true_subroutine, {}, { true_function } );

    uint16_t false_function = allocate_stack();
    _current_subr->add_insn( structure, *false_subroutine, {}, { false_function } );

    uint16_t true_alternative = allocate_stack();
    _current_subr->add_insn( function_structure, cthu::builtin_operation::opt,
                             { true_condition, true_function }, { true_alternative } );

    uint16_t false_alternative = allocate_stack();
    _current_subr->add_insn( function_structure, cthu::builtin_operation::opt,
                             { false_condition, false_function }, { false_alternative } );

    uint16_t continuation = allocate_stack();
    _current_subr->add_insn( function_structure, cthu::builtin_operation::join,
                             { true_alternative, false_alternative }, { continuation } );

    cthu::insn call{ function_structure, cthu::builtin_operation::call, { continuation } };

    for ( llvm::Value *value : arguments )
        call.add_in( use( value, arithmetic_structure( value ) ) );

    drop_remaining_values();

    if ( !return_type->isVoidTy() )
    {
        uint16_t output = _next_stack ++;
        call.add_out( output );
        _current_subr->add_out( output );
    }

    _current_subr->add_insn( std::move( call ) );
}

void codegen::visitBranchInst( llvm::BranchInst &instruction )
{
    if ( instruction.isConditional() )
        return visitConditionalBranch( instruction );

    llvm::BasicBlock *target = instruction.getSuccessor( 0 );
    auto &target_inputs = _block_inputs[ target ];

    auto function_stack = allocate_stack();
    _current_subr->add_insn( _symtab.get_structure( instruction.getFunction() ),
                             _symtab.get_subroutine( target ), {}, { function_stack } );

    llvm::Type *return_type = instruction.getFunction()->getReturnType();
    cthu::insn call{ function_structure_name( target_inputs, return_type ),
                     cthu::builtin_operation::call, { function_stack } };

    for ( llvm::Value *value : target_inputs )
    {
        llvm::Value *incoming = incoming_value( *instruction.getParent(), *target, value );
        call.add_in( use( incoming, arithmetic_structure( incoming ) ) );
    }

    drop_remaining_values();

    if ( !return_type->isVoidTy() )
    {
        uint16_t output = _next_stack ++;
        call.add_out( output );
        _current_subr->add_out( output );
    }

    _current_subr->add_insn( std::move( call ) );
    _pending_frees.push_back( function_stack );
}

void codegen::visitCallInst( llvm::CallInst &instruction )
{
    llvm::Function *callee = instruction.getCalledFunction();

    if ( callee && callee->isIntrinsic() )
        return;

    assert( callee && !callee->isDeclaration() && "only direct calls to defined functions are supported" );
    uint16_t function_stack = allocate_stack();
    _current_subr->add_insn( _symtab.get_structure( callee ),
                             _symtab.get_subroutine( &callee->getEntryBlock() ),
                             {}, { function_stack } );

    cthu::insn call{ function_structure_name( callee->getFunctionType() ), cthu::builtin_operation::call, { function_stack } };

    for ( llvm::Value *argument : instruction.args() )
        call.add_in( use( argument, arithmetic_structure( argument ) ) );

    if ( !instruction.getType()->isVoidTy() )
        call.add_out( define( &instruction ) );

    _current_subr->add_insn( std::move( call ) );
    _pending_frees.push_back( function_stack );
}

void codegen::visitPHINode( llvm::PHINode & )
{}

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
    _current_subr->add_insn( cthu::builtin_structure::boolean,
                             cthu::builtin_operation::dup, { condition },
                             { true_condition, condition_to_negate } );

    uint16_t false_condition = allocate_stack();
    _current_subr->add_insn( cthu::builtin_structure::boolean,
                             cthu::builtin_operation::logical_not,
                             { condition_to_negate }, { false_condition } );

    uint16_t true_result = allocate_stack();
    _current_subr->add_insn( structure, cthu::builtin_operation::opt,
                             { true_condition, when_true }, { true_result } );

    uint16_t false_result = allocate_stack();
    _current_subr->add_insn( structure, cthu::builtin_operation::opt,
                             { false_condition, when_false }, { false_result } );

    uint16_t out = define( &instruction );
    _current_subr->add_insn( structure, cthu::builtin_operation::join,
                             { true_result, false_result }, { out } );

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

    _current_subr->add_insn( structure, operation, { lhs, rhs }, { out } );
}

void codegen::cast_insn( llvm::CastInst &instruction,
                         cthu::builtin_structure source_structure,
                         cthu::builtin_structure cast_structure,
                         cthu::builtin_operation operation )
{
    uint16_t in = use( instruction.getOperand( 0 ), source_structure );
    uint16_t out = define( &instruction );

    _current_subr->add_insn( cast_structure, operation, { in }, { out } );
}

void codegen::bool_sext_insn( llvm::CastInst &instruction, unsigned width )
{
    uint16_t in = use( instruction.getOperand( 0 ), cthu::builtin_structure::boolean );
    uint16_t extended = allocate_stack();

    auto conversion = width == 8 ? cthu::builtin_structure::bool_8
                                 : cthu::builtin_structure::bool_32;
    _current_subr->add_insn( conversion, cthu::builtin_operation::ext,
                             { in }, { extended } );

    uint16_t zero = allocate_stack();
    auto signed_structure = arithmetic_structure( false, width );
    emit_nibble( zero, 0, signed_structure );

    uint16_t out = define( &instruction );
    _current_subr->add_insn( signed_structure, cthu::builtin_operation::sub,
                             { zero, extended }, { out } );

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
