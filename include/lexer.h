#ifndef SALT_LEXER_H
#define SALT_LEXER_H
#include <fstream>
#include <optional>
#include <sstream>
#include <stack>
#include <utility>
#include <vector>

#include "tokens.h"

struct Token {
        Tokens tokenType{};
        unsigned long tokenStart{};
        unsigned long tokenEnd{};
};

enum class IdentifierType {
        NORMAL, INTEGRAL, FLOATING, STR, CHAR
};

enum class InsideType {
        STR,
        INTERP,
        CHAR,
        BRACE // Used to Handle syntax like ${foo{}}
};

class Lexer {
private:
        unsigned long position{};

        std::optional<TokenDef> matchToken();

        void releaseIdentifier(std::string &currentIdentifier, IdentifierType currentIdentifierType);

        void skipComments();

        static void updateState(const TokenDef &token, std::stack<InsideType> &insideStack,
                         IdentifierType &currentIdentifierType);

        void run();
public:
        std::string source{};
        std::vector<Token> tokens{};

        explicit Lexer(std::string file) : source(std::move(file)) {
                run();
        }
};

#endif //SALT_LEXER_H
