#pragma once
#include <vector>
#include "TokenBlock.hpp"

class Resolver
{
private:
	/* data */
public:
	Resolver(/* args */);
	~Resolver();
	void resolveLeft(std::vector<TokenBlock> &fact);
	void resolve(std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> facts);
	void reduceRight(std::vector<TokenBlock> &fact);
    void printFact(std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>> &fact);
};
