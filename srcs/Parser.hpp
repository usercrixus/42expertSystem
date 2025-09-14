#pragma once
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include "TokenEffect.hpp"
#include "TokenBlock.hpp"
#include <tuple>

class Parser
{
private:
    std::string input_path;
    std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> facts;
    std::set<char> initial_facts;
    std::set<char> querie;
    unsigned int priority;

public:
    Parser(std::string input);
    ~Parser();
    void parsingManager(std::ifstream &in);
    void parseFact(std::string line);
    void parseQuerie(std::string line);
    void parseClassic(std::string line, std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>> &fact_line);
    void finalizeParsing();
    int parse();

    std::vector<std::tuple<TokenEffect, std::vector<TokenBlock>, std::vector<TokenBlock>>> &getFacts();
    std::set<char> &getQuerie();
    std::set<char> &getInitialFact();
};
