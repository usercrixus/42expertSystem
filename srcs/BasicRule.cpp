#include "BasicRule.hpp"
#include "LogicRule.hpp"
#include <sstream>
#include <ostream>

BasicRule::BasicRule() : lhs(), rhs_symbol(0), rhs_negated(false), origin(nullptr)
{
}

BasicRule::BasicRule(std::vector<TokenBlock> lhs_blocks, char symbol, bool negated, const LogicRule* orig)
    : lhs(std::move(lhs_blocks)), rhs_symbol(symbol), rhs_negated(negated), origin(orig)
{
}

std::string BasicRule::toString() const
{
    std::ostringstream oss;
    oss << renderSide(lhs) << " => ";
    if (rhs_negated)
        oss << '!';
    oss << rhs_symbol;
    return oss.str();
}

std::ostream &operator<<(std::ostream &os, const BasicRule &rule)
{
    os << rule.toString();
    return os;
}
