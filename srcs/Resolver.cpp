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
                fact[i].setPriority(0);
        }
    }
    if (fact.size() > 1)
        resolveLeft(fact);
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
    std::string desc = "Rule: " + rule->toString();
    if (lhs_fired)
        desc += " (LHS evaluated to true)";
    else
        desc += " (LHS evaluated to false, rule not applied)";
    
    rhr_value_e concl = lhs_fired ? (rule->rhs_negated ? R_FALSE : R_TRUE) : R_AMBIGOUS;
    current_trace.push_back({desc, q, rule, concl, lhs_fired});
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
    std::cout << "\n=== Reasoning for " << q << " ===\n";
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

void Resolver::resolveQuerie()
{
    for (auto &q : querie)
    {
        visiting.clear();
        current_trace.clear();
        rhr_value_e res = prove(q);
        printTrace(q, res);
    }
}
