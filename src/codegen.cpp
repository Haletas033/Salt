#include <codegen.h>

llvm::Type* Codegen::resolveType(const std::string& type) {
        if (type.ends_with('*')) {
                return llvm::PointerType::get(context, 0);
        }

        if (type == "u8"  || type == "i8")  return llvm::Type::getInt8Ty(context);
        if (type == "u16" || type == "i16") return llvm::Type::getInt16Ty(context);
        if (type == "u32" || type == "i32") return llvm::Type::getInt32Ty(context);
        if (type == "u64" || type == "i64") return llvm::Type::getInt64Ty(context);
        if (type == "f32")                  return llvm::Type::getFloatTy(context);
        if (type == "f64")                  return llvm::Type::getDoubleTy(context);
        if (type == "bool")                 return llvm::Type::getInt8Ty(context);
        if (type == "void")                 return llvm::Type::getVoidTy(context);
        throw std::logic_error("UNKNOWN TYPE '" + type + "'");
}

llvm::Value* Codegen::castTo(llvm::Value* value, llvm::Type* targetType, llvm::IRBuilder<>& builder) {
        if (value->getType() == targetType) return value;

        if (value->getType()->isIntegerTy() && targetType->isIntegerTy()) {
                return builder.CreateIntCast(value, targetType, true);
        }
        if (value->getType()->isFloatingPointTy() && targetType->isFloatingPointTy()) {
                return builder.CreateFPCast(value, targetType);
        }
        if (value->getType()->isIntegerTy() && targetType->isFloatingPointTy()) {
                return builder.CreateSIToFP(value, targetType);
        }
        if (value->getType()->isFloatingPointTy() && targetType->isIntegerTy()) {
                return builder.CreateFPToSI(value, targetType);
        }

        throw std::logic_error("INCOMPATIBLE TYPES IN CAST");
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
                        if (value.name == "true")
                                return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 1);
                        if (value.name == "false")
                                return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), 0);
                        if (value.name == "SUCCESS")
                                return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
                        if (value.name == "FAILURE")
                                return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1);
                        if (const auto it = locals.find(value.name); it != locals.end()) {
                                return builder.CreateLoad(it->second->getAllocatedType(), it->second, value.name);
                        }
                        throw std::logic_error("UNKNOWN IDENTIFIER '" + value.name + "'");
                } else if constexpr (std::same_as<T, StrLiteral>) {
                        return builder.CreateGlobalString(value.value);
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
                                case Operator::Type::EQ:  return builder.CreateICmp(llvm::CmpInst::ICMP_EQ,  left, right);
                                case Operator::Type::NEQ: return builder.CreateICmp(llvm::CmpInst::ICMP_NE,  left, right);
                                case Operator::Type::LT:  return builder.CreateICmp(llvm::CmpInst::ICMP_SLT, left, right);
                                case Operator::Type::GT:  return builder.CreateICmp(llvm::CmpInst::ICMP_SGT, left, right);
                                case Operator::Type::LTE: return builder.CreateICmp(llvm::CmpInst::ICMP_SLE, left, right);
                                case Operator::Type::GTE: return builder.CreateICmp(llvm::CmpInst::ICMP_SGE, left, right);
                                case Operator::Type::LOGICAL_AND: {
                                        llvm::Value* leftBool = builder.CreateICmpNE(left, llvm::ConstantInt::get(left->getType(), 0));
                                        llvm::Value* rightBool = builder.CreateICmpNE(right, llvm::ConstantInt::get(right->getType(), 0));
                                        return builder.CreateAnd(leftBool, rightBool);
                                }
                                case Operator::Type::LOGICAL_OR: {
                                        llvm::Value* leftBool = builder.CreateICmpNE(left, llvm::ConstantInt::get(left->getType(), 0));
                                        llvm::Value* rightBool = builder.CreateICmpNE(right, llvm::ConstantInt::get(right->getType(), 0));
                                        return builder.CreateOr(leftBool, rightBool);
                                }

                                default: throw std::logic_error("UNKNOWN OPERATOR TYPE");
                        }
                } else if constexpr (std::same_as<T, FunctionCall>) {
                        return emitFunctionCall(value, builder);
                } else {
                        throw std::logic_error("UNKNOWN EXPRESSION TYPE");
                }
        }, expr);
}

void Codegen::emitAssignment(const Assignment& assign, llvm::IRBuilder<>& builder) {
        const auto it = locals.find(assign.name);
        if (it == locals.end()) throw std::logic_error("UNDEFINED IDENTIFIER '" + assign.name + "'");
        llvm::Value* val = emitExpr(*assign.value, builder);
        val = castTo(val, it->second->getAllocatedType(), builder);
        builder.CreateStore(val, it->second);
}

void Codegen::emitVariableDecl(const VariableDecl& decl, llvm::IRBuilder<>& entryBuilder, llvm::IRBuilder<>& builder) {
        if (locals.contains(decl.name)) {
                throw std::logic_error("REDEFINITION OF VARIABLE \'" + decl.name + "\'");
        }

        llvm::Type* targetType = resolveType(decl.type);
        llvm::AllocaInst* alloca = entryBuilder.CreateAlloca(resolveType(decl.type), nullptr, decl.name);

        if (decl.value.has_value()) {
                llvm::Value* value = emitExpr(*decl.value.value(), builder);
                value = castTo(value, targetType, builder);
                builder.CreateStore(value, alloca);
        }

        locals[decl.name] = alloca;
}

void Codegen::emitStatement(const Node& node, llvm::IRBuilder<>& builder, llvm::IRBuilder<>& entryBuilder) {
        std::visit([this, &builder, &entryBuilder]<typename V>(const V& value) {
            using T = std::decay_t<V>;
            if constexpr (std::is_same_v<T, ReturnStatement>) {
                emitReturnStatement(value, builder);
            } else if constexpr (std::is_same_v<T, VariableDecl>) {
                emitVariableDecl(value, entryBuilder, builder);
            } else if constexpr (std::is_same_v<T, Assignment>) {
                emitAssignment(value, builder);
            } else if constexpr (std::is_same_v<T, IfStatement>) {
                emitIfStatement(value, builder, entryBuilder);
            } else if constexpr (std::is_same_v<T, WhileStatement>) {
                emitWhileStatement(value, builder, entryBuilder);
            } else if constexpr (std::is_same_v<T, BreakStatement>) {
                emitBreakStatement(builder);
            } else if constexpr (std::is_same_v<T, ContinueStatement>) {
                emitContinueStatement(builder);
            } else if constexpr (std::is_same_v<T, FunctionCall>) {
                emitFunctionCall(value, builder);
            } else {
                throw std::logic_error("UNSUPPORTED NODE IN FUNCTION BODY");
            }
        }, node.value);
}

void Codegen::emitIfStatement(const IfStatement& statement, llvm::IRBuilder<>& builder, llvm::IRBuilder<>& entryBuilder) {
        llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(context, "if.then", currentFunction);
        llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, "if.else", currentFunction);
        llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(context, "if.merge", currentFunction);

        llvm::Value* cond = emitExpr(*statement.condition, builder);
        builder.CreateCondBr(cond, thenBlock, elseBlock);

        builder.SetInsertPoint(thenBlock);
        for (const auto& node : statement.body) { emitStatement(node, builder, entryBuilder); }
        if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(mergeBlock);

        builder.SetInsertPoint(elseBlock);
        if (statement.elseBody.has_value()) {
                for (const auto& node : *statement.elseBody) { emitStatement(node, builder, entryBuilder); }
        }
        if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(mergeBlock);

        builder.SetInsertPoint(mergeBlock);
}

void Codegen::emitBreakStatement(llvm::IRBuilder<>& builder) {
        if (!currentLoopExit) throw std::logic_error("BREAK OUTSIDE OF LOOP");
        builder.CreateBr(currentLoopExit);

        llvm::BasicBlock* deadBlock = llvm::BasicBlock::Create(context, "dead", currentFunction);
        builder.SetInsertPoint(deadBlock);
}

void Codegen::emitContinueStatement(llvm::IRBuilder<>& builder) {
        if (!currentLoopCond) throw std::logic_error("CONTINUE OUTSIDE OF LOOP");
        builder.CreateBr(currentLoopCond);

        llvm::BasicBlock* deadBlock = llvm::BasicBlock::Create(context, "dead", currentFunction);
        builder.SetInsertPoint(deadBlock);
}

void Codegen::emitWhileStatement(const WhileStatement& statement, llvm::IRBuilder<>& builder, llvm::IRBuilder<>& entryBuilder) {
        llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(context, "while.cond", currentFunction);
        llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(context, "while.body", currentFunction);
        llvm::BasicBlock* exitBlock = llvm::BasicBlock::Create(context, "while.exit", currentFunction);

        builder.CreateBr(condBlock);

        builder.SetInsertPoint(condBlock);
        llvm::Value* cond = emitExpr(*statement.condition, builder);
        builder.CreateCondBr(cond, bodyBlock, exitBlock);

        llvm::BasicBlock* prevLoopExit = currentLoopExit;
        llvm::BasicBlock* prevLoopCond = currentLoopCond;

        currentLoopExit = exitBlock;
        currentLoopCond = condBlock;

        builder.SetInsertPoint(bodyBlock);
        for (const auto& node : statement.body) {
                emitStatement(node, builder, entryBuilder);
        }
        if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(condBlock);

        builder.SetInsertPoint(exitBlock);

        currentLoopExit = prevLoopExit;
        currentLoopCond = prevLoopCond;
}

llvm::CallInst *Codegen::emitFunctionCall(const FunctionCall& call, llvm::IRBuilder<>& builder) {
        llvm::Function* callee = module.getFunction(call.name);
        if (!callee) throw std::logic_error("UNDEFINED FUNCTION '" + call.name + "'");

        std::vector<llvm::Value*> argValues;

        size_t i = 0;
        for (const auto& arg : call.args) {
                llvm::Value* value = emitExpr(*arg, builder);

                if (i < callee->getFunctionType()->getNumParams()) {
                        llvm::Type* expectedType = callee->getFunctionType()->getParamType(i);
                        value = castTo(value, expectedType, builder);
                }

                argValues.push_back(value);
                ++i;
        }
        return builder.CreateCall(callee, argValues);
}

void Codegen::emitReturnStatement(const ReturnStatement& statement, llvm::IRBuilder<>& builder) {
        if (currentFunction->getReturnType()->isVoidTy()) {
                builder.CreateRetVoid();
                return;
        }
        llvm::Value* val = emitExpr(statement.value, builder);
        val = castTo(val, currentFunction->getReturnType(), builder);
        builder.CreateRet(val);
}

llvm::Function* Codegen::emitPrototype(const FunctionPrototype& prototype) {
        if (llvm::Function* existing = module.getFunction(prototype.name)) {
                return existing;
        }

        std::vector<llvm::Type*> paramTypes;
        for (const auto&[type, name] : prototype.params) {
                if (type == "void") continue;
                paramTypes.push_back(resolveType(type));
        }
        llvm::FunctionType* funcType = llvm::FunctionType::get(
            resolveType(prototype.returnType), paramTypes, prototype.isVariadic
        );
        return llvm::Function::Create(
            funcType, llvm::Function::ExternalLinkage, prototype.name, module
        );
}

void Codegen::emitFunctionDef(const FunctionDef& functionDef) {
        if (functionDef.body.empty()) {
                emitPrototype(functionDef.prototype);
                return;
        }

        locals.clear();

        llvm::Function* function = emitPrototype(functionDef.prototype);

        currentFunction = function;

        llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(context, "entry", function);
        llvm::IRBuilder entryBuilder(entryBlock);
        entryBuilder.SetInsertPoint(entryBlock, entryBlock->begin());
        llvm::IRBuilder builder(entryBlock);

        // Handle args
        size_t i = 0;
        for (auto& arg : function->args()) {
                arg.setName(functionDef.prototype.params[i].name);
                llvm::AllocaInst* alloca = entryBuilder.CreateAlloca(arg.getType(), nullptr, arg.getName());
                entryBuilder.CreateStore(&arg, alloca);
                locals[std::string(arg.getName())] = alloca;
                ++i;
        }

        // Handle body
        for (const auto& node : functionDef.body) {
                emitStatement(node, builder, entryBuilder);
        }

        if (!builder.GetInsertBlock()->getTerminator()) {
                if (currentFunction->getReturnType()->isVoidTy()) {
                        builder.CreateRetVoid();
                }
        }
}

void Codegen::emitExternC(const ExternC& externC) {
        for (const auto& proto : externC.prototypes) {
                emitPrototype(proto);
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
                        } else if constexpr (std::is_same_v<T, ExternC>) {
                                emitExternC(value);
                        } else if constexpr (std::is_same_v<T, FunctionDef>) {
                                emitFunctionDef(value);
                        }  else {
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
