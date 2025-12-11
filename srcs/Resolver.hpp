#pragma once
#include <vector>
#include "LogicRule.hpp"
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>

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

struct ReasoningStep
{
	std::string description;
	char symbol;
	const BasicRule* rule;
	rhr_value_e conclusion;
	bool lhs_fired;
};

class Resolver
{
private:
	std::set<char> querie;
	std::vector<BasicRule> &basic_rules;
	std::set<char> initial_facts;
	std::unordered_map<char, rhr_value_e> memo;
	std::unordered_set<char> visiting;
	bool initial_cached = false;
	std::vector<ReasoningStep> current_trace;
	bool evalLHS(std::vector<TokenBlock> lhs);
	rhr_value_e prove(char q);
	void recordInitialFact(char q);
	void recordRuleConsidered(char q, const BasicRule* rule, bool lhs_fired);
	void recordMemoHit(char q, rhr_value_e val);
	void printTrace(char q, rhr_value_e result);

public:
	Resolver(std::set<char> querie, std::vector<BasicRule> &basic_rules, std::set<char> initial_facts);
	~Resolver();
	void resolveLeft(std::vector<TokenBlock> &fact);
	void resolveQuerie();
};
