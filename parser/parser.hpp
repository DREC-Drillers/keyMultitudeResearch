#pragma once

#include <cstddef>
#include <filesystem>
#include <lexer/token_stream.h>
#include <variant>
#include <vector>

namespace key_multitude {

struct InsertKey {
    int key;
};

struct CountInRange {
    int left;
    int right;
};

struct FindKthSmallest {
    std::size_t rank;
};

struct CountLessThan {
    int key;
};

using Command =
    std::variant<InsertKey, CountInRange, FindKthSmallest, CountLessThan>;

class Parser {
public:
    std::vector<Command> parse(const std::filesystem::path& path) const;
    std::vector<Command> parse(lexer::TokenStream& tokens) const;
};

} // namespace key_multitude
