#pragma once

#include <vector>
#include <utility>
#include <string>
#include "TokenEffect.hpp"
#include "TokenBlock.hpp"
#include "BasicRule.hpp"

class LogicRule
{
public:
    TokenEffect arrow;
    std::vector<TokenBlock> lhs;
    std::vector<TokenBlock> rhs;

    LogicRule();
    LogicRule(const TokenEffect &arrow_token, std::vector<TokenBlock> lhs_blocks, std::vector<TokenBlock> rhs_blocks);
    std::string toString() const;
    std::vector<BasicRule> deduceBasics() const;
};

std::ostream &operator<<(std::ostream &os, const LogicRule &rule);
std::string renderSide(const std::vector<TokenBlock> &side);
