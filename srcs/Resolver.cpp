#include "Resolver.hpp"
#include <iostream>

Resolver::Resolver(std::set<char> querie, std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> &facts, std::set<char> initial_facts)
    : querie(querie), facts(facts), initial_facts(initial_facts)
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

bool Resolver::isInterBlockAmbiguity(const std::vector<TokenBlock> &fact, std::vector<bool> &block_has_q_pos, std::vector<bool> &block_has_q_neg, std::vector<bool> &block_has_or_xor)
{
    for (size_t i = 0; i < fact.size(); ++i)
    {
        if (block_has_or_xor[i])
        {
            if ((i > 0 && (block_has_q_pos[i - 1] || block_has_q_neg[i - 1])) ||
                (i + 1 < fact.size() && (block_has_q_pos[i + 1] || block_has_q_neg[i + 1])))
            {
                return true;
            }
        }
    }
    return false;
}

rhr_status_e Resolver::innerBlockAmbiguity(char q, const std::vector<TokenBlock> &fact, std::vector<bool> &block_has_q_pos, std::vector<bool> &block_has_q_neg, std::vector<bool> &block_has_or_xor)
{
    bool found_q_pos = false;
    bool found_q_neg = false;
    for (size_t i = 0; i < fact.size(); ++i)
    {
        const TokenBlock &tokenBlock = fact[i];
        for (size_t j = 0; j < tokenBlock.size(); ++j)
        {
            char token = tokenBlock[j].type;
            if (token == '|' || token == '^')
                block_has_or_xor[i] = true;
            else if (token == q)
            {
                bool is_neg = (j > 0 && tokenBlock[j - 1].type == '!');
                if (is_neg)
                {
                    found_q_neg = true;
                    block_has_q_neg[i] = true;
                }
                else
                {
                    found_q_pos = true;
                    block_has_q_pos[i] = true;
                }
            }
        }
        if (block_has_or_xor[i] && (block_has_q_pos[i] || block_has_q_neg[i]))
            return AMBIGOUS;
    }
    if (found_q_neg && found_q_pos)
        return AMBIGOUS;
    if (found_q_pos)
        return TRUE;
    if (found_q_neg)
        return NOT;
    return AMBIGOUS;
}

rhr_status_e Resolver::getStatus(char q, const std::vector<TokenBlock> &fact)
{
    std::vector<bool> block_has_q_pos(fact.size(), false);
    std::vector<bool> block_has_q_neg(fact.size(), false);
    std::vector<bool> block_has_or_xor(fact.size(), false);
    rhr_status_e status = innerBlockAmbiguity(q, fact, block_has_q_pos, block_has_q_neg, block_has_or_xor);
    if (status == AMBIGOUS)
        return AMBIGOUS;
    if (isInterBlockAmbiguity(fact, block_has_q_pos, block_has_q_neg, block_has_or_xor))
        return AMBIGOUS;
    return status;
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

bool Resolver::isMentionQ(char q, const std::vector<TokenBlock> &rhr)
{
    for (auto &blk : rhr)
    {
        for (size_t i = 0; i < blk.size(); ++i)
        {
            if (blk[i].type == q)
                return true;
        }
    }
    return false;
}

rhr_value_e Resolver::prove(char q)
{
    std::unordered_map<char, rhr_value_e>::iterator memoIt = memo.find(q);
    if (memoIt != memo.end())
        return memoIt->second;

    auto initiaTrueIt = initial_facts.find(q);
    if (initiaTrueIt != initial_facts.end())
        return memo[q] = R_TRUE;

    if (visiting.count(q))
        return R_FALSE;
    visiting.insert(q);

    bool isFactTrue = false;
    bool isFactFalse = false;

    for (auto &fact : facts)
    {
        if (isMentionQ(q, std::get<2>(fact)))
        {
            rhr_status_e status = getStatus(q, std::get<2>(fact));
            if (status != AMBIGOUS)
            {
                std::vector<TokenBlock> lhs = std::get<1>(fact);
                if (evalLHS(lhs))
                {
                    if (status == TRUE)
                        isFactTrue = true;
                    else if (status == NOT)
                        isFactFalse = true;
                }
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

void Resolver::resolveQuerie()
{
    for (auto &q : querie)
    {
        visiting.clear();
        rhr_value_e res = prove(q);
        if (res == R_TRUE)
            std::cout << q << " is true" << std::endl;
        else if (res == R_AMBIGOUS)
            std::cout << q << " is ambigous" << std::endl;
        else
            std::cout << q << " is false" << std::endl;
    }
}
