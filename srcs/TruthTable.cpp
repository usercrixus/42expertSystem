#include "TruthTable.hpp"
#include "LogicRule.hpp"
#include "TokenBlock.hpp"
#include "TokenEffect.hpp"
#include <sstream>
#include <algorithm>

bool VariableState::isCompatibleWith(const VariableState &other) const
{
    for (const auto &pair : values)
    {
        auto it = other.values.find(pair.first);
        if (it != other.values.end() && it->second != pair.second)
            return false;
    }
    return true;
}

VariableState VariableState::merge(const VariableState &other) const
{
    VariableState result = *this;
    for (const auto &pair : other.values)
    {
        result.values[pair.first] = pair.second;
    }
    return result;
}

std::string VariableState::toString() const
{
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto &pair : values)
    {
        if (!first)
            oss << ", ";
        oss << pair.first << "=" << (pair.second ? "T" : "F");
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::ostream &operator<<(std::ostream &os, const VariableState &state)
{
    os << state.toString();
    return os;
}

static unsigned int getMaxPriority(const std::vector<TokenBlock> &fact)
{
    unsigned int maxPriority = 0;
    for (const TokenBlock &token_block : fact)
    {
        if (token_block.getPriority() > maxPriority)
            maxPriority = token_block.getPriority();
    }
    return maxPriority;
}

static bool resolveLeft(std::vector<TokenBlock> &fact)
{
    if (fact.empty())
        return false;
    while (true)
    {
        unsigned int priority = getMaxPriority(fact);
        for (size_t i = 0; i < fact.size();)
        {
            if (fact[i].getPriority() == priority)
            {
                fact[i].execute();
                if (i != 0)
                {
                    fact[i - 1].push_back(fact[i][0]);
                    fact.erase(fact.begin() + i);
                    continue;
                }
                if (fact.size() > 1)
                {
                    fact[1].insert(fact[1].begin(), fact[0][0]);
                    fact.erase(fact.begin());
                    continue;
                }
                fact[i].setPriority(0);
            }
            ++i;
        }
        if (fact.size() == 1)
        {
            if (fact[0].size() > 1)
                fact[0].execute();
            return fact[0][0].effect;
        }
    }
}

static bool evaluateSide(const std::vector<TokenBlock> &side, const std::map<char, bool> &values)
{
    std::vector<TokenBlock> blocks = side;
    for (TokenBlock &block : blocks)
    {
        for (TokenEffect &tk : block)
        {
            if (tk.type >= 'A' && tk.type <= 'Z')
            {
                auto it = values.find(tk.type);
                tk.effect = (it != values.end()) ? it->second : false;
            }
        }
    }
    return resolveLeft(blocks);
}

static std::set<char> collectVariables(const std::vector<TokenBlock> &side)
{
    std::set<char> vars;
    for (const TokenBlock &block : side)
    {
        for (const TokenEffect &tk : block)
        {
            if (tk.type >= 'A' && tk.type <= 'Z')
                vars.insert(tk.type);
        }
    }
    return vars;
}

TruthTable TruthTable::fromBasicRule(const BasicRule &rule)
{
    TruthTable table;
    
    table.variables = collectVariables(rule.lhs);
    table.variables.insert(rule.rhs_symbol);
    
    size_t num_vars = table.variables.size();
    size_t num_combinations = 1 << num_vars;
    std::vector<char> var_list(table.variables.begin(), table.variables.end());
    for (size_t i = 0; i < num_combinations; ++i)
    {
        std::map<char, bool> state;
        for (size_t j = 0; j < num_vars; ++j)
        {
            state[var_list[j]] = (i >> j) & 1;
        }
        bool lhs_val = evaluateSide(rule.lhs, state);
        bool rhs_val = rule.rhs_negated ? !state[rule.rhs_symbol] : state[rule.rhs_symbol];
        
        bool rule_satisfied = !lhs_val || rhs_val;
        
        if (rule_satisfied)
        {
            table.valid_states.insert(VariableState(state));
        }
    }
    
    return table;
}

TruthTable TruthTable::filterByFacts(const std::map<char, bool> &known_facts) const
{
    TruthTable filtered;
    filtered.variables = variables;
    
    for (const VariableState &state : valid_states)
    {
        bool compatible = true;
        
        for (const auto &fact : known_facts)
        {
            auto it = state.values.find(fact.first);
            if (it != state.values.end() && it->second != fact.second)
            {
                compatible = false;
                break;
            }
        }
        
        if (compatible)
        {
            filtered.valid_states.insert(state);
        }
    }
    
    return filtered;
}

TruthTable TruthTable::filterByResults(const std::set<char> &initial_facts, const std::map<char, rhr_value_e> &base_results) const
{
    std::map<char, bool> known_facts;
    for (char fact : initial_facts)
        known_facts[fact] = true;
    for (const auto &entry : base_results)
    {
        if (entry.second == R_TRUE)
            known_facts[entry.first] = true;
        else if (entry.second == R_FALSE)
            known_facts[entry.first] = false;
    }
    return filterByFacts(known_facts);
}

TruthTable TruthTable::conjunction(const TruthTable &t1, const TruthTable &t2)
{
    TruthTable result;
    
    result.variables = t1.variables;
    result.variables.insert(t2.variables.begin(), t2.variables.end());
    
    for (const VariableState &state1 : t1.valid_states)
    {
        for (const VariableState &state2 : t2.valid_states)
        {
            if (state1.isCompatibleWith(state2))
            {
                VariableState merged = state1.merge(state2);
                result.valid_states.insert(merged);
            }
        }
    }
    
    return result;
}

TruthTable TruthTable::conjunctionAll(const std::vector<TruthTable> &tables)
{
    if (tables.empty())
        return TruthTable();
    
    TruthTable result = tables[0];
    
    for (size_t i = 1; i < tables.size(); ++i)
    {
        result = conjunction(result, tables[i]);
        
        // Early exit if no valid states remain
        if (!result.hasValidState())
            break;
    }
    
    return result;
}

std::set<bool> TruthTable::getPossibleValues(char var) const
{
    std::set<bool> possible;
    
    for (const VariableState &state : valid_states)
    {
        auto it = state.values.find(var);
        if (it != state.values.end())
        {
            possible.insert(it->second);
        }
    }
    
    return possible;
}

bool TruthTable::mustBeTrue(char var) const
{
    std::set<bool> possible = getPossibleValues(var);
    return possible.size() == 1 && *possible.begin() == true;
}

bool TruthTable::mustBeFalse(char var) const
{
    std::set<bool> possible = getPossibleValues(var);
    return possible.size() == 1 && *possible.begin() == false;
}

rhr_value_e TruthTable::clampValue(char var, rhr_value_e current) const
{
    if (mustBeTrue(var))
        return R_TRUE;
    if (mustBeFalse(var))
        return R_FALSE;
    return current;
}

std::string TruthTable::toString() const
{
    std::ostringstream oss;
    
    if (variables.empty())
    {
        oss << "Empty truth table\n";
        return oss.str();
    }
    
    std::vector<char> var_list(variables.begin(), variables.end());
    for (char var : var_list)
    {
        oss << var << " | ";
    }
    
    oss << std::endl << std::string(var_list.size() * 4, '-') << std::endl;
    
    if (valid_states.empty())
    {
        oss << "(No valid states - contradiction!)\n";
        return oss.str();
    }
    
    for (const VariableState &state : valid_states)
    {
        for (char var : var_list)
        {
            auto it = state.values.find(var);
            if (it != state.values.end())
            {
                oss << (it->second ? "T" : "F") << " | ";
            }
            else
            {
                oss << "? | ";
            }
        }
        oss << std::endl;
    }
    
    oss << "\nTotal valid states: " << valid_states.size() << "\n";
    
    return oss.str();
}

std::ostream &operator<<(std::ostream &os, const TruthTable &table)
{
    os << table.toString();
    return os;
}
