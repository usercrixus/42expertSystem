#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include <ostream>
#include "ReasoningTypes.hpp"

struct BasicRule;
struct TokenBlock;

struct VariableState
{
    std::map<char, bool> values;
    
    VariableState() = default;
    VariableState(const std::map<char, bool> &v) : values(v) {}
    
    bool operator<(const VariableState &other) const { return values < other.values; }
    bool operator==(const VariableState &other) const { return values == other.values; }
    
    /** check if this state is compatible with another (no conflicting values) */
    bool isCompatibleWith(const VariableState &other) const;
    
    /** merge two compatible states into one */
    VariableState merge(const VariableState &other) const;
    
    /** convert to string */
    std::string toString() const;
};

std::ostream &operator<<(std::ostream &os, const VariableState &state);

struct TruthTable
{
    /** variables involved in this truth table */
    std::set<char> variables;
    // states that are compatible with the rule
    std::set<VariableState> valid_states;
    
    TruthTable() = default;
    
    /** generate truth table from a basic rule */
    static TruthTable fromBasicRule(const BasicRule &rule);
    /** check if there's at least one valid state */
    bool hasValidState() const { return !valid_states.empty(); }
    /** count the number of valid states */
    size_t countValidStates() const { return valid_states.size(); }
    /** filter states by known facts */
    TruthTable filterByFacts(const std::map<char, bool> &known_facts) const;
    /** filter states by known facts derived from base results and initial facts */
    TruthTable filterByResults(const std::set<char> &initial_facts, const std::map<char, rhr_value_e> &base_results) const;
    /** combine two truth tables */
    static TruthTable conjunction(const TruthTable &t1, const TruthTable &t2);
    /** combine multiple truth tables */
    static TruthTable conjunctionAll(const std::vector<TruthTable> &tables);
    /** get all possible values a variable can have */
    std::set<bool> getPossibleValues(char var) const;
    /** check if variable must be true in all valid states */
    bool mustBeTrue(char var) const;
    /** check if variable must be false in all valid states */
    bool mustBeFalse(char var) const;
    /** clamp a tri-state value using this truth table, when available */
    rhr_value_e clampValue(char var, rhr_value_e current) const;
    
    /** convert to string */
    std::string toString() const;
};

std::ostream &operator<<(std::ostream &os, const TruthTable &table);
