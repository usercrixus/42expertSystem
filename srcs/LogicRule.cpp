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

static bool needsParenthesesForNegation(const std::vector<TokenBlock> &blocks)
{
    // Multiple blocks always need parentheses
    if (blocks.size() > 1)
        return true;

    if (blocks.size() == 1)
        return blocks[0].hasAnyOperator({'+', '|', '^'});

    return false;
}

static std::vector<TokenBlock> negateBlocks(const std::vector<TokenBlock> &blocks)
{
    std::vector<TokenBlock> negated;

    bool any_non_empty = false;
    for (const auto &b : blocks)
        if (!b.empty())
            { any_non_empty = true; break; }
    if (!any_non_empty)
        return negated;
    
    // Check for double negation: single block starting with !
    if (blocks.size() == 1 && !blocks[0].empty() && blocks[0][0].type == '!')
    {
        TokenBlock simplified = blocks[0].extractRange(1, SIZE_MAX, blocks[0].getPriority());
        if (!simplified.empty())
            negated.push_back(simplified);
        return negated;
    }

    bool needed = needsParenthesesForNegation(blocks);
    if (needed)
    {
        // Add negation operator at base priority
        negated.push_back(TokenBlock(0, '!'));
        
        // Find the minimum priority in the blocks
        unsigned int min_priority = UINT_MAX;
        for (const TokenBlock &block : blocks)
        {
            if (!block.empty() && block.getPriority() < min_priority)
                min_priority = block.getPriority();
        }
        
        // Add the blocks with increased priority (parentheses)
        for (const TokenBlock &block : blocks)
        {
            if (block.empty())
                continue;
            unsigned int new_priority = block.getPriority() - min_priority + 1;
            TokenBlock paren_block = block.withPriority(new_priority);
            negated.push_back(paren_block);
        }
    }
    else // single symbol or already has parentheses
    {
        TokenBlock neg_block(0, '!');

        for (const TokenBlock &block : blocks)
            if (!block.empty())
                neg_block.appendTokens(block);
        
        if (!neg_block.empty())
            negated.push_back(neg_block);
    }
    
    return negated;
}

static void parenthesizeBlocks(std::vector<TokenBlock> &blocks)
{
    for (TokenBlock &blk : blocks)
    {
        if (!blk.empty())
            blk = blk.withPriority(blk.getPriority() + 1);
    }
}

static void appendNegatedToLhs(std::vector<TokenBlock> &lhs, const std::vector<TokenBlock> &to_negate)
{
    if (to_negate.empty())
        return;
    
    std::vector<TokenBlock> negated = negateBlocks(to_negate);
    if (negated.empty())
        return;

    bool lhs_simple = lhs.size() == 1 && lhs[0].hasAnyOperator({'+', '|', '^'}) == false;
    if (!lhs.empty() && !lhs_simple)
        parenthesizeBlocks(lhs);
    TokenBlock and_block(0, '+');
    if (lhs_simple)
        lhs[0].appendTokens(and_block);
    if (negated[0].getPriority() == 0)
    {
        if (lhs_simple)
            lhs[0].appendTokens(negated[0]);
        else {
            and_block.appendTokens(negated[0]);
            lhs.push_back(and_block);
        }
        lhs.insert(lhs.end(), negated.begin() + 1, negated.end());
    } else {
        if (!lhs_simple)
            lhs.push_back(and_block);
        lhs.insert(lhs.end(), negated.begin(), negated.end());
    }
}

// apply De Morgan's law: !(A+B) = !A|!B and !(A|B) = !A+!B
static std::vector<LogicRule> applyDeMorgan(const LogicRule &rule)
{
    std::vector<LogicRule> res;
    
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
        res.push_back(rule);
        return res;
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
        TokenBlock prefix_block = neg_block.extractRange(0, neg_token_index, base_priority);
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
    
    res.emplace_back(rule.arrow, rule.lhs, transformed_rhs);
    return res;
}

static std::vector<LogicRule> expandEquivalence(const LogicRule &rule)
{
    std::vector<LogicRule> expanded;
    
    if (rule.arrow.type == '=')
    {
        // A <=> B becomes: A => B and B => A
        expanded.emplace_back(TokenEffect('>'), rule.lhs, rule.rhs);
        expanded.emplace_back(TokenEffect('>'), rule.rhs, rule.lhs);
    }
    else
    {
        expanded.push_back(rule);
    }
    
    return expanded;
}

static int getOperatorPriority(char op)
{
    if (op == '^') return 0;  // XOR: lowest priority
    if (op == '|') return 1;  // OR: medium priority
    if (op == '+') return 2;  // AND: highest priority
    return 3;                  // non-operator
}

// [A, |, B, +, C] at p0 -> [A, |] at p0 and [B, +, C] at p1
static std::vector<TokenBlock> normalizeBlocksByOperatorPriority(const std::vector<TokenBlock> &blocks)
{
    std::vector<TokenBlock> result;
    
    for (const TokenBlock &block : blocks)
    {
        if (block.empty())
            continue;
        
        // Single operator or symbol - keep as is
        if (block.size() <= 1)
        {
            result.push_back(block);
            continue;
        }
        
        // Check if block has multiple operator types at base priority
        std::set<char> operators_in_block;
        for (const TokenEffect &tk : block)
        {
            if (tk.type == '+' || tk.type == '|' || tk.type == '^')
                operators_in_block.insert(tk.type);
        }
        
        // If only one operator type or no operators, keep block as is
        if (operators_in_block.size() <= 1)
        {
            result.push_back(block);
            continue;
        }
        
        // Multiple operator types found - split by lowest-priority operator first
        // Find the lowest-priority operator in this block
        char split_operator = 0;
        int lowest_priority = INT_MAX;
        size_t split_index = 0;
        
        for (size_t i = 0; i < block.size(); ++i)
        {
            int op_priority = getOperatorPriority(block[i].type);
            if (op_priority < lowest_priority)
            {
                lowest_priority = op_priority;
                split_operator = block[i].type;
                split_index = i;
            }
        }
        
        if (split_operator == 0)
        {
            // No operators found (shouldn't happen)
            result.push_back(block);
            continue;
        }
        
        // Split at the lowest-priority operator
        // Left part stays at current priority
        TokenBlock left_part = block.extractRange(0, split_index, block.getPriority());
        if (!left_part.empty())
            result.push_back(left_part);
        
        // Operator itself stays at current priority
        result.push_back(TokenBlock(block.getPriority(), split_operator));
        
        // Right part moves to higher priority (parentheses)
        TokenBlock right_part = block.extractRange(split_index + 1, SIZE_MAX, block.getPriority() + 1);
        if (!right_part.empty())
        {
            // Recursively normalize the right part (it may contain more mixed operators)
            std::vector<TokenBlock> normalized_right = normalizeBlocksByOperatorPriority({right_part});
            result.insert(result.end(), normalized_right.begin(), normalized_right.end());
        }
    }
    
    return result;
}

// A => B + C becomes A => B and A => C
static std::vector<LogicRule> splitByAndAtLowestPriority(const LogicRule &rule, unsigned int min_priority)
{
    std::vector<LogicRule> splits;
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
            splits.emplace_back(TokenEffect('>'), rule.lhs, sub_rhs);
    
    return splits;
}

// A => B | C becomes: A + !B => C and A + !C => B
static std::vector<LogicRule> expandOrOperator(const LogicRule &rule, size_t block_index, size_t token_index)
{
    std::vector<LogicRule> or_rules;
    const TokenBlock &block = rule.rhs[block_index];
    
    // Extract left operand (all tokens before OR)
    std::vector<TokenBlock> left_rhs;
    left_rhs.insert(left_rhs.end(), rule.rhs.begin(), rule.rhs.begin() + block_index);
    
    TokenBlock left_block = block.extractRange(0, token_index, block.getPriority());
    if (!left_block.empty())
        left_rhs.push_back(left_block);
    
    // Extract right operand (all tokens after OR)
    TokenBlock right_block = block.extractRange(token_index + 1, SIZE_MAX, block.getPriority());
    
    std::vector<TokenBlock> right_rhs;
    if (!right_block.empty())
        right_rhs.push_back(right_block);
    right_rhs.insert(right_rhs.end(), rule.rhs.begin() + block_index + 1, rule.rhs.end());
    
    // Create rule 1: A + !left => right
    std::vector<TokenBlock> new_lhs_1 = rule.lhs; // existing LHS
    if (!left_rhs.empty())
        appendNegatedToLhs(new_lhs_1, left_rhs);
    or_rules.emplace_back(TokenEffect('>'), new_lhs_1, right_rhs);
    
    // Create rule 2: A + !right => left
    std::vector<TokenBlock> new_lhs_2 = rule.lhs;
    if (!right_rhs.empty())
        appendNegatedToLhs(new_lhs_2, right_rhs);
    or_rules.emplace_back(TokenEffect('>'), new_lhs_2, left_rhs);
    
    return or_rules;
}

// A => B ^ C becomes:
//   A + !B => C
//   A + !C => B
//   A => !(B + C)  (constraint: negation will be further expanded by De Morgan)
static std::vector<LogicRule> expandXorOperator(const LogicRule &rule, size_t block_index, size_t token_index)
{
    std::vector<LogicRule> xor_rules;
    const TokenBlock &block = rule.rhs[block_index];
    
    // Extract left operand
    std::vector<TokenBlock> left_rhs;
    left_rhs.insert(left_rhs.end(), rule.rhs.begin(), rule.rhs.begin() + block_index);

    TokenBlock left_block = block.extractRange(0, token_index, block.getPriority());
    if (!left_block.empty())
        left_rhs.push_back(left_block);
    
    // Extract right operand
    TokenBlock right_block = block.extractRange(token_index + 1, block.size(), block.getPriority());
    
    std::vector<TokenBlock> right_rhs;
    if (!right_block.empty())
        right_rhs.push_back(right_block);
    right_rhs.insert(right_rhs.end(), rule.rhs.begin() + block_index + 1, rule.rhs.end());
    
    // Rule 1: A + !left => right
    std::vector<TokenBlock> new_lhs_1 = rule.lhs;
    if (!left_rhs.empty())
        appendNegatedToLhs(new_lhs_1, left_rhs);
    xor_rules.emplace_back(TokenEffect('>'), new_lhs_1, right_rhs);
    
    // Rule 2: A + !right => left
    std::vector<TokenBlock> new_lhs_2 = rule.lhs;
    if (!right_block.empty())
        appendNegatedToLhs(new_lhs_2, right_rhs);
    xor_rules.emplace_back(TokenEffect('>'), new_lhs_2, left_rhs);
    
    // Rule 3: A => !(left + right)
    std::vector<TokenBlock> constraint_rhs;
    for (const TokenBlock &blk : left_rhs)
        constraint_rhs.push_back(blk);
    constraint_rhs.push_back(TokenBlock(block.getPriority(), '+'));
    for (const TokenBlock &blk : right_rhs)
        constraint_rhs.push_back(blk);
    
    std::vector<TokenBlock> negated_constraint = negateBlocks(constraint_rhs);
    
    xor_rules.emplace_back(TokenEffect('>'), rule.lhs, negated_constraint);
    
    return xor_rules;
}

static std::vector<LogicRule> expandRhs(const LogicRule &rule)
{
    std::vector<LogicRule> result;

    LogicRule normalized_rule = rule;
    normalized_rule.rhs = normalizeBlocksByOperatorPriority(rule.rhs);
    
    // Find lowest priority block
    unsigned int min_priority = UINT_MAX;
    for (const TokenBlock &block : normalized_rule.rhs)
    {
        unsigned int p = block.getPriority();
        if (p < min_priority)
            min_priority = p;
    }
    
    // check AND operators (+)
    bool has_and_at_lowest = false;
    for (const TokenBlock &block : normalized_rule.rhs)
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
        return splitByAndAtLowestPriority(normalized_rule, min_priority);
    
    // Look for OR or XOR and expand them
    for (size_t i = 0; i < normalized_rule.rhs.size(); ++i)
    {
        const TokenBlock &block = normalized_rule.rhs[i];
        for (size_t j = 0; j < block.size(); ++j)
        {
            if (block[j].type == '|')
                return expandOrOperator(normalized_rule, i, j);
            else if (block[j].type == '^')
                return expandXorOperator(normalized_rule, i, j);
        }
    }
    result.push_back(normalized_rule);
    return result;
}

static void extractBasicRules(const LogicRule &rule, const LogicRule *origin, std::vector<BasicRule> &basics)
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
                    basics.emplace_back(rule.lhs, symbol, is_negated, origin);
                }
            }
        }
    }
}

std::vector<BasicRule> LogicRule::deduceBasics() const
{
    std::vector<BasicRule> basics;
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
            extractBasicRules(current, origin, basics);
        else
        {
            std::vector<LogicRule> expanded = expandRhs(current);
            for (const LogicRule &rule : expanded)
            {
                to_process.push({rule, origin});
            }
        }
    }

    if (basics.size() == 1) {
        // We don't want "deduced from itself" for single basic rules
        basics[0].origin = nullptr;
    }
    
    return basics;
}
