#pragma once

#include <vector>
#include <utility>
#include <string>
#include "TokenEffect.hpp"
#include "TokenBlock.hpp"

// Simple wrapper around the previous tuple-based rule representation.
// Holds an implication arrow token plus the LHS and RHS token blocks.
struct LogicRule
{
    TokenEffect arrow;
    std::vector<TokenBlock> lhs;
    std::vector<TokenBlock> rhs;

    LogicRule();
    LogicRule(const TokenEffect &arrow_token, std::vector<TokenBlock> lhs_blocks, std::vector<TokenBlock> rhs_blocks);
    std::string toString() const;
};

std::ostream &operator<<(std::ostream &os, const LogicRule &rule);
