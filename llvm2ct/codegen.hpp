#pragma once

#include "symtab.hpp"

#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/InstVisitor.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace llvm2ct
{
    struct codegen : llvm::InstVisitor< codegen >
    {
        llvm::LLVMContext &_context;
        std::unique_ptr< llvm::Module > _module;
        cthu::symtab _symtab;

        /* Which Cthu stack currently holds a given LLVM value, scoped to
         * whichever block is presently being visited — stack numbering is
         * local to one subroutine, so this is reset per basic block.
         *
         * An operand only has 9 encoding bits (511 usable stacks), so
         * slots are reclaimed as soon as a value's last use within the
         * block has been processed, tracked via _remaining_uses (populated
         * by a pre-scan in visitBasicBlock).
         *
         * A freed slot goes into _pending_frees, not straight into
         * _free_stacks — a stack may only appear once total across an
         * instruction's inputs *and* output, so define() must not be able
         * to reuse a slot one of THIS SAME instruction's own operands just
         * freed. commit_frees() moves _pending_frees into _free_stacks and
         * must be called only once an instruction (inputs + output) is
         * fully built — see binop_insn. Values passed to successor blocks
         * are included in _remaining_uses by visitBasicBlock. */
        llvm::DenseMap< llvm::Value *, uint16_t > _stack_of;
        llvm::DenseMap< llvm::Value *, unsigned > _remaining_uses;
        llvm::DenseMap< llvm::BasicBlock *, std::vector< llvm::Value * > > _block_inputs;
        std::vector< uint16_t > _free_stacks;
        std::vector< uint16_t > _pending_frees;
        uint16_t _next_stack = 0;

        template< typename... Stacks >
        void add_free_stacks( Stacks... stacks ) { ( _free_stacks.push_back( stacks ), ... ); }

        /* The subr_t being filled in while visiting the current block's
         * instructions — set in visitBasicBlock, read by the instruction
         * visitors. A pointer, not a reference, since it's reseated once
         * per block. */
        cthu::subr_t *_current_subr = nullptr;

        codegen( llvm::LLVMContext &context, std::unique_ptr< llvm::Module > module );

        /* Raw "give me an unused stack" — no llvm::Value to key it by, for
         * a dup's second output (see binop_insn), which doesn't correspond
         * to any single Value. define() is this plus registering the
         * result under a Value. */
        uint16_t allocate_stack();

        uint16_t define( llvm::Value *value );

        /* structure is passed through to materialize_constant() if value
         * turns out to be a fresh ConstantInt — see there for why it can't
         * decide its own structure. Defaults to i32 for call sites with
         * no natural signedness context (e.g. visitStoreInst). */
        uint16_t use( llvm::Value *value,
                      cthu::builtin_structure structure = cthu::builtin_structure::i32 );
        void drop_unused( llvm::Value *value );
        void drop_remaining_values();
        void commit_frees();

        /* use() calls this the first time it sees a value with no producer
         * of its own — that's correct for an llvm::Argument (its value
         * arrives via the call), but a ConstantInt needs an actual value
         * emitted, or the compiled output reads from a stack nothing ever
         * wrote to. Values above 15 are assembled one hexadecimal digit at
         * a time with cons_N, shl, and or.
         *
         * structure is the caller's choice, not derived here: a
         * ConstantInt is uniqued by LLVM (every literal `5` of type i32 is
         * the same Value*), so debug_type() on the constant itself can
         * resolve to an arbitrary one of possibly several unrelated source
         * variables that happen to share that value — unlike an
         * instruction's own (unshared) result, there's no reliable
         * per-occurrence signal to read here. */
        void materialize_constant( uint16_t stack, llvm::ConstantInt &c,
                                   cthu::builtin_structure structure );
        void emit_nibble( uint16_t stack, uint8_t value, cthu::builtin_structure structure );
        void append_nibble( uint16_t accumulator, uint16_t out, uint8_t value,
                            cthu::builtin_structure structure );

        void compute_block_inputs( llvm::Function &function );

        /* Takes the base Instruction, not BinaryOperator, so this covers
         * ICmpInst (2 operands in, 1 out, same as a binary op) too. */
        void binop_insn( llvm::Instruction &instruction,
                          cthu::builtin_structure structure,
                          cthu::builtin_operation operation );
        void cast_insn( llvm::CastInst &instruction,
                        cthu::builtin_structure source_structure,
                        cthu::builtin_structure cast_structure,
                        cthu::builtin_operation operation );
        void bool_sext_insn( llvm::CastInst &instruction, unsigned width );

        using llvm::InstVisitor< codegen >::visit;
        void visit( llvm::Instruction &instruction );

        void visitModule( llvm::Module &module );
        void visitFunction( llvm::Function &function );
        void visitBasicBlock( llvm::BasicBlock &block );

        void visitReturnInst( llvm::ReturnInst &instruction );
        void visitBranchInst( llvm::BranchInst &instruction );
        void visitConditionalBranch( llvm::BranchInst &instruction );
        void visitCallInst( llvm::CallInst &instruction );
        void visitPHINode( llvm::PHINode &instruction );
        void visitSelectInst( llvm::SelectInst &instruction );
        void visitICmpInst( llvm::ICmpInst &instruction );
        void visitTruncInst( llvm::TruncInst &instruction );
        void visitSExtInst( llvm::SExtInst &instruction );
        void visitZExtInst( llvm::ZExtInst &instruction );

        void visitAdd( llvm::BinaryOperator &instruction );
        void visitSub( llvm::BinaryOperator &instruction );
        void visitMul( llvm::BinaryOperator &instruction );
        void visitUDiv( llvm::BinaryOperator &instruction );
        void visitSDiv( llvm::BinaryOperator &instruction );
        void visitURem( llvm::BinaryOperator &instruction );
        void visitSRem( llvm::BinaryOperator &instruction );
        void visitShl( llvm::BinaryOperator &instruction );
        void visitLShr( llvm::BinaryOperator &instruction );
        void visitAShr( llvm::BinaryOperator &instruction );
        void visitAnd( llvm::BinaryOperator &instruction );
        void visitOr( llvm::BinaryOperator &instruction );
        void visitXor( llvm::BinaryOperator &instruction );
    };
}
