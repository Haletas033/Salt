#include "parser.h"

#include <iostream>

Token Parser::peek(const size_t offset = 0) const {
        if (tokens.size() < position + offset) { throw std::logic_error("TRIED TO READ OUT OF BOUNDS TOKEN"); }
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

void Parser::parseAnnotation() {
        const size_t start = position;
        Annotation result{};
        ++position;

        result.name = getTokenStr(expect(Tokens::IDENTIFIER));
        expect(Tokens::L_BRACE);
        while (true) {
                if (
                        peek().tokenType == Tokens::IDENTIFIER
                        || peek().tokenType == Tokens::INT_LITERAL
                        || peek().tokenType == Tokens::FLOAT_LITERAL
                        || peek().tokenType == Tokens::STR_LITERAL
                        || peek().tokenType == Tokens::CHAR_LITERAL
                ) {
                        std::string name{getTokenStr(peek())};
                        result.args.push_back(name);
                        ++position;
                } else {
                        throw std::logic_error("UNEXPECTED TOKEN IN ANNOTATION");
                }

                if (peek().tokenType == Tokens::R_BRACE) break;
                expect(Tokens::COMMA);
        }
        expect(Tokens::R_BRACE);
        expect(Tokens::SEMI_COLON);
        program.push_back(Node{
            .tokenStart = start,
            .tokenEnd = position,
            .value = result
        });

        std::cout << "name: " << result.name << "\n";
        for (const auto& arg : result.args) {
                std::cout << "arg: " << arg << "\n";
        }
}

void Parser::parseFunctionDef() {

}

void Parser::run() {
        while (peek().tokenType != Tokens::EOF_TOKEN) {
                if (peek().tokenType == Tokens::AT_SIGN) {
                        parseAnnotation();
                        continue;
                }

                // if (peek().tokenType == Tokens::PERCENT) {
                //         if (getTokenStr(peek(1)) == "const") continue;
                //         if (peek(3).tokenType == Tokens::L_PARENTHESES) {
                //                 parseFunctionDef();
                //         }
                //         continue;
                // }

                throw std::logic_error("UNKNOWN TOKEN TYPE " + tokenStrings[static_cast<int>(peek().tokenType)]);
        }
}
