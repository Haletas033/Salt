#ifndef SALT_CODEGEN_H
#define SALT_CODEGEN_H
#include <utility>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/IR/LegacyPassManager.h>

#include "parser.h"

class Codegen {
private:
        Program program;
        llvm::LLVMContext context;

        void emitAnnotation(const Annotation &annotation);

        llvm::Value *emitExpr(const Expr &expr, llvm::IRBuilder<> &builder);

        void emitReturnStatement(const ReturnStatement &statement, llvm::IRBuilder<> &builder);

        void emitFunctionDef(const FunctionDef &functionDef);

        void run();

public:
        llvm::Module module;
        Codegen(Program program, const std::string& filename)
        : program(std::move(program)),
              module(filename, context) {
                        run();
        }
};

#endif //SALT_CODEGEN_H
