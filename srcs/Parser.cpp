#include "Parser.hpp"
#include "TruthTable.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

Parser::Parser(std::string input) : input_path(input), priority(0)
{
}

Parser::~Parser()
{
}

void Parser::parseFact(std::string line)
{
	for (size_t i = 1; i < line.size(); i++)
	{
		if (line[i] == '#')
			break;
		else if (line[i] == ' ')
			continue;
		else if (line[i] >= 'A' && line[i] <= 'Z')
			initial_facts.emplace(line[i]);
		else
		{
			std::string msg = std::string("Syntax error in initial_facts: ") + line[i];
			throw std::logic_error(msg);
		}
	}
}

void Parser::parseQuerie(std::string line)
{
	for (size_t i = 1; i < line.size(); i++)
	{
		if (line[i] == '#')
			break;
		else if (line[i] == ' ')
			continue;
		else if (line[i] >= 'A' && line[i] <= 'Z')
			querie.emplace(line[i]);
		else
		{
			std::string msg = std::string("Syntax error in initial_facts: ") + line[i];
			throw std::logic_error(msg);
		}
	}
}

void Parser::parseClassic(std::string line, LogicRule &fact_line)
{
	std::string buff;
	int side = 1;
	// Don't pre-create initial block; let first token create it at correct priority
	for (size_t i = 0; i < line.size(); i++)
	{
		if (line[i] == '#')
			break;
		else if (line[i] == ' ' || line[i] == '\n')
			buff.clear();
		else
		{
			std::vector<TokenBlock> &tokenSide = (side == 1) ? fact_line.lhs : fact_line.rhs;
			buff.push_back(line[i]);
			if (buff == "(")
			{
				++priority; // enter higher-priority group
				buff.clear();
			}
			else if (buff == ")")
			{
				--priority; // exit group; do not create an empty block
				buff.clear();
			}
			else if (buff == "!" || buff == "+" || buff == "|" || buff == "^")
			{
				if (tokenSide.empty() || tokenSide.back().getPriority() != static_cast<unsigned int>(priority))
					tokenSide.emplace_back(priority);
				tokenSide.back().emplace_back(TokenEffect(buff[0]));
				buff.clear();
			}
			else if (buff == "=>" || buff == "<=>")
			{
				fact_line.arrow = TokenEffect(buff[1]);
				side = 2;
				if (fact_line.rhs.empty())
					fact_line.rhs.emplace_back(priority);
				buff.clear();
			}
			else if (buff.size() == 1 && buff[0] >= 'A' && buff[0] <= 'Z')
			{
				if (tokenSide.empty() || tokenSide.back().getPriority() != static_cast<unsigned int>(priority))
					tokenSide.emplace_back(priority);
				tokenSide.back().emplace_back(TokenEffect(buff[0]));
				buff.clear();
			}
			else if (buff.size() >= 3)
				throw std::logic_error("Input file format do not manage: " + buff + " token");
		}
	}
}

void Parser::parsingManager(std::ifstream &in)
{
	std::string line;
	while (std::getline(in, line))
	{
		line.erase(0, line.find_first_not_of(" "));
		if (line.size() > 0 && line[0] != '#')
		{
			if (line[0] == '=')
				parseFact(line);
			else if (line[0] == '?')
				parseQuerie(line);
			else
			{
				facts.emplace_back();
				parseClassic(line, facts.back());
			}
		}
	}
}

void Parser::finalizeParsing()
{
	for (LogicRule &fact : facts)
	{
		for (TokenBlock &token_block : fact.lhs)
		{
			for (TokenEffect &token_effect : token_block)
			{
				if (initial_facts.find(token_effect.type) != initial_facts.end())
					token_effect.effect = true;
			}
		}
		for (TokenBlock &token_block : fact.rhs)
		{
			for (TokenEffect &token_effect : token_block)
			{
				if (initial_facts.find(token_effect.type) != initial_facts.end())
					token_effect.effect = true;
			}
		}
	}
}

int Parser::parse()
{
	std::ifstream in(this->input_path);
	if (!in)
		return (std::cerr << "Error: cannot open file " << this->input_path << "\n", 1);
	parsingManager(in);
	finalizeParsing();
	expandRules();
	return 0;
}

void Parser::expandRules()
{
	for (const LogicRule &rule : facts)
	{
		std::vector<BasicRule> basics = rule.deduceBasics();
		basic_rules.insert(basic_rules.end(), basics.begin(), basics.end());
	}
	
	// Generate truth tables for all basic rules
	std::vector<TruthTable> tables;
	for (const BasicRule &rule : basic_rules)
	{
		TruthTable table = TruthTable::fromBasicRule(rule);
		std::cout << "Truth Table for Basic Rule: " << rule.toString() << "\n";
		std::cout << table.toString() << "\n";
		tables.push_back(table);
	}
	combined_truth_table = TruthTable::conjunctionAll(tables);
}

std::vector<LogicRule> &Parser::getFacts()
{
	return facts;
}

std::vector<BasicRule> &Parser::getBasicRules()
{
	return basic_rules;
}

std::set<char> &Parser::getQuerie()
{

    return querie;
}

std::set<char> &Parser::getInitialFact()
{
    return initial_facts;
}

TruthTable &Parser::getCombinedTruthTable()
{
	//std::cout << "Combined Truth Table:\n" << combined_truth_table.toString() << "\n";
	return combined_truth_table;
}

bool Parser::hasValidStateWithInitialFacts() const
{
	// Convert initial_facts set to map
	std::map<char, bool> known_facts;
	for (char c : initial_facts)
	{
		known_facts[c] = true;
	}
	
	// Filter truth table by initial facts
	TruthTable filtered = combined_truth_table.filterByFacts(known_facts);
	
	//std::cout << "Filtered Truth Table with Initial Facts:\n" << filtered.toString() << "\n";
	// Check if any valid state remains
	return filtered.hasValidState();
}
