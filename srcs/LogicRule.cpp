#include "LogicRule.hpp"
#include <sstream>
#include <ostream>

LogicRule::LogicRule() : arrow(0), lhs(), rhs()
{
}

LogicRule::LogicRule(const TokenEffect &arrow_token, std::vector<TokenBlock> lhs_blocks, std::vector<TokenBlock> rhs_blocks)
    : arrow(arrow_token), lhs(std::move(lhs_blocks)), rhs(std::move(rhs_blocks))
{
}

namespace
{
    std::string renderSide(const std::vector<TokenBlock> &side)
    {
        std::ostringstream oss;
        int currentPriority = 0;

        for (size_t i = 0; i < side.size(); ++i)
        {
            int p = static_cast<int>(side[i].getPriority());
            if (p > currentPriority)
                oss << std::string(static_cast<size_t>(p - currentPriority), '(');
            else if (p < currentPriority)
                oss << std::string(static_cast<size_t>(currentPriority - p), ')');

            for (const TokenEffect &tk : side[i])
            {
                if (tk.type != 0)
                    oss << tk.type;
            }

            currentPriority = p;
        }

        if (currentPriority > 0)
            oss << std::string(static_cast<size_t>(currentPriority), ')');

        return oss.str();
    }

    std::string arrowToString(const TokenEffect &arrow)
    {
        if (arrow.type == '>')
            return "=>";
        if (arrow.type == '=')
            return "<=>";
        return "=>";
    }
}

std::string LogicRule::toString() const
{
    std::ostringstream oss;
    oss << renderSide(lhs) << ' ' << arrowToString(arrow) << ' ' << renderSide(rhs);
    return oss.str();
}

std::ostream &operator<<(std::ostream &os, const LogicRule &rule)
{
    os << rule.toString();
    return os;
}
