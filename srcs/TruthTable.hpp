#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include <ostream>

// Forward declarations
struct BasicRule;
struct TokenBlock;

// Represents a single variable assignment state
struct VariableState
{
    std::map<char, bool> values;
    
    VariableState() = default;
    VariableState(const std::map<char, bool> &v) : values(v) {}
    
    bool operator<(const VariableState &other) const { return values < other.values; }
    bool operator==(const VariableState &other) const { return values == other.values; }
    
    // Check if this state is compatible with another (common variables have same values)
    bool isCompatibleWith(const VariableState &other) const;
    
    // Merge two compatible states
    VariableState merge(const VariableState &other) const;
    
    std::string toString() const;
};

std::ostream &operator<<(std::ostream &os, const VariableState &state);

// Truth table storing only valid states (where the rule is satisfied)
struct TruthTable
{
    std::set<char> variables;              // All variables involved
    std::set<VariableState> valid_states;  // Only states where rule is satisfied
    
    TruthTable() = default;
    
    // Generate truth table from a BasicRule
    static TruthTable fromBasicRule(const BasicRule &rule);
    
    // Check if any valid state exists
    bool hasValidState() const { return !valid_states.empty(); }
    
    // Get number of valid states
    size_t countValidStates() const { return valid_states.size(); }
    
    // Filter states by known facts (removes states incompatible with known facts)
    TruthTable filterByFacts(const std::map<char, bool> &known_facts) const;
    
    // Conjunction: returns table with states valid in both tables
    static TruthTable conjunction(const TruthTable &t1, const TruthTable &t2);
    
    // Conjunction of multiple tables
    static TruthTable conjunctionAll(const std::vector<TruthTable> &tables);
    
    // Get all possible values for a variable in valid states
    // Returns empty set if variable not in table, {false}, {true}, or {false, true}
    std::set<bool> getPossibleValues(char var) const;
    
    // Check if a variable must be true in all valid states
    bool mustBeTrue(char var) const;
    
    // Check if a variable must be false in all valid states
    bool mustBeFalse(char var) const;
    
    std::string toString() const;
};

std::ostream &operator<<(std::ostream &os, const TruthTable &table);
