#pragma once

#include <vector>
#include <string>
#include "TokenBlock.hpp"

class LogicRule;

/**
 * Atomic rule with a single RHS symbol and a tokenized LHS.
 **/
class BasicRule
{
public:
    /**
     * create an empty rule with no sides.
     **/
    BasicRule();
    /**
     * construct a basic rule from a tokenized LHS and a RHS symbol.
     * @param lhs_blocks tokenized left-hand side expression.
     * @param symbol right-hand side symbol.
     * @param negated whether the RHS symbol is negated.
     * @param orig original logic rule that produced this basic rule.
     **/
    BasicRule(std::vector<TokenBlock> lhs_blocks, char symbol, bool negated, const LogicRule* orig);
    /**
     * convert rule to a compact string representation.
     **/
    std::string toString() const;
    /**
     * debug output showing internal structure.
     **/
    std::string structureToString() const;

    /**
     * tokenized left-hand side expression.
     **/
    std::vector<TokenBlock> lhs;
    /**
     * right-hand side symbol.
     **/
    char rhs_symbol;
    /**
     * whether the RHS symbol is negated.
     **/
    bool rhs_negated;
    /**
     * original logic rule this was deduced from.
     **/
    const LogicRule* origin;
};

/**
 * stream helper to print a rule.
 **/
std::ostream &operator<<(std::ostream &os, const BasicRule &rule);
