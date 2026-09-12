#include "parser.h"

Token Parser::peek(const size_t offset = 0) const {
        if (tokens.size() <= position + offset) { throw std::logic_error("TRIED TO READ OUT OF BOUNDS TOKEN"); }
        return tokens[position+offset];
}

std::string_view Parser::getTokenStr(const Token &token) const {
        return std::string_view(source).substr(token.tokenStart, token.tokenEnd - token.tokenStart);
}

Token Parser::expect(const Tokens tokenType) {
        const Token token = tokens[position];
        if (token.tokenType == tokenType) {
                ++position;
                return token;
        }

        const std::string& expectedType = tokenStrings[static_cast<int>(tokenType)];
        const std::string& receivedType = tokenStrings[static_cast<int>(token.tokenType)];
        std::string errorMessage = "EXPECTED \'" + expectedType + "\' BUT GOT \'" + receivedType + "\' INSTEAD AT ";
        errorMessage.append(std::to_string(token.tokenStart));
        throw std::logic_error(errorMessage);
}

std::optional<Operator> Parser::getOperator() const {
        Operator result{};
        switch (peek().tokenType) {
                case Tokens::PLUS:
                        result = {Operator::Type::ADD, 5};
                        break;

                case Tokens::SUB:
                        result = {Operator::Type::SUB, 5};
                        break;

                case Tokens::STAR:
                        result = {Operator::Type::MUL, 6};
                        break;

                case Tokens::DIV:
                        result = {Operator::Type::DIV, 6};
                        break;

                case Tokens::PERCENT:
                        result = {Operator::Type::MOD, 6};
                        break;

                case Tokens::AND:
                        result = {Operator::Type::AND, 4};
                        break;

                case Tokens::XOR:
                        result = {Operator::Type::XOR, 3};
                        break;

                case Tokens::OR:
                        result = {Operator::Type::OR, 2};
                        break;

                default:
                        return std::nullopt;
        }
        return result;
}

Expr Parser::parsePrimary() {
        Expr result{};
        switch (peek().tokenType) {
                case Tokens::L_PARENTHESES: {
                        ++position;
                        result = parseExpr();
                        expect(Tokens::R_PARENTHESES);
                        return result;
                }

                case Tokens::INT_LITERAL: {
                        result = IntLiteral{std::stoi(std::string{getTokenStr(peek())})};
                        ++position;
                        return result;
                }

                case Tokens::FLOAT_LITERAL: {
                        result = FloatLiteral{std::stof(std::string{getTokenStr(peek())})};
                        ++position;
                        return result;
                }

                case Tokens::STR: {
                        ++position;
                        result = StrLiteral{std::string{getTokenStr(expect(Tokens::STR_LITERAL))}};
                        expect(Tokens::STR);
                        return result;
                }

                case Tokens::IDENTIFIER:
                        result = Identifier{std::string{getTokenStr(peek())}};
                        ++position;
                        return result;
                default:
                        throw std::logic_error("UNEXPECTED TOKEN \'" + tokenStrings[static_cast<int>(peek().tokenType)] + "\'");
        }
}

Expr Parser::parseExpr(int minPrecedence) {
        Expr left = parsePrimary();
        while (true) {
                std::optional<Operator> op = getOperator();
                if (!op.has_value()) { break; }
                if (op->precedence < minPrecedence) { break; }
                ++position;
                Expr right = parseExpr(op->precedence + 1);
                left = BinaryOp{
                        std::make_unique<Expr>(std::move(left)),
                        std::make_unique<Expr>(std::move(right)),
                        op->type
                };
        }
        return left;
}

Annotation Parser::parseAnnotation() {
        const size_t start = position;
        Annotation result{};
        ++position;

        result.name = getTokenStr(expect(Tokens::IDENTIFIER));
        expect(Tokens::L_BRACE);
        while (true) {
                if (peek().tokenType == Tokens::STR) {
                        ++position;
                        // TODO(parser) Add support for interp
                        const std::string name{getTokenStr(expect(Tokens::STR_LITERAL))};
                        result.args.push_back({AnnotationArg::Type::STR, name});
                        expect(Tokens::STR);
                } else if (peek().tokenType == Tokens::CHAR) {
                        ++position;
                        const std::string name{getTokenStr(expect(Tokens::CHAR_LITERAL))};
                        result.args.push_back({AnnotationArg::Type::CHAR, name});
                        expect(Tokens::CHAR);
                } else if (peek().tokenType == Tokens::IDENTIFIER) {
                        const std::string name{getTokenStr(peek())};
                        result.args.push_back({AnnotationArg::Type::IDENTIFIER, name});
                        ++position;
                } else if (peek().tokenType == Tokens::INT_LITERAL) {
                        const std::string name{getTokenStr(peek())};
                        result.args.push_back({AnnotationArg::Type::INTEGER, name});
                        ++position;
                } else if (peek().tokenType == Tokens::FLOAT_LITERAL) {
                        const std::string name{getTokenStr(peek())};
                        result.args.push_back({AnnotationArg::Type::FLOATING, name});
                        ++position;
                } else {
                        throw std::logic_error("UNEXPECTED TOKEN \'" + tokenStrings[static_cast<int>(peek().tokenType)] + '\'');
                }

                if (peek().tokenType == Tokens::R_BRACE) break;
                expect(Tokens::COMMA);
        }
        expect(Tokens::R_BRACE);
        expect(Tokens::SEMI_COLON);

        return result;
}

ReturnStatement Parser::parseReturnStatement() {
        ReturnStatement result{};
        ++position;

        // Handle literals
        if (peek().tokenType != Tokens::L_BRACE) {
                result.value = parseExpr();
                expect(Tokens::SEMI_COLON);
                return result;
        }

        // Handle braced
        ++position;
        result.value = parseExpr();
        if (peek().tokenType == Tokens::SEMI_COLON) { ++position; } // Allow both semi-colon and no semi-colon in braces
        expect(Tokens::R_BRACE);
        expect(Tokens::SEMI_COLON);
        return result;
}

FunctionDef Parser::parseFunctionDef() {
        FunctionDef result{};
        ++position;
        // TODO(parser) Skip any tokens before the type
        // TODO(parser) Handle types like i32*
        result.returnType = getTokenStr(expect(Tokens::IDENTIFIER));
        result.name = getTokenStr(expect(Tokens::IDENTIFIER));
        expect(Tokens::L_PARENTHESES);

        // Handle args
        if (peek().tokenType != Tokens::R_PARENTHESES) {
                if (const auto& token = getTokenStr(peek()); token != "void") {
                        while (true) {
                                Parameter param{};
                                param.type = getTokenStr(expect(Tokens::IDENTIFIER));
                                param.name = getTokenStr(expect(Tokens::IDENTIFIER));
                                result.parameters.push_back(param);
                                if (peek().tokenType == Tokens::R_PARENTHESES) {
                                        ++position;
                                        break;
                                }
                                expect(Tokens::COMMA);
                        }
                } else {
                        result.parameters.push_back({"void", ""});
                        ++position;
                        expect(Tokens::R_PARENTHESES);
                }
        } else { ++position; }
        expect(Tokens::L_BRACE);

        // Handle body
        while (peek().tokenType != Tokens::R_BRACE) {
                if (peek().tokenType == Tokens::IDENTIFIER && getTokenStr(peek()) == "return") {
                        size_t start = position;
                        ReturnStatement statement = parseReturnStatement();
                        result.body.push_back(Node{
                            .tokenStart = start,
                            .tokenEnd = position,
                            .value = std::move(statement)
                        });
                        break;
                }

                throw std::logic_error("UNEXPECTED TOKEN \'" + tokenStrings[static_cast<int>(peek().tokenType)] + "\'");
        }

        expect(Tokens::R_BRACE);

        return result;
}

void Parser::run() {
        while (peek().tokenType != Tokens::EOF_TOKEN) {
                if (peek().tokenType == Tokens::AT_SIGN) {
                        const size_t start = position;
                        Annotation annotation = parseAnnotation();
                        program.push_back({start, position, annotation});
                        continue;
                }

                if (peek().tokenType == Tokens::PERCENT) {
                        int i = 1;
                        while (true) {
                                if (peek(i).tokenType == Tokens::L_PARENTHESES) {
                                        const size_t start = position;
                                        FunctionDef functionDef = parseFunctionDef();
                                        program.push_back({start, position, std::move(functionDef)});
                                        break;
                                }

                                if (const auto type = peek(i).tokenType; type == Tokens::EQUALS || type == Tokens::SEMI_COLON) {
                                        throw std::logic_error("VARIABLE DECLARATIONS NOT YET IMPLEMENTED");
                                        // TODO(parser) Parse variable creation
                                        break;
                                }

                                if (peek(i).tokenType == Tokens::L_BRACE) {
                                        throw std::logic_error("STRUCT / CLASS / INTERFACE DECLARATIONS NOT YET IMPLEMENTED");
                                        // TODO(parser) Deduce between struct, class, and interface then parse
                                        break;
                                }

                                if (const auto type =  peek(i).tokenType; type != Tokens::IDENTIFIER) {
                                        throw std::logic_error("UNEXPECTED TOKEN \'" + tokenStrings[static_cast<int>(type)] + "\'");
                                }

                                ++i;
                        }

                        continue;
                }

                throw std::logic_error("UNKNOWN TOKEN TYPE " + tokenStrings[static_cast<int>(peek().tokenType)]);
        }
}
