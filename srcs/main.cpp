#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include "Resolver.hpp"

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    Parser parser(argv[1]);
    if (parser.parse() != 0)
        return 1;
    Resolver resolver;
    resolver.resolve(parser.getFacts());
    resolver.resolveQuerie(parser.getQuerie(), parser.getFacts());
    return 0;
}
