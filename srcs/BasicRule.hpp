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
    /** convert to string */
    std::string toString() const;
    /** debug output showing internal structure */
    std::string structureToString() const;

    std::vector<TokenBlock> lhs;
    char rhs_symbol;
    bool rhs_negated;
    // which rule this was deduced from
    const LogicRule* origin;
};

std::ostream &operator<<(std::ostream &os, const BasicRule &rule);
