#include <codegen.h>

void Codegen::emitAnnotation(const Annotation& annotation) {

}

llvm::Value *Codegen::emitExpr(const Expr &expr, llvm::IRBuilder<> &builder) {
        return std::visit([this, &builder]<typename V>(const V& value) -> llvm::Value* {
                using T = std::decay_t<V>;
                if constexpr (std::same_as<T, IntLiteral>) {
                        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), value.value);
                } else if constexpr (std::same_as<T, FloatLiteral>) {
                        return llvm::ConstantFP::get(llvm::Type::getFloatTy(context), value.value);
                } else if constexpr (std::same_as<T, Identifier>) {
                        if (value.name == "SUCCESS")
                                return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
                        if (value.name == "FAILURE")
                                return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1);
                        throw std::logic_error("UNKNOWN IDENTIFIER '" + value.name + "'");
                } else if constexpr (std::same_as<T, StrLiteral>) {
                        throw std::logic_error("STR_LITERAL IS CURRENTLY NOT SUPPORTED");
                } else if constexpr (std::same_as<T, BinaryOp>) {
                        llvm::Value *left = emitExpr(*value.lvalue, builder);
                        llvm::Value *right = emitExpr(*value.rvalue, builder);
                        switch (value.op.type) {
                                case Operator::Type::ADD: return builder.CreateAdd(left, right);
                                case Operator::Type::SUB: return builder.CreateSub(left, right);
                                case Operator::Type::MUL: return builder.CreateMul(left, right);
                                case Operator::Type::DIV: return builder.CreateSDiv(left, right);
                                default: throw std::logic_error("UNKNOWN OPERATOR TYPE");
                        }
                } else {
                        throw std::logic_error("UNKNOWN EXPRESSION TYPE");
                }
        }, expr);
}

void Codegen::emitReturnStatement(const ReturnStatement& statement, llvm::IRBuilder<>& builder) {
        builder.CreateRet(emitExpr(statement.value, builder));
}

void Codegen::emitFunctionDef(const FunctionDef& functionDef) {
        llvm::Type* returnType = nullptr;
        if (functionDef.returnType == "i32") {
                returnType = llvm::Type::getInt32Ty(context);
        } else {
                throw std::logic_error("UNKNOWN TYPE '" + functionDef.returnType + "'");
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, false);

        llvm::Function* function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, functionDef.name, module);

        llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(context, "entry", function);
        llvm::IRBuilder builder(entryBlock);

        // Handle body
        for (const auto& node : functionDef.body) {
                std::visit([this, &builder]<typename V>(const V& value) {
                        using T = std::decay_t<V>;
                        if constexpr (std::is_same_v<T, ReturnStatement>) {
                                emitReturnStatement(value, builder);
                        } else {
                                throw std::logic_error("UNSUPPORTED NODE IN FUNCTION BODY");
                        }
                }, node.value);
        }
}

void Codegen::run() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();

        const llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
        module.setTargetTriple(triple);

        std::string error{};
        const llvm::Target* target =
            llvm::TargetRegistry::lookupTarget(triple, error);

        if (!target) {
                throw std::logic_error("FAILED TO CREATE TARGET: " + error);
        }

        const std::string cpu = "generic";
        const std::string features{};

        const llvm::TargetOptions options;

        const auto machine = target->createTargetMachine(
                triple,
                cpu,
                features,
                options,
                llvm::Reloc::PIC_
        );

        if (!machine) {
                throw std::logic_error("FAILED TO CREATE TARGET MACHINE");
        }

        for (const Node& node : program) {
                std::visit([this]<typename V>(const V& value) {
                        using T = std::decay_t<V>;
                        if constexpr (std::is_same_v<T, Annotation>) {
                                emitAnnotation(value);
                        } else if constexpr (std::is_same_v<T, FunctionDef>) {
                                emitFunctionDef(value);
                        } else {
                                throw std::logic_error("UNSUPPORTED NODE TYPE");
                        }
                }, node.value);
        }

        std::error_code ec;
        llvm::raw_fd_ostream output("output.o", ec);
        llvm::legacy::PassManager pass;
        machine->addPassesToEmitFile(pass, output, nullptr, llvm::CodeGenFileType::ObjectFile);
        pass.run(module);
        output.flush();

        system("clang -o output output.o");
}
