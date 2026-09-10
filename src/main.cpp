#include <iostream>
#include <fstream>

#include "codegen.h"
#include "lexer.h"
#include "parser.h"

int main(const int argc, char **argv) {
        if (argc != 2) {
                std::cerr << "INVALID NUMBER OF ARGUMENTS. EXPECTED 1\n";
                return 1;
        }

        const std::string filePath = argv[1];
        std::ifstream fileContents(argv[1]);

        if (!fileContents.is_open()) {
                std::cerr << "INVALID PATH \'" << filePath << "\'\n";
                return 1;
        }

        Lexer lexer(fileContents);

        Parser parser(lexer.source, lexer.tokens);

        Codegen codegen(parser.program, "test");

        codegen.module.print(llvm::outs(), nullptr);
}
