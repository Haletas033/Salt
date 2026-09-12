#include <codegen.h>

llvm::Type* Codegen::resolveType(const std::string& type) {
        if (type == "i32") return llvm::Type::getInt32Ty(context);
        if (type == "i64") return llvm::Type::getInt64Ty(context);
        if (type == "f32") return llvm::Type::getFloatTy(context);
        if (type == "f64") return llvm::Type::getDoubleTy(context);
        if (type == "void") return llvm::Type::getVoidTy(context);
        throw std::logic_error("UNKNOWN TYPE '" + type + "'");
}

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
                        if (const auto it = locals.find(value.name); it != locals.end()) {
                                return builder.CreateLoad(it->second->getAllocatedType(), it->second, value.name);
                        }
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
                                case Operator::Type::MOD: return builder.CreateSRem(left, right);
                                case Operator::Type::AND: return builder.CreateAnd(left, right);
                                case Operator::Type::XOR: return builder.CreateXor(left, right);
                                case Operator::Type::OR:  return builder.CreateOr(left, right);
                                default: throw std::logic_error("UNKNOWN OPERATOR TYPE");
                        }
                } else {
                        throw std::logic_error("UNKNOWN EXPRESSION TYPE");
                }
        }, expr);
}

void Codegen::emitAssignment(const Assignment& assign, llvm::IRBuilder<>& builder) {
        const auto it = locals.find(assign.name);
        if (it == locals.end()) throw std::logic_error("UNDEFINED IDENTIFIER '" + assign.name + "'");
        llvm::Value* val = emitExpr(*assign.value, builder);
        builder.CreateStore(val, it->second);
}

void Codegen::emitVariableDecl(const VariableDecl& decl, llvm::IRBuilder<>& entryBuilder, llvm::IRBuilder<>& builder) {
        if (locals.contains(decl.name)) {
                throw std::logic_error("REDEFINITION OF VARIABLE \'" + decl.name + "\'");
        }

        llvm::AllocaInst* alloca = entryBuilder.CreateAlloca(resolveType(decl.type), nullptr, decl.name);

        if (decl.value.has_value()) {
                llvm::Value* value = emitExpr(*decl.value.value(), builder);
                builder.CreateStore(value, alloca);
        }

        locals[decl.name] = alloca;
}


void Codegen::emitReturnStatement(const ReturnStatement& statement, llvm::IRBuilder<>& builder) {
        builder.CreateRet(emitExpr(statement.value, builder));
}

void Codegen::emitFunctionDef(const FunctionDef& functionDef) {
        locals.clear();

        llvm::Type* returnType = nullptr;
        returnType = resolveType(functionDef.returnType);

        std::vector<llvm::Type*> paramTypes;
        for (const auto&[type, name] : functionDef.parameters) {
                if (type == "void") continue;
                paramTypes.push_back(resolveType(type));
        }

        llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, false);

        llvm::Function* function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, functionDef.name, module);

        llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(context, "entry", function);
        llvm::IRBuilder entryBuilder(entryBlock);
        entryBuilder.SetInsertPoint(entryBlock, entryBlock->begin());
        llvm::IRBuilder builder(entryBlock);

        // Handle args
        size_t i = 0;
        for (auto& arg : function->args()) {
                arg.setName(functionDef.parameters[i].name);
                llvm::AllocaInst* alloca = entryBuilder.CreateAlloca(arg.getType(), nullptr, arg.getName());
                entryBuilder.CreateStore(&arg, alloca);
                locals[std::string(arg.getName())] = alloca;
                ++i;
        }

        // Handle body
        for (const auto& node : functionDef.body) {
                std::visit([this, &builder, &entryBuilder]<typename V>(const V& value) {
                        using T = std::decay_t<V>;
                        if constexpr (std::is_same_v<T, ReturnStatement>) {
                                emitReturnStatement(value, builder);
                        } else if constexpr(std::is_same_v<T, VariableDecl>) {
                                emitVariableDecl(value, entryBuilder, builder);
                        } else if constexpr (std::is_same_v<T, Assignment>) {
                                emitAssignment(value, builder);
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

        llvm::PassBuilder passBuilder;
        llvm::LoopAnalysisManager lam;
        llvm::FunctionAnalysisManager fam;
        llvm::CGSCCAnalysisManager cgam;
        llvm::ModuleAnalysisManager mam;

        passBuilder.registerModuleAnalyses(mam);
        passBuilder.registerCGSCCAnalyses(cgam);
        passBuilder.registerFunctionAnalyses(fam);
        passBuilder.registerLoopAnalyses(lam);
        passBuilder.crossRegisterProxies(lam, fam, cgam, mam);

        llvm::ModulePassManager mpm = passBuilder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
        mpm.run(module, mam);
        
        std::error_code ec;
        llvm::raw_fd_ostream output("output.o", ec);
        llvm::legacy::PassManager pass;
        machine->addPassesToEmitFile(pass, output, nullptr, llvm::CodeGenFileType::ObjectFile);
        pass.run(module);
        output.flush();

        system("clang -o output output.o");
}
