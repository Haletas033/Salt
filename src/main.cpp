#include <iostream>
#include <fstream>

#include "lexer.h"

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
        for (const auto &token : lexer.tokens) {
                std::cout << tokenStrings[static_cast<int>(token.tokenType)] << " ";
        }
}
