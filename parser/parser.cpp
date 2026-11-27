#include "parser.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include <file.h>
#include <lexer/lexer.h>

namespace key_multitude {
namespace {

int readKey(lexer::TokenStream& tokens, const std::string& message) {
    const auto& token = tokens.peekToken();
    const auto key = tokens.expectInt(message);
    if (!std::in_range<int>(key)) {
        tokens.failAtToken(token, "Key is out of range for int");
    }
    return static_cast<int>(key);
}

} // namespace

std::vector<Command> Parser::parse(const std::filesystem::path& path) const {
    lexer::Lexer input(generalFunctions::readFile(path), path);
    auto tokens = input.tokenize();
    return parse(tokens);
}

std::vector<Command> Parser::parse(lexer::TokenStream& tokens) const {
    std::vector<Command> commands;
    while (!tokens.atEnd()) {
        const auto& commandToken = tokens.peekToken();
        const auto& command = tokens.expectIdent("Expected command k, q, m or n");
        if (command == "k") {
            commands.emplace_back(InsertKey{readKey(tokens, "Expected a key after k")});
        } else if (command == "q") {
            const auto left = readKey(tokens, "Expected the left range boundary");
            const auto& rightToken = tokens.peekToken();
            const auto right = readKey(tokens, "Expected the right range boundary");
            if (left > right) {
                tokens.failAtToken(rightToken, "Right boundary must be at least the left boundary");
            }
            commands.emplace_back(CountInRange{left, right});
        } else if (command == "m") {
            const auto& rankToken = tokens.peekToken();
            const auto rank = tokens.expectInt("Expected a rank after m");
            if (rank <= 0 || !std::in_range<std::size_t>(rank)) {
                tokens.failAtToken(rankToken, "Expected a positive rank in range for size_t");
            }
            commands.emplace_back(FindKthSmallest{static_cast<std::size_t>(rank)});
        } else if (command == "n") {
            commands.emplace_back(CountLessThan{readKey(tokens, "Expected a key after n")});
        } else {
            tokens.failAtToken(commandToken, "Unknown command: " + command);
        }
    }

    return commands;
}

} // namespace key_multitude
