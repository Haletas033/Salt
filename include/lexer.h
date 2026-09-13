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
        size_t tokenStart{};
        size_t tokenEnd{};

        size_t line{};
        size_t column{};

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
        size_t position{};
        size_t currentLine = 1;
        size_t currentColumn = 1;

        std::optional<TokenDef> matchToken();

        void advance();

        void advance(size_t count);

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
