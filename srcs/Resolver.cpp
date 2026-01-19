#include "Resolver.hpp"
#include "LogicRule.hpp"
#include <iostream>
#include <map>
#include <stdexcept>

Resolver::Resolver(std::set<char> querie, std::vector<BasicRule> &basic_rules, std::set<char> initial_facts, const TruthTable &truth_table)
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
    reasoning.reset();
}

rhr_value_e Resolver::finalizeOutcome(const RuleOutcome &outcome) const
{
    if (outcome.definite_true && outcome.definite_false)
        return R_AMBIGOUS;
    if (outcome.definite_true)
        return R_TRUE;
    if (outcome.definite_false)
        return R_FALSE;
    if (outcome.possible_true || outcome.possible_false)
        return R_AMBIGOUS;
    return R_FALSE;
}

void Resolver::updateOutcomeFromRule(rhr_value_e lhs_result, const BasicRule &rule, RuleOutcome &outcome)
{
    if (lhs_result == R_TRUE)
    {
        if (!rule.rhs_negated)
            outcome.definite_true = true;
        else
            outcome.definite_false = true;
    }
    else if (lhs_result == R_AMBIGOUS)
    {
        outcome.possible_true = true;
        outcome.possible_false = true;
    }
}

rhr_value_e Resolver::getTokenValue(TriToken &token, bool negated_context)
{
    if (token.has_value)
        return token.value;
    if (token.type >= 'A' && token.type <= 'Z')
    {
        token.value = prove(token.type, negated_context);
        token.has_value = true;
        return token.value;
    }
    throw std::logic_error("Token value requested for non-value token");
}

void Resolver::executeNotTri(std::vector<TriToken> &tokens, bool negated_context)
{
    size_t i = 0;
    while (i < tokens.size())
    {
        if (tokens[i].type == '!')
        {
            if (i + 1 == tokens.size())
                throw std::logic_error("operator ! has no var attached\n");
            TriToken &next = tokens[i + 1];
            rhr_value_e val = getTokenValue(next, !negated_context);
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
            rhr_value_e lval = getTokenValue(left, negated_context);
            rhr_value_e rval = getTokenValue(right, negated_context);
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

rhr_value_e Resolver::executeTriBlock(std::vector<TriToken> &tokens, bool negated_context)
{
    if (tokens.empty())
        throw std::logic_error("TriBlock::execute: empty block");
    executeNotTri(tokens, negated_context);
    executeOthersTri(tokens, '+', negated_context);
    executeOthersTri(tokens, '|', negated_context);
    executeOthersTri(tokens, '^', negated_context);
    if (tokens.size() != 1)
        throw std::logic_error("TriBlock::execute: reduction did not converge");
    return getTokenValue(tokens[0], negated_context);
}

unsigned int Resolver::getMaxPriority(std::vector<TriBlock> &blocks)
{
    unsigned int max_priority = 0;
    for (const TriBlock &block : blocks)
        if (block.priority > max_priority)
            max_priority = block.priority;
    return max_priority;
}

bool Resolver::isNegatedContext(size_t i, std::vector<TriBlock> &blocks)
{
    if (i != 0 && !blocks[i - 1].tokens.empty())
    {
        const TriToken &prev = blocks[i - 1].tokens.back();
        if (prev.type == '!')
            return true;
    }
    return false;
}

rhr_value_e Resolver::resolveLeftTri(std::vector<TriBlock> &blocks)
{
    if (blocks.empty())
        throw std::logic_error("resolveLeftTri: empty expression");
    unsigned int max_priority = getMaxPriority(blocks);
    for (size_t i = 0; i < blocks.size(); i++)
    {
        if (blocks[i].priority == max_priority)
        {
            executeTriBlock(blocks[i].tokens, isNegatedContext(i, blocks));
            TriToken result = blocks[i].tokens[0];
            result.type = 0;
            result.has_value = true;
            if (i != 0)
            {
                blocks[i - 1].tokens.push_back(result);
                blocks.erase(blocks.begin() + i);
            }
            else if (blocks.size() > 1)
            {
                blocks[1].tokens.insert(blocks[1].tokens.begin(), result);
                blocks.erase(blocks.begin());
            }
            else
                blocks[i].priority = 0;
        }
    }
    if (blocks.size() != 1 || blocks[0].tokens.size() > 1)
        return resolveLeftTri(blocks);
    return blocks[0].tokens[0].value;
}

std::vector<Resolver::TriBlock> Resolver::buildTriBlockVector(const std::vector<TokenBlock> &lhs)
{
    std::vector<Resolver::TriBlock> blocks;
    blocks.reserve(lhs.size());
    for (const TokenBlock &block : lhs)
    {
        Resolver::TriBlock tri_block;
        tri_block.priority = block.getPriority();
        tri_block.tokens.reserve(block.size());
        for (const TokenEffect &tk : block)
        {
            Resolver::TriToken tri_token;
            tri_token.type = tk.type;
            tri_token.value = R_FALSE;
            tri_token.has_value = false;
            tri_block.tokens.push_back(tri_token);
        }
        blocks.push_back(tri_block);
    }
    return blocks;
}

bool Resolver::handleVisiting(char q, bool negated_context, rhr_value_e &result)
{
    std::unordered_map<char, bool>::iterator visitingIt = visiting.find(q);
    if (visitingIt == visiting.end())
        return false;
    if (visitingIt->second != negated_context)
        result = R_AMBIGOUS;
    else
        result = R_FALSE;
    return true;
}

bool Resolver::handleQInitialFact(char q, rhr_value_e &result)
{
    auto initiaTrueIt = initial_facts.find(q);
    if (initiaTrueIt == initial_facts.end())
        return false;
    reasoning.recordInitialFact(q);
    result = R_TRUE;
    memo[q] = result;
    return true;
}

bool Resolver::handleQMemo(char q, rhr_value_e &result)
{
    std::unordered_map<char, rhr_value_e>::iterator memoIt = memo.find(q);
    if (memoIt == memo.end())
        return false;
    reasoning.recordMemoHit(q, memoIt->second);
    result = memoIt->second;
    return true;
}

bool Resolver::isQHandled(char q, rhr_value_e &result, bool negated_context)
{
    if (handleQMemo(q, result) || handleQInitialFact(q, result) || handleVisiting(q, negated_context, result))
        return true;
    return false;
}

rhr_value_e Resolver::prove(char q, bool negated_context)
{
    rhr_value_e result = R_FALSE;
    if (isQHandled(q, result, negated_context))
        return result;
    visiting[q] = negated_context;
    RuleOutcome outcome = {false, false, false, false};
    for (const BasicRule &rule : basic_rules)
    {
        if (rule.rhs_symbol == q)
        {
            std::vector<Resolver::TriBlock> blocks = buildTriBlockVector(rule.lhs);
            rhr_value_e lhs_result = resolveLeftTri(blocks);
            reasoning.recordRuleConsidered(q, &rule, lhs_result == R_TRUE);
            updateOutcomeFromRule(lhs_result, rule, outcome);
        }
    }
    visiting.erase(q);
    result = finalizeOutcome(outcome);
    memo[q] = result;
    return result;
}

void Resolver::outputResult(char q, rhr_value_e res)
{
    std::string resultStr = (res == R_TRUE ? "true" : res == R_FALSE ? "false"
                                                                     : "ambiguous");
    std::cout << q << " = " << resultStr << std::endl;
}

bool Resolver::buildFilteredTruthTable(const std::map<char, rhr_value_e> &base_results, TruthTable &filtered) const
{
    if (!truth_table.hasValidState())
        return false;
    filtered = truth_table.filterByResults(initial_facts, base_results);
    return filtered.hasValidState();
}

std::map<char, rhr_value_e> Resolver::computeBaseResults(const std::set<char> &facts)
{
    std::map<char, rhr_value_e> base_results;
    for (char q : facts)
    {
        resetEvaluationState();
        base_results[q] = prove(q, false);
    }
    return base_results;
}

void Resolver::resolve()
{
    std::map<char, rhr_value_e> base_results = computeBaseResults(truth_table.variables);
    TruthTable filtered_truth_table;
    bool has_truth_table = buildFilteredTruthTable(base_results, filtered_truth_table);

    for (char q : truth_table.variables)
    {
        rhr_value_e res = base_results.count(q) ? base_results[q] : R_FALSE;
        if (has_truth_table)
            res = filtered_truth_table.clampValue(q, res);
        outputResult(q, res);
    }
}

// ------------------------ //

ReasoningStep &Resolver::getReasoning()
{
    return reasoning;
}

const ReasoningStep &Resolver::getReasoning() const
{
    return reasoning;
}

void Resolver::changeFacts(const std::set<char> &new_facts)
{
    initial_facts = new_facts;
    resetEvaluationState();

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

// todelete

rhr_value_e Resolver::resolveQuery(char q, const TruthTable &filtered, bool has_truth_table)
{
    resetEvaluationState();
    rhr_value_e res = prove(q, false);
    if (!has_truth_table)
        return res;
    return filtered.clampValue(q, res);
}
