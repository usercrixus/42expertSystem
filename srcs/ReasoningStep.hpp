#pragma once

#include <set>
#include <string>
#include <vector>
#include <ostream>
#include <map>
#include "BasicRule.hpp"
#include "ReasoningTypes.hpp"

enum class RuleStatus
{
    FIRED_TRUE,
    FIRED_FALSE,
    NOT_FIRED,
    AMBIGUOUS_CYCLE,
    AMBIGUOUS_DEPENDS
};

struct RuleEvaluation
{
    const BasicRule* rule;
    RuleStatus status;
    std::set<char> blocking_vars;  // Variables that caused ambiguity
    char cycle_var;                // If AMBIGUOUS_CYCLE, which var caused it
};

/**
 * Collects and prints explanation traces for the resolver.
 **/
class ReasoningStep
{
public:
    ReasoningStep();
    ~ReasoningStep();

    void setEnabled(bool enabled);
    bool isEnabled() const;
    void reset();
    
    // Record different events
    void recordInitialFact(char q);
    
    /**
     * Record a rule evaluation with its full status
     **/
    void recordRuleEvaluation(char q, const BasicRule* rule, RuleStatus status,
                              const std::set<char> &blocking_vars = {},
                              char cycle_var = 0);
    
    /**
     * Record the final outcome after prove() completes
     **/
    void recordProveResult(char q, rhr_value_e result);
    
    /**
     * Record when truth table clamping changes a result
     **/
    void recordTruthTableClamp(char q, rhr_value_e before, rhr_value_e after,
                               const std::string &reason);
    
    void printTrace(char q, std::ostream &os) const;
    void printInitialFacts(const std::set<char> &initial_facts, std::ostream &os) const;

private:
    struct SymbolTrace
    {
        std::vector<RuleEvaluation> rule_evals;
        rhr_value_e prove_result = R_FALSE;
        bool was_initial_fact = false;
        bool was_clamped = false;
        rhr_value_e clamped_from = R_FALSE;
        rhr_value_e clamped_to = R_FALSE;
        std::string clamp_reason;
        bool trace_complete = false;
    };
    
    std::map<char, SymbolTrace> traces;
    bool capture_trace;
    
    std::string formatRuleEvaluation(char q, const RuleEvaluation &eval) const;
    std::string formatConclusion(char q, const SymbolTrace &trace) const;
};
