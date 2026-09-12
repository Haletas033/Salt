#ifndef SALT_PARSER_H
#define SALT_PARSER_H
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lexer.h"

struct BinaryOp;
struct Node;

struct IntLiteral {
        int value{};
};

struct FloatLiteral {
        float value{};
};

struct StrLiteral {
        std::string value{};
};

struct Identifier {
        std::string name{};
};

struct Operator {
        enum class Type {
                ADD, SUB,
                MUL, DIV,
                MOD, AND,
                XOR, OR
        };
        Type type{};
        int precedence{};
};

using Expr = std::variant<IntLiteral, FloatLiteral, StrLiteral, Identifier, BinaryOp>;
struct BinaryOp {
        std::unique_ptr<Expr> lvalue{};
        std::unique_ptr<Expr> rvalue{};
        Operator op{};
};

struct AnnotationArg {
        enum class Type { IDENTIFIER, INTEGER, FLOATING, STR, CHAR};
        Type type;
        std::string value;
};

struct Annotation {
        std::string name{};
        std::vector<AnnotationArg> args{};
};

struct ReturnStatement {
        Expr value;
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

        std::optional<Operator> getOperator() const;

        Expr parsePrimary();

        Expr parseExpr(int minPrecedence);

        Annotation parseAnnotation();

        ReturnStatement parseReturnStatement();

        FunctionDef parseFunctionDef();

        [[nodiscard]] std::string_view getTokenStr(const Token& token) const;

        void run();
public:
        Program program;

        Parser(const std::string& source, const std::vector<Token>& tokens) : source(source), tokens(tokens) {
                run();
        }
};

#endif //SALT_PARSER_H
