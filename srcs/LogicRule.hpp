#pragma once

#include <vector>
#include <utility>
#include <string>
#include "TokenEffect.hpp"
#include "TokenBlock.hpp"
#include "BasicRule.hpp"

/**
 * Logical rule with tokenized left/right expressions and an operator.
 **/
class LogicRule
{
public:
    /**
     * implication or equivalence operator token (=> or <=>).
     **/
    TokenEffect arrow;
    /**
     * left-hand side expression tokens, grouped by priority blocks.
     **/
    std::vector<TokenBlock> lhs;
    /**
     * right-hand side expression tokens, grouped by priority blocks.
     **/
    std::vector<TokenBlock> rhs;

    /**
     * create an empty rule with no operator or sides.
     **/
    LogicRule();
    /**
     * construct a rule from an operator and both sides.
     **/
    LogicRule(const TokenEffect &arrow_token, std::vector<TokenBlock> lhs_blocks, std::vector<TokenBlock> rhs_blocks);
    /**
     * convert rule back to a compact string representation.
     **/
    std::string toString() const;
    /**
     * expand rule into basic rules (RHS with single variable).
     **/
    std::vector<BasicRule> deduceBasics() const;
};

/**
 * stream helper to print a rule.
 **/
std::ostream &operator<<(std::ostream &os, const LogicRule &rule);
/**
 * render a side (lhs or rhs) to a string, including parentheses by priority.
 **/
std::string renderSide(const std::vector<TokenBlock> &side);
