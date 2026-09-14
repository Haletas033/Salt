#include "../include/compiler.h"

#include <filesystem>

CompilerOptions Compiler::parseArgs(const int argc, char **argv) {
        CompilerOptions result{};
        for (int i = 1; i < argc; ++i) {
                const std::string arg = argv[i];
                if (arg == "--version") {
                        std::cout << VERSION << "\n";
                        return result;
                }
                if (arg == "--help") {
                        std::cout << HELP_MESSAGE << "\n";
                        return result;
                }
                if (arg == "--emit-ir") {
                        result.emitIR = true;
                } else if (arg == "--emit-tokens") {
                        result.emitTokens = true;
                } else if (arg == "-o") {
                        result.outputFile = argv[++i];
                } else {
                        if (!result.inputFile.empty())
                                throw std::logic_error("MULTIPLE INPUT FILES PROVIDED");
                        result.inputFile = argv[i];
                }
        }

        return result;
}

bool Compiler::needsRecompile(const std::string& source, const std::string& object) {
        struct stat srcStat{}, objStat{};

        if (stat(object.c_str(), &objStat) != 0) return true;

        if (stat(source.c_str(), &srcStat) != 0) {
                throw std::logic_error("SOURCE FILE NOT FOUND: " + source);
        }

        if (srcStat.st_mtime != objStat.st_mtime)
                return srcStat.st_mtime > objStat.st_mtime;
        return srcStat.st_mtim.tv_nsec > objStat.st_mtim.tv_nsec;
}

void Compiler::link() const {
        std::string cmd = "clang ";

        for (const auto& obj : requiredObjects) {
                cmd += obj + " ";
        }

        if (options.isLibrary) {
                cmd += "-shared -fPIC -o " + options.outputFile + ".so";
        } else {
                cmd += "-o " + options.outputFile;
        }

        system(cmd.c_str());
}

void Compiler::processAnnotations() {
        requiredObjects.push_back(options.outputFile + ".o");
        meta["CURRENT_DIR"] = std::filesystem::path(options.inputFile).parent_path().string();

        std::vector<Node> injected;

        for (const auto& node : program) {
                if (const auto* ann = std::get_if<Annotation>(&node.value)) {
                        if (ann->name == "meta" && ann->args.size() >= 2) {
                                meta[ann->args[0].value] = ann->args[1].value;
                        }

                        if (ann->name == "as") {
                                if (ann->args[0].value == "library") {
                                        options.isLibrary = true;
                                }
                        }

                        if (ann->name == "requires") {
                                for (const auto&[type, value] : ann->args) {
                                        std::string depPath = meta["CURRENT_DIR"] + "/" + value + ".salt";

                                        if (needsRecompile(depPath, value + ".o")) {
                                                CompilerOptions depOptions;
                                                depOptions.inputFile = depPath;
                                                depOptions.outputFile = value;
                                                depOptions.isDependency = true;
                                                Compiler dep(depOptions);
                                        }

                                        requiredObjects.push_back(value + ".o");
                                        std::ifstream depFile(depPath);
                                        if (!depFile.is_open()) throw std::logic_error("COULD NOT OPEN: " + depPath);
                                        Preprocessor depPrepro(depFile);
                                        Lexer depLexer(depPrepro.source, options);
                                        Parser depParser(depLexer.source, depLexer.tokens);

                                        for (const auto& depNode : depParser.program) {
                                                if (const auto* fn = std::get_if<FunctionDef>(&depNode.value)) {
                                                        FunctionDef proto;
                                                        proto.prototype = fn->prototype;
                                                        injected.push_back({0, 0, std::move(proto)});
                                                }
                                                if (const auto* s = std::get_if<StructDef>(&depNode.value)) {
                                                        injected.push_back({0, 0, *s});
                                                }
                                        }
                                }
                        }
                }
        }

        for (auto& n : injected) {
                program.push_back(std::move(n));
        }
}