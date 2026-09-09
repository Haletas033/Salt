#ifndef SALT_PARSER_H
#define SALT_PARSER_H
#include <string>
#include <variant>
#include <vector>

#include "lexer.h"

struct Node;

struct Annotation {
        std::string name{};
        std::vector<std::string> args{};
};

struct ReturnStatement {
        std::string output;
};

struct Parameter {
        std::string type;
        std::string name;
};

struct FunctionDef {
        std::string returnType{};
        std::string name{};
        std::vector<Parameter> parameters{};
        std::vector<Node> body{};
};

struct Node {
        size_t tokenStart;
        size_t tokenEnd;
        std::variant<Annotation, FunctionDef, ReturnStatement> value;
};

using Program = std::vector<Node>;

class Parser {
private:
        size_t position{0};
        const std::string& source;
        const std::vector<Token>& tokens;

        [[nodiscard]] Token peek(size_t offset) const;
        Token expect(Tokens tokenType);

        void parseAnnotation();

        void parseFunctionDef();

        [[nodiscard]] std::string_view getTokenStr(const Token& token) const;

        void run();
public:
        Program program;

        Parser(const std::string& source, const std::vector<Token>& tokens) : source(source), tokens(tokens) {
                run();
        }
};

#endif //SALT_PARSER_H
