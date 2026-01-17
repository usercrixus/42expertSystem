#include "ReasoningStep.hpp"
#include "LogicRule.hpp"
#include <iostream>

ReasoningStep::ReasoningStep() : capture_trace(false)
{
}

ReasoningStep::~ReasoningStep()
{
}

void ReasoningStep::setEnabled(bool enabled)
{
	capture_trace = enabled;
}

void ReasoningStep::reset()
{
	current_trace.clear();
}

const std::vector<ReasoningTraceStep> &ReasoningStep::getTrace() const
{
	return current_trace;
}

void ReasoningStep::recordInitialFact(char q)
{
	if (!capture_trace)
		return;
	current_trace.push_back({
		std::string("Initial fact: ") + q + " is given as true",
		q, nullptr, R_TRUE, false
	});
}

void ReasoningStep::recordRuleConsidered(char q, const BasicRule* rule, bool lhs_fired)
{
	if (!capture_trace || !lhs_fired)
		return;

	std::string desc = "Rule: " + rule->toString();
	desc += " shows ";
	desc += std::string(1, q);
	desc += rule->rhs_negated ? " false" : " true";

	if (rule->origin)
	{
		desc += " (deduced from rule: " + rule->origin->toString() + ")";
	}

	rhr_value_e concl = rule->rhs_negated ? R_FALSE : R_TRUE;
	current_trace.push_back({desc, q, rule, concl, true});
}

void ReasoningStep::recordMemoHit(char q, rhr_value_e val)
{
	if (!capture_trace)
		return;
	std::string valStr = (val == R_TRUE ? "true" : val == R_FALSE ? "false" : "ambiguous");
	current_trace.push_back({
		"Already resolved: " + std::string(1, q) + " = " + valStr,
		q, nullptr, val, false
	});
}

void ReasoningStep::printTrace(char q, rhr_value_e result, std::ostream &os) const
{
	os << "=== Reasoning for " << q << " ===\n";

	if (current_trace.empty())
	{
		os << "No assertion proves " << q << ", false by default." << std::endl;
		return;
	}
	for (const auto &step : current_trace)
	{
		if (step.symbol == q || step.lhs_fired)
			os << "  " << step.description << "\n";
	}

	std::string resultStr = (result == R_TRUE ? "true" : result == R_FALSE ? "false" : "ambiguous");
	os << "Conclusion: " << q << " is " << resultStr << "\n";
}

void ReasoningStep::printInitialFacts(const std::set<char> &initial_facts, std::ostream &os) const
{
	if (!initial_facts.empty())
	{
		os << "Initial facts: ";
		bool first = true;
		for (char fact : initial_facts)
		{
			if (!first) os << ", ";
			os << fact;
			first = false;
		}
		os << "\n";
	}
}
