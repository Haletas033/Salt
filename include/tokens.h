#ifndef SALT_TOKENS_H
#define SALT_TOKENS_H
#include <string_view>

enum class Tokens {
        #define T(name) name,
        #include <tokens.def>
        #undef T
};

inline std::vector<std::string> tokenStrings {
        #define T(name) #name,
        #include <tokens.def>
        #undef T
};

struct TokenDef {
        std::string_view text;
        Tokens token;
};

constexpr TokenDef token_defs[] = {
        {"#",  Tokens::HASH},
        {"@",  Tokens::AT_SIGN},
        {"(",  Tokens::L_PARENTHESES},
        {")",  Tokens::R_PARENTHESES},
        {"[",  Tokens::L_BRACKET},
        {"]",  Tokens::R_BRACKET},
        {"{",  Tokens::L_BRACE},
        {"}",  Tokens::R_BRACE},
        {"<", Tokens::L_ANGULAR},
        {">", Tokens::R_ANGULAR},

        {"${", Tokens::INTERP},
        {"\"", Tokens::STR},
        {"\'", Tokens::CHAR},

        {"%",  Tokens::PERCENT},
        {"*",  Tokens::STAR},
        {"*=", Tokens::STAR_ASSIGN},
        {"/",  Tokens::DIV},
        {"/=", Tokens::DIV_ASSIGN},
        {"+",  Tokens::PLUS},
        {"+=", Tokens::PLUS_ASSIGN},
        {"++", Tokens::INCREMENT},
        {"-",  Tokens::SUB},
        {"-=", Tokens::SUB_ASSIGN},
        {"--", Tokens::DECREMENT},

        {".",   Tokens::DOT},
        {"...", Tokens::ELLIPSE},
        {"<-", Tokens::L_ARROW},
        {"->", Tokens::R_ARROW},

        {";", Tokens::SEMI_COLON},
        {"~", Tokens::TILDA},
        {"&", Tokens::AND},
        {"&&", Tokens::AND_LOGICAL},
        {"|", Tokens::OR},
        {"||", Tokens::OR_LOGICAL},
        {"^", Tokens::XOR},

        {"!",  Tokens::NOT},
        {"!=", Tokens::NOT_EQUAL},
        {"=",  Tokens::EQUALS},
        {"==", Tokens::EQUALS_LOGICAL},

        {">=", Tokens::GREATER_THAN},
        {"<=", Tokens::LESS_THAN},

        {"?", Tokens::QUESTION},
        {",", Tokens::COMMA},
        {":", Tokens::COLON},
        {"\\", Tokens::BACKSLASH},
    };
#endif //SALT_TOKENS_H
