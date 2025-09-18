#pragma once
#include <vector>
#include "TokenEffect.hpp"

class TokenBlock : public std::vector<TokenEffect>
{
private:
    bool func_xor(bool a, bool b);
    bool func_or(bool a, bool b);
    bool func_and(bool a, bool b);
    unsigned int priority;

public:
    TokenBlock(unsigned int priority);
    ~TokenBlock();
    void executeNot();
    void executeOthers(char op_target, bool (TokenBlock::*func_op)(bool, bool));
    bool execute();
    unsigned int getPriority() const;
    void setPriority(unsigned int p);
};
