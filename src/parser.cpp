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
                const std::string output{getTokenStr(peek())};
                switch (peek().tokenType) {
                        case Tokens::INT_LITERAL: {
                                result.type = ReturnStatement::Type::INTEGER;
                                result.value = output;
                                break;
                        }

                        case Tokens::FLOAT_LITERAL: {
                                result.type = ReturnStatement::Type::FLOATING;
                                result.value = output;
                                break;
                        }

                        case Tokens::STR: {
                                ++position;
                                result.type = ReturnStatement::Type::STR;
                                result.value = getTokenStr(expect(Tokens::STR_LITERAL));
                                expect(Tokens::STR);
                                break;
                        }

                        case Tokens::CHAR: {
                                ++position;
                                result.type = ReturnStatement::Type::CHAR;
                                result.value = getTokenStr(expect(Tokens::CHAR_LITERAL));
                                expect(Tokens::CHAR);
                                break;
                        }

                        case Tokens::IDENTIFIER: {
                                throw std::logic_error("IDENTIFIERS AND STATEMENTS IN RETURNS NEED TO BE WRAPPED IN BRACES");
                                break;
                        }

                        default: {
                                throw std::logic_error("UNEXPECTED TOKEN \'" + tokenStrings[static_cast<int>(peek().tokenType)] + "\'");
                        }
                }
                ++position;
                expect(Tokens::SEMI_COLON);
                return result;
        }

        // Handle braced
        ++position;
        result.type = ReturnStatement::Type::IDENTIFIER;
        result.value = getTokenStr(expect(Tokens::IDENTIFIER));
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
                        result.body.push_back({start, position, statement});
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
                                        program.push_back({start, position, functionDef});
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
