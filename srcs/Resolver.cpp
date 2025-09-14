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
	//TODO you should say if the q value is not (!q) if its ambigous (q | x or q ^ x) or if its true (q) care it can be complexe equation
}

void Resolver::resolveQuerie(std::set<char> querie, std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> &facts)
{
	for (auto &q : querie)
	{
		rhr_value_e higher_value = FALSE;
		for (auto &fact : facts)
		{
			rhr_status_e status = getStatus(q, std::get<2>(fact));
			if (status == TRUE && std::get<1>(fact).at(0).at(0).effect == true || status == NOT && std::get<1>(fact).at(0).at(0).effect == false)
			{
				std::cout << q << " is " << "true" << std::endl;
				higher_value = TRUE;
				break;
			}
			else if (status == AMBIGOUS)
				higher_value = AMBIGOUS;
		}
		if (higher_value == TRUE)
			std::cout << q << " is " << "true" << std::endl;
		if (higher_value == AMBIGOUS)
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