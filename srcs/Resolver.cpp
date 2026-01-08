#include "Resolver.hpp"
#include <iostream>

Resolver::Resolver(std::set<char> querie, std::vector<BasicRule> &basic_rules, std::set<char> initial_facts)
    : querie(querie), basic_rules(basic_rules), initial_facts(initial_facts)
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

rhr_value_e Resolver::prove(char q)
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

    if (visiting.count(q))
        return R_FALSE;
    visiting.insert(q);

    bool isFactTrue = false;
    bool isFactFalse = false;
    
    for (const BasicRule &rule : basic_rules)
    {
        if (rule.rhs_symbol == q)
        {
            std::vector<TokenBlock> lhs = rule.lhs;
            bool fired = evalLHS(lhs);
            recordRuleConsidered(q, &rule, fired);
            
            if (fired)
            {
                if (!rule.rhs_negated)
                    isFactTrue = true;
                else
                    isFactFalse = true;
                    
                if (isFactTrue && isFactFalse)
                {
                    visiting.erase(q);
                    return memo[q] = R_AMBIGOUS;
                }
            }
        }
    }
    
    visiting.erase(q);
    return isFactTrue ? memo[q] = R_TRUE : memo[q] = R_FALSE;
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

bool Resolver::evalLHS(std::vector<TokenBlock> lhs)
{
    for (TokenBlock &tokenBlock : lhs)
    {
        for (TokenEffect &token : tokenBlock)
        {
            if (token.type >= 'A' && token.type <= 'Z')
            {
                if (token.effect != true)
                    token.effect = (prove(token.type) == R_TRUE);
            }
        }
    }
    resolveLeft(lhs);
    return lhs.size() == 1 && lhs[0].size() == 1 && lhs[0][0].effect;
}

void Resolver::resolveQuerie(bool print_trace)
{
    // Display initial facts
    if (!initial_facts.empty() && print_trace)
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
    for (auto &q : querie)
    {
        visiting.clear();
        current_trace.clear();
        rhr_value_e res = prove(q);
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
