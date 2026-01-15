#pragma once
#include <vector>
#include "LogicRule.hpp"
#include <set>
#include <unordered_map>
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
	struct TriToken
	{
		char type;
		rhr_value_e value;
		bool has_value;
	};

	struct TriBlock
	{
		unsigned int priority;
		std::vector<TriToken> tokens;
	};

	std::set<char> querie;
	std::vector<BasicRule> &basic_rules;
	std::set<char> initial_facts;
	std::unordered_map<char, rhr_value_e> memo;
	std::unordered_map<char, bool> visiting;
	bool initial_cached = false;
	std::vector<ReasoningStep> current_trace;
	rhr_value_e evalLHS(std::vector<TokenBlock> lhs);
	rhr_value_e resolveLeftTri(std::vector<TriBlock> &blocks);
	rhr_value_e executeTriBlock(std::vector<TriToken> &tokens);
	void executeNotTri(std::vector<TriToken> &tokens);
	void executeOthersTri(std::vector<TriToken> &tokens, char op_target);
	rhr_value_e getTokenValue(TriToken &token, bool negated_context);
	rhr_value_e prove(char q, bool negated_context);
	void recordInitialFact(char q);
	void recordRuleConsidered(char q, const BasicRule* rule, bool lhs_fired);
	void recordMemoHit(char q, rhr_value_e val);
	void printTrace(char q, rhr_value_e result);
	unsigned int getMaxPriority(std::vector<TokenBlock> &fact);
	void resolveLeft(std::vector<TokenBlock> &fact);
	/**
	 * Display initial facts
	 */
	void printInitialFact();

public:
	Resolver(std::set<char> querie, std::vector<BasicRule> &basic_rules, std::set<char> initial_facts);
	~Resolver();
	/**
	 * Resolve all queries and optionally print the reasoning trace.
	 */
	void resolveQuerie(bool print_trace = true);
	/**
	 * Update initial facts and clear resolver caches before re-evaluating queries.
	 */
	void changeFacts(const std::set<char> &new_facts);
};
