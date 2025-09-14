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

void Resolver::reduceRight(std::vector<TokenBlock> &fact)
{
}

void Resolver::resolve(std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> facts)
{
	for (std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>> &fact : facts)
	{
		resolveLeft(std::get<1>(fact));
		reduceRight(std::get<2>(fact));
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