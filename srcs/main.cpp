#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_set>
#include <cctype>

// (no extra utilities – keep main minimal)

int main(int argc, char **argv)
{
    if (argc != 2)
        return (std::cerr << "Usage: " << argv[0] << " <input_file>\n", 1);

    Parser parser(argv[1]);
    parser.parse();

    return 0;
}
