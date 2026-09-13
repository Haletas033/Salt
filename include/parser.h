#ifndef SALT_PARSER_H
#define SALT_PARSER_H
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lexer.h"

struct ArrowAccess;
struct FieldAccess;
struct Deref;
struct ArrayIndex;
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

struct AddressOf {
        std::string name;
};

struct SizeOf {
        std::string type;
};

using Expr = std::variant<
        IntLiteral,
        FloatLiteral,
        StrLiteral,
        AddressOf,
        ArrayIndex,
        Deref,
        FieldAccess,
        ArrowAccess,
        Identifier,
        SizeOf,
        BinaryOp,
        FunctionCall
>;
struct BinaryOp {
        std::unique_ptr<Expr> lvalue{};
        std::unique_ptr<Expr> rvalue{};
        Operator op{};
};

struct Deref {
        std::unique_ptr<Expr> pointer;
};

struct DerefAssignment {
        std::unique_ptr<Expr> pointer;
        std::unique_ptr<Expr> value;
};

struct ArrayIndex {
        std::string name;
        std::unique_ptr<Expr> index;
};

struct ArrayAssignment {
        std::string name;
        std::unique_ptr<Expr> index;
        std::unique_ptr<Expr> value;
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

struct BreakStatement{};
struct ContinueStatement{};

struct WhileStatement {
        std::unique_ptr<Expr> condition;
        std::vector<Node> body;
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
        std::unique_ptr<Expr> value;
};

struct Parameter {
        std::string type;
        std::string name;
};

struct FunctionPrototype {
        std::string returnType;
        std::string name;
        std::vector<Parameter> params;
        bool isVariadic = false;
};

struct FunctionDef {
        FunctionPrototype prototype{};
        std::vector<Node> body{};
};

struct StructDef {
        std::string name;
        std::vector<Parameter> fields;
};

struct FieldAccess {
        std::string object;
        std::string field;
};

struct FieldAssignment {
        std::string object;
        std::string field;
        std::unique_ptr<Expr> value;
};

struct ArrowAccess {
        std::string object;
        std::string field;
};

struct ArrowAssignment {
        std::string object;
        std::string field;
        std::unique_ptr<Expr> value;
};

struct ExternC {
        std::vector<FunctionPrototype> prototypes{};
};

struct Node {
        size_t tokenStart;
        size_t tokenEnd;
        std::variant<
                Annotation,
                ExternC,
                StructDef,
                FunctionDef,
                VariableDecl,
                FunctionCall,
                IfStatement,
                BreakStatement,
                ContinueStatement,
                WhileStatement,
                ArrayAssignment,
                DerefAssignment,
                FieldAssignment,
                ArrowAssignment,
                Assignment,
                ReturnStatement
        > value;
};

using Program = std::vector<Node>;

class Parser {
private:
        size_t position{0};
        const std::string& source;
        const std::vector<Token>& tokens;

        [[nodiscard]] Token peek(size_t offset) const;
        Token expect(Tokens tokenType);

        std::string parseType();

        static std::string unescapeString(const std::string &raw);

        [[nodiscard]] std::optional<Operator> getOperator() const;

        Expr parsePrimary();

        Expr parseExpr(int minPrecedence = 0);

        ArrayAssignment parseArrayAssignment();

        DerefAssignment parseDerefAssignment();

        Assignment parseAssignment();

        SizeOf parseSizeOf();

        VariableDecl parseVariableDecl();

        IfStatement parseIfStatement();

        WhileStatement parseWhileStatement();

        FunctionCall parseFunctionCall();

        ExternC parseExternC();

        Annotation parseAnnotation();

        ReturnStatement parseReturnStatement();

        FieldAssignment parseFieldAssignment();

        ArrowAssignment parseArrowAssignment();

        Node parseStatement();

        FunctionPrototype parsePrototype();

        FunctionDef parseFunctionDef();

        StructDef parseStructDef();

        [[nodiscard]] std::string_view getTokenStr(const Token& token) const;

        void run();
public:
        Program program;

        Parser(const std::string& source, const std::vector<Token>& tokens) : source(source), tokens(tokens) {
                run();
        }
};

#endif //SALT_PARSER_H
