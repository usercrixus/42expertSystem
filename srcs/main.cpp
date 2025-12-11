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
    //std::cout << "Initial Rules: " << std::endl;
    //for (const LogicRule &rule : parser.getFacts())
    //    std::cout << rule << std::endl;
    //std::cout << "Deduced Basic Rules:\n";
    //for (const BasicRule &rule : parser.getBasicRules()) {
    //    std::cout << rule << "\n";
    //    if (rule.origin != nullptr)
    //        std::cout << "\\-> From Logic Rule : " << *(rule.origin) << "\n\n";
    //}
    Resolver resolver(parser.getQuerie(), parser.getBasicRules(), parser.getInitialFact());
    resolver.resolveQuerie();
    return 0;
}
