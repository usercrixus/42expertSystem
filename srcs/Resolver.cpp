#include "Resolver.hpp"
#include <iostream>
#include <map>
#include <stdexcept>

Resolver::Resolver(std::set<char> querie, std::vector<BasicRule> &basic_rules, std::set<char> initial_facts, const TruthTable *truth_table)
    : querie(querie),
      basic_rules(basic_rules),
      initial_facts(initial_facts),
      truth_table(truth_table),
      reasoning()
{
}

Resolver::~Resolver()
{
}

ReasoningStep &Resolver::getReasoning()
{
    return reasoning;
}

const ReasoningStep &Resolver::getReasoning() const
{
    return reasoning;
}

static rhr_value_e resolveNot(rhr_value_e v)
{
    if (v == R_TRUE)
        return R_FALSE;
    if (v == R_FALSE)
        return R_TRUE;
    return R_AMBIGOUS;
}

static rhr_value_e resolveAnd(rhr_value_e a, rhr_value_e b)
{
    if (a == R_FALSE || b == R_FALSE)
        return R_FALSE;
    if (a == R_TRUE && b == R_TRUE)
        return R_TRUE;
    return R_AMBIGOUS;
}

static rhr_value_e resolveOr(rhr_value_e a, rhr_value_e b)
{
    if (a == R_TRUE || b == R_TRUE)
        return R_TRUE;
    if (a == R_FALSE && b == R_FALSE)
        return R_FALSE;
    return R_AMBIGOUS;
}

static rhr_value_e resolveXor(rhr_value_e a, rhr_value_e b)
{
    if (a == R_AMBIGOUS || b == R_AMBIGOUS)
        return R_AMBIGOUS;
    return (a != b) ? R_TRUE : R_FALSE;
}

void Resolver::resetEvaluationState()
{
    memo.clear();
    visiting.clear();
}

bool Resolver::handleMemoHit(char q, rhr_value_e &result)
{
    std::unordered_map<char, rhr_value_e>::iterator memoIt = memo.find(q);
    if (memoIt == memo.end())
        return false;
    reasoning.recordMemoHit(q, memoIt->second);
    result = memoIt->second;
    return true;
}

bool Resolver::handleInitialFact(char q, rhr_value_e &result)
{
    auto initiaTrueIt = initial_facts.find(q);
    if (initiaTrueIt == initial_facts.end())
        return false;
    reasoning.recordInitialFact(q);
    result = R_TRUE;
    memo[q] = result;
    return true;
}

bool Resolver::handleVisiting(char q, bool negated_context, bool direct_negation, rhr_value_e &result)
{
    std::unordered_map<char, bool>::iterator visitingIt = visiting.find(q);
    if (visitingIt == visiting.end())
        return false;
    if (visitingIt->second != negated_context)
        result = direct_negation ? R_FALSE : R_AMBIGOUS;
    else
        result = R_FALSE;
    return true;
}

bool Resolver::allowNegationAsFailure(const BasicRule &rule) const
{
    if (rule.origin == nullptr)
        return false;
    return false;
}

void Resolver::updateOutcomeFromRule(rhr_value_e lhs_result, const BasicRule &rule, RuleOutcome &outcome)
{
    if (lhs_result == R_TRUE)
    {
        if (!rule.rhs_negated)
            outcome.definite_true = true;
        else
            outcome.definite_false = true;
        return;
    }
    if (lhs_result == R_AMBIGOUS)
    {
        if (!rule.rhs_negated)
            outcome.possible_true = true;
        else
            outcome.possible_false = true;
    }
}

rhr_value_e Resolver::finalizeOutcome(const RuleOutcome &outcome) const
{
    if (outcome.definite_true && outcome.definite_false)
        return R_AMBIGOUS;
    if (outcome.definite_true)
        return R_TRUE;
    if (outcome.definite_false)
        return R_FALSE;
    if (outcome.possible_true && outcome.possible_false)
        return R_AMBIGOUS;
    if (outcome.possible_true || outcome.possible_false)
        return R_AMBIGOUS;
    return R_FALSE;
}

unsigned int Resolver::getMaxPriority(std::vector<TokenBlock> &fact)
{
    unsigned int maxPriority = 0;
    for (TokenBlock &token_block : fact)
    {
        if (token_block.getPriority() > maxPriority)
            maxPriority = token_block.getPriority();
    }
    return maxPriority;
}

void Resolver::resolveLeft(std::vector<TokenBlock> &fact)
{
    unsigned int priority = getMaxPriority(fact);
    for (size_t i = 0; i < fact.size(); i++)
    {
        if (fact[i].getPriority() == priority)
        {
            fact[i].execute();
            if (i != 0)
            {
                fact[i - 1].push_back(fact[i][0]);
                fact.erase(fact.begin() + i);
            }
            else
            {
                if (fact.size() > 1)
                {
                    fact[1].insert(fact[1].begin(), fact[0][0]);
                    fact.erase(fact.begin());
                }
                else
                    fact[0].setPriority(0);
            }
        }
    }
    if (fact.size() > 0)
        resolveLeft(fact);
}

rhr_value_e Resolver::prove(char q, bool negated_context, bool direct_negation)
{
    rhr_value_e result = R_FALSE;
    if (handleMemoHit(q, result))
        return result;
    if (handleInitialFact(q, result))
        return result;
    if (handleVisiting(q, negated_context, direct_negation, result))
        return result;

    visiting[q] = negated_context;

    RuleOutcome outcome = {false, false, false, false};
    for (const BasicRule &rule : basic_rules)
    {
        if (rule.rhs_symbol != q)
            continue;
        std::vector<TokenBlock> lhs = rule.lhs;
        bool allow_negation_as_failure = allowNegationAsFailure(rule);
        rhr_value_e lhs_result = evalLHS(lhs, allow_negation_as_failure);
        reasoning.recordRuleConsidered(q, &rule, lhs_result == R_TRUE);
        updateOutcomeFromRule(lhs_result, rule, outcome);
    }

    visiting.erase(q);

    result = finalizeOutcome(outcome);
    memo[q] = result;
    return result;
}

rhr_value_e Resolver::getTokenValue(TriToken &token, bool negated_context, bool direct_negation)
{
    if (token.has_value)
        return token.value;
    if (token.type >= 'A' && token.type <= 'Z')
    {
        token.value = prove(token.type, negated_context, direct_negation);
        token.has_value = true;
        return token.value;
    }
    throw std::logic_error("Token value requested for non-value token");
}

void Resolver::executeNotTri(std::vector<TriToken> &tokens, bool negated_context, bool allow_negation_as_failure)
{
    size_t i = 0;
    while (i < tokens.size())
    {
        if (tokens[i].type == '!')
        {
            if (i + 1 == tokens.size())
                throw std::logic_error("operator ! has no var attached\n");
            TriToken &next = tokens[i + 1];
            rhr_value_e val = getTokenValue(next, !negated_context, allow_negation_as_failure);
            next.value = resolveNot(val);
            next.has_value = true;
            next.type = 0;
            tokens.erase(tokens.begin() + i);
            if (i > 0)
                --i;
        }
        else
            ++i;
    }
}

void Resolver::executeOthersTri(std::vector<TriToken> &tokens, char op_target, bool negated_context)
{
    size_t i = 0;
    while (i < tokens.size())
    {
        if (tokens[i].type == op_target)
        {
            if (i == 0 || i + 1 == tokens.size())
                throw std::logic_error(std::string("operator ") + op_target + " has no var attached\n");
            TriToken &left = tokens[i - 1];
            TriToken &right = tokens[i + 1];
            rhr_value_e lval = getTokenValue(left, negated_context, false);
            rhr_value_e rval = getTokenValue(right, negated_context, false);
            rhr_value_e res = R_FALSE;
            if (op_target == '+')
                res = resolveAnd(lval, rval);
            else if (op_target == '|')
                res = resolveOr(lval, rval);
            else if (op_target == '^')
                res = resolveXor(lval, rval);
            tokens[i].type = 0;
            tokens[i].value = res;
            tokens[i].has_value = true;
            tokens.erase(tokens.begin() + i + 1);
            tokens.erase(tokens.begin() + i - 1);
            if (i > 0)
                --i;
        }
        else
            ++i;
    }
}

rhr_value_e Resolver::executeTriBlock(std::vector<TriToken> &tokens, bool negated_context, bool allow_negation_as_failure)
{
    if (tokens.empty())
        throw std::logic_error("TriBlock::execute: empty block");
    executeNotTri(tokens, negated_context, allow_negation_as_failure);
    executeOthersTri(tokens, '+', negated_context);
    executeOthersTri(tokens, '|', negated_context);
    executeOthersTri(tokens, '^', negated_context);
    if (tokens.size() != 1)
        throw std::logic_error("TriBlock::execute: reduction did not converge");
    return getTokenValue(tokens[0], negated_context, false);
}

rhr_value_e Resolver::resolveLeftTri(std::vector<TriBlock> &blocks, bool allow_negation_as_failure)
{
    if (blocks.empty())
        throw std::logic_error("resolveLeftTri: empty expression");
    while (true)
    {
        unsigned int max_priority = 0;
        for (const TriBlock &block : blocks)
            if (block.priority > max_priority)
                max_priority = block.priority;

        for (size_t i = 0; i < blocks.size();)
        {
            if (blocks[i].priority == max_priority)
            {
                bool negated_context = false;
                if (i != 0 && !blocks[i - 1].tokens.empty())
                {
                    const TriToken &prev = blocks[i - 1].tokens.back();
                    if (prev.type == '!')
                        negated_context = true;
                }
                executeTriBlock(blocks[i].tokens, negated_context, allow_negation_as_failure);
                TriToken result = blocks[i].tokens[0];
                result.type = 0;
                result.has_value = true;
                if (i != 0)
                {
                    blocks[i - 1].tokens.push_back(result);
                    blocks.erase(blocks.begin() + i);
                    continue;
                }
                if (blocks.size() > 1)
                {
                    blocks[1].tokens.insert(blocks[1].tokens.begin(), result);
                    blocks.erase(blocks.begin());
                    continue;
                }
                blocks[i].priority = 0;
            }
            ++i;
        }
        if (blocks.size() == 1)
        {
            if (blocks[0].tokens.size() > 1)
                executeTriBlock(blocks[0].tokens, false, allow_negation_as_failure);
            return getTokenValue(blocks[0].tokens[0], false, false);
        }
    }
}

rhr_value_e Resolver::evalLHS(std::vector<TokenBlock> lhs, bool allow_negation_as_failure)
{
    std::vector<TriBlock> blocks;
    blocks.reserve(lhs.size());
    for (const TokenBlock &block : lhs)
    {
        TriBlock tri_block;
        tri_block.priority = block.getPriority();
        tri_block.tokens.reserve(block.size());
        for (const TokenEffect &tk : block)
        {
            TriToken tri_token;
            tri_token.type = tk.type;
            tri_token.value = R_FALSE;
            tri_token.has_value = false;
            tri_block.tokens.push_back(tri_token);
        }
        blocks.push_back(tri_block);
    }
    return resolveLeftTri(blocks, allow_negation_as_failure);
}

std::set<char> Resolver::buildTruthTableFacts(const std::set<char> &queries) const
{
    std::set<char> facts_for_truth_table = queries;
    if (truth_table != nullptr)
        facts_for_truth_table.insert(truth_table->variables.begin(), truth_table->variables.end());
    return facts_for_truth_table;
}

std::map<char, rhr_value_e> Resolver::computeBaseResults(const std::set<char> &facts)
{
    std::map<char, rhr_value_e> base_results;
    for (char q : facts)
    {
        resetEvaluationState();
        reasoning.reset();
        base_results[q] = prove(q, false, false);
    }
    return base_results;
}

bool Resolver::buildFilteredTruthTable(const std::map<char, rhr_value_e> &base_results, TruthTable &filtered) const
{
    bool has_truth_table = truth_table != nullptr && truth_table->hasValidState();
    if (!has_truth_table)
        return false;

    filtered = truth_table->filterByResults(initial_facts, base_results);
    return filtered.hasValidState();
}

void Resolver::outputResult(char q, rhr_value_e res)
{
    std::string resultStr = (res == R_TRUE ? "true" : res == R_FALSE ? "false" : "ambiguous");
    std::cout << q << " = " << resultStr << std::endl;
}

rhr_value_e Resolver::resolveQuery(char q, const TruthTable &filtered, bool has_truth_table)
{
    resetEvaluationState();
    reasoning.reset();
    rhr_value_e res = prove(q, false, false);
    if (!has_truth_table)
        return res;
    return filtered.clampValue(q, res);
}

void Resolver::resolveQuerie()
{
    std::set<char> output_symbols = buildTruthTableFacts(querie);
    std::map<char, rhr_value_e> base_results = computeBaseResults(output_symbols);
    TruthTable filtered_truth_table;
    bool has_truth_table = buildFilteredTruthTable(base_results, filtered_truth_table);

    for (char q : output_symbols)
    {
        rhr_value_e res = base_results.count(q) ? base_results[q] : R_FALSE;
        if (has_truth_table)
            res = filtered_truth_table.clampValue(q, res);
        outputResult(q, res);
    }
}

void Resolver::changeFacts(const std::set<char> &new_facts)
{
    initial_facts = new_facts;
    resetEvaluationState();
    reasoning.reset();

    // Reset token effects in all rules
    for (BasicRule &rule : basic_rules)
    {
        for (TokenBlock &block : rule.lhs)
        {
            for (TokenEffect &tk : block)
            {
                if (tk.type >= 'A' && tk.type <= 'Z')
                    tk.effect = (initial_facts.find(tk.type) != initial_facts.end());
            }
        }
    }
}
