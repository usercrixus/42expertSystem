#pragma once
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include "LogicRule.hpp"
#include "TruthTable.hpp"

class Parser
{
private:
    std::string input_path;
    std::vector<LogicRule> facts;
    std::vector<BasicRule> basic_rules;
    std::set<char> initial_facts;
    std::set<char> querie;
    unsigned int priority;
    TruthTable combined_truth_table;
    void expandRules();

public:
    Parser(std::string input);
    ~Parser();
    void parsingManager(std::ifstream &in);
    /**
     * record what is after '=' (facts defined) in querie vector
     */
    void parseFact(std::string line);
    /**
     * record what is after '?' (queries requested) in initial_facts vector
     */
    void parseQuerie(std::string line);
    /**
     * Default file parser function.
     * Parse any line expect them starting by '=' or '?'
     */
    void parseClassic(std::string line, LogicRule &fact_line);
    /**
     * Marks tokens as true if they correspond to an initial fact
     * See the parsing structure for a better understanding
     */
    void finalizeParsing();
    /**
     * Parser routing logic. Parser entry point
     */
    int parse();

    std::vector<LogicRule> &getFacts();
    std::vector<BasicRule> &getBasicRules();
    std::set<char> &getQuerie();
    std::set<char> &getInitialFact();
    TruthTable &getCombinedTruthTable();
    bool hasValidStateWithInitialFacts() const;
};
