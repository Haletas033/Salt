#ifndef SALT_LEXER_H
#define SALT_LEXER_H
#include <fstream>
#include <optional>
#include <sstream>
#include <vector>

#include "tokens.h"

struct Token {
        Tokens tokenType{};
        unsigned long tokenStart{};
        unsigned long tokenEnd{};
};

enum class IdentifierType {
        NORMAL, INTEGRAL, FLOATING
};

class Lexer {
private:
        std::string source{};
        unsigned long position{};

        std::optional<TokenDef> matchToken();

        void releaseIdentifier(std::string &currentIdentifier, IdentifierType currentIdentifierType);

        void skipComments();

        void run();
public:
        std::vector<Token> tokens{};

        explicit Lexer(std::ifstream& file) {
                std::stringstream buffer{};
                buffer << file.rdbuf();
                source = buffer.str();

                run();
        }
};

#endif //SALT_LEXER_H
