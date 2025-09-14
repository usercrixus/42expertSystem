#include "TokenBlock.hpp"
#include <stdexcept>

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

bool TokenBlock::execute()
{
    if (this->empty())
        throw std::logic_error("TokenBlock::execute: empty block");
    executeNot();
    while (this->size() > 1)
    {
        bool reduced_something = false;
        size_t i = 0;
        while (i < (*this).size())
        {
            char op = (*this)[i].type;
            if (op == '+' || op == '|' || op == '^')
            {
                if (i == 0 || i + 1 == (*this).size())
                    throw std::logic_error(std::string("operator ") + op + " has no var attached\n");

                char ln = (*this)[i - 1].type;
                char rn = (*this)[i + 1].type;
                if (((ln >= 'A' && ln <= 'Z') || ln == 0) &&
                    ((rn >= 'A' && rn <= 'Z') || rn == 0))
                {
                    if (op == '+')
                        (*this)[i].effect = (*this)[i - 1].effect & (*this)[i + 1].effect;
                    if (op == '|')
                        (*this)[i].effect = (*this)[i - 1].effect | (*this)[i + 1].effect;
                    if (op == '^')
                        (*this)[i].effect = (*this)[i - 1].effect ^ (*this)[i + 1].effect;
                    (*this)[i].type = 0;
                    (*this).erase((*this).begin() + i + 1);
                    (*this).erase((*this).begin() + i - 1);
                    reduced_something = true;
                    if (i > 0)
                        --i;
                }
                else
                    throw std::logic_error(std::string("operator ") + op + " has no var attached\n");
            }
            else
                ++i;
        }
        if (!reduced_something)
            break;
    }
    if (this->size() != 1)
        throw std::logic_error("TokenBlock::execute: reduction did not converge");
    return (*this)[0].effect;
}
