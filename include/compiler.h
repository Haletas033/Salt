#ifndef SALT_COMPILER_H
#define SALT_COMPILER_H

#include <iostream>
#include <string>
#include <sys/stat.h>

#include "codegen.h"
#include "compilerOptions.h"
#include "parser.h"
#include "preprocessor.h"

constexpr std::string VERSION = "v0.1.5";

constexpr auto HELP_MESSAGE = R"(
Salt compiler v0.1.5
Usage: salt <file> [options]
Options:
  -o <name>      Output binary name (default: output)
  --emit-ir      Print LLVM IR to stdout
  --emit-tokens  Print token stream to stdout
  --version      Print version information
  --help         Print this help message
)";

class Compiler {
private:
        Program program{};
        CompilerOptions options;
        std::map<std::string, std::string> meta{};
        std::vector<std::string> requiredObjects;
public:

        static CompilerOptions parseArgs(int argc, char **argv);

        static bool needsRecompile(const std::string &source, const std::string &object);

        void link() const;

        void processAnnotations();

        explicit Compiler(const CompilerOptions& options) : options(options) {
                std::ifstream file(options.inputFile);
                Preprocessor preprocessor(file);

                Lexer lexer(preprocessor.source, options);

                Parser parser(lexer.source, lexer.tokens);
                program = std::move(parser.program);
                processAnnotations();
                Codegen codegen(std::move(program), options);

                if (!options.isDependency) {
                        link();
                }
        }
};

#endif //SALT_COMPILER_H
