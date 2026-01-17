#pragma once

#include <set>
#include <string>
#include <vector>
#include <ostream>
#include "BasicRule.hpp"
#include "ReasoningTypes.hpp"

/**
 * One trace entry describing how a symbol was derived.
 **/
struct ReasoningTraceStep
{
	std::string description;
	char symbol;
	const BasicRule* rule;
	rhr_value_e conclusion;
	bool lhs_fired;
};

/**
 * Collects and prints explanation traces for the resolver.
 **/
class ReasoningStep
{
public:
	ReasoningStep();
	~ReasoningStep();

	/**
	 * Enable or disable trace collection.
	 **/
	void setEnabled(bool enabled);
	/**
	 * Clear any previously collected trace steps.
	 **/
	void reset();
	/**
	 * Record a line describing an initial fact.
	 **/
	void recordInitialFact(char q);
	/**
	 * Record a fired rule that contributes to the current symbol.
	 **/
	void recordRuleConsidered(char q, const BasicRule* rule, bool lhs_fired);
	/**
	 * Record a memoized value hit during resolution.
	 **/
	void recordMemoHit(char q, rhr_value_e val);
	/**
	 * Print the trace for a given symbol and final result.
	 **/
	void printTrace(char q, rhr_value_e result, std::ostream &os) const;
	/**
	 * Print the initial facts heading.
	 **/
	void printInitialFacts(const std::set<char> &initial_facts, std::ostream &os) const;
	/**
	 * Access the collected trace steps.
	 **/
	const std::vector<ReasoningTraceStep> &getTrace() const;

private:
	// Accumulated trace steps for the current resolution.
	std::vector<ReasoningTraceStep> current_trace;
	// Enable or disable trace collection.
	bool capture_trace;
};
