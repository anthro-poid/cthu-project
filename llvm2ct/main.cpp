#include "codegen.hpp"
#include "print.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>

#include <fstream>
#include <set>

void print_files( auto &c )
{
    std::ofstream out( "out.ct" );

    for ( auto &[ key, structure ] : c._symtab.structures )
        out << *structure;

    std::ofstream prelude( "out.signatures.ct" );
    std::ofstream builtins( "out.structures.ct" );
    std::set< std::string > emitted_signatures;
    std::set< std::string > emitted_structures;

    auto emit_function_type = [ & ]( llvm::ArrayRef< llvm::Type * > inputs,
                                               llvm::Type *output )
    {
        std::string signature_name = llvm2ct::function_signature_name( inputs.size(), !output->isVoidTy() );
        std::string structure_name = llvm2ct::function_structure_name( inputs, output );

        if ( emitted_signatures.insert( signature_name ).second )
            cthu::print_function_signature( prelude, inputs, output );

        if ( emitted_structures.insert( structure_name ).second )
            cthu::print_function_structure( builtins, inputs, output );
    };

    for ( llvm::Function &function : *c._module )
    {
        if ( function.isDeclaration() )
            continue;

        llvm::Type *output = function.getReturnType();

        for ( llvm::BasicBlock &block : function )
        {
            std::vector< llvm::Type * > inputs;

            for ( llvm::Value *value : c._block_inputs[ &block ] )
                inputs.push_back( value->getType() );

            emit_function_type( inputs, output );
        }
    }
}

int main( int argc, char *argv[] )
{
    if ( argc != 2 )
    {
        llvm::errs() << "usage: <file.ll>\n";
        return -1;
    }

    llvm::SMDiagnostic error;
    llvm::LLVMContext context;
    auto module = llvm::parseIRFile( argv[ 1 ], error, context );

    if ( !module )
    {
        llvm::errs() << error.getMessage();
        return -1;
    }

    llvm2ct::codegen c{ context, std::move( module ) };
    c.visit( *c._module );

    print_files( c );
    return 0;
}
