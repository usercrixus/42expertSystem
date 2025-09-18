#include "TokenBlock.hpp"
#include <stdexcept>
#include <iostream>

TokenBlock::TokenBlock(unsigned int priority) : priority(priority)
{
}

TokenBlock::~TokenBlock()
{
}

void TokenBlock::executeNot()
{
    size_t i = 0;
    while (i < (*this).size())
    {
        char op = (*this)[i].type;
        if (op == '!')
        {
            if (i + 1 == (*this).size())
                throw std::logic_error("operator ! has no var attached\n");
            char rn = (*this)[i + 1].type;
            if ((rn >= 'A' && rn <= 'Z') || rn == 0)
            {
                (*this)[i + 1].effect = !(*this)[i + 1].effect;
                (*this)[i + 1].type = 0;
                (*this).erase((*this).begin() + i);
                if (i > 0)
                    --i;
            }
            else
                throw std::logic_error("operator ! has no var attached\n");
        }
        else
            ++i;
    }
}

bool TokenBlock::func_xor(bool a, bool b)
{
    return a ^ b;
}

bool TokenBlock::func_or(bool a, bool b)
{
    return a | b;
}

bool TokenBlock::func_and(bool a, bool b)
{
    return a & b;
}

void TokenBlock::executeOthers(char op_target, bool (TokenBlock::*func_op)(bool, bool))
{
    size_t i = 0;
    while (i < (*this).size())
    {
        char op = (*this)[i].type;
        if (op == op_target)
        {
            if (i == 0 || i + 1 == (*this).size())
                throw std::logic_error(std::string("operator ") + op_target + " has no var attached\n");

            char ln = (*this)[i - 1].type;
            char rn = (*this)[i + 1].type;
            if (((ln >= 'A' && ln <= 'Z') || ln == 0) &&
                ((rn >= 'A' && rn <= 'Z') || rn == 0))
            {
                (*this)[i].effect = (this->*func_op)((*this)[i - 1].effect, (*this)[i + 1].effect);
                (*this)[i].type = 0;
                (*this).erase((*this).begin() + i + 1);
                (*this).erase((*this).begin() + i - 1);
                if (i > 0)
                    --i;
            }
            else
                throw std::logic_error(std::string("operator ") + op + " has no var attached\n");
        }
        else
            ++i;
    }
}

bool TokenBlock::execute()
{
    if (this->empty())
        throw std::logic_error("TokenBlock::execute: empty block");
    executeNot();
    executeOthers('^', &TokenBlock::func_xor);
    executeOthers('|', &TokenBlock::func_or);
    executeOthers('+', &TokenBlock::func_and);
    if (this->size() != 1)
        throw std::logic_error("TokenBlock::execute: reduction did not converge");
    return (*this)[0].effect;
}

unsigned int TokenBlock::getPriority() const
{
    return priority;
}

void TokenBlock::setPriority(unsigned int p)
{
    priority = p;
}
