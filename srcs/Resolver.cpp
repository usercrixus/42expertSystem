#include "Resolver.hpp"
#include <iostream>

Resolver::Resolver(/* args */)
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
				fact[i].setPriority(0);
			}
		}
	}
	if (fact.size() > 1)
		resolveLeft(fact);
}

rhr_status_e Resolver::getStatus(char q, std::vector<TokenBlock> &fact)
{
	// Determine how `q` appears in the RHS expression represented by TokenBlocks.
	// Return NOT if `!q` appears, AMBIGOUS if `q` is involved in an OR or XOR,
	// otherwise TRUE if `q` appears positively. If `q` is absent, default to AMBIGOUS.
	bool found_q_pos = false;
	bool found_q_neg = false;
	bool ambiguous = false;

	if (fact.empty())
		return AMBIGOUS;

	std::vector<bool> block_has_q_pos(fact.size(), false);
	std::vector<bool> block_has_q_neg(fact.size(), false);
	std::vector<bool> block_has_or_xor(fact.size(), false);

	for (size_t i = 0; i < fact.size(); ++i)
	{
		TokenBlock &tb = fact[i];
		for (size_t j = 0; j < tb.size(); ++j)
		{
			char t = tb[j].type;
			if (t == '|' || t == '^')
				block_has_or_xor[i] = true;
			if (t == q)
			{
				bool is_neg = (j > 0 && tb[j - 1].type == '!');
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
		// If q is in this block and this block has OR/XOR, it's ambiguous.
		if (block_has_or_xor[i] && (block_has_q_pos[i] || block_has_q_neg[i]))
			ambiguous = true;
	}

	// Detect cross-block OR/XOR connecting to a block containing q
	for (size_t i = 0; i < fact.size(); ++i)
	{
		if (!block_has_or_xor[i])
			continue;
		if ((i > 0 && (block_has_q_pos[i - 1] || block_has_q_neg[i - 1])) ||
			(i + 1 < fact.size() && (block_has_q_pos[i + 1] || block_has_q_neg[i + 1])))
		{
			ambiguous = true;
			break;
		}
	}

	if (ambiguous)
		return AMBIGOUS;
	if (found_q_neg)
		return NOT;
	if (found_q_pos)
		return TRUE;
	return AMBIGOUS;
}

void Resolver::resolveQuerie(std::set<char> querie, std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> &facts)
{
    for (auto &q : querie)
    {
		rhr_value_e higher_value = R_FALSE;
        for (auto &fact : facts)
        {
            // Skip rules whose RHS does not reference q (or !q)
            bool rhs_mentions_q = false;
            for (auto &blk : std::get<2>(fact))
            {
                for (size_t i = 0; i < blk.size(); ++i)
                {
                    if (blk[i].type == q)
                    { rhs_mentions_q = true; break; }
                    if (blk[i].type == '!' && i + 1 < blk.size() && blk[i + 1].type == q)
                    { rhs_mentions_q = true; break; }
                }
                if (rhs_mentions_q) break;
            }
            if (!rhs_mentions_q)
                continue;

            rhr_status_e status = getStatus(q, std::get<2>(fact));
            if ((status == TRUE && std::get<1>(fact).at(0).at(0).effect == true) ||
                (status == NOT && std::get<1>(fact).at(0).at(0).effect == false))
            {
				higher_value = R_TRUE;
                break;
            }
            else if (status == AMBIGOUS)
				higher_value = R_AMBIGOUS;
        }
		if (higher_value == R_TRUE)
            std::cout << q << " is " << "true" << std::endl;
		else if (higher_value == R_AMBIGOUS)
            std::cout << q << " is " << "ambigous" << std::endl;
        else
            std::cout << q << " is " << "false" << std::endl;
    }
}

void Resolver::resolve(std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> &facts)
{
	for (std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>> &fact : facts)
	{
		resolveLeft(std::get<1>(fact));
		printFact(fact);
	}
}

void Resolver::printFact(std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>> &fact)
{
	std::cout << (std::get<1>(fact)).at(0).at(0).effect << " ";
	std::cout << std::get<0>(fact).type << " ";
	for (auto &v : std::get<2>(fact))
	{
		for (auto &t : v)
		{
			std::cout << t.type;
		}
	}
	std::cout << std::endl;
}
