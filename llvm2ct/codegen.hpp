#pragma once

#include "debuginfo.hpp"
#include "symtab.hpp"

#include <llvm/ADT/DenseMap.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/Constants.h>
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
         * fully built — see binop_insn.
         *
         * Known gap: this only counts uses within the defining block. Once
         * branches compile to calls between subroutines, a value passed to a
         * successor as a call argument is a use too, and isn't counted here
         * yet — needs revisiting before that lands, or a live value could
         * have its slot reused before the outgoing call reads it. */
        llvm::DenseMap< llvm::Value *, uint16_t > _stack_of;
        llvm::DenseMap< llvm::Value *, unsigned > _remaining_uses;
        std::vector< uint16_t > _free_stacks;
        std::vector< uint16_t > _pending_frees;
        uint16_t _next_stack = 0;

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

        /* struct_name is passed through to materialize_constant() if value
         * turns out to be a fresh ConstantInt — see there for why it can't
         * decide its own structure. Defaults to "i₃₂" for call sites with
         * no natural signedness context (e.g. visitStoreInst). */
        uint16_t use( llvm::Value *value, const std::string &struct_name = "i₃₂" );
        void commit_frees();

        /* use() calls this the first time it sees a value with no producer
         * of its own — that's correct for an llvm::Argument (its value
         * arrives via the call), but a ConstantInt needs an actual cons_N
         * emitted, or the compiled output reads from a stack nothing ever
         * wrote to. Only literal 0..15 are handled (cons_0..cons_15 are all
         * builtins.ct provides) — larger constants aren't handled yet.
         *
         * struct_name is the caller's choice, not derived here: a
         * ConstantInt is uniqued by LLVM (every literal `5` of type i32 is
         * the same Value*), so debug_type() on the constant itself can
         * resolve to an arbitrary one of possibly several unrelated source
         * variables that happen to share that value — unlike an
         * instruction's own (unshared) result, there's no reliable
         * per-occurrence signal to read here. */
        void materialize_constant( uint16_t stack, llvm::ConstantInt &c, const std::string &struct_name );

        /* The LLVM type's own bit width, not anything from debug info: C
         * promotes char/short arithmetic to int before the actual
         * operation (see codegen.cpp's visitAdd comment), so an add's own
         * type already reflects what's genuinely being computed, unlike
         * signedness, which IR alone never carries at all. Only 8 and 32
         * are wired up so far — builtin.hpp only has bv8/bv32 variants. */
        unsigned width_of( llvm::Value *value );

        /* "i₈"/"u₈"/"i₃₂"/"u₃₂" for the given signedness/width. */
        std::string struct_name( bool is_unsigned, unsigned width );

        /* struct_name_for a value whose signedness isn't already certain
         * from its own LLVM opcode: "u₃₂"/"u₈" if its debug type resolves
         * to an unsigned DIBasicType, "i₃₂"/"i₈" otherwise (signed, or no
         * debug info — add/sub/mul/etc. execute identically either way
         * today, see builtin.py's bv_add and friends, but a future
         * signedness-aware analysis would silently mis-tag an
         * actually-unsigned value without this). For udiv/urem/lshr/icmp
         * and friends, where the opcode itself already says signed or
         * unsigned unambiguously, call struct_name() directly instead —
         * going through debug info there could contradict the opcode
         * (e.g. default to "signed" on missing debug info for a UDiv). */
        std::string struct_name_for( llvm::Value *value );

        /* Takes the base Instruction, not BinaryOperator, so this covers
         * ICmpInst (2 operands in, 1 out, same as a binary op) too. */
        void binop_insn( llvm::Instruction &instruction,
                          const std::string &struct_name, const std::string &op_name, cthu::builtin code );

        using llvm::InstVisitor< codegen >::visit;
        void visit( llvm::Instruction &instruction );

        void visitModule( llvm::Module &module );
        void visitFunction( llvm::Function &function );
        void visitBasicBlock( llvm::BasicBlock &block );

        void visitReturnInst( llvm::ReturnInst &instruction );
        void visitICmpInst( llvm::ICmpInst &instruction );

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
