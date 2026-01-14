#include "Resolver.hpp"
#include <iostream>
#include <stdexcept>

static rhr_value_e triNot(rhr_value_e v)
{
    if (v == R_TRUE)
        return R_FALSE;
    if (v == R_FALSE)
        return R_TRUE;
    return R_AMBIGOUS;
}

static rhr_value_e triAnd(rhr_value_e a, rhr_value_e b)
{
    if (a == R_FALSE || b == R_FALSE)
        return R_FALSE;
    if (a == R_TRUE && b == R_TRUE)
        return R_TRUE;
    return R_AMBIGOUS;
}

static rhr_value_e triOr(rhr_value_e a, rhr_value_e b)
{
    if (a == R_TRUE || b == R_TRUE)
        return R_TRUE;
    if (a == R_FALSE && b == R_FALSE)
        return R_FALSE;
    return R_AMBIGOUS;
}

static rhr_value_e triXor(rhr_value_e a, rhr_value_e b)
{
    if (a == R_AMBIGOUS || b == R_AMBIGOUS)
        return R_AMBIGOUS;
    return (a != b) ? R_TRUE : R_FALSE;
}

Resolver::Resolver(std::set<char> querie, std::vector<BasicRule> &basic_rules, std::set<char> initial_facts)
    : querie(querie),
      basic_rules(basic_rules),
      initial_facts(initial_facts)
{
}

Resolver::~Resolver()
{
}

void Resolver::resolveLeft(std::vector<TokenBlock> &fact)
{
    unsigned int priority = 0;
    for (TokenBlock &token_block : fact)
    {
        if (token_block.getPriority() > priority)
            priority = token_block.getPriority();
    }
    for (size_t i = 0; i < fact.size(); i++)
    {
        if (fact[i].getPriority() == priority)
        {
            fact[i].execute();
            if (i != 0)
            {
                fact[i - 1].push_back(fact[i][0]);
                fact.erase(fact.begin() + i);
            }
            else
            {
                // If this was the first block and there's a following block,
                // prepend result to it so operators there have a left operand.
                if (fact.size() > 1)
                {
                    fact[1].insert(fact[1].begin(), fact[i][0]);
                    fact.erase(fact.begin());
                    // Index i now points to what was fact[1]; no need to adjust
                }
                else
                {
                    fact[i].setPriority(0);
                }
            }
        }
    }
    if (fact.size() > 1)
        resolveLeft(fact);
    else if (fact.size() == 1 && fact[0].size() > 1)
    {
        // Final reduction at base priority
        fact[0].setPriority(0);
        fact[0].execute();
    }
}

rhr_value_e Resolver::prove(char q, bool negated_context)
{
    std::unordered_map<char, rhr_value_e>::iterator memoIt = memo.find(q);
    if (memoIt != memo.end())
    {
        recordMemoHit(q, memoIt->second);
        return memoIt->second;
    }

    auto initiaTrueIt = initial_facts.find(q);
    if (initiaTrueIt != initial_facts.end())
    {
        recordInitialFact(q);
        return memo[q] = R_TRUE;
    }

    std::unordered_map<char, bool>::iterator visitingIt = visiting.find(q);
    if (visitingIt != visiting.end())
    {
        if (visitingIt->second != negated_context)
            return R_AMBIGOUS;
        return R_FALSE;
    }
    visiting[q] = negated_context;

    bool definite_true = false;
    bool definite_false = false;
    bool possible_true = false;
    bool possible_false = false;
    
    for (const BasicRule &rule : basic_rules)
    {
        if (rule.rhs_symbol == q)
        {
            std::vector<TokenBlock> lhs = rule.lhs;
            rhr_value_e lhs_result = evalLHS(lhs);
            recordRuleConsidered(q, &rule, lhs_result == R_TRUE);
            
            if (lhs_result == R_TRUE)
            {
                if (!rule.rhs_negated)
                    definite_true = true;
                else
                    definite_false = true;
            }
            else if (lhs_result == R_AMBIGOUS)
            {
                if (!rule.rhs_negated)
                    possible_true = true;
                else
                    possible_false = true;
            }
        }
    }
    
    possible_true = possible_true || definite_true;
    possible_false = possible_false || definite_false;

    visiting.erase(q);

    if (possible_true && possible_false)
        return memo[q] = R_AMBIGOUS;
    if (definite_true)
        return memo[q] = R_TRUE;
    if (definite_false)
        return memo[q] = R_FALSE;
    if (possible_true || possible_false)
        return memo[q] = R_AMBIGOUS;
    return memo[q] = R_FALSE;
}

void Resolver::recordInitialFact(char q)
{
    current_trace.push_back({
        std::string("Initial fact: ") + q + " is given as true",
        q, nullptr, R_TRUE, false
    });
}

void Resolver::recordRuleConsidered(char q, const BasicRule* rule, bool lhs_fired)
{
    if (!lhs_fired)
        return;
    
    std::string desc = "Rule: " + rule->toString();
    desc += " shows ";
    desc += std::string(1, q);
    desc += rule->rhs_negated ? " false" : " true";
    
    // Add origin information if the rule was deduced
    if (rule->origin)
    {
        desc += " (deduced from rule: " + rule->origin->toString() + ")";
    }
    
    rhr_value_e concl = rule->rhs_negated ? R_FALSE : R_TRUE;
    current_trace.push_back({desc, q, rule, concl, true});
}

void Resolver::recordMemoHit(char q, rhr_value_e val)
{
    std::string valStr = (val == R_TRUE ? "true" : val == R_FALSE ? "false" : "ambiguous");
    current_trace.push_back({
        "Already resolved: " + std::string(1, q) + " = " + valStr,
        q, nullptr, val, false
    });
}

void Resolver::printTrace(char q, rhr_value_e result)
{
    std::cout << "=== Reasoning for " << q << " ===\n";
    
    if (current_trace.empty())
    {
        std::cout << "No assertion proves " << q << ", false by default." << std::endl;
        return;
    }
    for (const auto &step : current_trace)
    {
        if (step.symbol == q || step.lhs_fired)
            std::cout << "  " << step.description << "\n";
    }
    
    std::string resultStr = (result == R_TRUE ? "true" : result == R_FALSE ? "false" : "ambiguous");
    std::cout << "Conclusion: " << q << " is " << resultStr << "\n";
}

rhr_value_e Resolver::getTokenValue(TriToken &token, bool negated_context)
{
    if (token.has_value)
        return token.value;
    if (token.type >= 'A' && token.type <= 'Z')
    {
        token.value = prove(token.type, negated_context);
        token.has_value = true;
        return token.value;
    }
    throw std::logic_error("Token value requested for non-value token");
}

void Resolver::executeNotTri(std::vector<TriToken> &tokens)
{
    size_t i = 0;
    while (i < tokens.size())
    {
        if (tokens[i].type == '!')
        {
            if (i + 1 == tokens.size())
                throw std::logic_error("operator ! has no var attached\n");
            TriToken &next = tokens[i + 1];
            rhr_value_e val = getTokenValue(next, true);
            next.value = triNot(val);
            next.has_value = true;
            next.type = 0;
            tokens.erase(tokens.begin() + i);
            if (i > 0)
                --i;
        }
        else
            ++i;
    }
}

void Resolver::executeOthersTri(std::vector<TriToken> &tokens, char op_target)
{
    size_t i = 0;
    while (i < tokens.size())
    {
        if (tokens[i].type == op_target)
        {
            if (i == 0 || i + 1 == tokens.size())
                throw std::logic_error(std::string("operator ") + op_target + " has no var attached\n");
            TriToken &left = tokens[i - 1];
            TriToken &right = tokens[i + 1];
            rhr_value_e lval = getTokenValue(left, false);
            rhr_value_e rval = getTokenValue(right, false);
            rhr_value_e res = R_FALSE;
            if (op_target == '+')
                res = triAnd(lval, rval);
            else if (op_target == '|')
                res = triOr(lval, rval);
            else if (op_target == '^')
                res = triXor(lval, rval);
            tokens[i].type = 0;
            tokens[i].value = res;
            tokens[i].has_value = true;
            tokens.erase(tokens.begin() + i + 1);
            tokens.erase(tokens.begin() + i - 1);
            if (i > 0)
                --i;
        }
        else
            ++i;
    }
}

rhr_value_e Resolver::executeTriBlock(std::vector<TriToken> &tokens)
{
    if (tokens.empty())
        throw std::logic_error("TriBlock::execute: empty block");
    executeNotTri(tokens);
    executeOthersTri(tokens, '^');
    executeOthersTri(tokens, '|');
    executeOthersTri(tokens, '+');
    if (tokens.size() != 1)
        throw std::logic_error("TriBlock::execute: reduction did not converge");
    return getTokenValue(tokens[0], false);
}

rhr_value_e Resolver::resolveLeftTri(std::vector<TriBlock> &blocks)
{
    if (blocks.empty())
        throw std::logic_error("resolveLeftTri: empty expression");
    while (true)
    {
        unsigned int max_priority = 0;
        for (const TriBlock &block : blocks)
            if (block.priority > max_priority)
                max_priority = block.priority;

        for (size_t i = 0; i < blocks.size();)
        {
            if (blocks[i].priority == max_priority)
            {
                executeTriBlock(blocks[i].tokens);
                TriToken result = blocks[i].tokens[0];
                result.type = 0;
                result.has_value = true;
                if (i != 0)
                {
                    blocks[i - 1].tokens.push_back(result);
                    blocks.erase(blocks.begin() + i);
                    continue;
                }
                if (blocks.size() > 1)
                {
                    blocks[1].tokens.insert(blocks[1].tokens.begin(), result);
                    blocks.erase(blocks.begin());
                    continue;
                }
                blocks[i].priority = 0;
            }
            ++i;
        }
        if (blocks.size() == 1)
        {
            if (blocks[0].tokens.size() > 1)
                executeTriBlock(blocks[0].tokens);
            return getTokenValue(blocks[0].tokens[0], false);
        }
    }
}

rhr_value_e Resolver::evalLHS(std::vector<TokenBlock> lhs)
{
    std::vector<TriBlock> blocks;
    blocks.reserve(lhs.size());
    for (const TokenBlock &block : lhs)
    {
        TriBlock tri_block;
        tri_block.priority = block.getPriority();
        tri_block.tokens.reserve(block.size());
        for (const TokenEffect &tk : block)
        {
            TriToken tri_token;
            tri_token.type = tk.type;
            tri_token.value = R_FALSE;
            tri_token.has_value = false;
            tri_block.tokens.push_back(tri_token);
        }
        blocks.push_back(tri_block);
    }
    return resolveLeftTri(blocks);
}

void Resolver::printInitialFact()
{
    if (!initial_facts.empty())
    {
        std::cout << "Initial facts: ";
        bool first = true;
        for (char fact : initial_facts)
        {
            if (!first) std::cout << ", ";
            std::cout << fact;
            first = false;
        }
        std::cout << "\n";
    }
}

void Resolver::resolveQuerie(bool print_trace)
{
    if (print_trace)
        printInitialFact();
    for (auto &q : querie)
    {
        visiting.clear();
        current_trace.clear();
        rhr_value_e res = prove(q, false);
        if (print_trace)
            printTrace(q, res);
        else
        {
            std::string resultStr = (res == R_TRUE ? "true" : res == R_FALSE ? "false" : "ambiguous");
            std::cout << q << " = " << resultStr << std::endl;
        }
    }
}

void Resolver::changeFacts(const std::set<char> &new_facts)
{
    initial_facts = new_facts;
    memo.clear();
    visiting.clear();
    current_trace.clear();

    // Reset token effects in all rules
    for (BasicRule &rule : basic_rules)
    {
        for (TokenBlock &block : rule.lhs)
        {
            for (TokenEffect &tk : block)
            {
                if (tk.type >= 'A' && tk.type <= 'Z')
                    tk.effect = (initial_facts.find(tk.type) != initial_facts.end());
            }
        }
    }
}
