#pragma once
#include <vector>
#include "BasicRule.hpp"
#include "ReasoningStep.hpp"
#include "ReasoningTypes.hpp"
#include "TruthTable.hpp"
#include <map>
#include <set>
#include <unordered_map>

/**
 * Resolves queries using rule evaluation and optional truth-table constraints.
 **/
class Resolver
{
private:
	/**
	 * Aggregates definite/possible outcomes during rule evaluation.
	 **/
	struct RuleOutcome
	{
		/** rule set proves the symbol true */
		bool definite_true;
		/** rule set proves the symbol false */
		bool definite_false;
		/** rule set may prove the symbol true */
		bool possible_true;
		/** rule set may prove the symbol false */
		bool possible_false;
	};

	/**
	 * Internal tri-state token used during expression evaluation.
	 **/
	struct TriToken
	{
		/** token type (operator or symbol). */
		char type;
		/** resolved value for the token. */
		rhr_value_e value;
		/** whether the value has been computed. */
		bool has_value;
	};

	/**
	 * Token block with a parsing priority.
	 **/
	struct TriBlock
	{
		/** operator precedence level for this block. */
		unsigned int priority;
		/** ordered tokens belonging to the block. */
		std::vector<TriToken> tokens;
	};

	/** query symbols to resolve. */
	std::set<char> querie;
	/** rules deduced from parsing logic expressions. */
	std::vector<BasicRule> &basic_rules;
	/** initial facts provided by the input file. */
	std::set<char> initial_facts;
	/** optional global truth table constraints. */
	TruthTable truth_table;
	/** trace recorder for --explain output. */
	ReasoningStep reasoning;
	/** memorized results for already-proven symbols. */
	std::unordered_map<char, rhr_value_e> memo;
	/** recursion tracking to detect cycles. */
	std::unordered_map<char, bool> visiting;

	/**
	 * Clear memorization and recursion tracking for a new resolution.
	 **/
	void resetEvaluationState();
	/**
	 * Resolve a symbol with recursion and memoization.
	 **/
	rhr_value_e prove(char q, bool negated_context);
	/**
	 * Check memo cache and record a trace if hit.
	 **/
	bool handleQMemo(char q, rhr_value_e &result);
    bool isQHandled(char q, rhr_value_e &result, bool negated_context);
    /**
     * Check initial facts and record a trace if matched.
     **/
    bool handleQInitialFact(char q, rhr_value_e &result);
	/**
	 * Handle recursion cycles based on negation context.
	 **/
	bool handleVisiting(char q, bool negated_context, rhr_value_e &result);
	/**
	 * Accumulate outcome flags from a single rule evaluation.
	 **/
	void updateOutcomeFromRule(rhr_value_e lhs_result, const BasicRule &rule, RuleOutcome &outcome);
    std::vector<TriBlock> buildTriBlockVector(const std::vector<TokenBlock> &lhs);
    /**
     * Finalize the boolean outcome into a tri-state value.
     **/
    rhr_value_e finalizeOutcome(const RuleOutcome &outcome) const;
	/**
	 * Reduce grouped token blocks by precedence.
	 **/
	rhr_value_e resolveLeftTri(std::vector<TriBlock> &blocks);
	/**
	 * Execute a single token block with tri-state operators.
	 **/
	rhr_value_e executeTriBlock(std::vector<TriToken> &tokens, bool negated_context);
	/**
	 * Apply NOT operations for a tri-state token block.
	 **/
	void executeNotTri(std::vector<TriToken> &tokens, bool negated_context);
	/**
	 * Apply a specific binary operator for a tri-state token block.
	 **/
	void executeOthersTri(std::vector<TriToken> &tokens, char op_target, bool negated_context);
	/**
	 * Resolve a token value, proving symbols as needed.
	 **/
	rhr_value_e getTokenValue(TriToken &token, bool negated_context);
	/**
	 * get the max priority of a std::vector<TokenBlock>
	 */
	unsigned int getMaxPriority(std::vector<TriBlock> &blocks);
    bool isNegatedContext(size_t i, std::vector<TriBlock> &blocks);
	/**
	 * Build a filtered truth table from known facts.
	 **/
	bool buildFilteredTruthTable(const std::map<char, rhr_value_e> &base_results, TruthTable &filtered) const;
	/**
	 * Print the final result for a query.
	 **/
	void outputResult(char q, rhr_value_e res);

public:
	/**
	 * Construct the resolver with rules, facts, and an optional truth table.
	 **/
	Resolver(std::set<char> querie, std::vector<BasicRule> &basic_rules, std::set<char> initial_facts, const TruthTable &truth_table);
	/**
	 * Destroy the resolver.
	 **/
	~Resolver();
	/**
	 * Access the reasoning trace helper.
	 **/
	ReasoningStep &getReasoning();
	/**
	 * Access the reasoning trace helper (const).
	 **/
	const ReasoningStep &getReasoning() const;
	/**
	 * Resolve all queries and print standard results.
	 */
	void resolve();
	/**
	 * Resolve one query with optional truth-table clamping.
	 */
	rhr_value_e resolveQuery(char q, const TruthTable &filtered, bool has_truth_table);
	/**
	 * Compute base results for a set of symbols.
	 */
	std::map<char, rhr_value_e> computeBaseResults(const std::set<char> &facts);
	/**
	 * Update initial facts and clear resolver caches before re-evaluating queries.
	 */
	void changeFacts(const std::set<char> &new_facts);
};
