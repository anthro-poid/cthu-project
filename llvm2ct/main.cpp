#include "codegen.hpp"
#include "print.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>

#include <fstream>
#include <set>

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

    std::ofstream out( "out.ct" );

    for ( auto &[ key, structure ] : c._symtab.structures )
        out << structure;

    std::ofstream prelude( "out.prelude.ct" );
    std::ofstream builtins( "out.builtins.ct" );
    std::set< std::string > emitted_signatures;
    std::set< std::string > emitted_structures;

    for ( llvm::Function &function : *c._module )
        if ( !function.isDeclaration() )
        {
            llvm::FunctionType *type = function.getFunctionType();

            if ( emitted_signatures.insert( cthu::function_signature_name( type ) ).second )
                cthu::print_function_signature( prelude, type );

            if ( emitted_structures.insert( cthu::function_type_name( type ) ).second )
                cthu::print_function_structure( builtins, type );
        }

    return 0;
}
