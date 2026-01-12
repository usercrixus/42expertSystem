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
    void parseFact(std::string line);
    void parseQuerie(std::string line);
    void parseClassic(std::string line, LogicRule &fact_line);
    void finalizeParsing();
    int parse();

    std::vector<LogicRule> &getFacts();
    std::vector<BasicRule> &getBasicRules();
    std::set<char> &getQuerie();
    std::set<char> &getInitialFact();
    TruthTable &getCombinedTruthTable();
    bool hasValidStateWithInitialFacts() const;
};
