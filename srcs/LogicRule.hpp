#pragma once

#include <vector>
#include <utility>
#include <string>
#include "TokenEffect.hpp"
#include "TokenBlock.hpp"

// Forward declaration for LogicRule::deduceBasics()
struct BasicRule;

struct LogicRule
{
    TokenEffect arrow;
    std::vector<TokenBlock> lhs;
    std::vector<TokenBlock> rhs;

    LogicRule();
    LogicRule(const TokenEffect &arrow_token, std::vector<TokenBlock> lhs_blocks, std::vector<TokenBlock> rhs_blocks);
    std::string toString() const;
    std::vector<BasicRule> deduceBasics() const;
};

std::ostream &operator<<(std::ostream &os, const LogicRule &rule);

struct BasicRule
{
    std::vector<TokenBlock> lhs;
    char rhs_symbol;
    bool rhs_negated;
    const LogicRule* origin;

    BasicRule();
    BasicRule(std::vector<TokenBlock> lhs_blocks, char symbol, bool negated, const LogicRule* orig);
    std::string toString() const;
};

std::ostream &operator<<(std::ostream &os, const BasicRule &rule);
