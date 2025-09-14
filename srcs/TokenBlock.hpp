#pragma once
#include <vector>
#include "TokenEffect.hpp"

class TokenBlock : public std::vector<TokenEffect>
{
private:
    unsigned int priority;
public:
    TokenBlock(unsigned int priority);
    ~TokenBlock();
    void executeNot();
    bool execute();
    unsigned int getPriority() const { return priority; }
    void setPriority(unsigned int p) { priority = p; }
};
