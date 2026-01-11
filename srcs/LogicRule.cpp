#include "LogicRule.hpp"
#include <sstream>
#include <ostream>
#include <queue>
#include <climits>
#include <set>

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

static bool hasNegatedParentheses(const std::vector<TokenBlock> &rhs)
{
    for (size_t i = 0; i < rhs.size(); ++i)
    {
        const TokenBlock &block = rhs[i];
        for (size_t j = 0; j < block.size(); ++j)
        {
            if (block[j].type == '!')
            {
                // Check if negation is at end of this block and followed by higher-priority blocks (parentheses)
                if (j + 1 >= block.size() && i + 1 < rhs.size() && rhs[i + 1].getPriority() > block.getPriority())
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// apply De Morgan's law: !(A+B) = !A|!B and !(A|B) = !A+!B
static std::vector<LogicRule> applyDeMorgan(const LogicRule &rule)
{
    std::vector<LogicRule> result;
    
    // Find negation operator followed by higher-priority blocks (parentheses)
    int neg_block_index = -1;
    size_t neg_token_index = 0;
    
    for (size_t i = 0; i < rule.rhs.size(); ++i)
    {
        const TokenBlock &block = rule.rhs[i];
        for (size_t j = 0; j < block.size(); ++j)
        {
            if (block[j].type == '!' && j + 1 >= block.size() && 
                i + 1 < rule.rhs.size() && rule.rhs[i + 1].getPriority() > block.getPriority())
            {
                neg_block_index = i;
                neg_token_index = j;
                break;
            }
        }
        if (neg_block_index >= 0) break;
    }
    
    if (neg_block_index < 0)
    {
        result.push_back(rule);
        return result;
    }
    
    // Extract the negated expression (higher-priority blocks after negation)
    const TokenBlock &neg_block = rule.rhs[neg_block_index];
    unsigned int base_priority = neg_block.getPriority();
    
    std::vector<TokenBlock> negated_expr;
    for (size_t i = neg_block_index + 1; i < rule.rhs.size(); ++i)
    {
        if (rule.rhs[i].getPriority() > base_priority)
            negated_expr.push_back(rule.rhs[i]);
        else
            break;
    }
    
    // Apply De Morgan's law: flip operators and negate each operand
    std::vector<TokenBlock> transformed_rhs;
    
    // Copy blocks before negation
    for (int i = 0; i < neg_block_index; ++i)
        transformed_rhs.push_back(rule.rhs[i]);
    
    // Copy tokens before the negation operator in the same block
    if (neg_token_index > 0)
    {
        TokenBlock prefix_block(base_priority);
        for (size_t j = 0; j < neg_token_index; ++j)
            prefix_block.push_back(neg_block[j]);
        if (!prefix_block.empty())
            transformed_rhs.push_back(prefix_block);
    }
    
    // Collect all operands and operators from the negated parentheses
    std::vector<TokenEffect> all_tokens;
    for (const TokenBlock &block : negated_expr)
    {
        for (const TokenEffect &tk : block)
        {
            if (tk.type == '+' || tk.type == '|' || tk.type == '^' ||
                (tk.type >= 'A' && tk.type <= 'Z') || tk.type == '!')
                all_tokens.push_back(tk);
        }
    }
    
    std::vector<TokenEffect> transformed_tokens;
    for (size_t i = 0; i < all_tokens.size(); ++i)
    {
        TokenEffect tk = all_tokens[i];
        
        if (tk.type == '+')
        {
            // AND becomes OR
            transformed_tokens.emplace_back(TokenEffect('|'));
        }
        else if (tk.type == '|')
        {
            // OR becomes AND
            transformed_tokens.emplace_back(TokenEffect('+'));
        }
        else if (tk.type == '^')
        {
            // XOR stays XOR
            transformed_tokens.emplace_back(TokenEffect('^'));
        }
        else if (tk.type >= 'A' && tk.type <= 'Z')
        {
            // Negate symbols
            transformed_tokens.emplace_back(TokenEffect('!'));
            transformed_tokens.push_back(tk);
        }
        else if (tk.type == '!')
        {
            // Double negation cancels - skip this and next symbol
            if (i + 1 < all_tokens.size() && 
                all_tokens[i + 1].type >= 'A' && all_tokens[i + 1].type <= 'Z')
            {
                transformed_tokens.push_back(all_tokens[i + 1]);
                i++;
            }
        }
    }
    
    // Rebuild blocks from transformed tokens at base priority
    if (!transformed_tokens.empty())
    {
        TokenBlock result_block(0);  // Base priority for the result
        for (const TokenEffect &tk : transformed_tokens)
            result_block.push_back(tk);
        transformed_rhs.push_back(result_block);
    }
    
    // Copy blocks after negated expression
    size_t end_index = neg_block_index + 1 + negated_expr.size();
    for (size_t i = end_index; i < rule.rhs.size(); ++i)
        transformed_rhs.push_back(rule.rhs[i]);
    
    result.emplace_back(rule.arrow, rule.lhs, transformed_rhs);
    return result;
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
        // Check double negation
        if (!left_block.empty() && left_block[0].type == '!')
        {
            TokenBlock simplified_block(0);
            for (size_t i = 1; i < left_block.size(); ++i)
                simplified_block.push_back(left_block[i]);
            new_lhs_1.push_back(simplified_block);
        } else {
            TokenBlock neg_block(0);
            neg_block.emplace_back(TokenEffect('!'));
            for (const TokenEffect &tk : left_block)
                neg_block.push_back(tk);
            new_lhs_1.push_back(neg_block);
        }
    }
    result.emplace_back(TokenEffect('>'), new_lhs_1, right_rhs);
    
    // Create rule 2: A + !right => left
    std::vector<TokenBlock> new_lhs_2 = rule.lhs;
    if (!right_block.empty())
    {
        new_lhs_2.push_back(TokenBlock(0));
        new_lhs_2.back().emplace_back(TokenEffect('+'));
        // Check double negation
        if (!right_block.empty() && right_block[0].type == '!')
        {
            TokenBlock simplified_block(0);
            for (size_t i = 1; i < right_block.size(); ++i)
                simplified_block.push_back(right_block[i]);
            new_lhs_2.push_back(simplified_block);
        } else  {
            TokenBlock neg_block(0);
            neg_block.emplace_back(TokenEffect('!'));
            for (const TokenEffect &tk : right_block)
                neg_block.push_back(tk);
            new_lhs_2.push_back(neg_block);
        }
    }
    result.emplace_back(TokenEffect('>'), new_lhs_2, left_rhs);
    
    return result;
}

// A => B ^ C becomes:
//   A + !B => C
//   A + !C => B
//   A => !(B + C)  (constraint: negation will be further expanded by De Morgan)
static std::vector<LogicRule> expandXorOperator(const LogicRule &rule, size_t block_index, size_t token_index)
{
    std::vector<LogicRule> result;
    const TokenBlock &block = rule.rhs[block_index];
    
    // Extract left operand
    std::vector<TokenBlock> left_rhs;
    left_rhs.insert(left_rhs.end(), rule.rhs.begin(), rule.rhs.begin() + block_index);
    
    TokenBlock left_block(block.getPriority());
    for (size_t k = 0; k < token_index; ++k)
        left_block.push_back(rule.rhs[block_index][k]);
    if (!left_block.empty())
        left_rhs.push_back(left_block);
    
    // Extract right operand
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
    
    // Rule 3: A => !(left + right)
    std::vector<TokenBlock> constraint_rhs;
    // Add negation operator at base priority
    TokenBlock neg_op(0);
    neg_op.emplace_back(TokenEffect('!'));
    constraint_rhs.push_back(neg_op);
    
    // Add left operand at higher priority (will be parenthesized)
    TokenBlock paren_left = left_block;
    paren_left.setPriority(1);
    constraint_rhs.push_back(paren_left);
    
    // Add AND operator at higher priority (inside parentheses)
    TokenBlock and_op(1);
    and_op.emplace_back(TokenEffect('+'));
    constraint_rhs.push_back(and_op);
    
    // Add right operand at higher priority (will be parenthesized)
    TokenBlock paren_right = right_block;
    paren_right.setPriority(1);
    constraint_rhs.push_back(paren_right);
    
    result.emplace_back(TokenEffect('>'), rule.lhs, constraint_rhs);
    
    return result;
}

static std::vector<LogicRule> expandOrInRhs(const LogicRule &rule)
{
    std::vector<LogicRule> result;
    
    // Find lowest priority block
    unsigned int min_priority = UINT_MAX;
    for (const TokenBlock &block : rule.rhs)
    {
        unsigned int p = block.getPriority();
        if (p < min_priority)
            min_priority = p;
    }
    
    // check AND operators (+)
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
    
    // Look for OR or XOR and expand them
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
    result.push_back(rule);
    return result;
}

static void extractBasicRules(const LogicRule &rule, const LogicRule *origin, std::vector<BasicRule> &result)
{
    std::set<std::pair<char, bool>> processed;
    
    for (const TokenBlock &block : rule.rhs)
    {
        for (size_t i = 0; i < block.size(); ++i)
        {
            char symbol = block[i].type;

            if (symbol >= 'A' && symbol <= 'Z')
            {
                bool is_negated = (i > 0 && block[i - 1].type == '!');
                
                std::pair<char, bool> symbol_pair = {symbol, is_negated};
                
                if (processed.find(symbol_pair) == processed.end())
                {
                    processed.insert(symbol_pair);
                    result.emplace_back(rule.lhs, symbol, is_negated, origin);
                }
            }
        }
    }
}

std::vector<BasicRule> LogicRule::deduceBasics() const
{
    std::vector<BasicRule> result;
    std::queue<std::pair<LogicRule, const LogicRule*>> to_process;

    std::vector<LogicRule> after_equiv = expandEquivalence(*this);
    for (const LogicRule &rule : after_equiv)
        to_process.push({rule, this});

    while (!to_process.empty())
    {
        auto [current, origin] = to_process.front();
        to_process.pop();

        if (hasNegatedParentheses(current.rhs))
        {
            std::vector<LogicRule> after_demorgan = applyDeMorgan(current);
            for (const LogicRule &rule : after_demorgan)
                to_process.push({rule, origin});
            continue;
        }
        
        // check if RHS is only atomic symbols connected by AND
        if (!hasOrXor(current.rhs))
            extractBasicRules(current, origin, result);
        else
        {
            std::vector<LogicRule> expanded = expandOrInRhs(current);
            for (const LogicRule &rule : expanded)
            {
                to_process.push({rule, origin});
            }
        }
    }

    if (result.size() == 1) {
        // We don't want "deduced from itself" for single basic rules
        result[0].origin = nullptr;
    }
    
    return result;
}
