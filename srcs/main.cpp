#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include "Resolver.hpp"

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [--explain]" << std::endl;
        return 1;
    }

    Parser parser(argv[1]);
    if (parser.parse() != 0)
        return 1;
    Resolver resolver(parser.getQuerie(), parser.getBasicRules(), parser.getInitialFact());
    bool print_trace = (argc > 2 && std::string(argv[2]) == "--explain");
    resolver.resolveQuerie(print_trace);
    return 0;
}
