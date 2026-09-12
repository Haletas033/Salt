#ifndef SALT_PARSER_H
#define SALT_PARSER_H
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lexer.h"

struct FunctionCall;
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
                XOR, OR,
                EQ, NEQ,
                LT, GT,
                LTE, GTE,
                LOGICAL_AND, LOGICAL_OR
        };
        Type type{};
        int precedence{};
};

using Expr = std::variant<IntLiteral, FloatLiteral, StrLiteral, Identifier, BinaryOp, FunctionCall>;
struct BinaryOp {
        std::unique_ptr<Expr> lvalue{};
        std::unique_ptr<Expr> rvalue{};
        Operator op{};
};

struct Assignment {
        std::string name;
        std::unique_ptr<Expr> value;
};

struct VariableDecl {
        std::string type;
        std::string name;
        std::optional<std::unique_ptr<Expr>> value;
};

struct IfStatement {
        std::unique_ptr<Expr> condition;
        std::vector<Node> body;
        std::optional<std::vector<Node>> elseBody;
};

struct FunctionCall {
        std::string name;
        std::vector<std::unique_ptr<Expr>> args;
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
        std::variant<Annotation, FunctionDef, VariableDecl, FunctionCall, IfStatement, Assignment, ReturnStatement> value;
};

using Program = std::vector<Node>;

class Parser {
private:
        size_t position{0};
        const std::string& source;
        const std::vector<Token>& tokens;

        [[nodiscard]] Token peek(size_t offset) const;
        Token expect(Tokens tokenType);

        [[nodiscard]] std::optional<Operator> getOperator() const;

        Expr parsePrimary();

        Expr parseExpr(int minPrecedence = 0);

        Assignment parseAssignment();

        VariableDecl parseVariableDecl();

        IfStatement parseIfStatement();

        FunctionCall parseFunctionCall();

        Annotation parseAnnotation();

        ReturnStatement parseReturnStatement();

        Node parseStatement();

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
