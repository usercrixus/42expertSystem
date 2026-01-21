#include "ReasoningStep.hpp"
#include "LogicRule.hpp"
#include <iostream>
#include <sstream>

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

bool ReasoningStep::isEnabled() const
{
    return capture_trace;
}

void ReasoningStep::reset()
{
    traces.clear();
}

void ReasoningStep::recordInitialFact(char q)
{
    if (!capture_trace)
        return;
    if (traces.find(q) != traces.end())
        return;
    traces[q].was_initial_fact = true;
    traces[q].prove_result = R_TRUE;
}

void ReasoningStep::recordRuleEvaluation(char q, const BasicRule* rule, RuleStatus status,
                                          const std::set<char> &blocking_vars, char cycle_var)
{
    if (!capture_trace)
        return;
    auto it = traces.find(q);
    if (it != traces.end() && it->second.trace_complete)
        return;
    traces[q].rule_evals.push_back({rule, status, blocking_vars, cycle_var});
}

void ReasoningStep::recordProveResult(char q, rhr_value_e result)
{
    if (!capture_trace)
        return;
    auto it = traces.find(q);
    if (it != traces.end() && it->second.trace_complete)
        return;
    traces[q].prove_result = result;
    traces[q].trace_complete = true;
}

void ReasoningStep::recordTruthTableClamp(char q, rhr_value_e before, rhr_value_e after,
                                           const std::string &reason)
{
    if (!capture_trace)
        return;
    traces[q].was_clamped = true;
    traces[q].clamped_from = before;
    traces[q].clamped_to = after;
    traces[q].clamp_reason = reason;
}

std::string ReasoningStep::formatRuleEvaluation(char q, const RuleEvaluation &eval) const
{
    std::ostringstream oss;
    std::string rule_str = eval.rule->toString();

    if (eval.rule->origin)
        rule_str += " (from: " + eval.rule->origin->toString() + ")";
    
    switch (eval.status)
    {
        case RuleStatus::FIRED_TRUE:
            oss << rule_str << " shows " << q << " true";
            break;
        case RuleStatus::FIRED_FALSE:
            oss << rule_str << " shows " << q << " false";
            break;
        case RuleStatus::NOT_FIRED:
            oss << rule_str << " did not fire (";
            if (!eval.blocking_vars.empty())
            {
                bool first = true;
                for (char var : eval.blocking_vars)
                {
                    if (!first) oss << ", ";
                    oss << var;
                    first = false;
                }
                oss << " false)";
            }
            else
            {
                oss << "LHS false)";
            }
            break;
        case RuleStatus::AMBIGUOUS_CYCLE:
            oss << rule_str << " could show " << q 
                << (eval.rule->rhs_negated ? " false" : " true")
                << " but " << eval.cycle_var << " creates a cycle";
            break;
        case RuleStatus::AMBIGUOUS_DEPENDS:
            oss << rule_str << " could show " << q 
                << (eval.rule->rhs_negated ? " false" : " true")
                << " but ";
            {
                bool first = true;
                for (char var : eval.blocking_vars)
                {
                    if (!first) oss << ", ";
                    oss << var;
                    first = false;
                }
            }
            oss << " is undetermined";
            break;
    }
    
    return oss.str();
}

std::string ReasoningStep::formatConclusion(char q, const SymbolTrace &trace) const
{
    std::ostringstream oss;
    
    if (trace.was_clamped && trace.clamped_from != trace.clamped_to)
    {
        oss << trace.clamp_reason << "\n";
    }
    
    std::string result_str;
    rhr_value_e final_result = trace.was_clamped ? trace.clamped_to : trace.prove_result;
    
    switch (final_result)
    {
        case R_TRUE: result_str = "true"; break;
        case R_FALSE: result_str = "false"; break;
        case R_AMBIGOUS: result_str = "ambiguous"; break;
    }
    
    oss << q << " is " << result_str;
    return oss.str();
}

void ReasoningStep::printTrace(char q, std::ostream &os) const
{
    os << "=== Reasoning for " << q << " ===\n";
    
    auto it = traces.find(q);
    if (it == traces.end())
    {
        os << "No rules apply to " << q << ", false by default.\n";
        os << q << " is false\n";
        return;
    }
    
    const SymbolTrace &trace = it->second;
    if (trace.was_initial_fact)
    {
        os << q << " is given as an initial fact.\n";
        os << q << " is true\n";
        return;
    }

    bool has_any_rule = false;
    for (const auto &eval : trace.rule_evals)
    {
        os << formatRuleEvaluation(q, eval) << "\n";
        has_any_rule = true;
    }
    if (!has_any_rule)
    {
        os << "No rules target " << q << ", false by default.\n";
    }
    os << formatConclusion(q, trace) << "\n";
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
