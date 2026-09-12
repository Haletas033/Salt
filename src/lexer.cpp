#include <iostream>
#include <lexer.h>

std::optional<TokenDef> Lexer::matchToken() {
        std::optional<TokenDef> best;

        for (const auto& def : token_defs) {
                std::string_view input{
                        source.data() + position,
                        source.size() - position
                    };

                if (input.starts_with(def.text)) {
                        if (!best || def.text.size() > best->text.size()) {
                                best = def;
                        }
                }
        }

        return best;
}

void Lexer::releaseIdentifier(std::string &currentIdentifier, const IdentifierType currentIdentifierType) {
        if (!currentIdentifier.empty()) {
                auto token = Token{
                        .tokenStart = position - currentIdentifier.size(),
                        .tokenEnd = position
                };
                switch (currentIdentifierType) {

                        case IdentifierType::NORMAL:
                                token.tokenType = Tokens::IDENTIFIER;
                                break;
                        case IdentifierType::INTEGRAL:
                                token.tokenType = Tokens::INT_LITERAL;
                                break;
                        case IdentifierType::FLOATING:
                                token.tokenType = Tokens::FLOAT_LITERAL;
                                break;
                        case IdentifierType::STR:
                                token.tokenType = Tokens::STR_LITERAL;
                                break;
                        case IdentifierType::CHAR:
                                token.tokenType = Tokens::CHAR_LITERAL;
                                break;
                }
                tokens.push_back(token);
                currentIdentifier.clear();
        }
}

void Lexer::skipComments() {
        const bool isSingleLineComment = position + 1 < source.size()
                ? source[position] == '/' && source[position+1] == '/' : false;

        if (isSingleLineComment) {
                while (position + 1 < source.size() && source[position] != '\n') { ++position; }
                if (position < source.size() && source[position] == '\n') { ++position; } // Skip newline
                return;
        }

        const bool isMultiLineComment = position + 1 < source.size()
                ? source[position] == '/' && source[position+1] == '*' : false;

        if (isMultiLineComment) {
                while (position + 1 < source.size()) {
                        ++position;
                        if (source[position] == '*' && source[position+1] == '/') { break; }
                }
                if (position + 1 < source.size()) { position += 2; } // Skip '*/'
        }
}

 void Lexer::updateState(const TokenDef& token, std::stack<InsideType>& insideStack, IdentifierType& currentIdentifierType) {

        switch (token.token) {
                case Tokens::INTERP: {
                        insideStack.push(InsideType::INTERP);
                        currentIdentifierType = IdentifierType::NORMAL;
                        return;
                }

                case Tokens::STR: {
                        if (!insideStack.empty() && insideStack.top() == InsideType::STR) {
                                insideStack.pop();
                                if (!insideStack.empty()) {
                                        switch (insideStack.top()) {
                                                case InsideType::STR:
                                                        currentIdentifierType = IdentifierType::STR;
                                                        break;
                                                case InsideType::INTERP:
                                                        currentIdentifierType = IdentifierType::NORMAL;
                                                        break;
                                                default:
                                                        currentIdentifierType = IdentifierType::NORMAL;
                                        }
                                } else {
                                        currentIdentifierType = IdentifierType::NORMAL;
                                }
                                return;
                        }

                        insideStack.push(InsideType::STR);
                        currentIdentifierType = IdentifierType::STR;
                        return;
                }

                case Tokens::CHAR: {
                        if (!insideStack.empty() && insideStack.top() == InsideType::CHAR) {
                                insideStack.pop();
                                currentIdentifierType = IdentifierType::NORMAL;
                                return;
                        }

                        insideStack.push(InsideType::CHAR);
                        currentIdentifierType = IdentifierType::CHAR;
                        return;
                }

                case Tokens::L_BRACE: {
                        insideStack.push(InsideType::BRACE);
                        return;
                }

                case Tokens::R_BRACE: {
                        if (!insideStack.empty() && (insideStack.top() == InsideType::BRACE || insideStack.top() == InsideType::INTERP)) {
                                insideStack.pop();
                                if (!insideStack.empty()) {
                                        switch (insideStack.top()) {
                                                case InsideType::STR:
                                                        currentIdentifierType = IdentifierType::STR;
                                                        break;
                                                case InsideType::CHAR:
                                                        currentIdentifierType = IdentifierType::CHAR;
                                                        break;
                                                default: currentIdentifierType = IdentifierType::NORMAL;
                                        }

                                }
                        }

                }

                default:;
        }
}

void Lexer::run() {
        std::stack<InsideType> insideStack{};
        std::string currentIdentifier{};
        IdentifierType currentIdentifierType{};

        while (position < source.size()) {
                skipComments();

                const bool dotPartOfFloat = position + 1 < source.size() ?
                        (isdigit(source[position+1]) || source[position+1] == 'f')
                        && currentIdentifierType == IdentifierType::INTEGRAL
                : false;

                if (source[position] == '.' && dotPartOfFloat) {/* skip dots that are part of floats */}
                else {
                        if (const auto match = matchToken()) {
                                const auto previousIdentifierType = currentIdentifierType;
                                updateState(match.value(), insideStack, currentIdentifierType);
                                const bool isInsideToken = match->token == Tokens::STR || match->token == Tokens::CHAR || match->token == Tokens::INTERP;
                                if (isInsideToken || (previousIdentifierType != IdentifierType::STR && previousIdentifierType != IdentifierType::CHAR)) {
                                        // Release current identifier if any
                                        releaseIdentifier(currentIdentifier, previousIdentifierType);

                                        const auto tokenStart = position;
                                        position += match->text.size();
                                        tokens.push_back(Token{
                                            .tokenType = match->token,
                                            .tokenStart = tokenStart,
                                            .tokenEnd = position
                                        });
                                        continue;
                                }
                        }
                }

                if (source[position] == ' ' || source[position] == '\n' || source[position] == '\t' || source[position] == '\r') {
                        if (currentIdentifierType != IdentifierType::STR && currentIdentifierType != IdentifierType::CHAR) {
                                releaseIdentifier(currentIdentifier, currentIdentifierType);
                                ++position;
                                continue;
                        }
                }

                if (currentIdentifier.empty() && currentIdentifierType != IdentifierType::STR && currentIdentifierType != IdentifierType::CHAR) {
                        currentIdentifier += source[position];
                        if (std::isalpha(currentIdentifier[0]) || currentIdentifier.starts_with('_'))
                                currentIdentifierType = IdentifierType::NORMAL;
                        if (std::isdigit(currentIdentifier[0]))
                                currentIdentifierType = IdentifierType::INTEGRAL;
                        if (currentIdentifier.starts_with('.'))
                                currentIdentifierType = IdentifierType::FLOATING;
                } else {
                        currentIdentifier += source[position];
                        if (currentIdentifierType == IdentifierType::INTEGRAL) {
                                if (source[position] == '.' || source[position] == 'f')
                                        currentIdentifierType = IdentifierType::FLOATING;
                        }
                }

                ++position;
        }
        releaseIdentifier(currentIdentifier, currentIdentifierType);
        tokens.push_back(Token{
            .tokenType = Tokens::EOF_TOKEN,
            .tokenStart = position - 1,
            .tokenEnd = position
        });
}
