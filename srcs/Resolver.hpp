#pragma once
#include <vector>
#include "TokenBlock.hpp"
#include <set>

enum rhr_status_e
{
	NOT,
	AMBIGOUS,
	TRUE
};

enum rhr_value_e
{
	R_FALSE,
	R_AMBIGOUS,
	R_TRUE
};

class Resolver
{
private:
	/* data */
public:
	Resolver(/* args */);
	~Resolver();
	void resolveLeft(std::vector<TokenBlock> &fact);
	void resolve(std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> &facts);
	rhr_status_e getStatus(char q, std::vector<TokenBlock> &fact);
	void resolveQuerie(std::set<char> querie, std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> &facts);
    void printFact(std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>> &fact);
};
