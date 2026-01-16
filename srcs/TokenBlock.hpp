#pragma once
#include <vector>
#include <string>
#include "TokenEffect.hpp"

/**
 * vector of TokenEffect
 * see TokenEffect class for more information
 */
class TokenBlock : public std::vector<TokenEffect>
{
private:
    /**
     * xor a b
     */
    bool func_xor(bool a, bool b);
    /**
     * or a b
     */
    bool func_or(bool a, bool b);
    /**
     * and a b
     */
    bool func_and(bool a, bool b);
    /**
     * resolve not effect
     */
    void executeNot();
    /**
     * resolve xor not and or and other effect as 'func_op'
     */
    void executeOthers(char op_target, bool (TokenBlock::*func_op)(bool, bool));
    // current priority level (relative to open parenthesis)
    unsigned int priority;

public:
    TokenBlock(unsigned int priority);
    TokenBlock(unsigned int priority, char initial);
    ~TokenBlock();
    /**
     * resolve the whole token block
     */
    bool execute();
    unsigned int getPriority() const;
    void setPriority(unsigned int p);
    /**
     * Create a copy of this block with a different priority
     */
    TokenBlock withPriority(unsigned int new_priority) const;
    /**
     * Extract a range of tokens into a new block
     */
    TokenBlock extractRange(size_t start, size_t end, unsigned int new_priority) const;
        /**
     * Check if block contains a specific operator type
     */
    bool hasOperator(char op) const;
    /**
     * Check if block contains any of the specified operators
     */
    bool hasAnyOperator(const std::vector<char> &ops) const;
    /**
     * Append all tokens from another block
     */
    void appendTokens(const TokenBlock &other);
    /**
     * Get a debug string showing the structure of this block
     */
    std::string structureToString() const;
};
