#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include <ostream>

struct BasicRule;
struct TokenBlock;

struct VariableState
{
    std::map<char, bool> values;
    
    VariableState() = default;
    VariableState(const std::map<char, bool> &v) : values(v) {}
    
    bool operator<(const VariableState &other) const { return values < other.values; }
    bool operator==(const VariableState &other) const { return values == other.values; }
    
    bool isCompatibleWith(const VariableState &other) const;
    
    VariableState merge(const VariableState &other) const;
    
    std::string toString() const;
};

std::ostream &operator<<(std::ostream &os, const VariableState &state);

struct TruthTable
{
    std::set<char> variables;
    std::set<VariableState> valid_states;
    
    TruthTable() = default;
    
    static TruthTable fromBasicRule(const BasicRule &rule);
    bool hasValidState() const { return !valid_states.empty(); }
    size_t countValidStates() const { return valid_states.size(); }
    TruthTable filterByFacts(const std::map<char, bool> &known_facts) const;
    static TruthTable conjunction(const TruthTable &t1, const TruthTable &t2);
    static TruthTable conjunctionAll(const std::vector<TruthTable> &tables);
    std::set<bool> getPossibleValues(char var) const;
    bool mustBeTrue(char var) const;
    bool mustBeFalse(char var) const;
    
    std::string toString() const;
};

std::ostream &operator<<(std::ostream &os, const TruthTable &table);
