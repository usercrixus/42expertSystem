#include "Parser.hpp"
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

void Parser::parseClassic(std::string line, std::vector<TokenBlock> &fact_line)
{
	std::string buff;
	if (fact_line.empty())
		fact_line.emplace_back(priority);
	for (size_t i = 0; i < line.size(); i++)
	{
		if (line[i] == '#')
			break;
		else if (line[i] == ' ')
		{
			buff.clear();
			continue;
		}
		buff.push_back(line[i]);
		if (buff == "(")
			fact_line.emplace_back(++priority);
		else if (buff == ")")
			fact_line.emplace_back(--priority);
		else if (buff == "!")
			fact_line.back().emplace_back(TokenEffect('!'));
		else if (buff == "+")
			fact_line.back().emplace_back(TokenEffect('+'));
		else if (buff == "|")
			fact_line.back().emplace_back(TokenEffect('|'));
		else if (buff == "^")
			fact_line.back().emplace_back(TokenEffect('^'));
		else if (buff == "=>")
			fact_line.back().emplace_back(TokenEffect('>'));
		else if (buff == "<=>")
			fact_line.back().emplace_back(TokenEffect('='));
		else if (buff.size() == 1 && buff[0] >= 'A' && buff[0] <= 'Z')
			fact_line.back().emplace_back(TokenEffect(buff[0]));
		else if (buff.size() >= 3)
			throw std::logic_error("Input file format do not manage: " + buff + " token");
	}
}

void Parser::parsingManager(std::ifstream &in)
{
	std::string line;
	while (std::getline(in, line))
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

void Parser::finalizeParsing()
{
	for (std::vector<TokenBlock> &fact : facts)
	{
		for (TokenBlock &token_block : fact)
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
	return 0;
}
