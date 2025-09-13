#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    Parser parser(argv[1]);
    if (parser.parse() != 0) {
        return 1; // parser already printed the error
    }

    const auto& initial = parser.getInitialFacts();
    const auto& queries = parser.getQueries();

    // For now: a query is True iff it is in initial facts; otherwise False.
    // (You’ll replace this with your inference later.)
    for (char q : queries) {
        bool v = (initial.find(q) != initial.end());
        std::cout << q << ": " << (v ? 'T' : 'F') << "\n";
    }

    return 0;
}
