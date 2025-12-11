#include "LogicRule.hpp"
#include <sstream>
#include <ostream>
#include <queue>
#include <climits>

LogicRule::LogicRule() : arrow(0), lhs(), rhs()
{
}

LogicRule::LogicRule(const TokenEffect &arrow_token, std::vector<TokenBlock> lhs_blocks, std::vector<TokenBlock> rhs_blocks)
    : arrow(arrow_token), lhs(std::move(lhs_blocks)), rhs(std::move(rhs_blocks))
{
}

static std::string renderSide(const std::vector<TokenBlock> &side)
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

static std::string arrowToString(const TokenEffect &arrow)
{
    if (arrow.type == '>')
        return "=>";
    if (arrow.type == '=')
        return "<=>";
    return "=>";
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

static bool hasOrXor(const std::vector<TokenBlock> &rhs)
{
    for (const TokenBlock &block : rhs)
    {
        for (const TokenEffect &tk : block)
        {
            if (tk.type == '|' || tk.type == '^')
                return true;
        }
    }
    return false;
}

static std::vector<char> extractSymbols(const std::vector<TokenBlock> &side)
{
    std::vector<char> symbols;
    for (const TokenBlock &block : side)
    {
        for (const TokenEffect &tk : block)
        {
            if (tk.type >= 'A' && tk.type <= 'Z')
                symbols.push_back(tk.type);
        }
    }
    return symbols;
}

static std::vector<LogicRule> expandEquivalence(const LogicRule &rule)
{
    std::vector<LogicRule> result;
    
    if (rule.arrow.type == '=')
    {
        // A <=> B becomes: A => B and B => A
        result.emplace_back(TokenEffect('>'), rule.lhs, rule.rhs);
        result.emplace_back(TokenEffect('>'), rule.rhs, rule.lhs);
    }
    else
    {
        result.push_back(rule);
    }
    
    return result;
}

// Helper: split a rule by AND operators at the lowest priority level
// A => B + C becomes A => B and A => C
static std::vector<LogicRule> splitByAndAtLowestPriority(const LogicRule &rule, unsigned int min_priority)
{
    std::vector<LogicRule> result;
    std::vector<std::vector<TokenBlock>> sub_expressions;
    std::vector<TokenBlock> current_sub;
    
    for (size_t i = 0; i < rule.rhs.size(); ++i)
    {
        const TokenBlock &block = rule.rhs[i];
        
        if (block.getPriority() == min_priority)
        {
            // This is a lowest-priority block - may contain symbols and AND operators
            // Split this block by AND operators
            std::vector<TokenEffect> current_tokens;
            
            for (size_t j = 0; j < block.size(); ++j)
            {
                const TokenEffect &tk = block[j];
                
                if (tk.type == '+')
                {
                    // AND operator found - save current tokens and create a sub-expression
                    if (!current_tokens.empty() || !current_sub.empty())
                    {
                        // Create a block with the accumulated tokens
                        if (!current_tokens.empty())
                        {
                            TokenBlock final_block(min_priority);
                            for (const auto &t : current_tokens)
                                if (t.type != 0)
                                    final_block.push_back(t);
                            if (!final_block.empty())
                                current_sub.push_back(final_block);
                        }
                        
                        // Save this sub-expression
                        if (!current_sub.empty())
                            sub_expressions.push_back(current_sub);
                        
                        current_sub.clear();
                        current_tokens.clear();
                    }
                }
                else if (tk.type != 0)
                {
                    // Non-null token that's not AND - accumulate it
                    current_tokens.push_back(tk);
                }
            }
            
            // Add any remaining tokens from this block
            if (!current_tokens.empty())
            {
                TokenBlock final_block(min_priority);
                for (const auto &t : current_tokens)
                    if (t.type != 0)
                        final_block.push_back(t);
                if (!final_block.empty())
                    current_sub.push_back(final_block);
            }
        }
        else
        {
            // Higher priority block - add to current sub-expression
            current_sub.push_back(block);
        }
    }
    
    // Add any remaining sub-expression
    if (!current_sub.empty())
        sub_expressions.push_back(current_sub);
    
    // Create a rule for each sub-expression
    for (const auto &sub_rhs : sub_expressions)
        if (!sub_rhs.empty())
            result.emplace_back(TokenEffect('>'), rule.lhs, sub_rhs);
    
    return result;
}

// Helper: expand OR operator in RHS
// A => B | C becomes: A + !B => C and A + !C => B
static std::vector<LogicRule> expandOrOperator(const LogicRule &rule, size_t block_index, size_t token_index)
{
    std::vector<LogicRule> result;
    const TokenBlock &block = rule.rhs[block_index];
    
    // Extract left operand (all tokens before OR)
    std::vector<TokenBlock> left_rhs;
    left_rhs.insert(left_rhs.end(), rule.rhs.begin(), rule.rhs.begin() + block_index);
    
    TokenBlock left_block(block.getPriority());
    for (size_t k = 0; k < token_index; ++k)
        left_block.push_back(rule.rhs[block_index][k]);
    if (!left_block.empty())
        left_rhs.push_back(left_block);
    
    // Extract right operand (all tokens after OR)
    TokenBlock right_block(block.getPriority());
    for (size_t k = token_index + 1; k < rule.rhs[block_index].size(); ++k)
        right_block.push_back(rule.rhs[block_index][k]);
    
    std::vector<TokenBlock> right_rhs;
    if (!right_block.empty())
        right_rhs.push_back(right_block);
    right_rhs.insert(right_rhs.end(), rule.rhs.begin() + block_index + 1, rule.rhs.end());
    
    // Create rule 1: A + !left => right
    std::vector<TokenBlock> new_lhs_1 = rule.lhs;
    if (!left_block.empty())
    {
        new_lhs_1.push_back(TokenBlock(0));
        new_lhs_1.back().emplace_back(TokenEffect('+'));
        
        TokenBlock neg_block(0);
        neg_block.emplace_back(TokenEffect('!'));
        for (const TokenEffect &tk : left_block)
            neg_block.push_back(tk);
        new_lhs_1.push_back(neg_block);
    }
    result.emplace_back(TokenEffect('>'), new_lhs_1, right_rhs);
    
    // Create rule 2: A + !right => left
    std::vector<TokenBlock> new_lhs_2 = rule.lhs;
    if (!right_block.empty())
    {
        new_lhs_2.push_back(TokenBlock(0));
        new_lhs_2.back().emplace_back(TokenEffect('+'));
        
        TokenBlock neg_block(0);
        neg_block.emplace_back(TokenEffect('!'));
        for (const TokenEffect &tk : right_block)
            neg_block.push_back(tk);
        new_lhs_2.push_back(neg_block);
    }
    result.emplace_back(TokenEffect('>'), new_lhs_2, left_rhs);
    
    return result;
}

// Helper: expand XOR operator in RHS with constraint
// A => B ^ C becomes: A + !B => C and A + !C => B and B + C => !A
static std::vector<LogicRule> expandXorOperator(const LogicRule &rule, size_t block_index, size_t token_index)
{
    std::vector<LogicRule> result;
    const TokenBlock &block = rule.rhs[block_index];
    
    // Extract left operand (all tokens before XOR)
    std::vector<TokenBlock> left_rhs;
    left_rhs.insert(left_rhs.end(), rule.rhs.begin(), rule.rhs.begin() + block_index);
    
    TokenBlock left_block(block.getPriority());
    for (size_t k = 0; k < token_index; ++k)
        left_block.push_back(rule.rhs[block_index][k]);
    if (!left_block.empty())
        left_rhs.push_back(left_block);
    
    // Extract right operand (all tokens after XOR)
    TokenBlock right_block(block.getPriority());
    for (size_t k = token_index + 1; k < rule.rhs[block_index].size(); ++k)
        right_block.push_back(rule.rhs[block_index][k]);
    
    std::vector<TokenBlock> right_rhs;
    if (!right_block.empty())
        right_rhs.push_back(right_block);
    right_rhs.insert(right_rhs.end(), rule.rhs.begin() + block_index + 1, rule.rhs.end());
    
    // Rule 1: A + !left => right
    std::vector<TokenBlock> new_lhs_1 = rule.lhs;
    if (!left_block.empty())
    {
        new_lhs_1.push_back(TokenBlock(0));
        new_lhs_1.back().emplace_back(TokenEffect('+'));
        
        TokenBlock neg_block(0);
        neg_block.emplace_back(TokenEffect('!'));
        for (const TokenEffect &tk : left_block)
            neg_block.push_back(tk);
        new_lhs_1.push_back(neg_block);
    }
    result.emplace_back(TokenEffect('>'), new_lhs_1, right_rhs);
    
    // Rule 2: A + !right => left
    std::vector<TokenBlock> new_lhs_2 = rule.lhs;
    if (!right_block.empty())
    {
        new_lhs_2.push_back(TokenBlock(0));
        new_lhs_2.back().emplace_back(TokenEffect('+'));
        
        TokenBlock neg_block(0);
        neg_block.emplace_back(TokenEffect('!'));
        for (const TokenEffect &tk : right_block)
            neg_block.push_back(tk);
        new_lhs_2.push_back(neg_block);
    }
    result.emplace_back(TokenEffect('>'), new_lhs_2, left_rhs);
    
    // Constraint Rule: left + right => !A (can't have both true)
    // This enforces exclusivity: if both operands are true, then the original LHS must be false
    std::vector<TokenBlock> constraint_lhs;
    constraint_lhs.push_back(left_block);
    
    constraint_lhs.push_back(TokenBlock(0));
    constraint_lhs.back().emplace_back(TokenEffect('+'));
    
    constraint_lhs.push_back(right_block);
    
    // Build RHS: negation of original LHS symbols
    std::vector<TokenBlock> constraint_rhs;
    std::vector<char> lhs_symbols = extractSymbols(rule.lhs);
    
    // If original LHS has multiple symbols, negate each one
    for (size_t i = 0; i < lhs_symbols.size(); ++i)
    {
        if (i > 0)
        {
            TokenBlock and_op(0);
            and_op.emplace_back(TokenEffect('+'));
            constraint_rhs.push_back(and_op);
        }
        
        TokenBlock neg_symbol(0);
        neg_symbol.emplace_back(TokenEffect('!'));
        neg_symbol.emplace_back(TokenEffect(lhs_symbols[i]));
        constraint_rhs.push_back(neg_symbol);
    }
    
    result.emplace_back(TokenEffect('>'), constraint_lhs, constraint_rhs);
    
    return result;
}

// Helper: expand OR/XOR at top level of RHS
// First factor out AND operators at lowest priority level if present
static std::vector<LogicRule> expandOrInRhs(const LogicRule &rule)
{
    std::vector<LogicRule> result;
    
    // Step 1: Find lowest priority level
    unsigned int min_priority = UINT_MAX;
    for (const TokenBlock &block : rule.rhs)
    {
        unsigned int p = block.getPriority();
        if (p < min_priority)
            min_priority = p;
    }
    
    // Step 2: Check if there are AND operators (+) at lowest priority
    // If yes, split by AND first: A => B + C becomes A => B and A => C
    bool has_and_at_lowest = false;
    for (const TokenBlock &block : rule.rhs)
    {
        if (block.getPriority() == min_priority)
        {
            for (const TokenEffect &tk : block)
            {
                if (tk.type == '+')
                {
                    has_and_at_lowest = true;
                    break;
                }
            }
        }
        if (has_and_at_lowest) break;
    }
    if (has_and_at_lowest)
        return splitByAndAtLowestPriority(rule, min_priority);
    
    // Step 3: Look for OR or XOR operators and expand them
    for (size_t i = 0; i < rule.rhs.size(); ++i)
    {
        const TokenBlock &block = rule.rhs[i];
        for (size_t j = 0; j < block.size(); ++j)
        {
            if (block[j].type == '|')
                return expandOrOperator(rule, i, j);
            else if (block[j].type == '^')
                return expandXorOperator(rule, i, j);
        }
    }
    
    // No OR/XOR found, return as-is
    result.push_back(rule);
    return result;
}

static void extractBasicRules(const LogicRule &rule, const LogicRule *origin, std::vector<BasicRule> &result)
{
    std::vector<char> rhs_symbols = extractSymbols(rule.rhs);
    
    for (char symbol : rhs_symbols)
    {
        // Check if symbol is negated
        bool is_negated = false;
        for (const TokenBlock &block : rule.rhs)
        {
            for (size_t i = 0; i < block.size(); ++i)
            {
                if (block[i].type == symbol && i > 0 && block[i - 1].type == '!')
                {
                    is_negated = true;
                    break;
                }
            }
            if (is_negated) break;
        }
        
        result.emplace_back(rule.lhs, symbol, is_negated, origin);
    }
}

std::vector<BasicRule> LogicRule::deduceBasics() const
{
    std::vector<BasicRule> result;
    std::queue<std::pair<LogicRule, const LogicRule*>> to_process;
    
    // Step 1: Expand equivalences first
    std::vector<LogicRule> after_equiv = expandEquivalence(*this);
    for (const LogicRule &rule : after_equiv)
        to_process.push({rule, this});
    
    // Step 2: Iteratively simplify until all rules have atomic RHS
    while (!to_process.empty())
    {
        auto [current, origin] = to_process.front();
        to_process.pop();
        
        if (!hasOrXor(current.rhs))
        {
            // RHS is atomic: extract basic rules
            extractBasicRules(current, origin, result);
        }
        else
        {
            // RHS has OR/XOR: split further
            std::vector<LogicRule> expanded = expandOrInRhs(current);
            for (const LogicRule &rule : expanded)
            {
                to_process.push({rule, origin});
            }
        }
    }
    
    return result;
}
