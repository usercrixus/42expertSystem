#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include "Resolver.hpp"

static bool parseInteractiveFacts(const std::string &line, std::set<char> &facts)
{
    facts.clear();
    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];
        if (c == ' ' || c == '\t')
            continue;
        if (c >= 'A' && c <= 'Z')
        {
            facts.insert(c);
        }
        else
        {
            std::cerr << "Invalid character in facts: " << c << std::endl;
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [--explain] [--interactive]" << std::endl;
        return 1;
    }

    bool print_trace = false;
    bool interactive_mode = false;
    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--explain")
            print_trace = true;
        else if (arg == "--interactive")
            interactive_mode = true;
        else
        {
            std::cerr << "Unknown option: " << arg << std::endl;
            return 1;
        }
    }

    Parser parser(argv[1]);
    if (parser.parse() != 0)
        return 1;
    if (!parser.getCombinedTruthTable().hasValidState())
    {
        std::cerr << "No valid states for the given rules." << std::endl;
        return 1;
    }
    Resolver resolver(parser.getQuerie(), parser.getBasicRules(), parser.getInitialFact());
    if (parser.hasValidStateWithInitialFacts())
    {
        resolver.resolveQuerie(print_trace);
    } else {
        std::cerr << "No valid states with the given initial facts.";
        if (interactive_mode)
            std::cerr << " Please try again.";
        std::cerr << std::endl;
    }

    if (interactive_mode)
    {
        std::cout << "Interactive mode: enter new initial facts (e.g. AB). Empty line to exit. Space for all false." << std::endl;
        std::string line;
        std::set<char> new_facts;
        while (true)
        {
            std::cout << "Initial facts = " << std::flush;
            if (!std::getline(std::cin, line) || line.empty())
                break;
            if (!parseInteractiveFacts(line, new_facts))
                continue; // Error parsing facts, prompt again

            parser.getInitialFact() = new_facts;
            if (!parser.hasValidStateWithInitialFacts())
            {
                std::cerr << "No valid states with the given initial facts. Please try again." << std::endl;
                continue;
            }
            resolver.changeFacts(parser.getInitialFact());
            resolver.resolveQuerie(print_trace);
        }
    }
    return 0;
}
