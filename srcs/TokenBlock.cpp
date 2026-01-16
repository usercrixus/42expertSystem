#include "TokenBlock.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>

TokenBlock::TokenBlock(unsigned int priority) : priority(priority)
{
}

TokenBlock::TokenBlock(unsigned int priority, char initial) : priority(priority)
{
    this->emplace_back(TokenEffect(initial));
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

TokenBlock TokenBlock::withPriority(unsigned int new_priority) const
{
    TokenBlock copy(new_priority);
    for (const TokenEffect &tk : *this)
        copy.push_back(tk);
    return copy;
}

TokenBlock TokenBlock::extractRange(size_t start, size_t end, unsigned int new_priority) const
{
    TokenBlock extracted(new_priority);
    if (end > this->size())
        end = this->size();
    for (size_t i = start; i < end; ++i)
        extracted.push_back((*this)[i]);
    return extracted;
}

bool TokenBlock::hasOperator(char op) const
{
    for (const TokenEffect &tk : *this)
    {
        if (tk.type == op)
            return true;
    }
    return false;
}

bool TokenBlock::hasAnyOperator(const std::vector<char> &ops) const
{
    for (const TokenEffect &tk : *this)
    {
        for (char op : ops)
        {
            if (tk.type == op)
                return true;
        }
    }
    return false;
}

void TokenBlock::appendTokens(const TokenBlock &other)
{
    for (const TokenEffect &tk : other)
        this->push_back(tk);
}

std::string TokenBlock::structureToString() const
{
    std::ostringstream oss;
    oss << "[priority=" << priority << ", size=" << this->size() << "]: ";
    for (size_t i = 0; i < this->size(); ++i)
    {
        const TokenEffect &tk = (*this)[i];
        if (tk.type == 0)
            oss << "(null)";
        else if (tk.type >= 'A' && tk.type <= 'Z')
            oss << "'" << tk.type << "'";
        else
            oss << "op(" << tk.type << ")";
        
        if (i + 1 < this->size())
            oss << " ";
    }
    return oss.str();
}