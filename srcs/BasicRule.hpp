#pragma once

#include <vector>
#include <string>
#include "TokenBlock.hpp"

class LogicRule;

class BasicRule
{
public:
    BasicRule();
    BasicRule(std::vector<TokenBlock> lhs_blocks, char symbol, bool negated, const LogicRule* orig);
    std::string toString() const;

    std::vector<TokenBlock> lhs;
    char rhs_symbol;
    bool rhs_negated;
    const LogicRule* origin;
};

std::ostream &operator<<(std::ostream &os, const BasicRule &rule);
