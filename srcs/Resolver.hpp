#pragma once
#include <vector>
#include "TokenBlock.hpp"
#include <set>
#include <unordered_map>
#include <unordered_set>

enum rhr_status_e
{
	NOT,
	AMBIGOUS,
	TRUE,
	FALSE
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
	std::set<char> querie;
	std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> &facts;
	std::set<char> initial_facts;
	std::unordered_map<char, rhr_value_e> memo;
	std::unordered_set<char> visiting;
	bool initial_cached = false;
	bool evalLHS(std::vector<TokenBlock> lhs);
	rhr_value_e prove(char q);
	bool isMentionQ(char q, const std::vector<TokenBlock> &rhr);
	bool isInterBlockAmbiguity(const std::vector<TokenBlock> &fact, std::vector<bool> &block_has_q_pos, std::vector<bool> &block_has_q_neg, std::vector<bool> &block_has_or_xor);
	rhr_status_e innerBlockAmbiguity(char q, const std::vector<TokenBlock> &fact, std::vector<bool> &block_has_q_pos, std::vector<bool> &block_has_q_neg, std::vector<bool> &block_has_or_xor);

public:
	Resolver(std::set<char> querie, std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> &facts, std::set<char> initial_facts);
	~Resolver();
	void resolveLeft(std::vector<TokenBlock> &fact);
	rhr_status_e getStatus(char q, const std::vector<TokenBlock> &fact);
	void resolveQuerie();
};
