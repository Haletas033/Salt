#include <codegen.h>

void Codegen::emitAnnotation(const Annotation& annotation) {

}

void Codegen::emitReturnStatement(const ReturnStatement& statement, llvm::Type* returnType, llvm::IRBuilder<>& builder) {
        std::string resolvedReturn{};
        if (statement.value == "SUCCESS")
                resolvedReturn = "0";
        else if (statement.value == "FAILURE")
                resolvedReturn = "1";
        else
                resolvedReturn = statement.value;

        if (returnType == llvm::Type::getInt32Ty(context)) {
                builder.CreateRet(llvm::ConstantInt::get(returnType, std::stoi(resolvedReturn)));
        }
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
                std::visit([this, &builder, returnType]<typename V>(const V& value) {
                        using T = std::decay_t<V>;
                        if constexpr (std::is_same_v<T, ReturnStatement>) {
                                emitReturnStatement(value, returnType, builder);
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
