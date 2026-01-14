#pragma once
#include <vector>
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
    ~TokenBlock();
    /**
     * resolve the whole token block
     */
    bool execute();
    unsigned int getPriority() const;
    void setPriority(unsigned int p);
};
