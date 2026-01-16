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

std::string BasicRule::structureToString() const
{
    std::ostringstream oss;
    oss << "BasicRule Structure:\n";
    oss << "  LHS blocks (" << lhs.size() << "):\n";
    
    for (size_t i = 0; i < lhs.size(); ++i)
    {
        const TokenBlock &block = lhs[i];
        oss << "    Block " << i << " [priority=" << block.getPriority() << ", size=" << block.size() << "]: ";
        for (size_t j = 0; j < block.size(); ++j)
        {
            const TokenEffect &tk = block[j];
            if (tk.type == 0)
                oss << "(null)";
            else if (tk.type >= 'A' && tk.type <= 'Z')
                oss << "'" << tk.type << "'";
            else
                oss << "op(" << tk.type << ")";
            
            if (j + 1 < block.size())
                oss << " ";
        }
        oss << "\n";
    }
    
    oss << "  RHS: ";
    if (rhs_negated)
        oss << "!";
    oss << rhs_symbol << "\n";
    
    return oss.str();
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
